"""Static checks on a decoded :class:`tools.bt.model.Tree`."""

from __future__ import annotations

from . import catalog as catalog_mod
from . import model
from .blob import Ptr, Ver
from .cereal_json import Num, OrderedObj


def validate(tree: model.Tree, cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    problems: list[str] = []

    def err(msg: str) -> None:
        problems.append(msg)

    seen: set[str] = set()

    def visit(node) -> None:
        if node.guid in seen:
            err(f"duplicate node guid {node.guid}")
        seen.add(node.guid)

        if isinstance(node, model.RandomSelector):
            if len(node.weights) != len(node.children):
                err(f"RandomSelector {node.guid[:8]}: {len(node.weights)} weights "
                    f"for {len(node.children)} children")
            if node.children and not any(w > 0 for w in node.weights):
                err(f"RandomSelector {node.guid[:8]}: all weights are 0")
        if isinstance(node, (model.OnceExecute, model.OnceSuccess)):
            pass  # single child enforced by the model
        if isinstance(node, model.Action):
            _check_action(node, cat, err)
        for c in model.children_of(node):
            visit(c)

    if tree.entry.child is None:
        # an empty tree is valid (matches T-Rex); note it though
        problems.append("note: tree has no root node (entryNode_.nextNode_ is null)")
    else:
        visit(tree.entry.child)

    for p in tree.params:
        if p.kind != "int":
            err(f"blackboard param {p.name!r}: kind {p.kind!r} unsupported (int only)")

    return problems


def _check_action(node: model.Action, cat: catalog_mod.Catalog, err) -> None:
    entry = cat.action_by_fqn(node.type_fqn)
    if entry is None:
        err(f"action {node.guid[:8]}: unknown type {node.type_fqn!r} "
            f"(not in catalog - run regen-catalog?)")
        return
    if node.params is None:
        err(f"action {node.guid[:8]} ({node.type_name}): no params blob")
        return
    want = [p["key"] for p in cat.params_of(entry)]
    got = [k for k in node.params.keys() if k != "value0"]
    if want != got:
        err(f"action {node.guid[:8]} ({node.type_name}): param keys {got} != catalog {want}")
    for p in cat.params_of(entry):
        if p["shape"] == "unknown":
            err(f"action {node.guid[:8]} ({node.type_name}): param {p['member']} has "
                f"unknown shape (type {p.get('type')!r}); round-trips but not settable")
