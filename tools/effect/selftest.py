"""Self-test / correctness gate for tools.effect.

Run:  python tools/effect/selftest.py         (from repo root)
      python -m tools.effect selftest

Exit 0 = all good, 1 = failure. No third-party dependencies.

Stages:
  1. xmlio formatting fidelity: serialize(parse(text)) == text for every real
     fixture (copied from the AndrewFM01 samples this toolkit was built from).
  2. presets/model round trip: build a tree purely via presets.py (no hand
     XML), parse -> serialize -> reparse, assert structurally stable.
  3. .meta round trip against real, already-shipped ParticleFile assets
     (proves the base_class_count=2 fix in tools/common/meta_base.py holds).
  4. install() path-convention check against a real nested asset.
  5. best-effort CUI compile: only runs if the pinned/overridden Effekseer
     1.7.3.0 CUI is present on this machine; compiles a fixture and a
     presets-built tree, checks exit 0 + EFKE/INFO header. Skipped elsewhere.
"""

from __future__ import annotations

import os
import sys
import traceback
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.effect import cli, meta, presets as p, xmlio  # noqa: E402
from tools.effect.model import Elem  # noqa: E402
from tools.common import cereal_json as cj  # noqa: E402

TESTDATA = _HERE.parent / "testdata"
FIXTURES = ["actionLines_shockwave", "smallTexturesRibbon", "drill"]

REAL_EFFECT_DIR = _REPO / "Assets" / "Art" / "Effect"
REAL_META_FIXTURES = [
    REAL_EFFECT_DIR / "Laser01.efkefc.meta",
    REAL_EFFECT_DIR / "tktk01" / "fireSpark.efkefc.meta",
    REAL_EFFECT_DIR / "MAGICALxSPIRAL" / "Salamander11.efkefc.meta",
]


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


def stage_formatting(r: Reporter) -> None:
    r.section("stage 1: xmlio formatting fidelity")
    for name in FIXTURES:
        path = TESTDATA / f"{name}.efkproj"
        try:
            xmlio.self_check_roundtrip(path)
            r.ok(path.name)
        except Exception:  # noqa: BLE001
            r.fail(path.name, traceback.format_exc())


def stage_presets_roundtrip(r: Reporter) -> None:
    r.section("stage 2: presets/model round trip")
    try:
        ring_block = p.ring(
            vertex_count=36,
            outer=p.xyz("Location", x=1.8), inner=p.xyz("Location", x=0),
            center_ratio=0.8,
            outer_color=p.color("OuterColor_Fixed", r=0, g=0, b=0),
            inner_color=p.color("InnerColor_Fixed", r=0, g=0, b=0, a=255),
        )
        n1 = p.ring_node("Node", ring_block=ring_block)
        sprite_block = p.sprite(billboard=2, color_all=p.color("ColorAll_Fixed", r=255, g=200, b=100))
        n2 = p.sprite_node(
            "Node", sprite_block=sprite_block,
            common=p.common_values(max_generation=50,
                                    generation_time={"center": 0.02, "max": 0.02, "min": 0.02}),
            scaling=p.scaling_values(fixed=p.xyz("Scale", x=1.0)),
        )
        proj = p.new_project(root_children=[n1, n2])
        text = xmlio.serialize(proj)
        rt = xmlio.parse(text)
        rt_text = xmlio.serialize(rt)
        if rt_text != text:
            raise AssertionError("presets-built tree is not stable under parse -> serialize")
        r.ok("presets-built ring+sprite tree stable")
    except Exception:  # noqa: BLE001
        r.fail("presets-built ring+sprite tree", traceback.format_exc())


def stage_meta_roundtrip(r: Reporter) -> None:
    r.section("stage 3: .meta round trip (real shipped ParticleFile assets)")
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
    r.section("stage 4: content_path_for() convention check")
    try:
        target_dir = REAL_EFFECT_DIR / "tktk01"
        got = meta.content_path_for("fireSpark", target_dir, _REPO)
        want = meta.read_meta(target_dir / "fireSpark.efkefc.meta")["content_path"]
        if got != want:
            raise AssertionError(f"content_path_for() = {got!r}, real asset has {want!r}")
        r.ok("content_path_for() matches real fireSpark.efkefc.meta (all-backslash)")
    except Exception:  # noqa: BLE001
        r.fail("content_path_for() convention", traceback.format_exc())


def _find_cui() -> Path | None:
    for candidate in (os.environ.get("EFFEKSEER_CUI"), cli.DEFAULT_CUI_PATH):
        if candidate and Path(candidate).exists():
            return Path(candidate)
    return None


def _compile_check(cui: Path, in_path: Path, out_path: Path) -> None:
    import subprocess
    result = subprocess.run(
        [str(cui), "-cui", "-in", str(in_path), "-o", str(out_path)],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        raise AssertionError(f"CUI exited {result.returncode}: {result.stderr[:300]}")
    if not out_path.exists():
        raise AssertionError("CUI exited 0 but produced no output file")
    header = out_path.read_bytes()[:16]
    if not header.startswith(b"EFKE") or b"INFO" not in header:
        raise AssertionError(f"output does not look like a valid .efkefc (header {header!r})")


def stage_cui_compile(r: Reporter) -> None:
    r.section("stage 5: CUI compile (best-effort, machine-specific)")
    cui = _find_cui()
    if cui is None:
        r.ok("skipped: no local Effekseer 1.7.3.0 CUI found "
             f"(checked $EFFEKSEER_CUI and {cli.DEFAULT_CUI_PATH})")
        return

    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        try:
            fixture = TESTDATA / "actionLines_shockwave.efkproj"
            out = tmp_path / "fixture.efkefc"
            _compile_check(cui, fixture, out)
            r.ok(f"compiled fixture {fixture.name}")
        except Exception:  # noqa: BLE001
            r.fail("compile real fixture", traceback.format_exc())

        try:
            proj = p.new_project(root_children=[
                p.ring_node("Node", ring_block=p.ring(vertex_count=36, center_ratio=0.8)),
            ])
            in_path = tmp_path / "presets_demo.efkproj"
            xmlio.write(in_path, proj)
            out = tmp_path / "presets_demo.efkefc"
            _compile_check(cui, in_path, out)
            r.ok("compiled presets-built tree")
        except Exception:  # noqa: BLE001
            r.fail("compile presets-built tree", traceback.format_exc())

        try:
            # Regression check for DRAWING_TYPE["sprite"]: a Sprite node's
            # RendererCommonValues.ColorTexture must actually reach the
            # compiled INFO chunk (as a UTF-16LE string) - it silently didn't
            # when "sprite" mapped to node-type 0 (which Effekseer treats as
            # "no drawing", not Sprite; the real value is 2), so nothing
            # referenced the texture and the CUI dropped it with no error.
            renderer = Elem("RendererCommonValues")
            renderer.set_path("ColorTexture", "Texture/selftestRegressionTex.png")
            node = p.sprite_node("Node", sprite_block=p.sprite(billboard=2),
                                  common=p.common_values(life={"center": 60, "max": 60, "min": 60}),
                                  renderer_common=renderer)
            proj = p.new_project(root_children=[node])
            in_path = tmp_path / "sprite_texture_demo.efkproj"
            xmlio.write(in_path, proj)
            out = tmp_path / "sprite_texture_demo.efkefc"
            _compile_check(cui, in_path, out)
            compiled_text = out.read_bytes().decode("utf-16-le", errors="ignore")
            if "selftestRegressionTex" not in compiled_text:
                raise AssertionError(
                    "Sprite node's ColorTexture did not survive CUI compile - "
                    "DRAWING_TYPE['sprite'] regressed back to node-type 0 (\"none\")?"
                )
            r.ok("Sprite node's ColorTexture survives CUI compile")
        except Exception:  # noqa: BLE001
            r.fail("sprite texture reaches compiled INFO chunk", traceback.format_exc())


def main() -> int:
    r = Reporter()
    stage_formatting(r)
    stage_presets_roundtrip(r)
    stage_meta_roundtrip(r)
    stage_content_path_convention(r)
    stage_cui_compile(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
