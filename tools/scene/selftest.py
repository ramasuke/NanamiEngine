"""Self-test / correctness gate for tools.scene.

Run:  python tools/scene/selftest.py         (from repo root)
      python -m tools.scene selftest

Exit 0 = all good, 1 = failure. No third-party dependencies.

Stages (added as the toolkit is built - mirrors tools/bt/selftest.py):
  0. cereal_json duplicate-key fidelity (shared with tools.bt - re-checked here
     since tools.scene is the consumer that actually depends on it).
  1. cereal_json formatting fidelity: dumps(loads(text)) == text for every fixture.
  2. Scene/Prefab model round-trip: read -> write -> byte-identical, including
     the ordered polymorphic_id / ptr_wrapper.id / cereal_class_version
     bookkeeping sequences AND Transform's repeated "child" key count.
  3. catalog.json freshness (regen-catalog --check), plus the base-class slot
     table: every base a component archives is known, and ImageRenderer's
     layout is ComponentBase / IInitRenderable / IUserInterfaceRenderable.
  4. GameObject/Component edit + inverse round-trip (add-then-remove restores
     the original bytes exactly), including a brand-new component keeping its
     empty mixin-base slots (value1/value2...) and a component whose base has
     its own fields (ColliderBase/NetworkComponent) still being refused.
  5. instantiate-prefab: two instantiations of the same prefab produce fresh,
     non-colliding guids and are tagged CopiedPrefabGameObject.
"""

from __future__ import annotations

import sys
import traceback
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.common import cereal_json  # noqa: E402

# (name, relative path) - a small, diverse set of real committed fixtures.
# Hyena.prefab is load-bearing: it is the only fixture with a multi-child
# Transform (childCount 3, three sibling "child" keys) and several components
# with bare empty-body mixin members ("value1".."value4") that collide on
# structural fingerprint - see reader._tag_value's literal_presence handling.
SCENE_FIXTURES = [
    ("OtherPlayerStatusUiScene", "Assets/Scene/OtherPlayerStatusUiScene.scene"),
    ("LoadingScene", "Assets/Scene/LoadingScene.scene"),
    ("GrassLandScene", "Assets/Scene/GrassLandScene.scene"),
]
PREFAB_FIXTURES = [
    ("SampleNetworkSpawnPrefab", "Assets/Prefab/SampleNetworkSpawnPrefab.prefab"),
    ("DealDamageText3D", "Assets/Prefab/UI/DealDamageText3D.prefab"),
    ("Hyena", "Assets/Prefab/Npc/Enemy/Hyena.prefab"),
]
# Assets/Prefab/UI/ChattingUI.prefab is deliberately excluded: it contains a
# pre-existing, unrelated data bug (a UI string serialised as raw Shift-JIS
# bytes inside an otherwise-UTF-8 file) that predates this toolkit - decoding
# it as UTF-8 correctly raises, which is the desired "fail loud" behaviour.


class Reporter:
    def __init__(self) -> None:
        self.failed = 0
        self.passed = 0

    def ok(self, name: str) -> None:
        self.passed += 1
        print(f"  PASS  {name}")

    def fail(self, name: str, err: str) -> None:
        self.failed += 1
        print(f"  FAIL  {name}\n        {err.strip().splitlines()[0] if err.strip() else err}")
        for line in err.strip().splitlines()[1:]:
            print(f"        {line}")

    def section(self, title: str) -> None:
        print(f"\n=== {title} ===")

    def finish(self) -> int:
        total = self.passed + self.failed
        print(f"\n{self.passed}/{total} checks passed"
              + (f", {self.failed} FAILED" if self.failed else ""))
        return 1 if self.failed else 0


def stage_dup_keys(r: Reporter) -> None:
    r.section("stage 0: OrderedObj duplicate-key fidelity")
    try:
        obj = cereal_json.loads('{"a": 1, "child": 10, "child": 20, "child": 30}')
        assert len(obj) == 4
        assert [v.value for k, v in obj.items() if k == "child"] == [10, 20, 30]
        r.ok("loads()/OrderedObj preserve duplicate sibling keys")
    except Exception:  # noqa: BLE001
        r.fail("OrderedObj duplicate-key fidelity", traceback.format_exc())


def stage_formatting(r: Reporter) -> None:
    r.section("stage 1: cereal_json formatting fidelity")
    for name, rel in SCENE_FIXTURES + PREFAB_FIXTURES:
        p = _REPO / rel
        try:
            cereal_json.self_check_roundtrip(p)
            r.ok(Path(rel).name)
        except Exception as e:  # noqa: BLE001
            r.fail(Path(rel).name, str(e))


def stage_catalog(r: Reporter) -> None:
    try:
        from tools.scene import catalog, catalog_scan
    except Exception:  # noqa: BLE001
        return
    r.section("stage 3: catalog freshness")
    ok, msg = catalog_scan.check_fresh()
    if ok:
        r.ok("catalog.json fresh")
    else:
        r.fail("catalog.json fresh", msg)

    # Base-class slots: every base some component archives must be in the
    # bases table, and ImageRenderer's layout (the shape behind the
    # add-component bug that dropped its mixin slots) must be recorded exactly.
    try:
        cat = catalog.load()
        referenced = {b["leaf"] for e in cat.components.values() for b in e.get("bases", [])}
        unknown = sorted(referenced - set(cat.bases))
        if unknown:
            raise AssertionError(
                f"bases archived by components but missing from the bases table: {unknown}")
        entry = cat.resolve_component("ImageRenderer")
        got = [(b["leaf"], b["key"]) for b in entry["bases"]]
        want = [("ComponentBase", "value0"), ("IInitRenderable", "value1"),
                ("IUserInterfaceRenderable", "value2")]
        if got != want:
            raise AssertionError(f"ImageRenderer bases {got} != {want}")
        for leaf in ("IInitRenderable", "IUserInterfaceRenderable"):
            if not cat.base_info(leaf)["empty"]:
                raise AssertionError(f"{leaf} should be an empty marker base")
        if cat.base_info("ColliderBase")["empty"]:
            raise AssertionError("ColliderBase has fields of its own and must not be marked empty")
        r.ok("bases table complete; ImageRenderer = ComponentBase/IInitRenderable/IUserInterfaceRenderable")
    except Exception:  # noqa: BLE001
        r.fail("catalog base-class slots", traceback.format_exc())


def stage_component_edits(r: Reporter) -> None:
    try:
        from tools.scene import edits, reader, writer
    except Exception:  # noqa: BLE001
        return
    r.section("stage 4: GameObject/Component edit + inverse round-trip")
    for _name, rel in SCENE_FIXTURES:
        p = _REPO / rel
        try:
            base = cereal_json.read_text(p)
            scene = reader.read_scene(base)
            if not scene.roots:
                r.ok(f"{Path(rel).name} (skipped: no root GameObject)")
                continue
            new_guid = "00000000-0000-4000-8000-000000000001"
            edits.add_gameobject(scene, parent=None, name="__selftest_probe__", guid=new_guid)
            edits.remove_gameobject(scene, new_guid)
            out = writer.write_scene(scene)
            if out != base:
                raise AssertionError("add-then-remove GameObject did not restore the original bytes")
            r.ok(f"{Path(rel).name} add/remove GameObject inverse")
        except Exception:  # noqa: BLE001
            r.fail(Path(rel).name, traceback.format_exc())

    for _name, rel in PREFAB_FIXTURES:
        p = _REPO / rel
        try:
            base = cereal_json.read_text(p)
            prefab = reader.read_prefab(base)
            new_guid = "00000000-0000-4000-8000-000000000002"
            edits.add_gameobject(prefab, parent="root", name="__selftest_probe__", guid=new_guid)
            edits.remove_gameobject(prefab, new_guid)
            out = writer.write_prefab(prefab)
            if out != base:
                raise AssertionError("add-then-remove GameObject did not restore the original bytes")
            r.ok(f"{Path(rel).name} add/remove GameObject inverse")
        except Exception:  # noqa: BLE001
            r.fail(Path(rel).name, traceback.format_exc())

    # component add/remove inverse, against a real fixture with at least one GameObject
    try:
        from tools.scene import catalog
        p = _REPO / "Assets/Prefab/SampleNetworkSpawnPrefab.prefab"
        base = cereal_json.read_text(p)
        prefab = reader.read_prefab(base)
        cat = catalog.load()
        comp = edits.add_component(prefab, prefab.root.guid, "AudioSource", cat=cat)
        edits.remove_component(prefab, prefab.root.guid, len(prefab.root.components) - 1)
        out = writer.write_prefab(prefab)
        if out != base:
            raise AssertionError("add-then-remove Component did not restore the original bytes")
        r.ok("SampleNetworkSpawnPrefab.prefab add/remove Component inverse")
    except Exception:  # noqa: BLE001
        r.fail("component add/remove inverse", traceback.format_exc())

    # Mixin base slots on a brand-new component - regression test for the bug
    # where add-component wrote only ComponentBase's value0 and dropped the
    # empty IInitRenderable/IUserInterfaceRenderable slots (value1/value2), so
    # the engine then read spriteFile_ positionally as a base class.
    try:
        import re as _re

        from tools.common.blob import Ver
        from tools.scene import catalog
        p = _REPO / "Assets/Prefab/UI/DealDamageText3D.prefab"
        base = cereal_json.read_text(p)
        prefab = reader.read_prefab(base)
        cat = catalog.load()
        root = prefab.root.guid
        comp = edits.add_component(prefab, root, "ImageRenderer", cat=cat)
        want = ["value0", "value1", "value2", "spriteFile_", "renderPriority_"]
        if comp.data.keys() != want:
            raise AssertionError(f"new ImageRenderer data keys {comp.data.keys()} != {want}")
        for k in ("value1", "value2"):
            v = comp.data[k]
            if not (isinstance(v, Ver) and v.literal_presence is True and len(v.body) == 0):
                raise AssertionError(f"{k} should be an empty, always-versioned base slot, got {v!r}")
        out = writer.write_prefab(prefab)
        if not _re.search(
            r'"value1": \{\s*"cereal_class_version": 0\s*\},\s*'
            r'"value2": \{\s*"cereal_class_version": 0\s*\},\s*"spriteFile_": \{',
            out,
        ):
            raise AssertionError("written text lacks the value1/value2 mixin slots before spriteFile_")
        again = reader.read_prefab(out).root.components[-1]
        if again.fqn != comp.fqn or again.data.keys() != want:
            raise AssertionError(f"re-read component keys {again.data.keys()} != {want}")
        edits.remove_component(prefab, root, len(prefab.root.components) - 1)
        if writer.write_prefab(prefab) != base:
            raise AssertionError("add-then-remove ImageRenderer did not restore the original bytes")
        r.ok("DealDamageText3D.prefab + ImageRenderer keeps its IInitRenderable/IUserInterfaceRenderable slots")

        comp = edits.add_component(prefab, root, "ModelRenderer", cat=cat)
        if comp.data.keys()[:5] != [f"value{i}" for i in range(5)]:
            raise AssertionError(f"new ModelRenderer should have 5 base slots, got {comp.data.keys()}")
        edits.remove_component(prefab, root, len(prefab.root.components) - 1)
        r.ok("ModelRenderer -> value0..value4 (ComponentBase + 4 marker bases)")

        for refused, base_leaf in (("BoxCollider", "ColliderBase"),
                                   ("NetworkTransform", "NetworkComponent")):
            try:
                edits.add_component(prefab, root, refused, cat=cat)
            except edits.EditError as e:
                if base_leaf not in str(e):
                    raise AssertionError(f"{refused} refused, but not because of {base_leaf}: {e}")
            else:
                raise AssertionError(f"{refused} (base {base_leaf} has its own fields) should be refused")
        r.ok("BoxCollider/NetworkTransform (ColliderBase/NetworkComponent) still refused")
    except Exception:  # noqa: BLE001
        r.fail("mixin base slots on add-component", traceback.format_exc())


def stage_instantiate_prefab(r: Reporter) -> None:
    try:
        from tools.scene import edits, model, reader
    except Exception:  # noqa: BLE001
        return
    r.section("stage 5: instantiate-prefab GUID freshness")
    try:
        scene = model.Scene(name="Probe", roots=[])
        prefab = reader.read_prefab_file(_REPO / "Assets/Prefab/Npc/Enemy/Hyena.prefab")

        def _collect_guids(node: model.GameObjectNode, out: set) -> None:
            out.add(node.guid)
            for comp in node.components:
                g = model.find_component_guid(comp)
                if g is not None:
                    out.add(g)
            for child in node.transform.children:
                _collect_guids(child, out)

        original_guids: set = set()
        _collect_guids(prefab.root, original_guids)

        copy1 = edits.instantiate_prefab(scene, prefab)
        copy2 = edits.instantiate_prefab(scene, prefab)
        guids1: set = set()
        guids2: set = set()
        _collect_guids(copy1, guids1)
        _collect_guids(copy2, guids2)

        if guids1 & original_guids:
            raise AssertionError("instantiated copy reused a guid from the source prefab")
        if guids1 & guids2:
            raise AssertionError("two instantiations of the same prefab produced colliding guids")
        if copy1.kind != model.KIND_COPIED_PREFAB or copy2.kind != model.KIND_COPIED_PREFAB:
            raise AssertionError("instantiated root is not tagged copied_prefab")
        r.ok(f"Hyena.prefab x2 -> {len(guids1)} + {len(guids2)} fresh, non-colliding guids")
    except Exception:  # noqa: BLE001
        r.fail("instantiate-prefab GUID freshness", traceback.format_exc())


def stage_model_roundtrip(r: Reporter) -> None:
    try:
        from tools.scene import reader, writer  # noqa: F401
    except Exception:  # noqa: BLE001
        return  # not built yet
    r.section("stage 2: Scene/Prefab model round-trip")
    from tools.common.diffcheck import assert_bookkeeping_equal, assert_semantically_equal

    def _check(rel: str, read_fn, write_fn) -> None:
        p = _REPO / rel
        try:
            orig_text = cereal_json.read_text(p)
            orig = cereal_json.loads(orig_text)
            model_obj = read_fn(orig_text)
            rt_text = write_fn(model_obj)
            rt = cereal_json.loads(rt_text)
            assert_semantically_equal(orig, rt)
            assert_bookkeeping_equal(orig, rt)
            if rt_text != orig_text:
                n = min(len(rt_text), len(orig_text))
                i = next((j for j in range(n) if rt_text[j] != orig_text[j]), n)
                raise AssertionError(
                    f"byte round-trip differs at offset {i}: "
                    f"exp {orig_text[max(0, i-40):i+40]!r} got {rt_text[max(0, i-40):i+40]!r}"
                )
            r.ok(f"{Path(rel).name} (byte-identical)")
        except Exception:  # noqa: BLE001
            r.fail(Path(rel).name, traceback.format_exc())

    for _name, rel in SCENE_FIXTURES:
        _check(rel, reader.read_scene, writer.write_scene)
    for _name, rel in PREFAB_FIXTURES:
        _check(rel, reader.read_prefab, writer.write_prefab)


def main() -> int:
    r = Reporter()
    stage_dup_keys(r)
    stage_formatting(r)
    stage_model_roundtrip(r)
    stage_catalog(r)
    stage_component_edits(r)
    stage_instantiate_prefab(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
