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
FIXTURES = [
    "actionLines_shockwave", "smallTexturesRibbon", "drill",
    # Added when Model/Track/LocationAbsValues/RotationValues-AxisPVA support
    # was built from a wider 310-file real-sample corpus (Effekseer素材/):
    "blue_laser",       # AndrewFM01 - Circle (Type=3) generation + AttractiveForce
    "Gohlem1",          # MAGICALxSPIRAL - Model (Type=5) DrawingValues
    "Sylph2",           # MAGICALxSPIRAL - Track (Type=6) DrawingValues
    "Water_Impact",     # MAGICALxSPIRAL - RotationValues AxisPVA (Type=3)
]

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


def _assert_roundtrip_stable(proj: Elem) -> None:
    text = xmlio.serialize(proj)
    rt_text = xmlio.serialize(xmlio.parse(text))
    if rt_text != text:
        raise AssertionError("presets-built tree is not stable under parse -> serialize")


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
        _assert_roundtrip_stable(proj)
        r.ok("presets-built ring+sprite tree stable")
    except Exception:  # noqa: BLE001
        r.fail("presets-built ring+sprite tree", traceback.format_exc())

    try:
        # Regression check: generation_location_circle() used to hardcode
        # Type="0" (should be "3" - every real Circle-shaped sample uses the
        # outer Type=3 selector; Type=0 is Point).
        circle = p.generation_location_circle(division=8, circle_type=1,
                                               radius={"center": 2, "max": 2, "min": 2})
        if circle.require("Type").text != "3":
            raise AssertionError(f"generation_location_circle() Type = {circle.require('Type').text!r}, expected '3'")
        r.ok("generation_location_circle() writes outer Type=3")
    except Exception:  # noqa: BLE001
        r.fail("generation_location_circle() Type", traceback.format_exc())

    try:
        model_block = p.model(
            model_path="Model/rock.efkmodel", lighting=False,
            color_fixed=p.color("Color_Fixed", r=24, g=16, b=32),
            color_easing=p.easing(
                "Color_Easing",
                start=p.random_color("Start", r={"center": 112, "max": 112, "min": 112}),
                end=p.random_color("End", r={"center": 60, "max": 60, "min": 60}),
                start_speed=20, end_speed=-30,
            ),
        )
        track_block = p.track(
            color_left=p.color("ColorLeft_Fixed", a=0),
            color_center=p.easing(
                "ColorCenter_Easing",
                start=p.random_color("Start", a={"center": 0, "max": 0, "min": 0}),
                end=p.random_color("End", a={"center": 192, "max": 192, "min": 192}),
            ),
        )
        n_model = p.model_node("Node", model_block=model_block)
        n_track = p.track_node(
            "Node", track_block=track_block,
            sound=p.sound_values(wave="Sound/x.wav", volume={"center": 1, "max": 1, "min": 1},
                                  pan_type=0, distance=10),
            location_abs=p.location_abs_values(attractive_force=0.025),
            generation_location=p.generation_location_point(
                location={"x": {"center": 0, "max": 1, "min": -1}}),
        )
        proj = p.new_project(root_children=[n_model, n_track])
        _assert_roundtrip_stable(proj)
        r.ok("presets-built model+track+sound+location_abs tree stable")
    except Exception:  # noqa: BLE001
        r.fail("presets-built model+track+sound+location_abs tree", traceback.format_exc())

    try:
        rotation = p.rotation_values(axis_pva=p.axis_pva(
            axis=p.xyz("Axis", y=p.pva("Y", center=0, max=0, min=0), z=p.pva("Z", center=1, max=1, min=1)),
            rotation={"center": 0, "max": 1808, "min": -1808},
            velocity={"center": 0, "max": 1, "min": -1},
        ))
        rotation2 = p.rotation_values(axis_easing=p.axis_easing(
            axis=p.xyz("Axis", z=p.pva("Z", center=1, max=1, min=1)),
            start={"center": 0, "max": 0, "min": 0}, end={"center": 90, "max": 90, "min": 90},
            start_speed=5, end_speed=-5,
        ))
        scaling = p.scaling_values(easing=p.easing(
            "Easing",
            start=p.elem("Start", Z=p.pva("Z", max=180, min=-180)),
            end=p.elem("End", Z=p.pva("Z", max=0, min=0)),
        ))
        scaling2 = p.scaling_values(single_easing=p.easing(
            "SingleEasing", start=p.pva("Start", center=0, max=0, min=0),
            end=p.pva("End", center=1.5, max=1.75, min=1.25), start_speed=30, end_speed=-30,
        ))
        location = p.location_values(easing=p.easing(
            "Easing",
            start=p.elem("Start", X=p.pva("X", max=0.2, min=-0.2)),
            end=p.elem("End", X=p.pva("X", max=3, min=-3)),
        ))
        n1 = p.node("Node", rotation=rotation, scaling=scaling, location=location,
                    common=p.common_values(location_effect_type=1, rotation_effect_type=1,
                                            scale_effect_type=1,
                                            remove_when_all_children_removed=False))
        n2 = p.node("Node", rotation=rotation2, scaling=scaling2,
                    generation_location=p.generation_location_sphere(
                        radius={"center": 2, "max": 2, "min": 2}))
        proj = p.new_project(root_children=[n1, n2])
        _assert_roundtrip_stable(proj)
        r.ok("presets-built Easing/AxisPVA/AxisEasing/SinglePVA tree stable")
    except Exception:  # noqa: BLE001
        r.fail("presets-built Easing/AxisPVA/AxisEasing/SinglePVA tree", traceback.format_exc())

    try:
        sprite_block = p.sprite(
            rendering_order=1,
            color_all_random=p.random_color("ColorAll_Random",
                                             r={"center": 0, "max": 255, "min": 0},
                                             a={"center": 255, "max": 255, "min": 255}),
            position_corners={"ll": {"x": -0.5, "y": 0}, "lr": {"x": 0.5, "y": 0},
                               "ul": {"x": -0.5, "y": 1}, "ur": {"x": 0.5, "y": 1}},
            color_corners={"ll": {"r": 255, "g": 0, "b": 0}, "ur": {"r": 0, "g": 0, "b": 255}},
        )
        renderer = p.renderer_common(
            color_texture="Texture/t.png",
            fade_in={"frame": 4, "start_speed": 0, "end_speed": 0}, fade_out={"frame": 8},
            ztest=False, color_inherit_type=2, distortion=True, distortion_intensity=0.3,
            uv_scroll={"speed": {"x": 16, "y": 0}},
        )
        n1 = p.sprite_node("Node", sprite_block=sprite_block, renderer_common=renderer)
        n2 = p.node("Node", renderer_common=p.renderer_common(
            uv_fixed={"start": {"x": 0, "y": 0}, "size": {"x": 0.5, "y": 0.5}}),
            drawing=p.drawing_values("sprite", p.sprite()))
        n3 = p.node("Node", renderer_common=p.renderer_common(
            uv_animation={"size": {"x": 64, "y": 64}, "frame_length": 2,
                          "frame_count_x": 4, "frame_count_y": 4, "loop_type": 1}),
            drawing=p.drawing_values("sprite", p.sprite()))
        proj = p.new_project(root_children=[n1, n2, n3])
        _assert_roundtrip_stable(proj)
        r.ok("presets-built Sprite corners/random-color + RendererCommonValues UV/fade tree stable")
    except Exception:  # noqa: BLE001
        r.fail("presets-built Sprite/RendererCommonValues tree", traceback.format_exc())

    try:
        # ColorAll_Easing (Sprite/Ribbon) and Ring's independent per-position
        # OuterColor/CenterColor/InnerColor Random/Easing selectors - found
        # after the initial Model/Track/Sound pass, via a second look at the
        # same 310-file corpus (confirmed real: MAGICALxSPIRAL/AquaPoint.efkproj
        # for ColorAll_Easing + the block-level DrawnAs/ColorSpace siblings on
        # *_Random; AndrewFM01/boss_death.efkproj for Ribbon's ColorAll=2).
        sprite_easing = p.sprite(color_all_easing=p.easing(
            "ColorAll_Easing",
            start=p.random_color("Start", r={"center": 255, "max": 255, "min": 255}, drawn_as=1),
            end=p.random_color("End", r={"center": 0, "max": 0, "min": 0}, drawn_as=1),
        ))
        ribbon_random = p.ribbon(color_all_random=p.random_color(
            "ColorAll_Random", r={"center": 127, "max": 255, "min": 0},
            drawn_as=1, color_space=0))
        ring_modes = p.ring(
            center_ratio=0.8,
            outer_color_random=p.random_color("OuterColor_Random", a={"center": 200, "max": 255, "min": 150}),
            center_color_easing=p.easing(
                "CenterColor_Easing",
                start=p.random_color("Start", g={"center": 0, "max": 0, "min": 0}),
                end=p.random_color("End", g={"center": 255, "max": 255, "min": 255}),
            ),
            inner_color=p.color("InnerColor_Fixed", r=0, g=0, b=0, a=255),
        )
        proj = p.new_project(root_children=[
            p.sprite_node("Node", sprite_block=sprite_easing),
            p.ribbon_node("Node", ribbon_block=ribbon_random),
            p.ring_node("Node", ring_block=ring_modes),
        ])
        _assert_roundtrip_stable(proj)
        r.ok("presets-built ColorAll_Easing + Ring per-position color-mode tree stable")
    except Exception:  # noqa: BLE001
        r.fail("presets-built ColorAll_Easing/Ring color-mode tree", traceback.format_exc())


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
            # Model/Track are the two DrawingValues kinds added from the
            # wider 310-file corpus (DRAWING_TYPE 5/6) - confirm the real
            # CUI still accepts them (and the LocationAbsValues/SoundValues/
            # generation_location_point blocks alongside), not just that our
            # own xmlio round-trips them.
            model_node = p.model_node(
                "Node", model_block=p.model(model_path="Model/rock.efkmodel"),
                location_abs=p.location_abs_values(attractive_force=0.025))
            track_node = p.track_node(
                "Node", track_block=p.track(color_left=p.color("ColorLeft_Fixed", a=128)),
                sound=p.sound_values(wave="Sound/x.wav", distance=10),
                generation_location=p.generation_location_point())
            proj = p.new_project(root_children=[model_node, track_node])
            in_path = tmp_path / "model_track_demo.efkproj"
            xmlio.write(in_path, proj)
            out = tmp_path / "model_track_demo.efkefc"
            _compile_check(cui, in_path, out)
            r.ok("compiled Model+Track presets-built tree")
        except Exception:  # noqa: BLE001
            r.fail("compile Model+Track presets-built tree", traceback.format_exc())

        try:
            proj = p.new_project(root_children=[
                p.sprite_node("Node", sprite_block=p.sprite(color_all_easing=p.easing(
                    "ColorAll_Easing",
                    start=p.random_color("Start", r={"center": 255, "max": 255, "min": 255}),
                    end=p.random_color("End", r={"center": 0, "max": 0, "min": 0}),
                ))),
                p.ring_node("Node", ring_block=p.ring(
                    center_ratio=0.8,
                    outer_color_random=p.random_color("OuterColor_Random",
                                                       a={"center": 200, "max": 255, "min": 150}))),
            ])
            in_path = tmp_path / "color_modes_demo.efkproj"
            xmlio.write(in_path, proj)
            out = tmp_path / "color_modes_demo.efkefc"
            _compile_check(cui, in_path, out)
            r.ok("compiled ColorAll_Easing/Ring-color-mode presets-built tree")
        except Exception:  # noqa: BLE001
            r.fail("compile ColorAll_Easing/Ring-color-mode presets-built tree", traceback.format_exc())

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
