"""Structural + parameter edits on a :class:`tools.bt.model.Tree`.

Every function mutates the tree in place and is designed so a change followed by
its inverse restores the original bytes (see selftest stage 6). ``apply`` runs a
batch of edit ops atomically (build -> apply all -> caller validates -> one write).
"""

from __future__ import annotations

import copy
from typing import Any, Optional

from . import catalog as catalog_mod
from . import model
from .blob import Ptr, Ver
from .cereal_json import Num, OrderedObj

EMPTY_GUID = "00000000-0000-0000-0000-000000000000"

_KIND_ALIASES = {
    "selector": model.Selector,
    "sequence": model.Sequence,
    "random": model.RandomSelector,
    "random-selector": model.RandomSelector,
    "once-exec": model.OnceExecute,
    "once-execute": model.OnceExecute,
    "once-success": model.OnceSuccess,
    "action": model.Action,
}


class EditError(RuntimeError):
    pass


# ---------------------------------------------------------------------------
# blob builders (from-scratch actions)
# ---------------------------------------------------------------------------
def _leaf(name: str) -> str:
    return name.split("<", 1)[0].rsplit("::", 1)[-1]


def _default_scalar(shape: str, default: Any):
    if shape in ("int", "enum"):
        try:
            return Num.of_int(int(default))
        except (TypeError, ValueError):
            return Num.of_int(0)
    if shape == "float":
        try:
            return Num.of_float(float(default))
        except (TypeError, ValueError):
            return Num.of_float(0.0)
    if shape == "bool":
        return bool(default) if isinstance(default, bool) else str(default).lower() == "true"
    if shape == "string":
        return "" if default is None else str(default)
    return Num.of_int(0)


def _vec(n: int) -> OrderedObj:
    return OrderedObj([(f"value{i}", Num.of_float(0.0)) for i in range(n)])


def _field_blob(field_type_leaf: str, guid: str = EMPTY_GUID) -> Ver:
    guid_ver = Ver(("type", "Guid"), 0, OrderedObj([("value_", guid)]))
    holder = Ver(("type", f"FieldHolder<{field_type_leaf}>"), 0,
                 OrderedObj([("value0", guid_ver)]))
    ptr = Ptr(exact=True, null=False, fqn=None, wrapper="shared", data=holder)
    return Ver(("type", f"Field<{field_type_leaf}>"), 0, OrderedObj([("value0", ptr)]))


def _param_blob(cat: catalog_mod.Catalog, pinfo: dict) -> Any:
    shape = pinfo.get("shape")
    if shape in ("int", "float", "bool", "string", "enum"):
        return _default_scalar(shape, pinfo.get("default"))
    if shape == "vec2":
        return _vec(2)
    if shape == "vec3":
        return _vec(3)
    if shape == "quat":
        return _vec(4)
    if shape == "field":
        return _field_blob(_leaf(pinfo.get("type", "?")))
    if shape == "vector":
        return []
    if shape == "nested":
        leaf = _leaf(pinfo.get("type", "?"))
        sub = cat.type_by_leaf(leaf)
        body = OrderedObj()
        if cat.action_by_leaf(leaf):  # nested type derives from ActionBase
            body.append("value0", Ver(("type", "ActionBase"), 0, OrderedObj()))
        for sp in cat.params_of(sub):
            body.append(sp["key"], _param_blob(cat, sp))
        return Ver(("type", leaf), int((sub or {}).get("version", 0)), body)
    # unknown -> emit a 0 so the file still loads; validate will flag it
    return Num.of_int(0)


def action_blob(cat: catalog_mod.Catalog, entry: dict) -> OrderedObj:
    out = OrderedObj()
    out.append("value0", Ver(("type", "ActionBase"), 0, OrderedObj()))
    for p in cat.params_of(entry):
        out.append(p["key"], _param_blob(cat, p))
    return out


# ---------------------------------------------------------------------------
# navigation
# ---------------------------------------------------------------------------
def _find_parent(tree: model.Tree, guid: str):
    """Return (parent_node, container_list_or_None, index) for the node `guid`."""
    if tree.entry.child is not None and tree.entry.child.guid == guid:
        return tree.entry, None, 0
    for node, parent, _c, _i in tree.walk():
        kids = model.children_of(parent) if parent is not tree.entry else []
        for i, k in enumerate(kids):
            if k.guid == guid:
                return parent, kids, i
    raise EditError(f"node not found: {guid}")


def _resolve_parent(tree: model.Tree, guid: str):
    if guid in ("entry", tree.entry.guid):
        return tree.entry
    n = tree.find(guid)
    if n is None:
        raise EditError(f"parent not found: {guid}")
    return n


# ---------------------------------------------------------------------------
# structural edits
# ---------------------------------------------------------------------------
def _make_node(kind: str, *, name: Optional[str], action_type: Optional[str],
               pos, node_guid: str, cat: catalog_mod.Catalog):
    cls = _KIND_ALIASES.get(kind)
    if cls is None:
        raise EditError(f"unknown node kind: {kind!r}")
    pos = tuple(pos) if pos else (0.0, 0.0)
    if cls is model.Action:
        if not action_type:
            raise EditError("an action node needs --type")
        entry = cat.resolve_action(action_type)
        if entry is None:
            raise EditError(f"unknown action type: {action_type!r} (see: regen-catalog)")
        return model.Action(guid=node_guid, pos=pos, name=name or entry["class"],
                            type_fqn=entry["fqn"], action_version=int(entry.get("version", 0)),
                            params=action_blob(cat, entry))
    if cls is model.RandomSelector:
        return model.RandomSelector(guid=node_guid, pos=pos)
    if cls in (model.Selector, model.Sequence):
        return cls(guid=node_guid, pos=pos)
    if cls is model.OnceExecute:
        return model.OnceExecute(guid=node_guid, pos=pos)
    return model.OnceSuccess(guid=node_guid, pos=pos)


def _attach(parent, node, index: Optional[int], weight: int) -> None:
    if isinstance(parent, model.Entry):
        if parent.child is not None:
            raise EditError("entry already has a child; remove or move it first")
        parent.child = node
    elif isinstance(parent, (model.Selector, model.Sequence)):
        i = len(parent.children) if index is None else index
        parent.children.insert(i, node)
    elif isinstance(parent, model.RandomSelector):
        i = len(parent.children) if index is None else index
        parent.children.insert(i, node)
        parent.weights.insert(i, int(weight))
    elif isinstance(parent, (model.OnceExecute, model.OnceSuccess)):
        if parent.child is not None:
            raise EditError(f"{type(parent).__name__} already has a child")
        parent.child = node
    else:
        raise EditError(f"{type(parent).__name__} cannot take children")


def add_node(tree: model.Tree, *, parent_guid: str, kind: str, name: str | None = None,
             action_type: str | None = None, index: int | None = None, weight: int = 100,
             pos=None, node_guid: str | None = None,
             cat: catalog_mod.Catalog | None = None):
    cat = cat or catalog_mod.load()
    from . import meta as _meta
    node = _make_node(kind, name=name, action_type=action_type, pos=pos,
                      node_guid=node_guid or _meta.mint_guid(), cat=cat)
    _attach(_resolve_parent(tree, parent_guid), node, index, weight)
    return node


def _clone_node(node, mint):
    """Deep-copy a node subtree, minting a fresh guid at every level (the
    "pure tree" format forbids two nodes sharing a guid) and deep-copying
    each Action's param blob so editing the clone can never mutate the
    original."""
    if isinstance(node, model.Action):
        return model.Action(guid=mint(), pos=node.pos, name=node.name,
                            type_fqn=node.type_fqn, action_version=node.action_version,
                            params=copy.deepcopy(node.params))
    if isinstance(node, model.RandomSelector):
        return model.RandomSelector(guid=mint(), pos=node.pos,
                                    children=[_clone_node(c, mint) for c in node.children],
                                    weights=list(node.weights))
    if isinstance(node, (model.Selector, model.Sequence)):
        return type(node)(guid=mint(), pos=node.pos,
                          children=[_clone_node(c, mint) for c in node.children])
    if isinstance(node, model.OnceExecute):
        return model.OnceExecute(guid=mint(), pos=node.pos, state=node.state,
                                 child=_clone_node(node.child, mint) if node.child else None)
    if isinstance(node, model.OnceSuccess):
        return model.OnceSuccess(guid=mint(), pos=node.pos,
                                 child=_clone_node(node.child, mint) if node.child else None)
    raise EditError(f"cannot copy a node of type {type(node).__name__}")


def copy_node(tree: model.Tree, *, src_guid: str, parent_guid: str,
             index: int | None = None, weight: int = 100, pos=None):
    """Deep-copy the subtree at ``src_guid`` and attach the copy under
    ``parent_guid``. The two most common uses this unblocks: duplicating one
    weighted branch of a `RandomSelector` with a couple of fields tweaked
    (rather than rebuilding it node-by-node with `add-node`), and cloning a
    whole alternate branch (e.g. an "enraged" attack pool) from an existing
    one so only the diff needs hand-authoring afterwards.
    """
    src = tree.find(src_guid)
    if src is None:
        raise EditError(f"node not found: {src_guid}")
    if isinstance(src, model.Entry):
        raise EditError("cannot copy the entry node")
    from . import meta as _meta
    clone = _clone_node(src, _meta.mint_guid)
    if pos is not None:
        clone.pos = tuple(pos)
    _attach(_resolve_parent(tree, parent_guid), clone, index, weight)
    return clone


def _detach(tree: model.Tree, guid: str):
    parent, kids, idx = _find_parent(tree, guid)
    if kids is None:  # entry child
        node = parent.child
        parent.child = None
        return node
    node = kids[idx]
    kids.pop(idx)
    if isinstance(parent, model.RandomSelector) and idx < len(parent.weights):
        parent.weights.pop(idx)
    if isinstance(parent, (model.OnceExecute, model.OnceSuccess)):
        parent.child = None
    return node


def remove_node(tree: model.Tree, guid: str):
    return _detach(tree, guid)


def move_node(tree: model.Tree, *, guid: str, parent_guid: str,
              index: int | None = None, weight: int = 100):
    node = _detach(tree, guid)
    _attach(_resolve_parent(tree, parent_guid), node, index, weight)
    return node


# ---------------------------------------------------------------------------
# parameter edits
# ---------------------------------------------------------------------------
def _coerce(shape: str, raw: str):
    if shape in ("int", "enum"):
        return Num.of_int(int(raw, 0))
    if shape == "float":
        return Num.of_float(float(raw))
    if shape == "bool":
        return raw.strip().lower() in ("1", "true", "yes", "on")
    if shape == "string":
        return raw
    if shape in ("vec2", "vec3", "quat"):
        parts = [p.strip() for p in raw.replace(" ", ",").split(",") if p.strip()]
        return OrderedObj([(f"value{i}", Num.of_float(float(x))) for i, x in enumerate(parts)])
    if shape == "field":
        return raw.strip().upper()
    raise EditError(f"cannot set a param of shape {shape!r}")


def _unwrap_nested(val):
    """The writable field container of a shape='nested' param's current value,
    whichever representation this occurrence happens to use: ``Ver``-wrapped
    (this exact slot carries a literal ``cereal_class_version`` - always true
    for a struct freshly scaffolded by :func:`add_node`) or a plain
    ``OrderedObj`` (an existing occurrence of a struct type this particular
    file never version-tracks - see ``_tag_value`` in reader.py). Both forms
    round-trip identically; this just picks the dict to index into next.
    """
    return val.body if isinstance(val, Ver) else val


def _set_field_guid(val, guid: str) -> None:
    """Write a shape='field' param's guid, whichever representation this slot
    ended up in: the fully ``Ver``-tagged form (``Ver -> Ptr -> Ver -> Ver``)
    a top-level catalog-typed member gets, or the flattened Ptr-only form
    (``Ptr -> OrderedObj -> OrderedObj``) a FIELD(T) buried two-or-more levels
    inside a plain-tagged (pinfo-losing) existing struct falls back to - see
    ``_tag_value``/``_tag_plain`` in reader.py for why the two forms exist.
    """
    outer = val.body if isinstance(val, Ver) else val
    holder = outer["value0"].data
    inner = holder.body if isinstance(holder, Ver) else holder
    guid_slot = inner["value0"]
    target = guid_slot.body if isinstance(guid_slot, Ver) else guid_slot
    target["value_"] = guid


def _leaf_param(cat: catalog_mod.Catalog, entry: Optional[dict], key: str) -> dict:
    for p in cat.params_of(entry):
        if key in (p["key"], p["member"]):
            return p
    type_name = (entry or {}).get("class", entry)
    raise EditError(f"{type_name}: no param {key!r} (see: show / validate)")


def _set_nested_param(cat: catalog_mod.Catalog, node: model.Action, entry: Optional[dict],
                      dotted_key: str, raw: str) -> str:
    """Resolve a dotted path (e.g. ``attackPower_.value_`` or
    ``spawnPosition_.targetObject_``) through one or more shape='nested'
    struct members and write the leaf, working for both freshly-added nodes
    and existing ones (see :func:`_unwrap_nested` / :func:`_set_field_guid`).
    """
    parts = dotted_key.split(".")
    cur_entry = entry
    container = node.params
    for i, part in enumerate(parts):
        pinfo = _leaf_param(cat, cur_entry, part)
        jkey = pinfo["key"]
        if jkey not in container:
            raise EditError(f"param {jkey!r} missing from the stored blob; regen-catalog?")
        val = container[jkey]
        if i == len(parts) - 1:
            shape = pinfo["shape"]
            if shape == "field":
                _set_field_guid(val, _coerce("field", raw))
            elif shape in catalog_mod.SETTABLE_SHAPES:
                container[jkey] = _coerce(shape, raw)
            else:
                raise EditError(
                    f"{node.type_name}: {dotted_key} has shape {shape!r}; not settable"
                )
            return pinfo["member"]
        if pinfo["shape"] != "nested":
            raise EditError(
                f"{node.type_name}: {'.'.join(parts[:i + 1])} is not a nested struct "
                f"(shape {pinfo['shape']!r}); cannot descend into it"
            )
        container = _unwrap_nested(val)
        cur_entry = cat.type_by_leaf(pinfo.get("type", "?"))
        if cur_entry is None:
            raise EditError(f"unknown nested struct type {pinfo.get('type')!r}")
    raise EditError("empty dotted key")  # unreachable: dotted_key.split always has >=1 part


def set_params(tree: model.Tree, guid: str, assignments: dict[str, str],
               cat: catalog_mod.Catalog | None = None) -> list[str]:
    """Set one or more of an action's parameters.

    A plain key (``rate_``) sets a top-level, directly-settable param, exactly
    as before. A dotted key (``attackPower_.value_``,
    ``spawnPosition_.targetObject_``, ``spawnPosition_.offset_``) reaches
    inside shape='nested' struct members - e.g. the ``PhysicsPower``/
    ``Position``/``WriteBlackBoard``/``WaitSeconds``/``PlaySE`` structs
    embedded in ``PhysicsAttack``/``RadiateProjectile``/``PlayAnimation`` -
    which a bare key cannot address (nested's own shape is never in
    ``SETTABLE_SHAPES``).
    """
    cat = cat or catalog_mod.load()
    node = tree.find(guid)
    if not isinstance(node, model.Action):
        raise EditError(f"{guid} is not an action node")
    entry = cat.action_by_fqn(node.type_fqn)
    touched: list[str] = []
    for key, raw in assignments.items():
        if "." in key:
            touched.append(_set_nested_param(cat, node, entry, key, raw))
            continue
        pinfo = _leaf_param(cat, entry, key)
        shape = pinfo["shape"]
        if shape not in catalog_mod.SETTABLE_SHAPES:
            raise EditError(
                f"{node.type_name}.{pinfo['member']} has shape {shape!r}; tools/bt v1 "
                f"cannot set it directly - reach inside it with a dotted key "
                f"(e.g. {key}.<field>_) or edit the .enemyBehaviourData by hand"
            )
        jkey = pinfo["key"]
        if jkey not in node.params:
            raise EditError(f"param {jkey!r} missing from the stored blob; regen-catalog?")
        if shape == "field":
            _set_field_guid(node.params[jkey], _coerce(shape, raw))
        else:
            node.params[jkey] = _coerce(shape, raw)
        touched.append(pinfo["member"])
    return touched


def set_weight(tree: model.Tree, *, child_guid: str | None = None,
               parent_guid: str | None = None, index: int | None = None,
               weight: int = 100) -> None:
    if child_guid is not None:
        parent, kids, idx = _find_parent(tree, child_guid)
    else:
        parent = _resolve_parent(tree, parent_guid)
        idx = index
    if not isinstance(parent, model.RandomSelector):
        raise EditError("set-weight only applies to a RandomSelector's children")
    if idx is None or not (0 <= idx < len(parent.weights)):
        raise EditError(f"weight index out of range: {idx}")
    parent.weights[idx] = int(weight)


def add_bb_param(tree: model.Tree, name: str, value: int) -> None:
    if any(p.name == name for p in tree.params):
        raise EditError(f"blackboard param {name!r} already exists")
    tree.params.append(model.BbParam(name=name, value=int(value), kind="int"))


def remove_bb_param(tree: model.Tree, name: str) -> None:
    before = len(tree.params)
    tree.params[:] = [p for p in tree.params if p.name != name]
    if len(tree.params) == before:
        raise EditError(f"no blackboard param named {name!r}")


# ---------------------------------------------------------------------------
# batch
# ---------------------------------------------------------------------------
def apply(tree: model.Tree, ops: list[dict], cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    log: list[str] = []
    for i, op in enumerate(ops):
        kind = op.get("op")
        try:
            if kind == "add-node":
                n = add_node(tree, parent_guid=op["parent"], kind=op["kind"],
                             name=op.get("name"), action_type=op.get("type"),
                             index=op.get("index"), weight=int(op.get("weight", 100)),
                             pos=op.get("pos"), node_guid=op.get("guid"), cat=cat)
                log.append(f"add-node {op['kind']} -> {n.guid}")
            elif kind == "copy-node":
                n = copy_node(tree, src_guid=op["node"], parent_guid=op["parent"],
                             index=op.get("index"), weight=int(op.get("weight", 100)),
                             pos=op.get("pos"))
                log.append(f"copy-node {op['node']} -> {n.guid} (under {op['parent']})")
            elif kind == "remove-node":
                remove_node(tree, op["node"])
                log.append(f"remove-node {op['node']}")
            elif kind == "move-node":
                move_node(tree, guid=op["node"], parent_guid=op["parent"],
                          index=op.get("index"), weight=int(op.get("weight", 100)))
                log.append(f"move-node {op['node']} -> {op['parent']}")
            elif kind == "set-params":
                touched = set_params(tree, op["node"], op.get("set", {}), cat=cat)
                log.append(f"set-params {op['node']} {touched}")
            elif kind == "set-weight":
                set_weight(tree, child_guid=op.get("node"), parent_guid=op.get("parent"),
                           index=op.get("index"), weight=int(op["weight"]))
                log.append(f"set-weight {op.get('node') or op.get('parent')}")
            elif kind == "add-bb-param":
                add_bb_param(tree, op["name"], int(op["value"]))
                log.append(f"add-bb-param {op['name']}")
            elif kind == "remove-bb-param":
                remove_bb_param(tree, op["name"])
                log.append(f"remove-bb-param {op['name']}")
            else:
                raise EditError(f"unknown op {kind!r}")
        except Exception as e:  # noqa: BLE001
            raise EditError(f"op #{i} ({kind}): {e}") from e
    return log
