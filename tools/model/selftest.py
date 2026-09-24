"""tools.model のセルフテスト / 正しさのゲート。

実行:  python tools/model/selftest.py         (リポジトリルートから)
       python -m tools.model selftest

終了コード 0 = すべて OK、1 = 失敗。ステージ 1-8 はサードパーティ依存無し。
ステージ 9 (GUI 自動化) はできる範囲で行い、pywinauto、DxLibModelViewer の exe、
テスト用 .fbx が揃っていなければきれいにスキップする。

ステージ:
  1. 配布済みの実 Mv1File アセットに対する .meta の往復 (base_class_count=2 の形が
     成り立つことの証明。tools/common/meta_base.py の ParticleFile の修正と同じ種類)。
  2. 実際のネストしたアセットに対する content_path_for() の規則チェック
     (すべてバックスラッシュの contentPath_。tools/effect の ParticleFile
     バインディングと同じ)。
  3. install() の GUID 再利用の挙動。合成した "MV11" ヘッダのバイト列を使う
     (本物の DxLibModelViewer の出力は不要)。
  4. install --textures の一括コピー: 認識できる画像ファイルを
     <dest-dir>/textures/ にコピーし、画像でないファイルは無視し、元ディレクトリが
     無ければエラーにする。素朴なファイルシステムのコピー - サードパーティ依存無し。
  5. 配布済みの実 .mv1 に対する mv1.decode()/texture_paths(): デコード後のサイズが
     ヘッダと一致し、テクスチャ参照が中にあると分かっているものと一致する。
  6. テクスチャ収集 (convert/install --with-textures): サブフォルダ、*.fbm フォルダ、
     絶対パス、出力先の外にあるテクスチャを参照するリテラルのみの合成 .mv1 で、
     探索順、サブフォルダの維持、欠けたテクスチャの報告を確認する。
  7. mv1.encode(): 配布済みの実 .mv1 で decode(encode(body)) == body
     (かつ出力が DxLib のサイズの x1.25 以内) と、合成したエッジケース -
     空の本体、keycode のエスケープ、最大一致長を超える連続、3 バイトの
     インデックスが要る距離。
  8. 既知のモデルと Assets/ 以下の全 .mv1 での materials()、そして
     set-emissive: 対象マテリアルの自己発光 RGB だけが変わること、
     all/名前/インデックス指定が順に適用されること、不正な指定 / 未知の名前 /
     アニメーションのみのファイルは書き込まずに拒否されること、convert が
     --mode anim --emissive を拒否すること。
  9. できる範囲で、全保存モード (mesh/anim/full) で convert() を最初から最後まで:
     pywinauto、DxLibModelViewer の exe ($DXLIB_MODELVIEWER または
     cli.DEFAULT_MODELVIEWER_PATH)、テスト用 .fbx ($TOOLS_MODEL_TEST_FBX) が
     すべて揃っているときだけ実行する。それ以外ではスキップ - このツールキットには
     コミット済みの .fbx フィクスチャが無い (tools/model/README.md 参照)。
"""

from __future__ import annotations

import argparse
import os
import random
import struct
import sys
import tempfile
import traceback
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.model import cli, meta, mv1  # noqa: E402
from tools.common import cereal_json as cj  # noqa: E402

REAL_META_FIXTURES = [
    _REPO / "Assets" / "Art" / "Animation" / "Man" / "Death.mv1.meta",
    _REPO / "Assets" / "Art" / "Animation" / "Man" / "Jump.mv1.meta",
    _REPO / "Assets" / "Art" / "Models" / "Fantasy" / "DirtyHouse" / "dirtyHouse.mv1.meta",
    _REPO / "Assets" / "Art" / "Models" / "Monster" / "Hyenas" / "Hyenas_A4_AllMotion.mv1.meta",
]

_KEYCODE = 0xA3


def synthetic_mv1(body: bytes) -> bytes:
    """圧縮ストリームが ``body`` をリテラルだけで持つ正しい ``.mv1`` (keycode の
    バイトはすべて keycode,keycode とエスケープ) - 本物の LZ エンコーダ無しで
    ``mv1.decode`` を通すにはこれで十分。"""
    payload = body.replace(bytes([_KEYCODE]), bytes([_KEYCODE, _KEYCODE]))
    return b"MV11" + struct.pack("<IIB", len(body), 9 + len(payload), _KEYCODE) + payload


# ヘッダ + cli._MV1_MIN_SIZE を超えるパディング。エスケープを試すため keycode のバイトを 1 つ入れる
_SYNTHETIC_MV1 = synthetic_mv1(b"\x00" * 150 + bytes([_KEYCODE]) + b"\x00" * 150)

# 配布済みの実アセットに入っていると分かっているテクスチャ参照 (mv1.texture_paths の出力)。
REAL_TEXTURE_FIXTURES = {
    _REPO / "Assets" / "Art" / "Models" / "Fantasy" / "DirtyHouse" / "dirtyHouse.mv1": [
        "textures\\Wall_Roughness.png", "textures\\Roof_normal.png", "textures\\Wall_base.png",
        "textures\\Wood_normal.png", "textures\\Wood_base.png", "textures\\Roof_base.png",
    ],
    _REPO / "Assets" / "Art" / "Models" / "Road" / "BrickRoad.mv1": ["道のテクスチャ\\road block.png"],
    _REPO / "Assets" / "Art" / "Models" / "Basic" / "Cube.mv1": [],
    _REPO / "Assets" / "Art" / "Animation" / "Man" / "Jump.mv1": [],
}


class Reporter:
    def __init__(self) -> None:
        self.failed = 0
        self.passed = 0

    def ok(self, name: str) -> None:
        self.passed += 1
        print(f"  PASS  {name}")

    def fail(self, name: str, err: str) -> None:
        self.failed += 1
        lines = err.strip().splitlines() or [err]
        print(f"  FAIL  {name}\n        {lines[0]}")
        for line in lines[1:]:
            print(f"        {line}")

    def section(self, title: str) -> None:
        print(f"\n=== {title} ===")

    def finish(self) -> int:
        total = self.passed + self.failed
        print(f"\n{self.passed}/{total} checks passed"
              + (f", {self.failed} FAILED" if self.failed else ""))
        return 1 if self.failed else 0


def stage_meta_roundtrip(r: Reporter) -> None:
    r.section("stage 1: .meta round trip (real shipped Mv1File assets)")
    for path in REAL_META_FIXTURES:
        try:
            if not path.exists():
                r.ok(f"{path.name} (skipped: not present)")
                continue
            orig = cj.loads(cj.read_text(path))
            info = meta.read_meta(path)
            rt = cj.loads(meta.render_meta(info["name"], info["guid"], info["content_path"]))
            if cj.dumps(rt) != cj.dumps(orig):
                raise AssertionError("re-rendered .meta does not match the original byte-for-byte")
            r.ok(path.relative_to(_REPO).as_posix())
        except Exception:  # noqa: BLE001
            r.fail(path.name, traceback.format_exc())


def stage_content_path_convention(r: Reporter) -> None:
    r.section("stage 2: content_path_for() convention check")
    try:
        target_dir = _REPO / "Assets" / "Art" / "Animation" / "Man"
        got = meta.content_path_for("Test", target_dir, _REPO)
        want = "Assets\\Art\\Animation\\Man\\Test.mv1"
        if got != want:
            raise AssertionError(f"content_path_for() = {got!r}, expected {want!r}")
        r.ok("content_path_for() matches real Assets/Art/Animation/Man siblings (all-backslash)")
    except Exception:  # noqa: BLE001
        r.fail("content_path_for() convention", traceback.format_exc())


def stage_install_guid_reuse(r: Reporter) -> None:
    r.section("stage 3: install GUID reuse + looks_like_mv1() (synthetic MV11 header)")
    try:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src.mv1"
            src.write_bytes(_SYNTHETIC_MV1)
            dest = Path(tmp) / "out" / "Test.mv1"
            ns = argparse.Namespace(mv1=str(src), source=None, textures=None, dest=str(dest), with_textures=False)
            cli.cmd_install(ns)
            meta_path = dest.with_suffix(dest.suffix + ".meta")
            first = meta.read_meta(meta_path)
            meta_bytes = meta_path.read_bytes()
            cli.cmd_install(ns)
            second = meta.read_meta(meta_path)
            if first["guid"] != second["guid"] or meta_path.read_bytes() != meta_bytes:
                raise AssertionError("re-install re-minted the GUID / rewrote the .meta")
            if cli.looks_like_mv1(dest):
                raise AssertionError(f"looks_like_mv1() flagged a well-formed synthetic file: "
                                      f"{cli.looks_like_mv1(dest)}")
        r.ok("install keeps an existing asset's .meta/GUID on re-install; looks_like_mv1() accepts MV11 header")
    except Exception:  # noqa: BLE001
        r.fail("install GUID reuse", traceback.format_exc())

    try:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            bad = Path(tmp) / "bad.mv1"
            bad.write_bytes(b"NOPE")
            problems = cli.looks_like_mv1(bad)
            if not problems:
                raise AssertionError("looks_like_mv1() accepted a bogus header/size")
        r.ok("looks_like_mv1() rejects a bogus header/size")
    except Exception:  # noqa: BLE001
        r.fail("looks_like_mv1() negative case", traceback.format_exc())


def stage_texture_copy(r: Reporter) -> None:
    r.section("stage 4: install --textures bulk-copy")
    try:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            src_dir = Path(tmp) / "textures_src"
            src_dir.mkdir()
            (src_dir / "Diffuse.png").write_bytes(b"not a real png, content doesn't matter")
            (src_dir / "Normal.PNG").write_bytes(b"case-insensitive extension check")
            (src_dir / "notes.txt").write_bytes(b"should not be copied")
            sub_dir = src_dir / "nested"
            sub_dir.mkdir()
            (sub_dir / "Deep.png").write_bytes(b"should not be copied either - no recursion")

            mv1_src = Path(tmp) / "src.mv1"
            mv1_src.write_bytes(_SYNTHETIC_MV1)
            dest = Path(tmp) / "out" / "Test.mv1"
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=str(src_dir), dest=str(dest), with_textures=False)
            cli.cmd_install(ns)

            textures_dest = dest.parent / "textures"
            copied = sorted(p.name for p in textures_dest.iterdir()) if textures_dest.is_dir() else []
            if copied != ["Diffuse.png", "Normal.PNG"]:
                raise AssertionError(f"copied {copied!r}, expected exactly ['Diffuse.png', 'Normal.PNG'] "
                                      "(non-image and nested files must be excluded)")
        r.ok("install --textures copies only top-level recognized image files")
    except Exception:  # noqa: BLE001
        r.fail("install --textures bulk-copy", traceback.format_exc())

    try:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            empty_dir = Path(tmp) / "empty"
            empty_dir.mkdir()
            mv1_src = Path(tmp) / "src.mv1"
            mv1_src.write_bytes(_SYNTHETIC_MV1)
            dest = Path(tmp) / "out" / "Test.mv1"
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=str(empty_dir), dest=str(dest), with_textures=False)
            cli.cmd_install(ns)  # 例外にならないこと - 画像ゼロは警告であってエラーではない
            textures_dest = dest.parent / "textures"
            if textures_dest.exists() and any(textures_dest.iterdir()):
                raise AssertionError("an empty source directory should not have produced any output files")
        r.ok("install --textures on an image-less directory warns but does not fail")
    except Exception:  # noqa: BLE001
        r.fail("install --textures empty source dir", traceback.format_exc())

    try:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            mv1_src = Path(tmp) / "src.mv1"
            mv1_src.write_bytes(_SYNTHETIC_MV1)
            dest = Path(tmp) / "out" / "Test.mv1"
            missing_dir = Path(tmp) / "does_not_exist"
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=str(missing_dir), dest=str(dest), with_textures=False)
            try:
                cli.cmd_install(ns)
            except cli.CliError:
                pass
            else:
                raise AssertionError("a missing --textures directory should raise CliError")
        r.ok("install --textures rejects a missing source directory")
    except Exception:  # noqa: BLE001
        r.fail("install --textures missing source dir", traceback.format_exc())


def stage_mv1_decode(r: Reporter) -> None:
    r.section("stage 5: mv1.decode() / texture_paths() (real shipped .mv1 assets)")
    for path, want in REAL_TEXTURE_FIXTURES.items():
        name = path.relative_to(_REPO).as_posix()
        try:
            if not path.exists():
                r.ok(f"{name} (skipped: not present)")
                continue
            data = path.read_bytes()
            declared = struct.unpack_from("<I", data, 4)[0]
            body = mv1.decode(data)
            if len(body) != declared:
                raise AssertionError(f"decoded {len(body)} bytes, header declares {declared}")
            got = mv1.texture_paths(path)
            if got != want:
                raise AssertionError(f"texture_paths() = {got!r}, expected {want!r}")
            if cli.looks_like_mv1(path):
                raise AssertionError(f"looks_like_mv1() flagged a real asset: {cli.looks_like_mv1(path)}")
            r.ok(f"{name}: {declared} bytes decoded, {len(got)} texture reference(s)")
        except Exception:  # noqa: BLE001
            r.fail(name, traceback.format_exc())

    try:
        truncated = synthetic_mv1(b"x" * 400)[:-10]
        try:
            mv1.decode(truncated)
        except ValueError:
            pass
        else:
            raise AssertionError("decode() accepted a truncated stream")
        with tempfile.TemporaryDirectory() as tmp:
            bad = Path(tmp) / "truncated.mv1"
            bad.write_bytes(truncated)
            if not cli.looks_like_mv1(bad):
                raise AssertionError("looks_like_mv1() accepted a truncated stream")
        r.ok("decode()/looks_like_mv1() reject a truncated compressed stream")
    except Exception:  # noqa: BLE001
        r.fail("truncated stream", traceback.format_exc())


def stage_texture_collection(r: Reporter) -> None:
    r.section("stage 6: --with-textures texture collection (synthetic .mv1)")
    refs = [
        "textures\\Base.png",           # 相対のサブフォルダ。元ディレクトリにそのまま存在する
        "Model.fbm\\Normal.png",        # .fbm 名がディスク上と違う -> *.fbm の glob で見つかる
        "C:\\Artist\\Machine\\Base.png",  # 絶対パス。同名の相対参照でカバーされる
        "C:\\Artist\\Machine\\Solo.tga",  # 絶対パスのみ -> 出力先の直下に置かれる
        "Loose.jpg",                    # 素の名前。<src>/Loose.jpg にだけ存在する
        "..\\Outside.png",              # 出力先の外に出る -> 見つからないと報告
        "textures\\Missing.png",        # どこにも無い -> 見つからないと報告
    ]
    body = b"\x01\x02" + b"".join(r_.encode("utf-8") + b"\x00\x07" for r_ in refs) + b"\x00" * 64
    try:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            (src / "textures").mkdir(parents=True)
            (src / "textures" / "Base.png").write_bytes(b"base")
            (src / "Other_Name.fbm").mkdir()
            (src / "Other_Name.fbm" / "Normal.png").write_bytes(b"normal")
            (src / "Solo.tga").write_bytes(b"solo")
            (src / "Loose.jpg").write_bytes(b"loose")
            (Path(tmp) / "Outside.png").write_bytes(b"outside")
            mv1_src = src / "Model.mv1"
            mv1_src.write_bytes(synthetic_mv1(body))

            if mv1.texture_paths(mv1_src) != refs:
                raise AssertionError(f"texture_paths() = {mv1.texture_paths(mv1_src)!r}, expected {refs!r}")

            dest = Path(tmp) / "out" / "Model.mv1"
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=None, dest=str(dest),
                                     with_textures=True)
            try:
                cli.cmd_install(ns)
            except cli.CliError as e:
                msg = str(e)
                if "..\\Outside.png" not in msg or "textures\\Missing.png" not in msg or "2 texture(s)" not in msg:
                    raise AssertionError(f"missing-texture error doesn't list exactly the 2 unplaceable refs: {msg}")
            else:
                raise AssertionError("install --with-textures should fail when a referenced texture is missing")

            out = dest.parent
            placed = sorted(p.relative_to(out).as_posix() for p in out.rglob("*") if p.is_file())
            want = ["Loose.jpg", "Model.fbm/Normal.png", "Model.mv1", "Model.mv1.meta", "Solo.tga", "textures/Base.png"]
            if placed != want:
                raise AssertionError(f"dest contains {placed!r}, expected {want!r}")
            if (out / "Model.fbm" / "Normal.png").read_bytes() != b"normal":
                raise AssertionError("Model.fbm/Normal.png was not copied from the *.fbm fallback")
        r.ok("install --with-textures: relative/sub-folder, *.fbm fallback, absolute, bare name, missing + escaping refs")
    except Exception:  # noqa: BLE001
        r.fail("--with-textures collection", traceback.format_exc())

    try:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            (src / "textures").mkdir(parents=True)
            (src / "textures" / "Base.png").write_bytes(b"base")
            mv1_src = src / "Model.mv1"
            mv1_src.write_bytes(synthetic_mv1(b"\x01textures\\Base.png\x00" + b"\x00" * 300))
            dest = Path(tmp) / "out" / "Model.mv1"
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=None, dest=str(dest),
                                     with_textures=True)
            cli.cmd_install(ns)
            if (dest.parent / "textures" / "Base.png").read_bytes() != b"base":
                raise AssertionError("textures/Base.png was not placed next to the installed .mv1")
            cli.cmd_install(ns)  # 再インストール: テクスチャは配置済み。失敗してはいけない
        r.ok("install --with-textures succeeds when every reference resolves (and on re-install)")
    except Exception:  # noqa: BLE001
        r.fail("--with-textures all-found", traceback.format_exc())

    try:
        ns = argparse.Namespace(file=str(_REPO / "tools" / "model" / "selftest.py"), out="unused.mv1",
                                 mode="anim", with_textures=True, emissive=[], force=False,
                                 modelviewer_path=None, timeout=1.0, debug_dir=None)
        try:
            cli.cmd_convert(ns)
        except cli.CliError as e:
            if "--mode anim" not in str(e):
                raise AssertionError(f"unexpected error: {e}")
        else:
            raise AssertionError("convert --mode anim --with-textures should be rejected")
        r.ok("convert rejects --mode anim --with-textures before launching anything")
    except Exception:  # noqa: BLE001
        r.fail("convert anim+textures rejection", traceback.format_exc())


ENCODE_FIXTURES = [
    _REPO / "Assets" / "Art" / "Models" / "Basic" / "Cube.mv1",
    _REPO / "Assets" / "Art" / "Models" / "Fantasy" / "DirtyHouse" / "dirtyHouse.mv1",
    _REPO / "Assets" / "Art" / "Models" / "Monster" / "Hyenas" / "Hyenas_A4_AllMotion.mv1",
]
_MAX_ENCODE_RATIO = 1.25

_CUBE_MV1 = _REPO / "Assets" / "Art" / "Models" / "Basic" / "Cube.mv1"
_DIRTY_HOUSE_MV1 = _REPO / "Assets" / "Art" / "Models" / "Fantasy" / "DirtyHouse" / "dirtyHouse.mv1"
_ANIM_ONLY_MV1 = _REPO / "Assets" / "Art" / "Animation" / "Man" / "Jump.mv1"


def stage_mv1_encode(r: Reporter) -> None:
    r.section("stage 7: mv1.encode() round trip")
    for path in ENCODE_FIXTURES:
        name = path.relative_to(_REPO).as_posix()
        try:
            if not path.exists():
                r.ok(f"{name} (skipped: not present)")
                continue
            data = path.read_bytes()
            body = mv1.decode(data)
            encoded = mv1.encode(body)
            if mv1.decode(encoded) != body:
                raise AssertionError("decode(encode(body)) != body")
            ratio = len(encoded) / len(data)
            if ratio > _MAX_ENCODE_RATIO:
                raise AssertionError(f"encoded {len(encoded)} bytes vs DxLib's {len(data)} "
                                      f"(x{ratio:.2f} > x{_MAX_ENCODE_RATIO}) - is the LZ search broken?")
            r.ok(f"{name}: round trip, {len(encoded)} bytes (x{ratio:.3f} of DxLib's output)")
        except Exception:  # noqa: BLE001
            r.fail(name, traceback.format_exc())

    rng = random.Random(1)
    far_block = rng.randbytes(70000)
    synthetic = {
        "empty body": b"",
        "3-byte body": b"abc",
        "every byte value (keycode escapes)": bytes(range(256)) * 8 + rng.randbytes(5000),
        "runs longer than the max match length": b"\x00" * 20000 + b"\x01" * 9000 + b"\x00" * 3,
        "repeat at a 3-byte distance": far_block + far_block,
    }
    for label, body in synthetic.items():
        try:
            encoded = mv1.encode(body)
            if mv1.decode(encoded) != body:
                raise AssertionError("decode(encode(body)) != body")
            r.ok(f"synthetic {label}: round trip ({len(body)} -> {len(encoded)} bytes)")
        except Exception:  # noqa: BLE001
            r.fail(f"synthetic {label}", traceback.format_exc())


def stage_materials(r: Reporter) -> None:
    r.section("stage 8: materials() / set-emissive")
    try:
        if not _CUBE_MV1.exists() or not _DIRTY_HOUSE_MV1.exists():
            r.ok("known material tables (skipped: fixtures not present)")
        else:
            cube = mv1.materials(mv1.decode(_CUBE_MV1.read_bytes()))
            if [m.name for m in cube] != ["Material"] or cube[0].emissive[:3] != (0.453125,) * 3:
                raise AssertionError(f"Cube.mv1 materials = {cube!r}")
            house = mv1.materials(mv1.decode(_DIRTY_HOUSE_MV1.read_bytes()))
            if [m.name for m in house] != ["Wall", "wood_1", "Roof"]:
                raise AssertionError(f"dirtyHouse.mv1 material names = {[m.name for m in house]!r}")
            r.ok("Cube.mv1 = [Material] (emissive 0.453125), dirtyHouse.mv1 = [Wall, wood_1, Roof]")
    except Exception:  # noqa: BLE001
        r.fail("known material tables", traceback.format_exc())

    try:
        files = 0
        count = 0
        for path in sorted((_REPO / "Assets").rglob("*.mv1")):
            try:
                count += len(mv1.materials(mv1.decode(path.read_bytes())))
            except ValueError as e:
                raise AssertionError(f"{path.relative_to(_REPO).as_posix()}: {e}") from e
            files += 1
        r.ok(f"every .mv1 under Assets/ has a readable material table ({files} files, {count} materials)")
    except Exception:  # noqa: BLE001
        r.fail("material table corpus", traceback.format_exc())

    if not _DIRTY_HOUSE_MV1.exists():
        r.ok("set-emissive (skipped: dirtyHouse.mv1 not present)")
        return

    def set_emissive(src: Path, out: Path | None, *specs: str) -> None:
        cli.cmd_set_emissive(argparse.Namespace(mv1=str(src), out=str(out) if out else None, emissive=list(specs)))

    try:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "dirtyHouse.mv1"
            src.write_bytes(_DIRTY_HOUSE_MV1.read_bytes())
            before = mv1.decode(src.read_bytes())
            set_emissive(src, None, "Roof=1,0.5,0.25")
            after = mv1.decode(src.read_bytes())
            table = mv1.materials(after)
            if table[2].emissive[:3] != (1.0, 0.5, 0.25):
                raise AssertionError(f"Roof emissive = {table[2].emissive!r}")
            roof = mv1.materials(before)[2].offset + 0x3C
            changed = [i for i in range(len(before)) if before[i] != after[i]]
            if len(before) != len(after) or any(not roof <= i < roof + 12 for i in changed):
                raise AssertionError(f"bytes outside Roof's emissive RGB changed: {changed[:10]}")
            if cli.looks_like_mv1(src):
                raise AssertionError(f"looks_like_mv1() flagged the patched file: {cli.looks_like_mv1(src)}")
        r.ok("set-emissive Roof=1,0.5,0.25 changes only Roof's emissive RGB (in place)")
    except Exception:  # noqa: BLE001
        r.fail("set-emissive by name", traceback.format_exc())

    try:
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "sub" / "out.mv1"
            set_emissive(_DIRTY_HOUSE_MV1, out, "0,0,0", "Roof=2,2,2", "1=0.5,0.5,0.5")
            got = [m.emissive[:3] for m in mv1.materials(mv1.decode(out.read_bytes()))]
            if got != [(0.0, 0.0, 0.0), (0.5, 0.5, 0.5), (2.0, 2.0, 2.0)]:
                raise AssertionError(f"emissive after all/name/index specs = {got!r}")
        r.ok("set-emissive --out: all-materials spec, later name/index specs override it")
    except Exception:  # noqa: BLE001
        r.fail("set-emissive spec order", traceback.format_exc())

    rejections = [
        ("unknown material name", _DIRTY_HOUSE_MV1, ["Glass=1,1,1"], "Roof"),
        ("two components", _DIRTY_HOUSE_MV1, ["1,1"], "R,G,B"),
        ("negative component", _DIRTY_HOUSE_MV1, ["1,-1,1"], ">= 0"),
        ("animation-only .mv1", _ANIM_ONLY_MV1, ["1,1,1"], "no materials"),
    ]
    for label, src, specs, needle in rejections:
        try:
            if not src.exists():
                r.ok(f"set-emissive rejects {label} (skipped: fixture not present)")
                continue
            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp) / "out.mv1"
                try:
                    set_emissive(src, out, *specs)
                except cli.CliError as e:
                    if needle not in str(e):
                        raise AssertionError(f"error does not mention {needle!r}: {e}")
                else:
                    raise AssertionError("expected CliError")
                if out.exists():
                    raise AssertionError("a rejected set-emissive still wrote its output")
            r.ok(f"set-emissive rejects {label}")
        except Exception:  # noqa: BLE001
            r.fail(f"set-emissive rejects {label}", traceback.format_exc())

    try:
        ns = argparse.Namespace(file=str(_REPO / "tools" / "model" / "selftest.py"), out="unused.mv1",
                                 mode="anim", with_textures=False, emissive=["1,1,1"], force=False,
                                 modelviewer_path=None, timeout=1.0, debug_dir=None)
        try:
            cli.cmd_convert(ns)
        except cli.CliError as e:
            if "--mode anim" not in str(e):
                raise AssertionError(f"unexpected error: {e}")
        else:
            raise AssertionError("convert --mode anim --emissive should be rejected")
        r.ok("convert rejects --mode anim --emissive before launching anything")
    except Exception:  # noqa: BLE001
        r.fail("convert anim+emissive rejection", traceback.format_exc())


def _find_modelviewer() -> Path | None:
    for candidate in (os.environ.get("DXLIB_MODELVIEWER"), cli.DEFAULT_MODELVIEWER_PATH):
        if candidate and Path(candidate).exists():
            return Path(candidate)
    return None


def stage_e2e_convert(r: Reporter) -> None:
    r.section("stage 9: end-to-end convert() in every save mode (best-effort, machine-specific)")
    try:
        import pywinauto  # noqa: F401
    except ImportError:
        r.ok("skipped: pywinauto not installed (pip install pywinauto)")
        return

    exe = _find_modelviewer()
    if exe is None:
        r.ok("skipped: no local DxLibModelViewer exe found "
             "(checked $DXLIB_MODELVIEWER and cli.DEFAULT_MODELVIEWER_PATH)")
        return

    fbx_env = os.environ.get("TOOLS_MODEL_TEST_FBX")
    if not fbx_env or not Path(fbx_env).exists():
        r.ok("skipped: no test .fbx (set $TOOLS_MODEL_TEST_FBX; this toolkit has no committed fixture)")
        return

    from tools.model import dxlib_modelviewer
    timeout = float(os.environ.get("TOOLS_MODEL_TEST_TIMEOUT", "240"))
    sizes: dict[str, int] = {}
    with tempfile.TemporaryDirectory() as tmp:
        for mode in dxlib_modelviewer.SAVE_MODES:
            out = Path(tmp) / f"selftest_output_{mode}.mv1"
            try:
                # デバッグ一式は調査用に残るよう `tmp` の外に置く。
                dxlib_modelviewer.convert(Path(fbx_env), out, exe, mode=mode, timeout=timeout,
                                           debug_dir=None)
                problems = cli.looks_like_mv1(out)
                if problems:
                    raise AssertionError(f"converted output failed looks_like_mv1(): {problems}")
                sizes[mode] = len(mv1.decode(out.read_bytes()))
                r.ok(f"--mode {mode}: converted {fbx_env} -> {out.name} ({sizes[mode]} bytes decoded)")
            except dxlib_modelviewer.AutomationError as e:
                r.fail(f"end-to-end convert() --mode {mode}",
                       f"step {e.step!r}: {e}\ndebug info saved to: {e.debug_path}")
            except Exception:  # noqa: BLE001
                r.fail(f"end-to-end convert() --mode {mode}", traceback.format_exc())
    if len(sizes) == len(dxlib_modelviewer.SAVE_MODES):
        # メッシュとアニメーションの両方を持つテスト用 .fbx でのみ意味がある。
        if sizes["full"] > sizes["mesh"] and sizes["full"] > sizes["anim"]:
            r.ok("full output is larger than both mesh-only and anim-only outputs")
        else:
            r.fail("mode output sizes", f"expected full > mesh and full > anim, got {sizes} "
                   "(fine if $TOOLS_MODEL_TEST_FBX has no animation or no mesh)")


def main() -> int:
    r = Reporter()
    stage_meta_roundtrip(r)
    stage_content_path_convention(r)
    stage_install_guid_reuse(r)
    stage_texture_copy(r)
    stage_mv1_decode(r)
    stage_texture_collection(r)
    stage_mv1_encode(r)
    stage_materials(r)
    stage_e2e_convert(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
