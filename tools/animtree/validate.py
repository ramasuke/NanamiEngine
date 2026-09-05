"""Static checks on a decoded :class:`tools.animtree.model.Tree`.

A ``note:``-prefixed problem is informational (the CLI still writes the file);
anything else is a hard failure that aborts the write with nothing changed -
the same convention ``tools.bt``/``tools.scene`` use.
"""

from __future__ import annotations

from . import catalog as catalog_mod
from . import model

# a real .animTree can accumulate a stray, absurdly large float in a member
# like blendAnimationOffset_secs_ from editor drag/uninitialised-memory
# artifacts (see Assets/Animations/SwordManAnimation.animTree's ComboAttack1
# node, 111111112360531590000.0) - flag it, but only as a note: it round-trips
# losslessly and isn't this toolkit's place to silently "fix".
_SUSPICIOUS_ABS = 1e6


def validate(tree: model.Tree, cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    problems: list[str] = []

    def err(msg: str) -> None:
        problems.append(msg)

    # -- node guids -----------------------------------------------------
    seen: set[str] = set()
    for node in (tree.entry, tree.any_state, *tree.nodes):
        if node.guid in seen:
            err(f"duplicate node guid {node.guid}")
        seen.add(node.guid)
        _check_node(node, cat, err)

    all_guids = seen

    # -- transitions ------------------------------------------------------
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

    # -- params -------------------------------------------------------------
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
    for pinfo in cat.params_of(entry):
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
