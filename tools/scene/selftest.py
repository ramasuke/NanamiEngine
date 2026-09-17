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
     non-colliding guids and are tagged CopiedPrefabGameObject; Guid references
     into the copied tree (FirstEventDragon's BoneSync TransformSync targets)
     are re-pointed at the copy.
  6. GameObject mark_: setting a mark upgrades only that GameObject type to the
     post-mark class version (every node of the type gets mark_), and a .prefab
     root's unversioned mark_ lands between transform_ and the value0 tail.
  7. The write path (cli_edit._commit) runs the cereal_class_version audit on the
     rendered text, so an edit that moves a Field<T>'s first occurrence is
     refused instead of being written and only caught by a later `validate`.
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
    ("StageLoadingScene", "Assets/Scene/StageLoadingScene.scene"),
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

    try:
        from tools.common.blob import Ptr, Ver
        from tools.common.cereal_json import OrderedObj

        def _collect_guid_values(blob, out: list) -> None:
            if isinstance(blob, Ptr):
                _collect_guid_values(blob.data, out)
            elif isinstance(blob, Ver):
                _collect_guid_values(blob.body, out)
            elif isinstance(blob, OrderedObj):
                for key, value in blob.items():
                    if key == "value_" and isinstance(value, str):
                        out.append(value)
                    else:
                        _collect_guid_values(value, out)
            elif isinstance(blob, list):
                for item in blob:
                    _collect_guid_values(item, out)

        def _collect_tree(node: model.GameObjectNode, guids: set, values: list, bone_syncs: list) -> None:
            guids.add(node.guid)
            for comp in node.components:
                g = model.find_component_guid(comp)
                if g is not None:
                    guids.add(g)
                _collect_guid_values(comp.data, values)
                if comp.fqn == "NanamiEngine::Module::Component::BoneSync":
                    bone_syncs.append(comp)
            for child in node.transform.children:
                _collect_tree(child, guids, values, bone_syncs)

        dragon = reader.read_prefab_file(_REPO / "Assets/Prefab/Npc/Enemy/FirstEventDragon.prefab")
        source_guids: set = set()
        _collect_tree(dragon.root, source_guids, [], [])

        for label, copied_root in (
            ("instantiate-prefab", edits.instantiate_prefab(model.Scene(name="Probe", roots=[]), dragon)),
            ("copy-prefab", edits.copy_prefab(dragon).root),
        ):
            copy_guids: set = set()
            copy_values: list = []
            bone_syncs: list = []
            _collect_tree(copied_root, copy_guids, copy_values, bone_syncs)

            stale = [v for v in copy_values if v in source_guids]
            if stale:
                raise AssertionError(f"{label}: {len(stale)} Guid reference(s) still point into the source prefab")
            if not bone_syncs:
                raise AssertionError(f"{label}: FirstEventDragon copy has no BoneSync")
            target_values: list = []
            for bone_sync in bone_syncs:
                _collect_guid_values(bone_sync.data, target_values)
            targets = [v for v in target_values if v not in {model.find_component_guid(c) for c in bone_syncs}]
            if not targets or any(v not in copy_guids for v in targets):
                raise AssertionError(f"{label}: BoneSync TransformSync targets are not re-pointed at the copy")
            r.ok(f"FirstEventDragon.prefab {label} -> {len(targets)} TransformSync targets re-pointed at the copy")
    except Exception:  # noqa: BLE001
        r.fail("instantiate-prefab Guid reference remap", traceback.format_exc())


def stage_gameobject_mark(r: Reporter) -> None:
    try:
        from tools.scene import model, reader, writer
    except Exception:  # noqa: BLE001
        return
    r.section("stage 6: GameObject mark_ versioning")

    def _walk(roots):
        stack = list(roots)
        while stack:
            node = stack.pop()
            yield node
            stack.extend(node.transform.children)

    try:
        scene = reader.read_scene_file(_REPO / "Assets/Scene/GrassLandScene.scene")
        target = scene.roots[0]
        target_fqn = model.GAMEOBJECT_FQN_BY_KIND[target.kind]
        target.mark = model.MARK_NAMES.index("LabelRed")
        text = writer.write_scene(scene)

        loaded = cereal_json.loads(text)
        first_slot = loaded[f"gameObject_{0}"]["ptr_wrapper"]["data"]
        version = int(first_slot["cereal_class_version"].value)
        if version != model.GAMEOBJECT_CLASS_VERSION[target_fqn]:
            raise AssertionError(f"marked type kept class version {version}")

        back = reader.read_scene(text)
        for node in _walk(back.roots):
            same_type = model.GAMEOBJECT_FQN_BY_KIND[node.kind] == target_fqn
            if node.guid == target.guid:
                if node.mark != target.mark:
                    raise AssertionError(f"mark not preserved: {node.mark}")
            elif same_type and node.mark != 0:
                raise AssertionError(f"{node.name}: same-type node without mark_ (got {node.mark})")
            elif not same_type and node.mark is not None:
                raise AssertionError(f"{node.name}: unmarked type gained mark_")
        if writer.write_scene(back) != text:
            raise AssertionError("marked scene does not round-trip byte-identically")
        r.ok("GrassLandScene: marking one root upgrades only its type and round-trips")

        prefab = reader.read_prefab_file(_REPO / "Assets/Prefab/Npc/Enemy/Hyena.prefab")
        # Children may already carry mark_ (their own type was upgraded); marking
        # the root must leave them exactly as they were, whatever that is.
        child_marks = [n.mark for n in _walk(prefab.root.transform.children)]
        prefab.root.mark = model.MARK_NAMES.index("DiamondBlue")
        text = writer.write_prefab(prefab)
        keys = list(cereal_json.loads(text).keys())
        if not keys.index("transform_") < keys.index("mark_") < keys.index("value0"):
            raise AssertionError(f"prefab root key order wrong: {keys}")
        back = reader.read_prefab(text)
        if back.root.mark != prefab.root.mark:
            raise AssertionError(f"prefab root mark not preserved: {back.root.mark}")
        if [n.mark for n in _walk(back.root.transform.children)] != child_marks:
            raise AssertionError("marking the prefab root changed its children's mark_")
        r.ok("Hyena.prefab: root mark_ sits between transform_ and value0")
    except Exception:  # noqa: BLE001
        r.fail("GameObject mark_ versioning", traceback.format_exc())


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


def stage_write_path_audit(r: Reporter) -> None:
    try:
        import contextlib
        import io

        from tools.scene import catalog, cli_edit, edits, reader
    except Exception:  # noqa: BLE001
        return
    r.section("stage 7: write-path cereal_class_version audit")
    # StageLoadingScene has no component type the catalog misses, so the audit
    # reports "missing" findings as hard failures rather than downgrading them.
    rel = "Assets/Scene/StageLoadingScene.scene"
    p = _REPO / rel
    cat = catalog.load()

    def commit(leaf: str) -> tuple[int, str]:
        base = cereal_json.read_text(p)
        scene = reader.read_scene(base)
        edits.add_component(scene, scene.roots[0].guid, leaf, cat=cat)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = cli_edit._commit(p, scene, "scene", base, dry_run=True)
        if cereal_json.read_text(p) != base:
            raise AssertionError("a --dry-run commit wrote to the fixture")
        return code, out.getvalue()

    try:
        # ModelRenderer on the root puts a Field<Mv1File> - a type this file has
        # never carried - ahead of everything else, which is the shape that broke
        # GameManage.scene (a new FIELD landing before the keyed occurrence).
        code, out = commit("ModelRenderer")
        if code != 1 or "first occurrence of Field<Mv1File>" not in out:
            raise AssertionError(f"a first-occurrence Field<Mv1File> should be refused, got "
                                 f"code {code}:\n{out}")
        r.ok("add-component ModelRenderer -> refused (Field<Mv1File> first occurrence)")
    except Exception:  # noqa: BLE001
        r.fail("write path refuses a missing first-occurrence key", traceback.format_exc())

    try:
        code, out = commit("AudioSource")
        if code != 0:
            raise AssertionError(f"an edit that introduces no new Field<T> must still commit:\n{out}")
        r.ok("add-component AudioSource -> still commits")
    except Exception:  # noqa: BLE001
        r.fail("write path still allows a sound edit", traceback.format_exc())


def main() -> int:
    r = Reporter()
    stage_dup_keys(r)
    stage_formatting(r)
    stage_model_roundtrip(r)
    stage_catalog(r)
    stage_component_edits(r)
    stage_instantiate_prefab(r)
    stage_gameobject_mark(r)
    stage_write_path_audit(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
