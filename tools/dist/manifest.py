"""manifest.json のビルド / 読み書き / 差分。

配信の単位は「``Assets/`` のツリーをそのまま鏡写しにしたもの」。マニフェストはその一覧で、
クライアント (``Packages/AssetUpdater``) が起動時にこれだけを落として手元の ``installed.json``
と突き合わせ、変わったファイルだけ取得する。

エントリは 2 種類ある:

* **アセット** - ``.meta`` を持つファイル。``guid`` / ``metaHash`` / ``metaSize`` を伴う。
  ``.meta`` は guid と ``contentPath_`` の出どころ (``AssetFactory::RegisterLoader`` が
  ``filePath + ".meta"`` から実体を作る) なので、本体と必ずペアで動かす。
* **随伴ファイル** - ``.meta`` を持たないが実行時に要るファイル。``.efkefc`` が参照する
  ``.efkmodel``、``.mv1`` が ``<名前>.fbm/`` 相対で参照するテクスチャ、コンパイル済みシェーダ
  (``.pso`` / ``.vso``) など。guid が無く、path が identity になる。

「``.meta`` が無ければ配信対象外」は **誤り** で、それだと随伴ファイルが丸ごと落ちて
エフェクトとモデルがテクスチャ無しで描画される。そのため除外は「開発専用のものだけを
名指しする denylist」にしてある。
"""

from __future__ import annotations

import hashlib
import json
import sys
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

_REPO = Path(__file__).resolve().parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.common.cereal_json import OrderedObj, loads  # noqa: E402

SCHEMA = 1

#: 配信しないディレクトリ (リポジトリルートからの POSIX 相対)
EXCLUDE_DIRS: tuple[str, ...] = (
    "Assets/Scripts",  # exe にコンパイルされるので配る必要が無い
)

#: どの階層にあっても、この名前のディレクトリの下は配信しない (小文字)。
#: `_Source/` は変換前の原本置き場 (.fbx / .blend / .efkproj とその素材) で、実行時の参照先は
#: 変換後のファイルの隣にある同名のコピーの方。.blend には作業した PC のユーザー名とフルパスが入る
EXCLUDE_DIR_NAMES: frozenset[str] = frozenset({"_source"})

#: 配信しない拡張子 (小文字)
EXCLUDE_SUFFIXES: tuple[str, ...] = (
    ".meta",     # 本体エントリに畳むので単独では出さない
    ".fbx",      # モデルの原本。実行時は .mv1 だけ要る
    ".blend",    # 同上。作業した PC のユーザー名とフルパスが入っている
    ".blend1",   # Blender の自動バックアップ
    ".efkproj",  # エフェクトの原本。実行時は .efkefc だけ要る
    ".h",
    ".cpp",
    ".bak",
)

#: 配信しないファイル名 (小文字)
EXCLUDE_NAMES: frozenset[str] = frozenset({"desktop.ini", "thumbs.db", ".ds_store"})

_HASH_CHUNK = 1 << 20


@dataclass
class Entry:
    path: str
    hash: str
    size: int
    guid: str = ""
    meta_hash: str = ""
    meta_size: int = 0

    @property
    def total_size(self) -> int:
        return self.size + self.meta_size

    @property
    def is_asset(self) -> bool:
        return bool(self.meta_hash)

    def to_json(self) -> dict:
        return {
            "guid": self.guid,
            "path": self.path,
            "hash": self.hash,
            "size": self.size,
            "metaHash": self.meta_hash,
            "metaSize": self.meta_size,
        }

    @staticmethod
    def from_json(obj: dict) -> "Entry":
        return Entry(
            path=obj["path"],
            hash=obj["hash"],
            size=int(obj.get("size", 0)),
            guid=obj.get("guid", ""),
            meta_hash=obj.get("metaHash", ""),
            meta_size=int(obj.get("metaSize", 0)),
        )


@dataclass
class ScanResult:
    entries: list[Entry] = field(default_factory=list)
    excluded: list[str] = field(default_factory=list)
    #: 本体エントリに畳まれた .meta。除外ではないので別勘定にする
    folded_meta: list[str] = field(default_factory=list)
    missing_guid: list[str] = field(default_factory=list)
    non_ascii: list[str] = field(default_factory=list)
    #: CP932 のまま残っている .meta (読めてはいる)
    shift_jis_meta: list[str] = field(default_factory=list)

    @property
    def asset_count(self) -> int:
        return sum(1 for e in self.entries if e.is_asset)

    @property
    def companion_count(self) -> int:
        return sum(1 for e in self.entries if not e.is_asset)

    @property
    def total_bytes(self) -> int:
        return sum(e.total_size for e in self.entries)


def is_excluded(rel_posix: str) -> bool:
    """``rel_posix`` はリポジトリルートからの POSIX 相対パス。"""
    lowered = rel_posix.lower()
    name = rel_posix.rsplit("/", 1)[-1]
    if name.lower() in EXCLUDE_NAMES:
        return True
    for suffix in EXCLUDE_SUFFIXES:
        if lowered.endswith(suffix):
            return True
    if any(part in EXCLUDE_DIR_NAMES for part in lowered.split("/")[:-1]):
        return True
    for directory in EXCLUDE_DIRS:
        if lowered == directory.lower() or lowered.startswith(directory.lower() + "/"):
            return True
    return False


def _find_guid(node) -> str | None:
    if isinstance(node, OrderedObj):
        for key, value in node.items():
            if key == "guid_" and isinstance(value, OrderedObj) and "value_" in value:
                raw = value["value_"]
                if isinstance(raw, str):
                    return raw
            found = _find_guid(value)
            if found:
                return found
    elif isinstance(node, list):
        for item in node:
            found = _find_guid(item)
            if found:
                return found
    return None


def decode_meta(raw: bytes) -> tuple[str, str]:
    """``.meta`` のバイト列を (テキスト, 使った encoding) にする。

    大半は UTF-8 だが、``/execution-charset:utf-8`` を入れる前のエンジンが書いた
    ``.meta`` が 70 件ほど CP932 のまま残っている (``contentPath_`` に日本語を含むもの)。
    guid 自体は ASCII なのでどちらで読んでも取れる。
    """
    if raw.startswith(b"\xef\xbb\xbf"):
        raw = raw[3:]
    try:
        return raw.decode("utf-8"), "utf-8"
    except UnicodeDecodeError:
        return raw.decode("cp932", errors="replace"), "cp932"


def read_guid(meta_path: Path) -> tuple[str, str]:
    """``.meta`` から ``(guid_.value_, encoding)`` を取り出す。読めなければ guid は空文字。

    「痩せた」プロキシ (Mv1File 等) と「太った」ScriptableObject (SwordManInitStatus 等) で
    ペイロードの形は違うが、guid_ はどちらも 1 個だけなので再帰探索で一意に決まる。
    """
    try:
        text, encoding = decode_meta(meta_path.read_bytes())
        root = loads(text)
    except Exception:  # noqa: BLE001 - 壊れた .meta 1 個でビルド全体を止めない
        return "", ""
    return _find_guid(root) or "", encoding


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(_HASH_CHUNK):
            digest.update(chunk)
    return digest.hexdigest()


class HashCache:
    """mtime + size をキーにした SHA-256 のキャッシュと、SHA-256 をキーにした参照リスト
    (``tools/dist/refs.py``) のキャッシュ。

    1.76 GB / 3,400 ファイルを毎回読み直すのは無駄なので、変わっていないファイルは
    前回の値を使い回す (``.mv1`` の展開も全部で約 18 秒かかる)。キャッシュが無い / 壊れていても
    結果は変わらない。
    """

    def __init__(self, path: Path | None) -> None:
        self.path = path
        self._entries: dict[str, list] = {}
        self._refs: dict[str, list[str] | None] = {}
        self.hits = 0
        self.misses = 0
        self.ref_hits = 0
        self.ref_misses = 0
        if path is not None and path.exists():
            try:
                data = json.loads(path.read_text(encoding="utf-8"))
            except Exception:  # noqa: BLE001
                data = {}
            if not isinstance(data, dict):
                data = {}
            if "hashes" in data:
                self._entries = data["hashes"] if isinstance(data["hashes"], dict) else {}
                self._refs = data.get("refs") if isinstance(data.get("refs"), dict) else {}
            else:
                # 参照キャッシュを足す前の形式 (パス -> [mtime, size, sha256] の平たい dict)
                self._entries = data

    def hash_of(self, path: Path, key: str) -> str:
        stat = path.stat()
        cached = self._entries.get(key)
        if cached and cached[0] == stat.st_mtime_ns and cached[1] == stat.st_size:
            self.hits += 1
            return cached[2]

        self.misses += 1
        digest = sha256_file(path)
        self._entries[key] = [stat.st_mtime_ns, stat.st_size, digest]
        return digest

    def refs_of(self, digest: str, read: Callable[[], list[str] | None]) -> list[str] | None:
        """中身が ``digest`` のファイルの参照リスト。読めないファイルの None もキャッシュする。"""
        if digest in self._refs:
            self.ref_hits += 1
            return self._refs[digest]
        self.ref_misses += 1
        refs = read()
        self._refs[digest] = refs
        return refs

    def save(self) -> None:
        if self.path is None:
            return
        self.path.write_text(json.dumps({"hashes": self._entries, "refs": self._refs}), encoding="utf-8")


def scan(assets_root: Path, repo_root: Path, cache: HashCache) -> ScanResult:
    result = ScanResult()
    if not assets_root.is_dir():
        raise FileNotFoundError(f"assets root がありません: {assets_root}")

    for path in sorted(assets_root.rglob("*")):
        if not path.is_file():
            continue

        rel = path.relative_to(repo_root).as_posix()
        if rel.lower().endswith(".meta"):
            result.folded_meta.append(rel)
            continue
        if is_excluded(rel):
            result.excluded.append(rel)
            continue

        entry = Entry(path=rel, hash=cache.hash_of(path, rel), size=path.stat().st_size)

        meta_path = path.with_name(path.name + ".meta")
        if meta_path.exists():
            meta_rel = meta_path.relative_to(repo_root).as_posix()
            entry.meta_hash = cache.hash_of(meta_path, meta_rel)
            entry.meta_size = meta_path.stat().st_size
            entry.guid, encoding = read_guid(meta_path)
            if not entry.guid:
                result.missing_guid.append(rel)
            if encoding == "cp932":
                result.shift_jis_meta.append(meta_rel)

        if not rel.isascii():
            result.non_ascii.append(rel)

        result.entries.append(entry)

    return result


def build(scan_result: ScanResult, version: str, required_client_version: str, base_url: str) -> dict:
    return {
        "schema": SCHEMA,
        "version": version,
        "requiredClientVersion": required_client_version,
        "baseUrl": base_url,
        "totalBytes": scan_result.total_bytes,
        "entries": [e.to_json() for e in scan_result.entries],
    }


def dump(manifest: dict, path: Path) -> None:
    """LF / BOM 無し / UTF-8。rapidjson がそのまま読める形にする。"""
    text = json.dumps(manifest, ensure_ascii=False, indent=2) + "\n"
    path.write_bytes(text.encode("utf-8"))


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def entries_of(manifest: dict) -> list[Entry]:
    return [Entry.from_json(obj) for obj in manifest.get("entries", [])]


@dataclass
class Diff:
    added: list[Entry] = field(default_factory=list)
    changed: list[Entry] = field(default_factory=list)
    removed_paths: list[str] = field(default_factory=list)

    @property
    def download_bytes(self) -> int:
        return sum(e.total_size for e in self.added) + sum(e.total_size for e in self.changed)

    @property
    def is_up_to_date(self) -> bool:
        return not self.added and not self.changed and not self.removed_paths


def diff(installed: dict, remote: dict) -> Diff:
    """Packages/AssetUpdater の ``ManifestDiff::Between`` と同じ規則で差分を出す。"""
    installed_by_path = {e.path: e for e in entries_of(installed)}
    remote_entries = entries_of(remote)
    remote_paths = {e.path for e in remote_entries}

    result = Diff()
    for entry in remote_entries:
        previous = installed_by_path.get(entry.path)
        if previous is None:
            result.added.append(entry)
        elif previous.hash != entry.hash or previous.meta_hash != entry.meta_hash:
            result.changed.append(entry)

    result.removed_paths = [p for p in installed_by_path if p not in remote_paths]
    return result


def format_bytes(count: int) -> str:
    if count < 1024:
        return f"{count} B"
    if count < 1024 * 1024:
        return f"{count / 1024:.1f} KB"
    if count < 1024 * 1024 * 1024:
        return f"{count / (1024 * 1024):.1f} MB"
    return f"{count / (1024 * 1024 * 1024):.2f} GB"
