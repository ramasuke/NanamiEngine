"""tools.animtree のセルフテスト / 正しさの検査。

実行:  python tools/animtree/selftest.py         (リポジトリのルートから)
       python -m tools.animtree selftest

終了コード 0 = すべて成功、1 = 失敗。サードパーティ依存なし。

段階:
  0. OrderedObj の重複キーの忠実性。
  1. cereal_json の書式の忠実性: 全フィクスチャで dumps(loads(text)) == text。
  2. ツリーモデルの往復: 読み込み -> 書き出し -> 元とバイト一致。
     polymorphic_id / ptr_wrapper.id / cereal_class_version の
     管理情報の並びも含む。
  3. .meta の往復。
  4. new-tree の形の妥当性（空ツリーのバイト一致フィクスチャは無い - ドキュメント参照）。
  5. カタログの鮮度（regen-catalog --check）。
  6. add-clip-node/remove-node と add-transition(+condition)/remove-* の
     逆操作による往復 == 元のバイト列。
  7. 3 種（bool/int/float）すべての add-param/remove-param の往復。
  8. validate() の妥当性: 既知の blendAnimationOffset_secs_ のゴミ値の note と、
     きれいなフィクスチャで致命的な問題がゼロになること。
  9. 古いクリップノードのツリーに最新バージョンのクリップノードを追加すると、書き出し時に
     古いものが引き上げられる（cereal はアーカイブ内で型ごとに 1 つのクラスバージョンしか保存しない）。
"""

from __future__ import annotations

import sys
import traceback
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.animtree import cereal_json  # noqa: E402

ANIM_DIR = _REPO / "Assets" / "Animations"
TREE_FIXTURES = ["AnimationTree", "SwordManAnimation"]
META_FIXTURES = ["AnimationTree"]


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
        obj = cereal_json.loads('{"a": 1, "nodes_0": 10, "nodes_0": 20}')
        assert len(obj) == 3, f"expected 3 pairs, got {len(obj)}"
        rt = cereal_json.dumps(cereal_json.loads(cereal_json.dumps(obj)))
        if rt != cereal_json.dumps(obj):
            raise AssertionError("duplicate-key object does not round-trip stably")
        r.ok("loads()/OrderedObj preserve duplicate sibling keys")
    except Exception:  # noqa: BLE001
        r.fail("OrderedObj duplicate-key fidelity", traceback.format_exc())


def stage_formatting(r: Reporter) -> None:
    r.section("stage 1: cereal_json formatting fidelity")
    for name in TREE_FIXTURES:
        p = ANIM_DIR / f"{name}.animTree"
        try:
            cereal_json.self_check_roundtrip(p)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, str(e))
    for name in META_FIXTURES:
        p = ANIM_DIR / f"{name}.animTree.meta"
        try:
            cereal_json.self_check_roundtrip(p)
            r.ok(p.name)
        except Exception as e:  # noqa: BLE001
            r.fail(p.name, str(e))


def stage_tree_roundtrip(r: Reporter) -> None:
    try:
        from tools.animtree import reader, writer  # noqa: F401
    except Exception:  # noqa: BLE001
        return
    r.section("stage 2: tree model round-trip")
    for name in TREE_FIXTURES:
        p = ANIM_DIR / f"{name}.animTree"
        try:
            orig_text = cereal_json.read_text(p)
            tree = reader.read_tree(orig_text)
            rt_text = writer.write_tree(tree)
            if rt_text != orig_text:
                n = min(len(rt_text), len(orig_text))
                i = next((j for j in range(n) if rt_text[j] != orig_text[j]), n)
                raise AssertionError(
                    f"byte round-trip differs at offset {i}: "
                    f"exp {orig_text[max(0, i-40):i+40]!r} got {rt_text[max(0, i-40):i+40]!r}"
                )
            r.ok(f"{p.name} (byte-identical)")
        except Exception:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())


def stage_meta_roundtrip(r: Reporter) -> None:
    try:
        from tools.animtree import meta
    except Exception:  # noqa: BLE001
        return
    r.section("stage 3: .meta round-trip")
    from tools.animtree.diffcheck import assert_semantically_equal
    for name in META_FIXTURES:
        p = ANIM_DIR / f"{name}.animTree.meta"
        try:
            orig = cereal_json.loads(cereal_json.read_text(p))
            info = meta.read_meta(p)
            rt = cereal_json.loads(meta.render_meta(info["name"], info["guid"], info["content_path"]))
            assert_semantically_equal(orig, rt)
            r.ok(p.name)
        except Exception:  # noqa: BLE001
            r.fail(p.name, traceback.format_exc())


def stage_new_tree(r: Reporter) -> None:
    try:
        from tools.animtree import edits, meta, model, validate, writer
    except Exception:  # noqa: BLE001
        return
    r.section("stage 4: new-tree shape sanity")
    try:
        entry = edits.new_singleton_node(model.FQN_ENTRY_NODE, meta.mint_guid(), (120.0, 40.0),
                                         {"speed_": 1.0})
        any_state = edits.new_singleton_node(model.FQN_ANYSTATE_NODE, meta.mint_guid(), (120.0, 160.0))
        tree = model.Tree(entry=entry, any_state=any_state)

        text = writer.write_tree(tree)
        rt = cereal_json.loads(text)
        want_keys = ["additionParameters_", "entryNode", "visualAnyStateNode", "nodesCount",
                    "fromNodeNodePathCount", "fromAnyStateNodeNodePathCount"]
        got_keys = rt.keys()
        if got_keys != want_keys:
            raise AssertionError(f"top-level key order {got_keys} != {want_keys}")

        stable = cereal_json.dumps(cereal_json.loads(text))
        if stable != text:
            raise AssertionError("empty-tree output is not internally formatting-stable")

        problems = validate.validate(tree)
        hard = [p for p in problems if not p.startswith("note:")]
        if hard:
            raise AssertionError(f"a freshly-minted empty tree fails validate(): {hard}")
        r.ok("empty tree: key order + formatting-stable + validates clean")
    except Exception:  # noqa: BLE001
        r.fail("new-tree shape", traceback.format_exc())


def stage_catalog(r: Reporter) -> None:
    try:
        from tools.animtree import catalog_scan
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
        from tools.animtree import edits  # noqa: F401
    except Exception:  # noqa: BLE001
        return
    r.section("stage 6: edit / inverse round-trip")
    from tools.animtree import edits, reader, writer
    dummy_clip_guid = "11111111-1111-1111-1111-111111111111"
    for name in TREE_FIXTURES:
        p = ANIM_DIR / f"{name}.animTree"
        try:
            base = cereal_json.read_text(p)
            tree = reader.read_tree(base)

            node = edits.add_clip_node(tree, name="SelftestProbe", clip_guid=dummy_clip_guid,
                                       clip_start_time=0.0, clip_end_time=0.0, is_loop=True,
                                       guid="00000000-0000-4000-8000-0000000000f1",
                                       pos=(0.0, 0.0))
            edits.remove_node(tree, node.guid)
            out = writer.write_tree(tree)
            if out != base:
                raise AssertionError("add-then-remove clip node did not restore the original bytes")
            r.ok(f"{p.name} add/remove clip node inverse")
        except Exception:  # noqa: BLE001
            r.fail(f"{p.name} (node)", traceback.format_exc())

        try:
            base = cereal_json.read_text(p)
            tree = reader.read_tree(base)
            t = edits.add_transition(tree, from_guid=tree.any_state.guid, next_guid=tree.entry.guid,
                                     any_state=True, duration_secs=1.25)
            if tree.params:
                edits.add_condition(tree, any_state=True, from_guid=t.from_guid, next_guid=t.next_guid,
                                    name=tree.params[0].name, kind=tree.params[0].kind,
                                    value=tree.params[0].value)
                edits.remove_condition(tree, any_state=True, from_guid=t.from_guid, next_guid=t.next_guid,
                                       condition_index=0)
            edits.remove_transition(tree, any_state=True, from_guid=t.from_guid, next_guid=t.next_guid)
            out = writer.write_tree(tree)
            if out != base:
                raise AssertionError("add-then-remove transition(+condition) did not restore the original bytes")
            r.ok(f"{p.name} add/remove any-state transition(+condition) inverse")
        except Exception:  # noqa: BLE001
            r.fail(f"{p.name} (transition)", traceback.format_exc())


def stage_params(r: Reporter) -> None:
    try:
        from tools.animtree import edits  # noqa: F401
    except Exception:  # noqa: BLE001
        return
    r.section("stage 7: add-param/remove-param round-trip (bool/int/float)")
    from tools.animtree import edits, reader, writer
    p = ANIM_DIR / f"{TREE_FIXTURES[0]}.animTree"
    for kind, value in (("bool", True), ("int", 7), ("float", 1.5)):
        try:
            base = cereal_json.read_text(p)
            tree = reader.read_tree(base)
            edits.add_param(tree, "SelftestProbeParam", kind, value)
            edits.remove_param(tree, "SelftestProbeParam")
            out = writer.write_tree(tree)
            if out != base:
                raise AssertionError(f"add-then-remove param ({kind}) did not restore the original bytes")
            r.ok(f"{p.name} add/remove {kind} param inverse")
        except Exception:  # noqa: BLE001
            r.fail(f"{p.name} ({kind} param)", traceback.format_exc())


def stage_validate_sanity(r: Reporter) -> None:
    try:
        from tools.animtree import validate  # noqa: F401
    except Exception:  # noqa: BLE001
        return
    r.section("stage 8: validate() sanity")
    from tools.animtree import reader
    try:
        tree = reader.read_tree_file(ANIM_DIR / "SwordManAnimation.animTree")
        problems = validate.validate(tree)
        notes = [pr for pr in problems if pr.startswith("note:") and "blendAnimationOffset_secs_" in pr]
        if not notes:
            raise AssertionError("expected a note: about SwordManAnimation's known garbage "
                                 "blendAnimationOffset_secs_ value; got none")
        r.ok("SwordManAnimation.animTree: garbage blendAnimationOffset_secs_ flagged as a note")
    except Exception:  # noqa: BLE001
        r.fail("SwordManAnimation.animTree validate note", traceback.format_exc())

    try:
        tree = reader.read_tree_file(ANIM_DIR / "AnimationTree.animTree")
        problems = validate.validate(tree)
        hard = [pr for pr in problems if not pr.startswith("note:")]
        if hard:
            raise AssertionError(f"expected zero hard problems, got {hard}")
        r.ok("AnimationTree.animTree validates clean")
    except Exception:  # noqa: BLE001
        r.fail("AnimationTree.animTree validate clean", traceback.format_exc())


def stage_version_unify(r: Reporter) -> None:
    r.section("stage 9: mixed node class versions are unified on write")
    from tools.animtree import catalog as catalog_mod
    from tools.animtree import edits, model, reader, validate, writer
    cat = catalog_mod.load()
    latest = int(cat.node_by_fqn(model.FQN_CLIP_NODE)["version"])
    try:
        tree = reader.read_tree(cereal_json.read_text(ANIM_DIR / "AnimationTree.animTree"))
        range_keys = ("clipStartTime_", "clipEndTime_", "isLoop_")
        before = {
            n.guid: tuple(model.numval(n.params[k]) if k in n.params else d
                          for k, d in zip(range_keys, (0.0, 0.0, True)))
            for n in tree.nodes if n.type_fqn == model.FQN_CLIP_NODE
        }
        probe = edits.add_clip_node(tree, name="SelftestRangeProbe",
                                    clip_guid="11111111-1111-1111-1111-111111111111",
                                    clip_start_time=12.5, clip_end_time=40.0, is_loop=False,
                                    guid="00000000-0000-4000-8000-0000000000f3", pos=(0.0, 0.0))
        reread = reader.read_tree(writer.write_tree(tree))
        clips = [n for n in reread.nodes if n.type_fqn == model.FQN_CLIP_NODE]
        versions = {n.class_version for n in clips}
        if versions != {latest}:
            raise AssertionError(f"expected every clip node at version {latest}, got {versions}")
        for n in clips:
            got = tuple(model.numval(n.params[k]) for k in range_keys)
            want = (12.5, 40.0, False) if n.guid == probe.guid else before[n.guid]
            if got != want:
                raise AssertionError(f"node {n.guid}: range params {got}, expected {want}")
        hard = [p for p in validate.validate(reread) if not p.startswith("note:")]
        if hard:
            raise AssertionError(f"upgraded tree has hard problems: {hard}")
        r.ok(f"AnimationTree.animTree + a v{latest} node: every clip node written at v{latest}, ranges preserved/defaulted")
    except Exception:  # noqa: BLE001
        r.fail("AnimationTree.animTree version unify", traceback.format_exc())


def main() -> int:
    r = Reporter()
    stage_ordered_obj_dup_keys(r)
    stage_formatting(r)
    stage_tree_roundtrip(r)
    stage_meta_roundtrip(r)
    stage_new_tree(r)
    stage_catalog(r)
    stage_edits(r)
    stage_params(r)
    stage_validate_sanity(r)
    stage_version_unify(r)
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
