"""Self-test / correctness gate for tools.bt.

Run:  python tools/bt/selftest.py         (from repo root)
      python -m tools.bt selftest

Exit 0 = all good, 1 = failure. No third-party dependencies.

Stages (added as the toolkit is built):
  1. cereal_json formatting fidelity: dumps(loads(text)) == text for every fixture.
  2. tree model round-trip: read -> write -> structural-equal to the original,
     including the ordered polymorphic_id / ptr_wrapper.id / cereal_class_version
     bookkeeping sequences.
  3. .meta round-trip.
  4. new-tree fidelity vs T-Rex.*.
  5. catalog freshness (regen-catalog --check).
  6. edit-then-inverse == original.
  8. copy-node deep-copies independently, and a dotted-key set-params reaches
     inside a shape='nested' param; both undo cleanly.
"""

from __future__ import annotations

import sys
import traceback
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.bt import cereal_json  # noqa: E402

BT_DIR = _REPO / "Assets" / "Data" / "EnemyBehaviour"
TREE_FIXTURES = ["T-Rex", "TrainingDummy", "HyenaBehaviour", "FirstEventDragon"]
META_FIXTURES = ["T-Rex", "TrainingDummy", "HyenaBehaviour"]

FRIENDLY_DIR = _REPO / "Assets" / "Data" / "FriendlyNpcBehviour"
FRIENDLY_TREE_FIXTURES = ["ActionInstructure", "Adventure", "IdleActionInstructure", "SampleAppearDragon"]
FRIENDLY_META_FIXTURES = list(FRIENDLY_TREE_FIXTURES)

# Documented v1 limitation (see docs/BehaviourTree.md): TrySwordManQuest's
# quest_ is a raw, un-modeled shared_ptr<ITakeableSwordManQuest> - a
# polymorphic object outside any scanned catalog - and its embedded
# questUiPrefab_ FIELD(Asset::PrefabGameObjectFile) happens to share its
# referenced type with another, catalog-modeled FIELD(...) elsewhere in the
# same file. Two genuinely different C++ Field<T> instantiations serialise
# identically when opaque, so the un-modeled occurrence cannot be bucketed
# into the same once-per-type version slot as the modeled one; the result is
# exactly one harmless extra `"cereal_class_version": 0` at each of the two
# nesting levels of that one modeled sibling occurrence (confirmed inert:
# cereal's JSON archives look members up by name, so an unread extra key is
# just ignored) - never a wrong value, never a missing key. Everything else
# in the file must still round-trip byte-for-byte.
KNOWN_LIMITATION_EXTRA_LINES = {"ActionInstructure": 2}


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


def stage_ordered_obj_dup_keys(r: Reporter) -> None:
    r.section("stage 0: OrderedObj duplicate-key fidelity")
    try:
        obj = cereal_json.loads('{"a": 1, "child": 10, "child": 20, "child": 30}')
        assert len(obj) == 4, f"expected 4 pairs, got {len(obj)}"
        vals = [v.value for k, v in obj.items() if k == "child"]
        assert vals == [10, 20, 30], f"expected [10, 20, 30], got {vals}"
        assert obj.values_for("child") == obj.values_for("child"), "values_for not stable"
        assert [v.value for v in obj.values_for("child")] == [10, 20, 30]
        rt = cereal_json.dumps(cereal_json.loads(cereal_json.dumps(obj)))
        if rt != cereal_json.dumps(obj):
            raise AssertionError("duplicate-key object does not round-trip stably")
        r.ok("loads()/OrderedObj preserve duplicate sibling keys")
    except Exception as e:  # noqa: BLE001
        r.fail("OrderedObj duplicate-key fidelity", traceback.format_exc())


def stage_formatting(r: Reporter) -> None:
    r.section("stage 1: cereal_json formatting fidelity")
    for name in TREE_FIXTURES:
        p = BT_DIR / f"{name}.enemyBehaviourData"
        try:
            cereal_json.self_check_roundtrip(p)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, str(e))
    for name in META_FIXTURES:
        p = BT_DIR / f"{name}.enemyBehaviourData.meta"
        try:
            cereal_json.self_check_roundtrip(p)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, str(e))
    for name in FRIENDLY_TREE_FIXTURES:
        p = FRIENDLY_DIR / f"{name}.friendBehaviourData"
        try:
            cereal_json.self_check_roundtrip(p)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, str(e))
    for name in FRIENDLY_META_FIXTURES:
        p = FRIENDLY_DIR / f"{name}.friendBehaviourData.meta"
        try:
            cereal_json.self_check_roundtrip(p)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, str(e))


def _strip_versions(node):
    """Recursively drop every 'cereal_class_version' key - used only to
    confirm the one documented ActionInstructure divergence really is
    exactly that (see KNOWN_LIMITATION_EXTRA_LINES) and nothing else."""
    if isinstance(node, cereal_json.OrderedObj):
        return cereal_json.OrderedObj(
            (k, _strip_versions(v)) for k, v in node.items() if k != "cereal_class_version"
        )
    if isinstance(node, list):
        return [_strip_versions(v) for v in node]
    return node


def _check_tree_roundtrip(r: Reporter, dir_: Path, names: list[str], ext: str, kind: str) -> None:
    from tools.bt import catalog as catalog_mod, reader, writer
    from tools.bt.diffcheck import assert_semantically_equal, assert_bookkeeping_equal

    cat = catalog_mod.load(kind=kind)
    for name in names:
        p = dir_ / f"{name}{ext}"
        try:
            orig_text = cereal_json.read_text(p)
            orig = cereal_json.loads(orig_text)
            tree = reader.read_tree(orig_text, cat=cat, kind=kind)
            if tree.kind != kind:
                raise AssertionError(f"tree.kind {tree.kind!r} != expected {kind!r}")
            rt_text = writer.write_tree(tree)
            rt = cereal_json.loads(rt_text)

            if rt_text == orig_text:
                r.ok(f"{p.name} (byte-identical)")
                continue

            expected_extra = KNOWN_LIMITATION_EXTRA_LINES.get(name)
            if expected_extra is None:
                assert_semantically_equal(orig, rt)
                assert_bookkeeping_equal(orig, rt)
                n = min(len(rt_text), len(orig_text))
                i = next((j for j in range(n) if rt_text[j] != orig_text[j]), n)
                raise AssertionError(
                    f"byte round-trip differs at offset {i}: "
                    f"exp {orig_text[max(0, i-40):i+40]!r} got {rt_text[max(0, i-40):i+40]!r}"
                )

            # known, documented limitation - confirm the divergence is
            # *exactly* that (harmless extra version keys) and nothing else.
            assert_semantically_equal(_strip_versions(orig), _strip_versions(rt))
            extra_lines = len(rt_text.splitlines()) - len(orig_text.splitlines())
            if extra_lines != expected_extra:
                raise AssertionError(
                    f"expected exactly {expected_extra} extra line(s) (known limitation - "
                    f"see docs/BehaviourTree.md), got {extra_lines}"
                )
            r.ok(f"{p.name} (known limitation: {expected_extra} harmless extra "
                f"cereal_class_version key(s) - see docs/BehaviourTree.md)")
        except Exception:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())


def stage_tree_roundtrip(r: Reporter) -> None:
    try:
        from tools.bt import model, reader, writer  # noqa: F401
    except Exception:  # noqa: BLE001
        return  # not built yet
    r.section("stage 2: tree model round-trip")
    _check_tree_roundtrip(r, BT_DIR, TREE_FIXTURES, ".enemyBehaviourData", "enemy")
    _check_tree_roundtrip(r, FRIENDLY_DIR, FRIENDLY_TREE_FIXTURES, ".friendBehaviourData", "friendly")


def stage_meta_roundtrip(r: Reporter) -> None:
    try:
        from tools.bt import meta
    except Exception:  # noqa: BLE001
        return
    r.section("stage 3: .meta round-trip")
    from tools.bt.diffcheck import assert_semantically_equal
    for name in META_FIXTURES:
        p = BT_DIR / f"{name}.enemyBehaviourData.meta"
        try:
            orig = cereal_json.loads(cereal_json.read_text(p))
            info = meta.read_meta(p)
            rt = cereal_json.loads(meta.render_meta(info["name"], info["guid"], info["content_path"]))
            assert_semantically_equal(orig, rt)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())
    for name in FRIENDLY_META_FIXTURES:
        p = FRIENDLY_DIR / f"{name}.friendBehaviourData.meta"
        try:
            orig = cereal_json.loads(cereal_json.read_text(p))
            info = meta.read_meta(p, kind="friendly")
            rt = cereal_json.loads(meta.render_meta(info["name"], info["guid"], info["content_path"],
                                                    kind="friendly"))
            assert_semantically_equal(orig, rt)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())


def stage_new_tree(r: Reporter) -> None:
    try:
        from tools.bt import meta, model, writer
    except Exception:  # noqa: BLE001
        return
    r.section("stage 4: new-tree / meta fidelity vs T-Rex")
    from tools.bt.diffcheck import assert_semantically_equal
    trex = cereal_json.loads(cereal_json.read_text(BT_DIR / "T-Rex.enemyBehaviourData"))
    trex_guid = trex["entryNode_"]["ptr_wrapper"]["data"]["value0"]["guid_"]["value_"]
    trex_pos = trex["entryNode_"]["ptr_wrapper"]["data"]["value0"]["position_"]
    pos = (trex_pos["value0"].value, trex_pos["value1"].value)
    tree = model.Tree(entry=model.Entry(guid=trex_guid, pos=pos, child=None), params=[])
    try:
        got = cereal_json.dumps(cereal_json.loads(writer.write_tree(tree)))
        want = cereal_json.read_text(BT_DIR / "T-Rex.enemyBehaviourData")
        if got != want:
            raise AssertionError("empty-tree shape does not match T-Rex")
        r.ok("empty tree == T-Rex shape")
    except Exception:  # noqa: BLE001
        r.fail("empty tree == T-Rex", traceback.format_exc())
    try:
        info = meta.read_meta(BT_DIR / "T-Rex.enemyBehaviourData.meta")
        got_m = cereal_json.loads(meta.render_meta(info["name"], info["guid"], info["content_path"]))
        want_m = cereal_json.loads(cereal_json.read_text(BT_DIR / "T-Rex.enemyBehaviourData.meta"))
        assert_semantically_equal(want_m, got_m)
        r.ok("meta render == T-Rex.meta")
    except Exception:  # noqa: BLE001
        r.fail("meta render == T-Rex.meta", traceback.format_exc())


def stage_catalog(r: Reporter) -> None:
    try:
        from tools.bt import catalog_scan
    except Exception:  # noqa: BLE001
        return
    r.section("stage 5: catalog freshness")
    ok, msg = catalog_scan.check_fresh(kind="enemy")
    if ok:
        r.ok("catalog.json fresh")
    else:
        r.fail("catalog.json fresh", msg)
    ok, msg = catalog_scan.check_fresh(kind="friendly")
    if ok:
        r.ok("catalog_friendly.json fresh")
    else:
        r.fail("catalog_friendly.json fresh", msg)


def _check_add_remove_sequence(r: Reporter, dir_: Path, name: str, ext: str, kind: str) -> None:
    from tools.bt import catalog as catalog_mod, edits, reader, writer

    p = dir_ / f"{name}{ext}"
    cat = catalog_mod.load(kind=kind)
    try:
        base = cereal_json.read_text(p)
        tree = reader.read_tree(base, cat=cat, kind=kind)
        # add a Sequence under entry's child, then remove it -> identical
        target = tree.entry.child
        if target is None or not hasattr(target, "children"):
            r.ok(f"{p.name} (skipped: no composite root)")
            return
        new_guid = "00000000-0000-4000-8000-000000000001"
        edits.add_node(tree, parent_guid=target.guid, kind="sequence",
                       node_guid=new_guid, cat=cat)
        edits.remove_node(tree, new_guid)
        out = writer.write_tree(tree)
        if out != base:
            raise AssertionError("add-then-remove did not restore the original bytes")
        r.ok(f"{p.name} add/remove sequence inverse")
    except Exception:  # noqa: BLE001
        r.fail(p.name, traceback.format_exc())


def stage_edits(r: Reporter) -> None:
    try:
        from tools.bt import edits  # noqa: F401
    except Exception:  # noqa: BLE001
        return
    r.section("stage 6: edit / inverse round-trip")
    for name in ["TrainingDummy", "HyenaBehaviour"]:
        _check_add_remove_sequence(r, BT_DIR, name, ".enemyBehaviourData", "enemy")
    for name in ["Adventure", "IdleActionInstructure"]:
        _check_add_remove_sequence(r, FRIENDLY_DIR, name, ".friendBehaviourData", "friendly")


def stage_scaffold(r: Reporter) -> None:
    try:
        from tools.bt import scaffold
    except Exception:  # noqa: BLE001
        return
    r.section("stage 7: add-action dry-run + wiring")
    try:
        params = [scaffold.parse_param("speed:float=1.5"), scaffold.parse_param("count:int")]
        log = scaffold.add_action("SelftestProbeAction", "Custom::Probe", params=params,
                                  version=1, dry_run=True)
        joined = "\n".join(log)
        need = ["create", "ActionHeaders.h: +", "vcxproj: + <ClCompile", "vcxproj: + <ClInclude"]
        missing = [n for n in need if n not in joined]
        if missing:
            raise AssertionError(f"dry-run log missing steps: {missing}\n{joined}")
        # confirm nothing was actually written for THIS probe
        probe = (_REPO / "Assets/Scripts/Core/Game/Npc/Enemy/Behaviour/Action"
                 "/Content/Custom/Probe/SelftestProbeAction")
        if probe.exists():
            raise AssertionError("dry-run created files on disk")
        if "SelftestProbeAction" in (_REPO / "NanamiEngine.vcxproj").read_text(encoding="utf-8-sig"):
            raise AssertionError("dry-run touched NanamiEngine.vcxproj")
        r.ok("add-action --dry-run plans 3 wiring points, writes nothing")
    except Exception:  # noqa: BLE001
        r.fail("add-action dry-run", traceback.format_exc())

    try:
        from tools.bt import npc_kind
        params = [scaffold.parse_param("speed:float=1.5")]
        log = scaffold.add_action("SelftestProbeAction", "Custom::Probe", params=params,
                                  version=1, dry_run=True, kind=npc_kind.FRIENDLY)
        joined = "\n".join(log)
        need = ["create", "ActionHeaders.h: +", "vcxproj: + <ClCompile", "vcxproj: + <ClInclude"]
        missing = [n for n in need if n not in joined]
        if missing:
            raise AssertionError(f"dry-run log missing steps: {missing}\n{joined}")
        probe = (_REPO / "Assets/Scripts/Core/Game/Npc/Friendly/Behaviour/Action"
                 "/Content/Custom/Probe/SelftestProbeAction")
        if probe.exists():
            raise AssertionError("dry-run created files on disk")
        if "SelftestProbeAction" in (_REPO / "NanamiEngine.vcxproj").read_text(encoding="utf-8-sig"):
            raise AssertionError("dry-run touched NanamiEngine.vcxproj")
        r.ok("add-action --npc-kind friendly --dry-run plans 3 wiring points, writes nothing")
    except Exception:  # noqa: BLE001
        r.fail("add-action dry-run (friendly)", traceback.format_exc())


def stage_copy_and_nested(r: Reporter) -> None:
    try:
        from tools.bt import edits  # noqa: F401
    except Exception:  # noqa: BLE001
        return
    r.section("stage 8: copy-node + dotted-key nested set-params")
    from tools.bt import edits, reader, writer
    for name in ["TrainingDummy", "HyenaBehaviour"]:
        p = BT_DIR / f"{name}.enemyBehaviourData"
        try:
            base = cereal_json.read_text(p)
            tree = reader.read_tree(base)
            target = tree.entry.child
            if target is None or not hasattr(target, "children"):
                r.ok(f"{p.name} (skipped: no composite root)")
                continue

            # add a PhysicsAttack action, reach its nested attackPower_.value_ and
            # finishedAttackWriteBlackBoard_.keyName_/.value_ (shape='nested', not
            # settable as a bare key) via a dotted key, deep-copy the whole node,
            # confirm the clone is independent, then undo everything -> original bytes.
            action_guid = "00000000-0000-4000-8000-000000000002"
            edits.add_node(tree, parent_guid=target.guid, kind="action",
                           action_type="EnemyStatus::PhysicsAttack", node_guid=action_guid)
            edits.set_params(tree, action_guid, {
                "attackPower_.value_": "7",
                "finishedAttackWriteBlackBoard_.keyName_": "State",
                "finishedAttackWriteBlackBoard_.value_": "1",
            })
            clone = edits.copy_node(tree, src_guid=action_guid, parent_guid=target.guid)
            edits.set_params(tree, clone.guid, {"attackPower_.value_": "13"})

            def power(guid: str) -> int:
                from tools.bt.blob import Ver
                n = tree.find(guid)
                v = n.params["attackPower_"]
                return (v.body if isinstance(v, Ver) else v)["value_"].value

            if power(action_guid) != 7:
                raise AssertionError(f"original attackPower_ changed: {power(action_guid)} != 7")
            if power(clone.guid) != 13:
                raise AssertionError(f"clone attackPower_ not independent: {power(clone.guid)} != 13")

            edits.remove_node(tree, clone.guid)
            edits.remove_node(tree, action_guid)
            out = writer.write_tree(tree)
            if out != base:
                raise AssertionError("add(+dotted set-params)+copy+remove did not restore the original bytes")
            r.ok(f"{p.name} copy-node + dotted set-params, then undo")
        except Exception:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())

    from tools.bt import catalog as catalog_mod
    friendly_cat = catalog_mod.load(kind="friendly")
    for name in ["Adventure"]:
        p = FRIENDLY_DIR / f"{name}.friendBehaviourData"
        try:
            base = cereal_json.read_text(p)
            tree = reader.read_tree(base, cat=friendly_cat, kind="friendly")
            target = tree.entry.child
            if target is None or not hasattr(target, "children"):
                r.ok(f"{p.name} (skipped: no composite root)")
                continue

            # add a MoveForRoute action (a plain, non-nested float param -
            # NpcStatus::RigidBody::MoveForRoute is a real Friendly action),
            # deep-copy it, confirm the clone is independent, then undo
            # everything -> original bytes.
            action_guid = "00000000-0000-4000-8000-000000000003"
            edits.add_node(tree, parent_guid=target.guid, kind="action",
                           action_type="NpcStatus::RigidBody::MoveForRoute",
                           node_guid=action_guid, cat=friendly_cat)
            edits.set_params(tree, action_guid, {"moveSpeed_": "5"}, cat=friendly_cat)
            clone = edits.copy_node(tree, src_guid=action_guid, parent_guid=target.guid)
            edits.set_params(tree, clone.guid, {"moveSpeed_": "9"}, cat=friendly_cat)

            def speed(guid: str) -> float:
                n = tree.find(guid)
                return n.params["moveSpeed_"].value

            if speed(action_guid) != 5:
                raise AssertionError(f"original moveSpeed_ changed: {speed(action_guid)} != 5")
            if speed(clone.guid) != 9:
                raise AssertionError(f"clone moveSpeed_ not independent: {speed(clone.guid)} != 9")

            edits.remove_node(tree, clone.guid)
            edits.remove_node(tree, action_guid)
            out = writer.write_tree(tree)
            if out != base:
                raise AssertionError("add+set-params+copy+remove did not restore the original bytes")
            r.ok(f"{p.name} copy-node + set-params, then undo")
        except Exception:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())


def main() -> int:
    r = Reporter()
    stage_ordered_obj_dup_keys(r)
    stage_formatting(r)
    stage_tree_roundtrip(r)
    stage_meta_roundtrip(r)
    stage_new_tree(r)
    stage_catalog(r)
    stage_edits(r)
    stage_scaffold(r)
    stage_copy_and_nested(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
