"""cereal-JSON text  ->  :class:`tools.animtree.model.Tree`.

Tolerant of the archive bookkeeping (polymorphic ids, ptr_wrapper ids,
cereal_class_version): node/transition/condition/param structure is decoded
into the model, while a node's non-guid/non-position fields become a tagged
:mod:`tools.common.blob` so the writer can rebuild their bookkeeping exactly.

Only "pure tree" archives are supported (every ``ptr_wrapper`` writes fresh
data); a back-reference raises :class:`PureTreeError`. Every real ``.animTree``
file the engine ever writes is one, since ``nodes_``/the transition lists are
plain arrays of freshly-owned objects (transitions reference nodes by GUID
*value*, never by cereal pointer identity - see model.py).
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver, fingerprint
from tools.common.cereal_json import Num, OrderedObj, loads, read_text

from . import catalog as catalog_mod
from . import model

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


class PureTreeError(RuntimeError):
    pass


class _Ctx:
    """Per-parse state: the archive's polymorphic type table."""

    def __init__(self, cat: catalog_mod.Catalog) -> None:
        self.cat = cat
        self.poly: dict[int, str] = {}

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
                    "- this file is a DAG, which tools/animtree does not support"
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


def _is_guid_obj(obj: OrderedObj) -> bool:
    keys = [k for k in obj.keys() if k != "cereal_class_version"]
    return keys == ["value_"] and isinstance(obj["value_"], str)


def _guid_value(obj: OrderedObj) -> str:
    _v, body = _strip_ccv(obj)
    return body["value_"]


def _vec2(obj: OrderedObj) -> tuple[Any, Any]:
    # kept as the raw Num objects loads() produced (not converted to plain
    # Python float) so an unedited value round-trips through its *original*
    # literal text - RapidJSON's Grisu2 float printer and Python's repr()
    # occasionally disagree on the least-significant digit of the same double
    # (see tools/common/cereal_json.py's module docstring), which would
    # otherwise break byte-identical round-tripping of derived (non-integral)
    # coordinates.
    return (obj["value0"], obj["value1"])


def _typed(kind: str, raw: Any) -> Any:
    # int/float: `raw` is already the Num loads() produced - keep it as-is
    # (see _vec2) rather than unwrapping to a plain Python number.
    if kind == "bool":
        return bool(raw)
    if kind in ("int", "float"):
        return raw
    raise ValueError(f"unknown kind {kind!r}")


# ---------------------------------------------------------------------------
# generic value tagging (mirrors tools.bt.reader._tag_value / _tag_field)
# ---------------------------------------------------------------------------
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
            return Ver(("type", "Guid"), v or 0, OrderedObj([("value_", body["value_"])]))
        return Ver(("fp", fingerprint(val)), v or 0, _tag_plain(ctx, body))

    if _is_guid_obj(val):
        return OrderedObj([("value_", val["value_"])])

    return _tag_plain(ctx, val)


def _tag_plain(ctx: _Ctx, obj: OrderedObj) -> OrderedObj:
    return OrderedObj((k, _tag_value(ctx, v, None)) for k, v in obj.items())


def _tag_field(ctx: _Ctx, val: OrderedObj, ftype: str) -> Ver:
    outer_v, outer_body = _strip_ccv(val)
    inner = outer_body["value0"]                       # ptr slot (exact) - FieldContext<T>
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


# ---------------------------------------------------------------------------
# node
# ---------------------------------------------------------------------------
def _is_base_stub(key: str, val: Any) -> bool:
    """The leading, unnamed ``archive(cereal::base_class<IAnimationNode>(this))``
    call every node's ``save()`` makes - always key "value0", never a catalog
    param, always either ``{}`` or ``{"cereal_class_version": N}``."""
    return key == "value0" and isinstance(val, OrderedObj) and \
        all(kk == "cereal_class_version" for kk in val.keys())


def _read_node(ctx: _Ctx, slot: OrderedObj, *, expected_fqn: Optional[str] = None) -> model.Node:
    s = ctx.ptr_slot(slot)
    fqn = s["fqn"] or expected_fqn
    if fqn is None:
        raise ValueError("node slot has no resolvable type")
    entry = ctx.cat.node_by_fqn(fqn)
    if entry is None:
        raise ValueError(f"unknown node type: {fqn!r} (regen-catalog?)")

    data = s["data"]
    v, data = _strip_ccv(data)
    class_version = v if v is not None else int(entry.get("version", 0))

    guid: Optional[str] = None
    pos: tuple[float, float] = (0.0, 0.0)
    params = OrderedObj()
    for key, val in data.items():
        if _is_base_stub(key, val):
            vv, _ = _strip_ccv(val)
            params.append(key, Ver(("type", "IAnimationNode"), vv if vv is not None else 0, OrderedObj()))
            continue
        pinfo = ctx.cat.param_by_key(entry, key)
        shape = pinfo.get("shape") if pinfo else None
        if shape == "self_guid":
            guid = _guid_value(val)
            continue
        if shape == "self_pos":
            pos = _vec2(val)
            continue
        params.append(key, _tag_value(ctx, val, pinfo))

    if guid is None:
        raise ValueError(f"node of type {fqn!r} has no guid_ field (regen-catalog?)")
    return model.Node(guid=guid, pos=pos, type_fqn=fqn, class_version=class_version, params=params)


# ---------------------------------------------------------------------------
# conditions / transitions
# ---------------------------------------------------------------------------
def _read_conditions(ctx: _Ctx, slot: OrderedObj) -> list[model.Condition]:
    data = slot["ptr_wrapper"]["data"]              # unique_ptr shape: {"valid":1, "data":...}
    _v, data = _strip_ccv(data)
    count = int(_num(data["value0"]))
    out: list[model.Condition] = []
    for i in range(1, count + 1):
        s = ctx.ptr_slot(data[f"value{i}"])
        kind = ctx.cat.kind_by_condition_fqn(s["fqn"])
        if kind is None:
            raise ValueError(f"unknown condition type: {s['fqn']!r}")
        _v2, body = _strip_ccv(s["data"])
        # body["value0"] is the IAnimationNodePathAdditionCondition base stub - skip
        out.append(model.Condition(name=body["name_"], kind=kind, value=_typed(kind, body["equalValue_"])))
    return out


def _read_transition(ctx: _Ctx, slot: OrderedObj) -> model.Transition:
    s = ctx.ptr_slot(slot)
    data = s["data"]
    _v, data = _strip_ccv(data)
    conditions = _read_conditions(ctx, data["additionConditionGroup_"])
    duration = data["transitionDuration_secs_"]  # raw Num - see _vec2
    from_guid = _guid_value(data["fromNodeGuid_"])
    next_guid = _guid_value(data["nextNodeGuid_"])
    visual_from_guid = _guid_value(data["visualFromNodeGuid_"]) if "visualFromNodeGuid_" in data else from_guid
    return model.Transition(from_guid=from_guid, next_guid=next_guid, visual_from_guid=visual_from_guid,
                            duration_secs=duration, conditions=conditions)


def _read_params(ctx: _Ctx, slot: OrderedObj) -> list[model.Param]:
    # ParameterGroup is not IObject-derived (no virtual base) - its save()/
    # load() take no version arg, so there is no polymorphic_id wrapper and no
    # cereal_class_version to strip at this level, unlike every other
    # shared_ptr in this file.
    data = slot["ptr_wrapper"]["data"]
    count = int(_num(data["value0"]))
    out: list[model.Param] = []
    for i in range(1, count + 1):
        s = ctx.ptr_slot(data[f"value{i}"])
        kind = ctx.cat.kind_by_param_fqn(s["fqn"])
        if kind is None:
            raise ValueError(f"unknown parameter type: {s['fqn']!r}")
        _v, body = _strip_ccv(s["data"])
        out.append(model.Param(name=body["name_"], kind=kind, value=_typed(kind, body["value_"])))
    return out


# ---------------------------------------------------------------------------
def read_tree(text: str, cat: catalog_mod.Catalog | None = None) -> model.Tree:
    cat = cat or catalog_mod.load()
    ctx = _Ctx(cat)
    root = loads(text)
    if "entryNode" not in root or "nodesCount" not in root:
        raise ValueError("not a .animTree file: missing entryNode/nodesCount")

    params = _read_params(ctx, root["additionParameters_"]) if "additionParameters_" in root else []
    entry = _read_node(ctx, root["entryNode"], expected_fqn=model.FQN_ENTRY_NODE)
    any_state = _read_node(ctx, root["visualAnyStateNode"], expected_fqn=model.FQN_ANYSTATE_NODE)

    count = int(_num(root["nodesCount"]))
    nodes = [_read_node(ctx, root[f"nodes_{i}"]) for i in range(count)]

    path_count = int(_num(root["fromNodeNodePathCount"])) if "fromNodeNodePathCount" in root else 0
    transitions = [_read_transition(ctx, root[f"fromNodeNodePath_{i}"]) for i in range(path_count)]

    any_path_count = (int(_num(root["fromAnyStateNodeNodePathCount"]))
                      if "fromAnyStateNodeNodePathCount" in root else 0)
    any_state_transitions = [_read_transition(ctx, root[f"fromAnyStateNodeNodePath_{i}"])
                             for i in range(any_path_count)]

    return model.Tree(entry=entry, any_state=any_state, nodes=nodes, transitions=transitions,
                      any_state_transitions=any_state_transitions, params=params)


def read_tree_file(path) -> model.Tree:
    return read_tree(read_text(path))
