"""cereal-JSON text  ->  :class:`tools.bt.model.Tree`.

Tolerant of the archive bookkeeping (polymorphic ids, ptr_wrapper ids,
cereal_class_version): node structure is decoded into the model, while each
action's ``data`` becomes a tagged :mod:`tools.bt.blob` so the writer can rebuild
its bookkeeping from scratch.

Only "pure tree" archives are supported (every ``ptr_wrapper`` writes fresh data);
a back-reference raises :class:`PureTreeError`.
"""

from __future__ import annotations

from typing import Any, Optional

from . import catalog as catalog_mod
from . import model
from .blob import Ptr, Ver, fingerprint
from .cereal_json import Num, OrderedObj, loads, read_text

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


class PureTreeError(RuntimeError):
    pass


class UnsupportedBbParam(RuntimeError):
    pass


_KIND_BY_FQN = {
    model.FQN_SELECTOR: model.Selector,
    model.FQN_SEQUENCE: model.Sequence,
    model.FQN_RANDOM: model.RandomSelector,
    model.FQN_ONCE_EXEC: model.OnceExecute,
    model.FQN_ONCE_SUCCESS: model.OnceSuccess,
    model.FQN_ACTION_NODE_ENEMY: model.Action,
    model.FQN_ACTION_NODE_FRIENDLY: model.Action,
}

# which Tree.kind an ActionNode wrapper FQN implies (see tools/bt/npc_kind.py)
_TREE_KIND_BY_ACTION_NODE_FQN = {
    model.FQN_ACTION_NODE_ENEMY: "enemy",
    model.FQN_ACTION_NODE_FRIENDLY: "friendly",
}

ANIM_PARAM_INT_FQN = "NanamiEngine::Module::AnimationTree::AnimationParameter<int>"


class _Ctx:
    """Per-parse state: the archive polymorphic type table."""

    def __init__(self, cat: catalog_mod.Catalog) -> None:
        self.cat = cat
        self.poly: dict[int, str] = {}
        #: Tree.kind, set the first time an ActionNode wrapper is seen (a
        #: file mixes only one flavor - see _read_node).
        self.tree_kind: Optional[str] = None

    # -- pointer slot ---------------------------------------------------
    def ptr_slot(self, slot: OrderedObj):
        pid = _num(slot["polymorphic_id"])
        if pid == 0:
            return dict(null=True, exact=False, fqn=None, wrapper="", data=None)
        exact = pid == EXACT_PID
        masked = pid & ~FIRST_BIT
        fqn = None
        if "polymorphic_name" in slot:
            fqn = slot["polymorphic_name"]
            if not exact:
                self.poly[masked] = fqn
        elif not exact:
            fqn = self.poly.get(masked)
        pw = slot["ptr_wrapper"]
        if "id" in pw:
            wrapper = "shared"
            if not (_num(pw["id"]) & FIRST_BIT):
                raise PureTreeError(
                    "shared object re-reference (ptr_wrapper.id without the 0x80000000 bit) "
                    "- this tree is a DAG, which tools/bt does not support"
                )
        elif "valid" in pw:
            wrapper = "unique"
        else:
            raise PureTreeError(f"ptr_wrapper without id/valid: {pw!r}")
        return dict(null=False, exact=exact, fqn=fqn, wrapper=wrapper, data=pw["data"])


def _num(v: Any) -> int:
    return v.value if isinstance(v, Num) else v


def _strip_ccv(obj: OrderedObj) -> tuple[Optional[int], OrderedObj]:
    """Split a leading ``cereal_class_version`` off an object."""
    if len(obj) and obj.keys()[0] == "cereal_class_version":
        v = _num(obj.values()[0])
        rest = OrderedObj(obj.items()[1:])
        return v, rest
    return None, obj


def _nodebase(nb: OrderedObj) -> tuple[str, tuple[float, float]]:
    _v, nb = _strip_ccv(nb)
    guid = nb["guid_"]["value_"]
    pos = nb["position_"]
    return guid, (float(_num(pos["value0"])), float(_num(pos["value1"])))


# ---------------------------------------------------------------------------
# blob tagging for action parameters
# ---------------------------------------------------------------------------
def _is_guid_obj(obj: OrderedObj) -> bool:
    keys = [k for k in obj.keys() if k != "cereal_class_version"]
    return keys == ["value_"] and isinstance(obj["value_"], str)


def _tag_value(ctx: _Ctx, val: Any, pinfo: Optional[dict]) -> Any:
    if isinstance(val, (Num, str, bool)) or val is None:
        return val
    if isinstance(val, list):
        return [_tag_value(ctx, x, None) for x in val]
    if not isinstance(val, OrderedObj):
        return val

    keys = val.keys()

    if pinfo and pinfo.get("shape") == "field":
        return _tag_field(ctx, val, pinfo.get("type", "?"))

    if "polymorphic_id" in keys:
        s = ctx.ptr_slot(val)
        if s["null"]:
            return Ptr(exact=False, null=True, fqn=None, wrapper="", data=None)
        return Ptr(
            exact=s["exact"], null=False, fqn=s["fqn"], wrapper=s["wrapper"],
            data=_tag_value(ctx, s["data"], None),
        )

    if keys and keys[0] == "cereal_class_version":
        v, body = _strip_ccv(val)
        if _is_guid_obj(val):
            key = ("type", "Guid")
            new_body = OrderedObj([("value_", body["value_"])])
            return Ver(key, v or 0, new_body)
        if pinfo and pinfo.get("shape") == "nested":
            leaf = pinfo.get("type", "?")
            sub = ctx.cat.type_by_leaf(leaf)
            return Ver(("type", leaf), v or 0, _tag_struct_body(ctx, body, sub))
        # Fallback: no catalog type info, so bucket by structural fingerprint.
        # fingerprint() ignores field *values* (a Field<T> reference's GUID
        # string, say), so two genuinely different C++ types that happen to
        # serialise identically when opaque (any Field<T>/FieldHolder<T> looks
        # the same regardless of T) would otherwise wrongly share one
        # once-per-key version slot with each other's real occurrences -
        # exactly the shape a FIELD(Asset::X) reference takes when it's
        # embedded inside something outside any scanned catalog (e.g. a Quest
        # object reached through a raw, un-modeled shared_ptr<ITakeable...>).
        # literal_presence sidesteps that: reproduce whether *this* occurrence
        # literally carried a cereal_class_version, instead of asking the
        # writer's global "have I emitted this key before" tracking to decide.
        return Ver(("fp", fingerprint(val)), v or 0, _tag_plain(ctx, body),
                  literal_presence=(v is not None))

    if _is_guid_obj(val):
        return OrderedObj([("value_", val["value_"])])

    return _tag_plain(ctx, val)


def _tag_plain(ctx: _Ctx, obj: OrderedObj) -> OrderedObj:
    return OrderedObj((k, _tag_value(ctx, v, None)) for k, v in obj.items())


def _tag_struct_body(ctx: _Ctx, body: OrderedObj, sub_entry: Optional[dict]) -> OrderedObj:
    """Tag the members of a nested struct/action-typed member."""
    out = OrderedObj()
    for k, v in body.items():
        if k == "value0" and isinstance(v, OrderedObj) and \
                all(kk == "cereal_class_version" for kk in v.keys()):
            vv, _ = _strip_ccv(v)
            out.append(k, Ver(("type", "ActionBase"), vv if vv is not None else 0, OrderedObj()))
            continue
        pinfo = ctx.cat.param_by_key(sub_entry, k) if sub_entry else None
        out.append(k, _tag_value(ctx, v, pinfo))
    return out


def _tag_field(ctx: _Ctx, val: OrderedObj, ftype: str) -> Ver:
    outer_v, outer_body = _strip_ccv(val)
    inner = outer_body["value0"]                       # ptr slot (exact)
    s = ctx.ptr_slot(inner)
    holder_data = s["data"]
    holder_v, holder_body = _strip_ccv(holder_data)
    guid_obj = holder_body["value0"]
    if isinstance(guid_obj, OrderedObj) and _is_guid_obj(guid_obj):
        guid_v, _gb = _strip_ccv(guid_obj)
        guid_tagged = Ver(("type", "Guid"), guid_v or 0, OrderedObj([("value_", guid_obj["value_"])]))
    else:
        guid_tagged = _tag_value(ctx, guid_obj, None)
    holder = Ver(("type", f"FieldHolder<{ftype}>"), holder_v or 0,
                 OrderedObj([("value0", guid_tagged)]))
    ptr = Ptr(exact=True, null=False, fqn=None, wrapper="shared", data=holder)
    return Ver(("type", f"Field<{ftype}>"), outer_v or 0, OrderedObj([("value0", ptr)]))


def _tag_action_data(ctx: _Ctx, adata: OrderedObj, cat_entry: Optional[dict]) -> tuple[int, OrderedObj]:
    action_v, rest = _strip_ccv(adata)
    members = OrderedObj()
    for k, v in rest.items():
        if k == "value0":
            vv, _ = _strip_ccv(v) if isinstance(v, OrderedObj) else (None, v)
            members.append("value0", Ver(("type", "ActionBase"),
                                         vv if vv is not None else 0, OrderedObj()))
            continue
        pinfo = ctx.cat.param_by_key(cat_entry, k) if cat_entry else None
        members.append(k, _tag_value(ctx, v, pinfo))
    return (action_v if action_v is not None else -1), members


# ---------------------------------------------------------------------------
# node structure
# ---------------------------------------------------------------------------
def _read_node(ctx: _Ctx, slot: OrderedObj):
    s = ctx.ptr_slot(slot)
    if s["null"]:
        return None
    fqn = s["fqn"]
    kind = _KIND_BY_FQN.get(fqn)
    if kind is None:
        raise ValueError(f"unknown node type in data: {fqn!r}")
    data = s["data"]
    _v, data = _strip_ccv(data)
    guid, pos = _nodebase(data["value0"])

    if kind in (model.Selector, model.Sequence):
        node = kind(guid=guid, pos=pos)
        node.children = [_read_node(ctx, c) for c in data["children_"]]
        return node
    if kind is model.RandomSelector:
        node = model.RandomSelector(guid=guid, pos=pos)
        node.children = [_read_node(ctx, c) for c in data["children_"]]
        node.weights = [int(_num(w)) for w in data.get("weights_", [])]
        if len(node.weights) != len(node.children):
            node.weights = [100] * len(node.children)
        return node
    if kind is model.OnceExecute:
        return model.OnceExecute(guid=guid, pos=pos,
                                 child=_read_node(ctx, data["child_"]),
                                 state=int(_num(data.get("state_", Num.of_int(0)))))
    if kind is model.OnceSuccess:
        return model.OnceSuccess(guid=guid, pos=pos, child=_read_node(ctx, data["child_"]))

    # ActionNode
    tk = _TREE_KIND_BY_ACTION_NODE_FQN.get(fqn)
    if tk is not None:
        if ctx.tree_kind is None:
            ctx.tree_kind = tk
        elif ctx.tree_kind != tk:
            raise ValueError(
                f"mixed ActionNode flavors in one file: {ctx.tree_kind!r} and {tk!r}"
            )
    name = data["name_"]
    a_slot = data["action_"]
    a = ctx.ptr_slot(a_slot)
    if a["fqn"] is None:
        raise ValueError("ActionNode without a resolvable action type")
    cat_entry = ctx.cat.action_by_fqn(a["fqn"])
    action_v, blob = _tag_action_data(ctx, a["data"], cat_entry)
    if action_v < 0:
        action_v = int(cat_entry.get("version", 0)) if cat_entry else 0
    return model.Action(guid=guid, pos=pos, name=name, type_fqn=a["fqn"],
                        action_version=action_v, params=blob)


def _read_params(ctx: _Ctx, slot: OrderedObj) -> list[model.BbParam]:
    data = slot["ptr_wrapper"]["data"]
    count = int(_num(data["value0"]))
    out: list[model.BbParam] = []
    for i in range(1, count + 1):
        p = data[f"value{i}"]
        s = ctx.ptr_slot(p)
        if s["fqn"] != ANIM_PARAM_INT_FQN:
            raise UnsupportedBbParam(
                f"blackboard parameter #{i} is {s['fqn']!r}; tools/bt v1 supports "
                f"{ANIM_PARAM_INT_FQN} only"
            )
        pd = s["data"]
        _v, pd = _strip_ccv(pd)
        out.append(model.BbParam(name=pd["name_"], value=int(_num(pd["value_"])), kind="int"))
    return out


def read_tree(text: str, cat: catalog_mod.Catalog | None = None,
             kind: str | None = None) -> model.Tree:
    """``kind`` (``"enemy"`` | ``"friendly"``), when given, is the caller's
    expectation from the file's own extension - cross-checked against the
    ActionNode flavor actually found in the data (see ``_read_node``) and
    used as-is for a tree with no action nodes at all (nothing to detect)."""
    cat = cat or catalog_mod.load()
    ctx = _Ctx(cat)
    root = loads(text)
    if "entryNode_" not in root:
        raise ValueError("not a .enemyBehaviourData/.friendBehaviourData file: missing entryNode_")

    es = ctx.ptr_slot(root["entryNode_"])
    edata = es["data"]
    _v, edata = _strip_ccv(edata)
    guid, pos = _nodebase(edata["value0"])
    entry = model.Entry(guid=guid, pos=pos, child=_read_node(ctx, edata["nextNode_"]))

    params: list[model.BbParam] = []
    if "parameters_" in root:
        params = _read_params(ctx, root["parameters_"])

    if ctx.tree_kind is not None and kind is not None and ctx.tree_kind != kind:
        raise ValueError(
            f"file kind mismatch: expected {kind!r} action nodes but found {ctx.tree_kind!r}"
        )
    return model.Tree(entry=entry, params=params, kind=ctx.tree_kind or kind or "enemy")


def read_tree_file(path) -> model.Tree:
    return read_tree(read_text(path))
