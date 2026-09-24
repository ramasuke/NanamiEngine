"""Self-test / correctness gate for tools.dist.

Run:  python tools/dist/selftest.py       (from repo root)
      python -m tools.dist selftest

Exit 0 = all good, 1 = failure. No third-party dependencies.

Stages:
  0. JSON の形が Packages/AssetUpdater の AssetManifest::TryParse と一致する
     (キー名がずれるとクライアントが黙って 0 件差分になる)。
  1. 除外ルール: 開発専用だけが落ち、随伴ファイル (.efkmodel / .fbm 内テクスチャ /
     .pso / .vso) は残る。
  2. 実在する .meta から guid を読める (痩せたプロキシと太った ScriptableObject の両方)。
  3. scan: アセット / 随伴ファイルの振り分けとハッシュ・サイズ。
  4. diff: 新規 / 変更 / 削除 とダウンロード量が C++ の ManifestDiff::Between と同じ規則。
  5. HashCache: 変更が無ければ再ハッシュしない / 中身が変われば無効化される。
  6. upload: 本体と .meta がそれぞれブロブになり、同じ中身は 1 つにまとまる。remote に無い
     ものだけを上げ、build 後に書き換わったファイルは上げない (rclone は呼ばない)。
  7. 参照チェック: .efkefc / .mv1 が Assets/ の外や配信対象外の実在ファイルを参照していたら
     報告し、配信されるもの・どこにも無いものは報告しない。参照リストは中身のハッシュでキャッシュされる。
"""

from __future__ import annotations

import json
import struct
import sys
import tempfile
import traceback
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.dist import manifest as mf  # noqa: E402
from tools.dist import refs  # noqa: E402
from tools.dist import upload as up  # noqa: E402
from tools.dist.config import DistConfig  # noqa: E402


class Reporter:
    def __init__(self) -> None:
        self.failed = 0
        self.passed = 0

    def ok(self, name: str) -> None:
        self.passed += 1
        print(f"  PASS  {name}")

    def fail(self, name: str, err: str) -> None:
        self.failed += 1
        lines = err.strip().splitlines()
        print(f"  FAIL  {name}\n        {lines[0] if lines else err}")
        for line in lines[1:]:
            print(f"        {line}")

    def skip(self, name: str, why: str) -> None:
        print(f"  SKIP  {name} ({why})")

    def section(self, title: str) -> None:
        print(f"\n=== {title} ===")

    def finish(self) -> int:
        total = self.passed + self.failed
        print(f"\n{self.passed}/{total} checks passed"
              + (f", {self.failed} FAILED" if self.failed else ""))
        return 1 if self.failed else 0


# The exact keys Packages/AssetUpdater/Manifest/AssetManifest.cpp reads.
ROOT_KEYS = {"schema", "version", "requiredClientVersion", "baseUrl", "entries"}
ENTRY_KEYS = {"guid", "path", "hash", "size", "metaHash", "metaSize"}


def _make_tree(root: Path) -> None:
    """Assets/ の実際の形を小さく再現する。"""
    def write(rel: str, data: bytes) -> None:
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def write_meta(rel: str, guid: str) -> None:
        body = {
            "value0": {
                "polymorphic_id": 2147483649,
                "polymorphic_name": "NanamiEngine::Module::Asset::Mv1File",
                "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                        "cereal_class_version": 1,
                        "value0": {"cereal_class_version": 0},
                        "contentPath_": rel.replace("/", "\\\\"),
                        "guid_": {"cereal_class_version": 0, "value_": guid},
                    },
                },
            }
        }
        write(rel + ".meta", json.dumps(body, indent=4).encode("utf-8"))

    # アセット: 本体 + .meta
    write("Assets/Art/Models/Hyena.mv1", b"hyena-model")
    write_meta("Assets/Art/Models/Hyena.mv1", "11111111-1111-1111-1111-111111111111")
    write("Assets/Audio/Howl.mp3", b"howl-sound")
    write_meta("Assets/Audio/Howl.mp3", "22222222-2222-2222-2222-222222222222")

    # 随伴ファイル: .meta 無しだが実行時に要る
    write("Assets/Art/Models/Hyena.fbm/Hyena_Diffuse.png", b"hyena-texture")
    write("Assets/Art/Effect/Foo/Model/rock1.efkmodel", b"efk-model")
    write("Assets/Art/Shaders/Tree/Tree_VS.vso", b"compiled-shader")

    # 開発専用: 落ちるべきもの
    write("Assets/Art/Models/Hyena.fbx", b"source-model")
    write("Assets/Art/Effect/_Source/Foo/Foo.efkproj", b"source-effect")
    write("Assets/Art/Effect/_Source/Foo/Model/rock1.efkmodel", b"source-companion")
    write("Assets/Scripts/Core/Game/Game.cpp", b"code")
    write("Assets/Scripts/Core/Game/Game.h", b"code")
    write("Assets/Art/Models/desktop.ini", b"junk")
    write("Assets/Data/Thing.swordManResource.meta.bak", b"backup")


def stage_json_shape(r: Reporter) -> None:
    r.section("stage 0: JSON shape matches the C++ reader")
    try:
        entry = mf.Entry(path="Assets/a.mv1", hash="deadbeef", size=10,
                         guid="G", meta_hash="cafe", meta_size=2)
        assert set(entry.to_json()) == ENTRY_KEYS, entry.to_json()
        r.ok("entry keys are exactly guid/path/hash/size/metaHash/metaSize")

        result = mf.ScanResult(entries=[entry])
        document = mf.build(result, "1.1.0", "1.0.0", "https://host/1.1.0/")
        assert ROOT_KEYS.issubset(set(document)), document.keys()
        assert document["schema"] == mf.SCHEMA
        assert document["entries"][0]["metaSize"] == 2
        r.ok("root keys include schema/version/requiredClientVersion/baseUrl/entries")

        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "manifest.json"
            mf.dump(document, out)
            raw = out.read_bytes()
            assert not raw.startswith(b"\xef\xbb\xbf"), "BOM があると rapidjson が困る"
            assert b"\r\n" not in raw, "CRLF ではなく LF で書く"
            assert mf.load(out) == document
            r.ok("dump is UTF-8 / LF / no BOM and round-trips")
    except Exception:  # noqa: BLE001
        r.fail("json shape", traceback.format_exc())


def stage_exclusions(r: Reporter) -> None:
    r.section("stage 1: exclusion rules")
    shipped = [
        "Assets/Art/Models/Hyena.mv1",
        "Assets/Art/Models/Hyena.fbm/Hyena_Diffuse.png",
        "Assets/Art/Effect/Foo/Model/rock1.efkmodel",
        "Assets/Art/Shaders/Tree/Tree_VS.vso",
        "Assets/Art/Shaders/Tree/Tree_PS.pso",
        "Assets/Art/Models/Dragon/M_Body.mat",
        # 同名の原本が _Source/ にあっても、.mv1 が参照するのは隣のこちら
        "Assets/Art/Models/Prop/EventBoard/EventNoticeBoard_Wood.png",
        # 名前に "source" を含むだけのディレクトリは対象外
        "Assets/Art/UI/SourceFrame/Frame.png",
    ]
    dropped = [
        "Assets/Art/Models/Hyena.mv1.meta",
        "Assets/Art/Models/Hyena.fbx",
        "Assets/Art/Effect/_Source/Foo/Foo.efkproj",
        "Assets/Art/Effect/_Source/Foo/Model/rock1.efkmodel",
        "Assets/Art/Models/Nature/Trees/_Source/Trees.blend",
        "Assets/Art/Models/Prop/EventBoard/_Source/EventNoticeBoard_Wood.png",
        "Assets/Art/UI/StageSelect/_Source/SelectFrameGlow_Source.png",
        "Assets/Art/Models/Somewhere/Model.blend",
        "Assets/Art/Models/Somewhere/Model.blend1",
        "Assets/Scripts/Core/Game/Game.cpp",
        "Assets/Scripts/Core/Game/Game.h",
        "Assets/Art/Models/desktop.ini",
        "Assets/Data/Thing.swordManResource.meta.bak",
    ]
    try:
        for path in shipped:
            assert not mf.is_excluded(path), f"配信すべきなのに除外された: {path}"
        r.ok(f"{len(shipped)} runtime files are kept (companions included)")
        for path in dropped:
            assert mf.is_excluded(path), f"除外すべきなのに残った: {path}"
        r.ok(f"{len(dropped)} dev-only files are dropped")
    except Exception:  # noqa: BLE001
        r.fail("exclusions", traceback.format_exc())


def stage_real_guid(r: Reporter) -> None:
    r.section("stage 2: guid read from committed .meta fixtures")
    thin = _REPO / "Assets/Brute.mv1.meta"
    fat = _REPO / "Assets/Data/PlayerAvatar/InitStatus/SwordMan/SwordManInitStatus.swordManInitStatus.meta"

    if thin.exists():
        try:
            guid, encoding = mf.read_guid(thin)
            assert guid == "6231DCB3-3256-41FC-9276-3998037AA2A1", guid
            assert encoding == "utf-8", encoding
            r.ok("thin proxy (Mv1File) guid")
        except Exception:  # noqa: BLE001
            r.fail("thin proxy guid", traceback.format_exc())
    else:
        r.skip("thin proxy guid", "Assets/Brute.mv1.meta が無い")

    if fat.exists():
        try:
            guid, _ = mf.read_guid(fat)
            assert len(guid) == 36 and guid.count("-") == 4, guid
            r.ok("fat ScriptableObject (SwordManInitStatus) guid")
        except Exception:  # noqa: BLE001
            r.fail("fat ScriptableObject guid", traceback.format_exc())
    else:
        r.skip("fat ScriptableObject guid", "fixture が無い")

    try:
        with tempfile.TemporaryDirectory() as tmp:
            # UTF-8 化以前のエンジンが書いた .meta は CP932 のまま残っている。
            # contentPath_ の日本語が UTF-8 として不正なので、素朴に読むと落ちる。
            body = (
                '{"value0":{"ptr_wrapper":{"data":{'
                '"contentPath_":"Assets\\\\Audio\\\\ドラゴンの鳴き声1.mp3",'
                '"guid_":{"value_":"AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"}}}}}'
            )
            cp932 = Path(tmp) / "sjis.meta"
            cp932.write_bytes(body.encode("cp932"))
            guid, encoding = mf.read_guid(cp932)
            assert guid == "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", guid
            assert encoding == "cp932", encoding
        r.ok("CP932 .meta still yields its guid (72 such files are committed)")
    except Exception:  # noqa: BLE001
        r.fail("CP932 .meta", traceback.format_exc())

    try:
        with tempfile.TemporaryDirectory() as tmp:
            broken = Path(tmp) / "broken.meta"
            broken.write_text("{ this is not json", encoding="utf-8")
            assert mf.read_guid(broken) == ("", "")
        r.ok("a broken .meta yields '' instead of raising")
    except Exception:  # noqa: BLE001
        r.fail("broken .meta", traceback.format_exc())


def stage_scan(r: Reporter) -> None:
    r.section("stage 3: scan splits assets from companions")
    try:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _make_tree(root)
            result = mf.scan(root / "Assets", root, mf.HashCache(None))

            paths = sorted(e.path for e in result.entries)
            assert paths == [
                "Assets/Art/Effect/Foo/Model/rock1.efkmodel",
                "Assets/Art/Models/Hyena.fbm/Hyena_Diffuse.png",
                "Assets/Art/Models/Hyena.mv1",
                "Assets/Art/Shaders/Tree/Tree_VS.vso",
                "Assets/Audio/Howl.mp3",
            ], paths
            r.ok("only runtime files become entries")

            assert result.asset_count == 2, result.asset_count
            assert result.companion_count == 3, result.companion_count
            r.ok("2 assets (with .meta) / 3 companions (without)")

            hyena = next(e for e in result.entries if e.path.endswith("Hyena.mv1"))
            assert hyena.guid == "11111111-1111-1111-1111-111111111111", hyena.guid
            assert hyena.size == len(b"hyena-model")
            assert hyena.meta_size > 0 and hyena.meta_hash
            r.ok("asset entry carries guid + metaHash + metaSize")

            texture = next(e for e in result.entries if e.path.endswith("Hyena_Diffuse.png"))
            assert texture.guid == "" and texture.meta_hash == "" and texture.meta_size == 0
            assert texture.total_size == texture.size
            r.ok("companion entry has no guid/meta and is identified by path")

            assert not result.missing_guid, result.missing_guid
            r.ok("no .meta failed to yield a guid")
    except Exception:  # noqa: BLE001
        r.fail("scan", traceback.format_exc())


def stage_diff(r: Reporter) -> None:
    r.section("stage 4: diff matches the C++ ManifestDiff::Between rules")
    try:
        def doc(entries: list[dict]) -> dict:
            return {"version": "x", "entries": entries}

        def entry(path: str, h: str, mh: str = "", size: int = 100, msize: int = 0) -> dict:
            return {"guid": "", "path": path, "hash": h, "size": size,
                    "metaHash": mh, "metaSize": msize}

        installed = doc([
            entry("same.mv1", "aaa", "m1"),
            entry("body-changed.mv1", "bbb", "m2"),
            entry("meta-changed.mv1", "ccc", "m3"),
            entry("gone.mv1", "ddd", "m4"),
        ])
        remote = doc([
            entry("same.mv1", "aaa", "m1"),
            entry("body-changed.mv1", "BBB", "m2"),
            entry("meta-changed.mv1", "ccc", "M3"),
            entry("new.mv1", "eee", "m5", size=50, msize=7),
        ])

        result = mf.diff(installed, remote)
        assert [e.path for e in result.added] == ["new.mv1"], result.added
        assert sorted(e.path for e in result.changed) == ["body-changed.mv1", "meta-changed.mv1"]
        assert result.removed_paths == ["gone.mv1"], result.removed_paths
        r.ok("added / changed (body or meta) / removed")

        assert result.download_bytes == 100 + 100 + 57, result.download_bytes
        r.ok("download bytes count body + meta of added and changed only")

        assert not result.is_up_to_date
        assert mf.diff(installed, installed).is_up_to_date
        r.ok("identical manifests are up to date")

        # installed.json が無いときは全件が新規、が C++ 側の初回挙動
        fresh = mf.diff({"entries": []}, remote)
        assert len(fresh.added) == 4 and not fresh.changed and not fresh.removed_paths
        r.ok("empty installed state makes every entry 'added'")
    except Exception:  # noqa: BLE001
        r.fail("diff", traceback.format_exc())


def stage_hash_cache(r: Reporter) -> None:
    r.section("stage 5: hash cache")
    try:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _make_tree(root)
            cache_path = root / "cache.json"

            first = mf.HashCache(cache_path)
            before = mf.scan(root / "Assets", root, first)
            first.save()
            assert first.misses > 0 and first.hits == 0
            r.ok(f"cold run hashes everything ({first.misses} files)")

            second = mf.HashCache(cache_path)
            after = mf.scan(root / "Assets", root, second)
            assert second.misses == 0, f"{second.misses} files were re-hashed"
            assert second.hits == first.misses
            r.ok("warm run re-hashes nothing")

            assert [e.hash for e in before.entries] == [e.hash for e in after.entries]
            r.ok("cached hashes equal freshly computed ones")

            changed = root / "Assets/Art/Models/Hyena.mv1"
            changed.write_bytes(b"hyena-model-v2")
            third = mf.HashCache(cache_path)
            result = mf.scan(root / "Assets", root, third)
            new_hash = next(e.hash for e in result.entries if e.path.endswith("Hyena.mv1"))
            old_hash = next(e.hash for e in before.entries if e.path.endswith("Hyena.mv1"))
            assert new_hash != old_hash
            assert third.misses >= 1
            r.ok("a rewritten file invalidates its cache entry")
    except Exception:  # noqa: BLE001
        r.fail("hash cache", traceback.format_exc())


def stage_upload(r: Reporter) -> None:
    r.section("stage 6: upload plan and staging")
    try:
        root = Path("R")
        document = {"entries": [
            {"path": "Assets/a.mv1", "hash": "h1", "size": 10, "metaHash": "m1", "metaSize": 2},
            {"path": "Assets/b.mv1", "hash": "h1", "size": 10, "metaHash": "m2", "metaSize": 3},
            {"path": "Assets/c.png", "hash": "h3", "size": 5, "metaHash": "", "metaSize": 0},
        ]}
        blobs = up.blobs_of(document, root)
        assert set(blobs) == {"h1", "m1", "m2", "h3"}, set(blobs)
        r.ok("body and .meta are separate blobs; identical content collapses; companions have no meta blob")

        assert blobs["m1"].source == root / "Assets/a.mv1.meta"
        r.ok(".meta blob is read from <path>.meta")

        todo = up.plan(blobs, {"h1", "m2"})
        assert [b.hash for b in todo] == ["h3", "m1"], [b.hash for b in todo]
        r.ok("only hashes missing on the remote are planned")
    except Exception:  # noqa: BLE001
        r.fail("upload plan", traceback.format_exc())

    try:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _make_tree(root)
            result = mf.scan(root / "Assets", root, mf.HashCache(None))
            document = mf.build(result, "1.0.0", "1.0.0", "https://x/files/")
            blobs = up.blobs_of(document, root)
            todo = up.plan(blobs, set())

            staging = root / "staging"
            staging.mkdir()
            assert up.stage(todo, staging) == []
            staged = sorted(p.name for p in staging.iterdir())
            assert staged == sorted(blobs), staged
            for path in staging.iterdir():
                assert mf.sha256_file(path) == path.name, path.name
            r.ok(f"every staged blob is named by its own SHA-256 ({len(staged)} blobs)")

            (root / "Assets/Audio/Howl.mp3").write_bytes(b"edited after build")
            howl_hash = next(e.hash for e in result.entries if e.path.endswith("Howl.mp3"))

            staging_again = root / "staging_again"
            staging_again.mkdir()
            problems = up.stage(todo, staging_again)
            assert len(problems) == 1 and "Howl.mp3" in problems[0], problems
            assert not (staging_again / howl_hash).exists()
            r.ok("a file edited after build is reported and never staged")

            assert len(up.verify(todo)) == 1
            r.ok("dry-run verify reports the same mismatch without copying")
    except Exception:  # noqa: BLE001
        r.fail("upload staging", traceback.format_exc())

    try:
        with_slash = DistConfig(remote="r2:b", public_base_url="https://pub-x.r2.dev/", rclone="rclone")
        without_slash = DistConfig(remote="r2:b", public_base_url="https://pub-x.r2.dev", rclone="rclone")
        assert with_slash.files_base_url == "https://pub-x.r2.dev/files/"
        assert without_slash.files_base_url == with_slash.files_base_url
        assert with_slash.manifest_url == "https://pub-x.r2.dev/manifest.json"
        assert DistConfig(remote="", public_base_url="", rclone="rclone").files_base_url == ""
        r.ok("baseUrl is <public>/files/ with or without a trailing slash")
    except Exception:  # noqa: BLE001
        r.fail("config urls", traceback.format_exc())


def _efkefc_bytes(paths: list[str]) -> bytes:
    """EFKE ヘッダ + 1710 形式の INFO (dependency list) だけの .efkefc。"""
    info = struct.pack("<ii", 1710, len(paths))
    for p in paths:
        info += struct.pack("<iii", 0, 0, len(p) + 1) + (p + "\0").encode("utf-16-le")
    return b"EFKE" + struct.pack("<i", 0) + b"INFO" + struct.pack("<I", len(info)) + info


def _mv1_bytes(paths: list[str]) -> bytes:
    """圧縮なし (キーバイト 0xFF が本文に現れない) の LZ ストリームを持つ .mv1。"""
    body = b"MV1-body\x00" + b"".join(p.encode("utf-8") + b"\x00" for p in paths)
    return b"MV11" + struct.pack("<IIB", len(body), 9 + len(body), 0xFF) + body


def _make_ref_tree(root: Path) -> None:
    def write(rel: str, data: bytes) -> None:
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    write("Outside/tex.png", b"desktop-only")
    write("Assets/Art/Effect/Foo/Texture/ok.png", b"shipped")
    write("Assets/Art/Effect/_Source/Foo/src.png", b"source-only")
    write("Assets/Art/Effect/Foo/Foo.efkefc", _efkefc_bytes([
        "Texture/ok.png",
        "../../../../Outside/tex.png",
        "../_Source/Foo/src.png",
        "Texture/missing.png",
    ]))
    write("Assets/Art/Models/textures/ok.png", b"shipped")
    write("Assets/Art/Models/M.mv1", _mv1_bytes([
        "textures\\ok.png",
        "..\\..\\..\\Outside\\tex.png",
        "C:\\NoSuchSourceDir\\ok.png",
    ]))
    write("Assets/Art/Models/Broken.mv1", b"hyena-model")


def stage_refs(r: Reporter) -> None:
    r.section("stage 7: .efkefc / .mv1 referencing files that are not shipped")
    try:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            _make_ref_tree(root)
            cache_path = root / "cache.json"

            first = mf.HashCache(cache_path)
            result = mf.scan(root / "Assets", root, first)
            report = refs.find_unshipped_refs(result, root, root / "Assets", first)
            first.save()
            got = sorted((u.path, u.ref, u.reason) for u in report.unshipped)
            assert got == sorted([
                ("Assets/Art/Effect/Foo/Foo.efkefc", "../../../../Outside/tex.png", refs.REASON_OUTSIDE),
                ("Assets/Art/Effect/Foo/Foo.efkefc", "../_Source/Foo/src.png", refs.REASON_EXCLUDED),
                ("Assets/Art/Models/M.mv1", "..\\..\\..\\Outside\\tex.png", refs.REASON_OUTSIDE),
            ]), got
            r.ok("outside Assets/ and excluded targets are reported; shipped and missing ones are not")

            assert report.unreadable == ["Assets/Art/Models/Broken.mv1"], report.unreadable
            r.ok("an unparsable .mv1 is listed as unreadable instead of raising")

            assert first.ref_misses == 3 and first.ref_hits == 0, (first.ref_misses, first.ref_hits)
            second = mf.HashCache(cache_path)
            again = refs.find_unshipped_refs(mf.scan(root / "Assets", root, second), root, root / "Assets", second)
            assert second.ref_misses == 0 and second.ref_hits == 3, (second.ref_misses, second.ref_hits)
            assert sorted((u.path, u.ref, u.reason) for u in again.unshipped) == got
            assert again.unreadable == report.unreadable
            r.ok("warm run reads no references (cached by content hash, unreadable included)")

            legacy_path = root / "legacy.json"
            legacy_path.write_text(json.dumps(first._entries), encoding="utf-8")
            legacy = mf.HashCache(legacy_path)
            mf.scan(root / "Assets", root, legacy)
            assert legacy.misses == 0 and legacy.hits == first.misses, (legacy.misses, legacy.hits)
            r.ok("a cache file from before the refs cache still provides hashes")
    except Exception:  # noqa: BLE001
        r.fail("unshipped refs", traceback.format_exc())


def stage_locked_files(r: Reporter) -> None:
    r.section("stage 8: fonts can't be patched while the game runs")
    try:
        def doc(pairs: list[tuple[str, str]]) -> dict:
            return {"entries": [{"path": p, "hash": h} for p, h in pairs]}

        live = doc([("Assets/Art/Font/ipam.ttf", "f1"), ("Assets/Art/Font/onryou.ttf", "f2"),
                    ("Assets/Art/Font/old.otf", "f3"), ("Assets/a.png", "p1")])
        new = doc([("Assets/Art/Font/ipam.ttf", "f1"), ("Assets/Art/Font/onryou.ttf", "F2"),
                   ("Assets/Art/Font/added.ttf", "f4"), ("Assets/a.png", "P1")])
        changed = up.locked_file_changes(live, new)
        assert changed == ["Assets/Art/Font/old.otf", "Assets/Art/Font/onryou.ttf"], changed
        r.ok("changed and removed fonts are caught; added fonts and non-fonts are not")

        assert up.locked_file_changes(live, live) == []
        assert up.locked_file_changes({"entries": []}, new) == []
        r.ok("no change / first release -> nothing to refuse")

        assert up.is_newer_version("1.1.0", "1.0.0")
        assert up.is_newer_version("1.10.0", "1.9.0")
        assert up.is_newer_version("1.0.1", "1.0")
        assert not up.is_newer_version("1.0.0", "1.0.0")
        assert not up.is_newer_version("1.0", "1.0.0")
        assert not up.is_newer_version("", "")
        r.ok("version comparison is numeric per dot-separated part (1.10 > 1.9)")
    except Exception:  # noqa: BLE001
        r.fail("locked files", traceback.format_exc())


def main() -> int:
    r = Reporter()
    stage_json_shape(r)
    stage_exclusions(r)
    stage_real_guid(r)
    stage_scan(r)
    stage_diff(r)
    stage_hash_cache(r)
    stage_upload(r)
    stage_refs(r)
    stage_locked_files(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
