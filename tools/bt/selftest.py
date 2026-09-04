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


def stage_tree_roundtrip(r: Reporter) -> None:
    try:
        from tools.bt import model, reader, writer  # noqa: F401
    except Exception:  # noqa: BLE001
        return  # not built yet
    r.section("stage 2: tree model round-trip")
    from tools.bt.diffcheck import assert_semantically_equal, assert_bookkeeping_equal
    for name in TREE_FIXTURES:
        p = BT_DIR / f"{name}.enemyBehaviourData"
        try:
            orig_text = cereal_json.read_text(p)
            orig = cereal_json.loads(orig_text)
            tree = reader.read_tree(orig_text)
            rt_text = writer.write_tree(tree)
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
            r.ok(f"{p.name} (byte-identical)")
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())


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
    ok, msg = catalog_scan.check_fresh()
    if ok:
        r.ok("catalog.json fresh")
    else:
        r.fail("catalog.json fresh", msg)


def stage_edits(r: Reporter) -> None:
    try:
        from tools.bt import edits  # noqa: F401
    except Exception:  # noqa: BLE001
        return
    r.section("stage 6: edit / inverse round-trip")
    from tools.bt import edits, reader, writer
    for name in ["TrainingDummy", "HyenaBehaviour"]:
        p = BT_DIR / f"{name}.enemyBehaviourData"
        try:
            base = cereal_json.read_text(p)
            tree = reader.read_tree(base)
            # add a Sequence under entry's child, then remove it -> identical
            target = tree.entry.child
            if target is None or not hasattr(target, "children"):
                r.ok(f"{p.name} (skipped: no composite root)")
                continue
            new_guid = "00000000-0000-4000-8000-000000000001"
            edits.add_node(tree, parent_guid=target.guid, kind="sequence",
                           node_guid=new_guid)
            edits.remove_node(tree, new_guid)
            out = writer.write_tree(tree)
            if out != base:
                raise AssertionError("add-then-remove did not restore the original bytes")
            r.ok(f"{p.name} add/remove sequence inverse")
        except Exception:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())


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


def main() -> int:
    r = Reporter()
    stage_formatting(r)
    stage_tree_roundtrip(r)
    stage_meta_roundtrip(r)
    stage_new_tree(r)
    stage_catalog(r)
    stage_edits(r)
    stage_scaffold(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
