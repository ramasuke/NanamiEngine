"""Structural + parameter edits on a :class:`tools.animtree.model.Tree`.

Every function mutates the tree in place and is designed so a change followed
by its inverse restores the original bytes (see selftest). ``apply`` runs a
batch of edit ops atomically (build -> apply all -> caller validates -> one
write) - the primary agent-facing interface, mirroring ``tools.bt.edits``/
``tools.scene.edits``.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any, Optional

from tools.common.blob import Ptr, Ver
from tools.common.cereal_json import Num, OrderedObj

from . import catalog as catalog_mod
from . import layout
from . import meta as meta_mod
from . import model

_REPO = Path(__file__).resolve().parents[2]

_GUID_RE = re.compile(r"^[0-9A-Fa-f]{8}(-[0-9A-Fa-f]{4}){3}-[0-9A-Fa-f]{12}$")


class EditError(RuntimeError):
    pass


# ---------------------------------------------------------------------------
# blob builders (from-scratch clip nodes)
# ---------------------------------------------------------------------------
def _leaf(name: str) -> str:
    return name.split("<", 1)[0].rsplit("::", 1)[-1]


def _field_blob(field_type_leaf: str, guid: str) -> Ver:
    guid_ver = Ver(("type", "Guid"), 0, OrderedObj([("value_", guid)]))
    holder = Ver(("type", f"FieldHolder<{field_type_leaf}>"), 0,
                 OrderedObj([("value0", guid_ver)]))
    ptr = Ptr(exact=True, null=False, fqn=None, wrapper="shared", data=holder)
    return Ver(("type", f"Field<{field_type_leaf}>"), 0, OrderedObj([("value0", ptr)]))


def _default_scalar(shape: str, value: Any) -> Any:
    if shape == "string":
        return "" if value is None else str(value)
    if shape == "float":
        return Num.of_float(float(value) if value is not None else 0.0)
    if shape == "int":
        return Num.of_int(int(value) if value is not None else 0)
    if shape == "bool":
        return bool(value) if value is not None else False
    # unknown/vector/... -> a harmless placeholder; validate() flags it, and a
    # human finishes it in the editor (mirrors tools.bt/tools.scene's own
    # limitation for shapes a param can't be constructed for from scratch).
    return Num.of_int(0)


def resolve_clip_arg(raw: str, repo_root: Path | None = None) -> str:
    """Accept a raw ``Mv1File`` asset GUID, or a path to a ``.mv1``/``.mv1.meta``
    file, resolved to that asset's guid via its ``.meta`` sidecar.

    Neither ``tools.bt`` nor ``tools.scene`` resolve a path for a ``field``-shaped
    CLI argument today - both just uppercase a raw GUID string - so there is no
    existing helper to reuse beyond that baseline; this is a new convenience
    specific to ``--clip``, built entirely from
    :func:`tools.common.meta_base.read_meta` (via :func:`tools.animtree.meta.read_mv1_meta`).
    """
    raw = raw.strip()
    if _GUID_RE.match(raw):
        return raw.upper()
    repo_root = repo_root or _REPO
    p = Path(raw)
    if not p.is_absolute() and not p.exists():
        p = repo_root / p
    if p.suffix == ".mv1":
        p = p.with_suffix(".mv1.meta")
    elif not str(p).endswith(".mv1.meta"):
        p = Path(str(p) + ".mv1.meta")
    if not p.exists():
        raise EditError(f"--clip: not a GUID and no such file: {raw!r} (looked for {p})")
    return meta_mod.read_mv1_meta(p)["guid"]


def default_node_params(entry: Optional[dict], overrides: Optional[dict] = None, *,
                        field_guid: Optional[str] = None) -> OrderedObj:
    """A brand-new node's params blob: every non-guid/non-position catalog
    param at its default (or ``overrides[member]``, if given), in catalog
    order. Generic over *any* node type in the catalog - used both for
    ``AnimationClipNode`` (:func:`add_clip_node`) and for the two fixed
    singletons (:func:`new_singleton_node`, e.g. ``new-tree``'s freshly-minted
    Entry/AnyState), so a future addable ``IAnimationNode`` subtype needs no
    new construction code here, only a catalog entry.
    """
    overrides = overrides or {}
    params = OrderedObj()
    for pinfo in (entry.get("params", []) if entry else []):
        key, member, shape = pinfo["key"], pinfo["member"], pinfo.get("shape")
        if shape in ("self_guid", "self_pos"):
            continue
        if shape == "field":
            if field_guid is None:
                raise EditError(f"{(entry or {}).get('class')}: param {member!r} is a Field<T> "
                                f"reference; no value given")
            params.append(key, _field_blob(_leaf(pinfo.get("type", "?")), field_guid))
            continue
        params.append(key, _default_scalar(shape, overrides.get(member)))
    return params


def new_singleton_node(fqn: str, guid: str, pos: tuple[float, float],
                       overrides: Optional[dict] = None,
                       cat: catalog_mod.Catalog | None = None) -> model.Node:
    """A freshly-minted Entry/AnyState node (``new-tree``'s two fixed
    singletons) - not addable/removable via the CLI, but still needs a fully
    catalog-shaped params blob to write correctly."""
    cat = cat or catalog_mod.load()
    entry = cat.node_by_fqn(fqn)
    if entry is None:
        raise EditError(f"{fqn} not in catalog (regen-catalog?)")
    params = default_node_params(entry, overrides)
    return model.Node(guid=guid, pos=tuple(pos), type_fqn=fqn,
                      class_version=int(entry.get("version", 0)), params=params)


def add_clip_node(tree: model.Tree, *, name: str, clip_guid: str, speed: float = 1.0,
                  blend_offset_secs: float = 0.0, model_anim_index: int = 0,
                  pos: Optional[tuple[float, float]] = None, guid: Optional[str] = None,
                  cat: catalog_mod.Catalog | None = None) -> model.Node:
    cat = cat or catalog_mod.load()
    entry = cat.resolve_node_type(model.FQN_CLIP_NODE)
    if entry is None or entry.get("singleton"):
        raise EditError("AnimationClipNode not in catalog (regen-catalog?)")
    overrides = {
        "name_": name,
        "speed_": speed,
        "blendAnimationOffset_secs_": blend_offset_secs,
        "modelAnimationIndex_": model_anim_index,
    }
    node_guid = guid or meta_mod.mint_guid()
    node_pos = tuple(pos) if pos else layout.grid_position(len(tree.nodes))
    params = default_node_params(entry, overrides, field_guid=clip_guid)
    node = model.Node(guid=node_guid, pos=node_pos, type_fqn=entry["fqn"],
                      class_version=int(entry.get("version", 0)), params=params)
    tree.nodes.append(node)
    return node


def remove_node(tree: model.Tree, guid: str, *, cascade: bool = False) -> model.Node:
    if guid in (tree.entry.guid, tree.any_state.guid):
        raise EditError("cannot remove the Entry/AnyState singleton nodes")
    node = next((n for n in tree.nodes if n.guid == guid), None)
    if node is None:
        raise EditError(f"node not found: {guid}")
    refs = [t for t in (tree.transitions + tree.any_state_transitions)
           if guid in (t.from_guid, t.next_guid, t.visual_from_guid)]
    if refs and not cascade:
        raise EditError(
            f"node {guid} is referenced by {len(refs)} transition(s); pass cascade=True "
            f"(--cascade) to remove them too, or remove the transitions first"
        )
    if cascade:
        tree.transitions[:] = [t for t in tree.transitions
                               if guid not in (t.from_guid, t.next_guid, t.visual_from_guid)]
        tree.any_state_transitions[:] = [t for t in tree.any_state_transitions
                                         if guid not in (t.from_guid, t.next_guid, t.visual_from_guid)]
    tree.nodes.remove(node)
    return node


def move_node(tree: model.Tree, guid: str, pos: tuple[float, float]) -> model.Node:
    node = tree.find_node(guid)
    if node is None:
        raise EditError(f"node not found: {guid}")
    node.pos = (float(pos[0]), float(pos[1]))
    return node


# ---------------------------------------------------------------------------
# node parameter edits
# ---------------------------------------------------------------------------
def _leaf_param(cat: catalog_mod.Catalog, entry: Optional[dict], key: str) -> dict:
    for p in cat.params_of(entry):
        if key in (p["key"], p["member"]):
            return p
    type_name = (entry or {}).get("class", entry)
    raise EditError(f"{type_name}: no param {key!r} (see: show)")


def _coerce(shape: str, raw: str) -> Any:
    if shape == "int":
        return Num.of_int(int(raw, 0))
    if shape == "float":
        return Num.of_float(float(raw))
    if shape == "bool":
        return raw.strip().lower() in ("1", "true", "yes", "on")
    if shape == "string":
        return raw
    if shape == "field":
        return raw.strip().upper()
    raise EditError(f"cannot set a param of shape {shape!r}")


def _coerce_vec2(raw: str) -> tuple[float, float]:
    parts = [p.strip() for p in raw.replace(" ", ",").split(",") if p.strip()]
    if len(parts) != 2:
        raise EditError(f"expected X,Y - got {raw!r}")
    return (float(parts[0]), float(parts[1]))


def _set_field_guid(val: Any, guid: str) -> None:
    outer = val.body if isinstance(val, Ver) else val
    holder = outer["value0"].data
    inner = holder.body if isinstance(holder, Ver) else holder
    guid_slot = inner["value0"]
    target = guid_slot.body if isinstance(guid_slot, Ver) else guid_slot
    target["value_"] = guid


def set_node_params(tree: model.Tree, guid: str, assignments: dict[str, str],
                    cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    node = tree.find_node(guid)
    if node is None:
        raise EditError(f"node not found: {guid}")
    entry = cat.node_by_fqn(node.type_fqn)
    if entry is None:
        raise EditError(f"{node.type_fqn} not in catalog (regen-catalog?)")
    touched: list[str] = []
    for key, raw in assignments.items():
        pinfo = _leaf_param(cat, entry, key)
        shape = pinfo.get("shape")
        if shape == "self_guid":
            raise EditError(f"{key}: a node's identity guid is not settable via set-node-params")
        if shape == "self_pos":
            node.pos = _coerce_vec2(raw)
            touched.append(pinfo["member"])
            continue
        if shape not in catalog_mod.SETTABLE_SHAPES:
            raise EditError(
                f"{node.type_fqn}.{pinfo['member']} has shape {shape!r}; tools/animtree v1 "
                f"cannot set it directly - edit the .animTree by hand or in the editor"
            )
        jkey = pinfo["key"]
        if node.params is None or jkey not in node.params:
            raise EditError(f"param {jkey!r} missing from the stored blob; regen-catalog?")
        if shape == "field":
            _set_field_guid(node.params[jkey], _coerce(shape, raw))
        else:
            node.params[jkey] = _coerce(shape, raw)
        touched.append(pinfo["member"])
    return touched


# ---------------------------------------------------------------------------
# transition addressing (no identity guid - positional or (from, next))
# ---------------------------------------------------------------------------
def _transition_list(tree: model.Tree, any_state: bool) -> list[model.Transition]:
    return tree.any_state_transitions if any_state else tree.transitions


def _find_transition_index(tree: model.Tree, *, any_state: bool, index: Optional[int] = None,
                           from_guid: Optional[str] = None, next_guid: Optional[str] = None) -> int:
    lst = _transition_list(tree, any_state)
    if index is not None:
        if not (0 <= index < len(lst)):
            raise EditError(f"transition index out of range: {index}")
        return index
    if from_guid is None or next_guid is None:
        raise EditError("must give --index, or both --from and --next")
    for i, t in enumerate(lst):
        if t.from_guid == from_guid and t.next_guid == next_guid:
            return i
    raise EditError(f"no transition {from_guid} -> {next_guid} (any_state={any_state})")


def add_transition(tree: model.Tree, *, from_guid: str, next_guid: str, any_state: bool = False,
                   duration_secs: float = 0.0, visual_from_guid: Optional[str] = None) -> model.Transition:
    if tree.find_node(from_guid) is None:
        raise EditError(f"--from: node not found: {from_guid}")
    if tree.find_node(next_guid) is None:
        raise EditError(f"--next: node not found: {next_guid}")
    if any_state and from_guid != tree.any_state.guid:
        raise EditError(
            f"an any-state transition's --from must be the AnyState node's guid ({tree.any_state.guid})"
        )
    if not any_state and from_guid == tree.any_state.guid:
        raise EditError(
            "a transition sourced from the AnyState node belongs in the any-state list "
            "(add-transition --any-state)"
        )
    t = model.Transition(from_guid=from_guid, next_guid=next_guid,
                         visual_from_guid=visual_from_guid or from_guid,
                         duration_secs=float(duration_secs), conditions=[])
    _transition_list(tree, any_state).append(t)
    return t


def remove_transition(tree: model.Tree, *, any_state: bool = False, index: Optional[int] = None,
                      from_guid: Optional[str] = None, next_guid: Optional[str] = None) -> model.Transition:
    lst = _transition_list(tree, any_state)
    idx = _find_transition_index(tree, any_state=any_state, index=index,
                                 from_guid=from_guid, next_guid=next_guid)
    return lst.pop(idx)


def set_transition_params(tree: model.Tree, *, any_state: bool = False, index: Optional[int] = None,
                          from_guid: Optional[str] = None, next_guid: Optional[str] = None,
                          duration_secs: Optional[float] = None,
                          visual_from_guid: Optional[str] = None) -> model.Transition:
    lst = _transition_list(tree, any_state)
    idx = _find_transition_index(tree, any_state=any_state, index=index,
                                 from_guid=from_guid, next_guid=next_guid)
    t = lst[idx]
    if duration_secs is not None:
        t.duration_secs = float(duration_secs)
    if visual_from_guid is not None:
        t.visual_from_guid = visual_from_guid
    return t


def _coerce_condition_value(kind: str, value: Any) -> Any:
    if isinstance(value, str):
        if kind == "bool":
            return value.strip().lower() in ("1", "true", "yes", "on")
        if kind == "int":
            return int(value, 0)
        if kind == "float":
            return float(value)
    return value


def add_condition(tree: model.Tree, *, any_state: bool = False, index: Optional[int] = None,
                  from_guid: Optional[str] = None, next_guid: Optional[str] = None,
                  name: str, kind: str, value: Any) -> model.Condition:
    if kind not in model.KINDS:
        raise EditError(f"unknown condition kind: {kind!r} (want bool|int|float)")
    lst = _transition_list(tree, any_state)
    idx = _find_transition_index(tree, any_state=any_state, index=index,
                                 from_guid=from_guid, next_guid=next_guid)
    c = model.Condition(name=name, kind=kind, value=_coerce_condition_value(kind, value))
    lst[idx].conditions.append(c)
    return c


def remove_condition(tree: model.Tree, *, any_state: bool = False, index: Optional[int] = None,
                     from_guid: Optional[str] = None, next_guid: Optional[str] = None,
                     condition_index: int) -> model.Condition:
    lst = _transition_list(tree, any_state)
    idx = _find_transition_index(tree, any_state=any_state, index=index,
                                 from_guid=from_guid, next_guid=next_guid)
    conds = lst[idx].conditions
    if not (0 <= condition_index < len(conds)):
        raise EditError(f"condition index out of range: {condition_index}")
    return conds.pop(condition_index)


# ---------------------------------------------------------------------------
# additionParameters_ (bool/int/float, unlike tools.bt's int-only blackboard)
# ---------------------------------------------------------------------------
def add_param(tree: model.Tree, name: str, kind: str, value: Any) -> model.Param:
    if kind not in model.KINDS:
        raise EditError(f"unknown parameter kind: {kind!r} (want bool|int|float)")
    if tree.find_param(name) is not None:
        raise EditError(f"parameter {name!r} already exists")
    p = model.Param(name=name, kind=kind, value=_coerce_condition_value(kind, value))
    tree.params.append(p)
    return p


def remove_param(tree: model.Tree, name: str) -> None:
    before = len(tree.params)
    tree.params[:] = [p for p in tree.params if p.name != name]
    if len(tree.params) == before:
        raise EditError(f"no parameter named {name!r}")


def set_param(tree: model.Tree, name: str, value: Any) -> model.Param:
    p = tree.find_param(name)
    if p is None:
        raise EditError(f"no parameter named {name!r}")
    p.value = _coerce_condition_value(p.kind, value)
    return p


# ---------------------------------------------------------------------------
# batch - the primary agent-facing interface
# ---------------------------------------------------------------------------
def apply(tree: model.Tree, ops: list[dict], cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    log: list[str] = []
    for i, op in enumerate(ops):
        op_kind = op.get("op")
        try:
            if op_kind == "add-clip-node":
                clip = resolve_clip_arg(op["clip"])
                n = add_clip_node(tree, name=op["name"], clip_guid=clip,
                                  speed=float(op.get("speed", 1.0)),
                                  blend_offset_secs=float(op.get("blend_offset_secs", 0.0)),
                                  model_anim_index=int(op.get("model_anim_index", 0)),
                                  pos=op.get("pos"), guid=op.get("guid"), cat=cat)
                log.append(f"add-clip-node {op['name']} -> {n.guid}")
            elif op_kind == "remove-node":
                remove_node(tree, op["node"], cascade=bool(op.get("cascade", False)))
                log.append(f"remove-node {op['node']}")
            elif op_kind == "set-node-params":
                touched = set_node_params(tree, op["node"], op.get("set", {}), cat=cat)
                log.append(f"set-node-params {op['node']} {touched}")
            elif op_kind == "move-node":
                move_node(tree, op["node"], tuple(op["pos"]))
                log.append(f"move-node {op['node']}")
            elif op_kind == "add-transition":
                t = add_transition(tree, from_guid=op["from"], next_guid=op["next"],
                                   any_state=bool(op.get("any_state", False)),
                                   duration_secs=float(op.get("duration_secs", 0.0)),
                                   visual_from_guid=op.get("visual_from"))
                log.append(f"add-transition {t.from_guid} -> {t.next_guid}")
            elif op_kind == "remove-transition":
                remove_transition(tree, any_state=bool(op.get("any_state", False)),
                                  index=op.get("index"), from_guid=op.get("from"), next_guid=op.get("next"))
                log.append("remove-transition")
            elif op_kind == "set-transition-params":
                set_transition_params(tree, any_state=bool(op.get("any_state", False)),
                                      index=op.get("index"), from_guid=op.get("from"), next_guid=op.get("next"),
                                      duration_secs=op.get("duration_secs"),
                                      visual_from_guid=op.get("visual_from"))
                log.append("set-transition-params")
            elif op_kind == "add-condition":
                add_condition(tree, any_state=bool(op.get("any_state", False)),
                             index=op.get("index"), from_guid=op.get("from"), next_guid=op.get("next"),
                             name=op["name"], kind=op["kind"], value=op["value"])
                log.append(f"add-condition {op['name']}")
            elif op_kind == "remove-condition":
                remove_condition(tree, any_state=bool(op.get("any_state", False)),
                                 index=op.get("index"), from_guid=op.get("from"), next_guid=op.get("next"),
                                 condition_index=int(op["condition_index"]))
                log.append("remove-condition")
            elif op_kind == "add-param":
                add_param(tree, op["name"], op["kind"], op["value"])
                log.append(f"add-param {op['name']}")
            elif op_kind == "remove-param":
                remove_param(tree, op["name"])
                log.append(f"remove-param {op['name']}")
            elif op_kind == "set-param":
                set_param(tree, op["name"], op["value"])
                log.append(f"set-param {op['name']}")
            else:
                raise EditError(f"unknown op {op_kind!r}")
        except Exception as e:  # noqa: BLE001
            raise EditError(f"op #{i} ({op_kind}): {e}") from e
    return log
