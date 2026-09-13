"""Self-test / correctness gate for tools.model.

Run:  python tools/model/selftest.py         (from repo root)
      python -m tools.model selftest

Exit 0 = all good, 1 = failure. No third-party dependencies for stages 1-3;
stage 4 (GUI automation) is best-effort and skips cleanly when pywinauto,
a DxLibModelViewer exe, and a test .fbx aren't all available.

Stages:
  1. .meta round trip against real, already-shipped Mv1File assets (proves
     the base_class_count=2 shape holds, same fix class as ParticleFile's in
     tools/common/meta_base.py).
  2. content_path_for() convention check against a real nested asset
     (all-backslash contentPath_, matching tools/effect's ParticleFile
     binding).
  3. install() GUID-reuse behaviour, using a synthetic "MV11"-header byte
     string (no real DxLibModelViewer output needed).
  4. install --textures bulk-copy: copies recognized image files into
     <dest-dir>/textures/, ignores non-image files, and errors on a missing
     source directory. Plain filesystem copying - no third-party deps.
  5. best-effort end-to-end convert(): only runs if pywinauto, a
     DxLibModelViewer exe ($DXLIB_MODELVIEWER or cli.DEFAULT_MODELVIEWER_PATH),
     and a test .fbx ($TOOLS_MODEL_TEST_FBX) are all present. Skipped
     elsewhere - this toolkit has no committed .fbx fixture (see
     tools/model/README.md).
"""

from __future__ import annotations

import argparse
import os
import sys
import traceback
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.model import cli, meta  # noqa: E402
from tools.common import cereal_json as cj  # noqa: E402

REAL_META_FIXTURES = [
    _REPO / "Assets" / "Art" / "Animation" / "Man" / "Death.mv1.meta",
    _REPO / "Assets" / "Art" / "Animation" / "Man" / "Jump.mv1.meta",
    _REPO / "Assets" / "Art" / "Models" / "Fantasy" / "DirtyHouse" / "dirtyHouse.mv1.meta",
    _REPO / "Assets" / "Art" / "Models" / "Monster" / "Hyenas" / "Hyenas_A4_AllMotion.mv1.meta",
]

_SYNTHETIC_MV1 = b"MV11" + b"\x00" * 300  # header + padding past cli._MV1_MIN_SIZE


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
            ns = argparse.Namespace(mv1=str(src), source=None, textures=None, dest=str(dest))
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
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=str(src_dir), dest=str(dest))
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
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=str(empty_dir), dest=str(dest))
            cli.cmd_install(ns)  # must not raise - zero images is a warning, not an error
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
            ns = argparse.Namespace(mv1=str(mv1_src), source=None, textures=str(missing_dir), dest=str(dest))
            try:
                cli.cmd_install(ns)
            except cli.CliError:
                pass
            else:
                raise AssertionError("a missing --textures directory should raise CliError")
        r.ok("install --textures rejects a missing source directory")
    except Exception:  # noqa: BLE001
        r.fail("install --textures missing source dir", traceback.format_exc())


def _find_modelviewer() -> Path | None:
    for candidate in (os.environ.get("DXLIB_MODELVIEWER"), cli.DEFAULT_MODELVIEWER_PATH):
        if candidate and Path(candidate).exists():
            return Path(candidate)
    return None


def stage_e2e_convert(r: Reporter) -> None:
    r.section("stage 5: end-to-end convert() (best-effort, machine-specific)")
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
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "selftest_output.mv1"
        try:
            dxlib_modelviewer.convert(Path(fbx_env), out, exe, timeout=60.0,
                                       debug_dir=Path(tmp) / "debug")
            problems = cli.looks_like_mv1(out)
            if problems:
                raise AssertionError(f"converted output failed looks_like_mv1(): {problems}")
            r.ok(f"converted {fbx_env} -> {out.name} via {exe}")
        except Exception:  # noqa: BLE001
            r.fail("end-to-end convert()", traceback.format_exc())


def main() -> int:
    r = Reporter()
    stage_meta_roundtrip(r)
    stage_content_path_convention(r)
    stage_install_guid_reuse(r)
    stage_texture_copy(r)
    stage_e2e_convert(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
