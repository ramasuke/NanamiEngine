"""manifest.json とその参照先を配信先 (rclone の remote) へ上げる。

サーバー側のレイアウト::

    <remote>/
    ├── manifest.json              唯一の可変ファイル。これを差し替えることがリリース
    ├── manifest-<version>.json    版ごとの控え (不変)。ロールバックはこれを manifest.json へ写すだけ
    └── files/<sha256>             本体と .meta を中身のハッシュ名で置く (不変)

クライアントは ``baseUrl + <hash>`` で取りに来るので、URL は常に 16 進の ASCII になり、
日本語のアセットパスを percent-encode する必要が無い。ローカルの置き場所はエントリの ``path``。

上げるのは「remote にまだ無いハッシュ」だけ。前回のマニフェストに依存しないので、途中で
止まっても再実行すれば残りだけが上がる。
"""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

VERSION_RE = re.compile(r"^[0-9A-Za-z._-]+$")

BLOB_CACHE_CONTROL = "Cache-Control: public, max-age=31536000, immutable"
MANIFEST_CACHE_CONTROL = "Cache-Control: no-cache"

#: ゲームの実行中ずっと開かれていて、クライアントがタイトル画面で差し替えられないもの。
#: フォントはエンジン (TtfFontFile) が起動時に AddFontResourceEx で登録し、終了まで外さない
RUNTIME_LOCKED_SUFFIXES: tuple[str, ...] = (".ttf", ".otf", ".ttc")


def _version_parts(version: str) -> list[int]:
    parts = []
    for piece in version.split("."):
        digits = "".join(ch for ch in piece if ch.isdigit())
        parts.append(int(digits) if digits else 0)
    return parts


def is_newer_version(left: str, right: str) -> bool:
    """left > right をドット区切りの数値として比べる (クライアントの HttpAssetUpdaterIsOlderVersion と同じ規則)"""
    a, b = _version_parts(left), _version_parts(right)
    size = max(len(a), len(b))
    return a + [0] * (size - len(a)) > b + [0] * (size - len(b))


def locked_file_changes(live: dict, new: dict) -> list[str]:
    """公開中の版にある、実行中は差し替えられないファイルのうち、変更・削除されるもの。

    追加は問題にならない (プレイヤーの PC にまだ無いファイルは開かれていないので書ける)。
    """
    def locked(document: dict) -> dict[str, str]:
        return {e["path"]: e["hash"] for e in document.get("entries", [])
                if e["path"].lower().endswith(RUNTIME_LOCKED_SUFFIXES)}

    before, after = locked(live), locked(new)
    return sorted(path for path, digest in before.items() if after.get(path) != digest)

_CHUNK = 1 << 20
_RCLONE_DIR_NOT_FOUND = 3


class UploadError(RuntimeError):
    pass


@dataclass(frozen=True)
class Blob:
    hash: str
    source: Path
    size: int


def blobs_of(manifest: dict, repo_root: Path) -> dict[str, Blob]:
    """マニフェストが参照する全ブロブ (本体 + .meta)。中身が同じものは 1 つにまとまる。"""
    blobs: dict[str, Blob] = {}
    for entry in manifest.get("entries", []):
        path = entry["path"]
        body_hash = entry["hash"]
        blobs.setdefault(body_hash, Blob(body_hash, repo_root / path, int(entry.get("size", 0))))

        meta_hash = entry.get("metaHash", "")
        if meta_hash:
            blobs.setdefault(meta_hash, Blob(meta_hash, repo_root / (path + ".meta"), int(entry.get("metaSize", 0))))
    return blobs


def plan(blobs: dict[str, Blob], existing: set[str]) -> list[Blob]:
    return [blob for digest, blob in sorted(blobs.items()) if digest not in existing]


def _check(blob: Blob, copy_to: Path | None) -> str | None:
    if not blob.source.is_file():
        return f"見つかりません: {blob.source}"

    digest = hashlib.sha256()
    out = copy_to.open("wb") if copy_to is not None else None
    try:
        with blob.source.open("rb") as src:
            while chunk := src.read(_CHUNK):
                digest.update(chunk)
                if out is not None:
                    out.write(chunk)
    finally:
        if out is not None:
            out.close()

    if digest.hexdigest() == blob.hash:
        return None
    if copy_to is not None:
        copy_to.unlink()
    return f"マニフェストと中身が違います (build 後に変更された?): {blob.source}"


def stage(blobs: list[Blob], staging_dir: Path) -> list[str]:
    """``staging_dir/<hash>`` へコピーしながらハッシュを取り直し、食い違ったものを返す。

    build のあとでファイルを編集すると、名前 (ハッシュ) と中身がずれたブロブを上げてしまう。
    不変ストレージでそれをやると取り返しがつかないので、実際に上げるバイト列で検証する。
    """
    problems = []
    for blob in blobs:
        problem = _check(blob, staging_dir / blob.hash)
        if problem:
            problems.append(problem)
    return problems


def verify(blobs: list[Blob]) -> list[str]:
    """dry-run 用。コピーせずにハッシュだけ取り直す。"""
    return [problem for blob in blobs if (problem := _check(blob, None))]


class Rclone:
    def __init__(self, exe: str, remote: str) -> None:
        self.exe = exe
        self.remote = remote.rstrip("/")

    def _run(self, args: list[str], capture: bool) -> subprocess.CompletedProcess:
        try:
            return subprocess.run([self.exe, *args], capture_output=capture, text=True,
                                  encoding="utf-8", errors="replace", check=False)
        except FileNotFoundError as error:
            raise UploadError(f"rclone が見つかりません: {self.exe}") from error

    def list_blob_hashes(self) -> set[str]:
        result = self._run(["lsf", f"{self.remote}/files", "--files-only"], capture=True)
        if result.returncode == _RCLONE_DIR_NOT_FOUND:
            return set()
        if result.returncode != 0:
            raise UploadError("files/ の一覧を取れませんでした: " + result.stderr.strip())
        return {line.strip() for line in result.stdout.splitlines() if line.strip()}

    def upload_blobs(self, staging_dir: Path) -> None:
        result = self._run([
            "copy", str(staging_dir), f"{self.remote}/files",
            "--immutable",
            "--transfers", "16",
            "--checkers", "16",
            "--header-upload", BLOB_CACHE_CONTROL,
            "--stats", "5s", "--stats-one-line", "--stats-log-level", "NOTICE",
        ], capture=False)
        if result.returncode != 0:
            raise UploadError("ブロブのアップロードに失敗しました (上のログを参照)")

    def root_file_names(self) -> set[str]:
        result = self._run(["lsf", self.remote, "--files-only"], capture=True)
        if result.returncode == _RCLONE_DIR_NOT_FOUND:
            return set()
        if result.returncode != 0:
            raise UploadError("remote の一覧を取れませんでした: " + result.stderr.strip())
        return {line.strip() for line in result.stdout.splitlines() if line.strip()}

    def read_file(self, remote_name: str) -> bytes:
        try:
            result = subprocess.run([self.exe, "cat", f"{self.remote}/{remote_name}"], capture_output=True, check=False)
        except FileNotFoundError as error:
            raise UploadError(f"rclone が見つかりません: {self.exe}") from error
        if result.returncode != 0:
            raise UploadError(f"{remote_name} を読めませんでした: " + result.stderr.decode("utf-8", "replace").strip())
        return result.stdout

    def upload_file(self, local: Path, remote_name: str, header: str) -> None:
        result = self._run(["copyto", str(local), f"{self.remote}/{remote_name}", "--header-upload", header], capture=True)
        if result.returncode != 0:
            raise UploadError(f"{remote_name} を上げられませんでした: " + result.stderr.strip())

    def read_live_manifest(self) -> dict | None:
        """いま公開中の manifest.json。まだ一度もリリースしていなければ None"""
        if "manifest.json" not in self.root_file_names():
            return None
        return json.loads(self.read_file("manifest.json").decode("utf-8"))

    def versioned_conflicts(self, local: Path, remote_name: str) -> bool:
        """同名がすでにあり、中身が違うなら True。

        rclone の ``--immutable`` は ``copyto`` (1 ファイル) では効かず、中身が違っても
        上書きしてしまうので、ここで自前で確かめる。
        """
        if remote_name not in self.root_file_names():
            return False
        return self.read_file(remote_name) != local.read_bytes()

    def upload_file_once(self, local: Path, remote_name: str, header: str) -> bool:
        """一度置いたら中身を変えさせない。同じ中身なら何もせず False、無ければ上げて True。"""
        if remote_name in self.root_file_names():
            if self.read_file(remote_name) == local.read_bytes():
                return False
            raise UploadError(f"{remote_name} はすでに別の内容で存在します。--version を上げて build し直してください")
        self.upload_file(local, remote_name, header)
        return True
