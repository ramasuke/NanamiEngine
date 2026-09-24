"""復号済み :class:`tools.animtree.model.Tree` の静的チェック。

``note:`` で始まる問題は情報のみ（CLI はファイルを書き出す）。
それ以外は致命的な失敗で、何も変更せずに書き込みを中止する -
``tools.bt``/``tools.scene`` と同じ規約。
"""

from __future__ import annotations

from . import catalog as catalog_mod
from . import model

# 実際の .animTree には、エディタのドラッグや未初期化メモリの名残で
# blendAnimationOffset_secs_ のようなメンバーに桁外れに大きい float が紛れ込むことがある
# （Assets/Animations/SwordManAnimation.animTree の ComboAttack1 ノードの
# 111111112360531590000.0 など）。指摘はするが note のみ: 劣化なく往復するし、
# このツールキットが黙って「直す」ものではない。
_SUSPICIOUS_ABS = 1e6


def validate(tree: model.Tree, cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    problems: list[str] = []

    def err(msg: str) -> None:
        problems.append(msg)

    # -- ノードの guid -----------------------------------------------------
    seen: set[str] = set()
    for node in (tree.entry, tree.any_state, *tree.nodes):
        if node.guid in seen:
            err(f"duplicate node guid {node.guid}")
        seen.add(node.guid)
        _check_node(node, cat, err)

    all_guids = seen

    # -- 遷移 ------------------------------------------------------
    for t in tree.transitions:
        _check_transition_endpoints(t, all_guids, err)
        if t.from_guid == tree.any_state.guid:
            err(f"transition {t.from_guid} -> {t.next_guid}: sourced from the AnyState node "
                f"but stored in the direct transition list (belongs in any-state transitions)")
        _check_conditions(t, tree, err)

    for t in tree.any_state_transitions:
        _check_transition_endpoints(t, all_guids, err)
        if t.from_guid != tree.any_state.guid:
            err(f"any-state transition -> {t.next_guid}: from_guid {t.from_guid} is not the "
                f"AnyState node's guid ({tree.any_state.guid})")
        _check_conditions(t, tree, err)

    # -- パラメータ -------------------------------------------------------------
    for p in tree.params:
        if p.kind not in model.KINDS:
            err(f"parameter {p.name!r}: kind {p.kind!r} unsupported (want bool|int|float)")

    return problems


def _check_node(node: model.Node, cat: catalog_mod.Catalog, err) -> None:
    entry = cat.node_by_fqn(node.type_fqn)
    if entry is None:
        err(f"node {node.guid}: unknown type {node.type_fqn!r} (not in catalog - run regen-catalog?)")
        return
    if node.params is None:
        err(f"node {node.guid} ({entry.get('leaf')}): no params blob")
        return
    for pinfo in cat.params_for_version(entry, int(node.class_version)):
        if pinfo.get("shape") in ("self_guid", "self_pos"):
            continue
        key = pinfo["key"]
        if key not in node.params:
            err(f"node {node.guid} ({entry.get('leaf')}): param {key!r} missing "
                f"(catalog/blob out of sync - regen-catalog?)")
            continue
        if pinfo.get("shape") == "unknown":
            err(f"note: node {node.guid} ({entry.get('leaf')}): param {pinfo['member']} has "
                f"unknown shape (type {pinfo.get('type')!r}); round-trips but not settable")
    member = pinfo_by_member(cat, entry, "blendAnimationOffset_secs_")
    if member is not None and member["key"] in node.params:
        val = model.numval(node.params[member["key"]])
        if isinstance(val, (int, float)) and abs(val) > _SUSPICIOUS_ABS:
            err(f"note: node {node.guid} ({entry.get('leaf')}): blendAnimationOffset_secs_ "
                f"looks like garbage ({val!r}); round-trips losslessly, not auto-corrected")


def pinfo_by_member(cat: catalog_mod.Catalog, entry, member: str):
    for p in cat.params_of(entry):
        if p.get("member") == member:
            return p
    return None


def _check_transition_endpoints(t: model.Transition, all_guids: set[str], err) -> None:
    for label, guid in (("from", t.from_guid), ("next", t.next_guid), ("visual_from", t.visual_from_guid)):
        if guid not in all_guids:
            err(f"transition {t.from_guid} -> {t.next_guid}: {label} guid {guid} does not "
                f"resolve to any node")


def _check_conditions(t: model.Transition, tree: model.Tree, err) -> None:
    for c in t.conditions:
        p = tree.find_param(c.name)
        if p is None:
            err(f"transition {t.from_guid} -> {t.next_guid}: condition references unknown "
                f"parameter {c.name!r}")
            continue
        if p.kind != c.kind:
            err(f"transition {t.from_guid} -> {t.next_guid}: condition {c.name!r} kind "
                f"{c.kind!r} != parameter kind {p.kind!r}")
