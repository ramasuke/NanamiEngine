"""Structural + Transform edit primitives on a :class:`tools.scene.model.Scene`
or :class:`tools.scene.model.Prefab`, plus the batch :func:`apply` entry point.

Mirrors ``tools.bt.edits``: every function mutates its target in place; a
caller builds a model in memory, applies N edits, calls
``tools.scene.validate`` once, and writes once (see the CLI in ``cli_edit.py``).
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver
from tools.common.cereal_json import Num, OrderedObj
from tools.common.meta_base import mint_guid

from . import catalog as catalog_mod
from . import mathutil, model

EMPTY_GUID = "00000000-0000-0000-0000-000000000000"


class EditError(RuntimeError):
    pass


# ---------------------------------------------------------------------------
# lookup
# ---------------------------------------------------------------------------
def _search(nodes: list[model.GameObjectNode], guid: str):
    for i, n in enumerate(nodes):
        if n.guid == guid:
            return n, nodes, i
        found = _search(n.transform.children, guid)
        if found is not None:
            return found
    return None


def find_gameobject(target: Any, guid: str):
    """``(node, container, index)`` such that ``container[index] is node``, or
    ``(node, None, None)`` if ``guid`` is a Prefab's own root (it has no
    containing list - most edits reject operating on it directly), or ``None``
    if not found anywhere in ``target``."""
    if isinstance(target, model.Prefab):
        if target.root.guid == guid:
            return target.root, None, None
        return _search(target.root.transform.children, guid)
    if isinstance(target, model.Scene):
        return _search(target.roots, guid)
    raise TypeError(f"expected Scene or Prefab, got {type(target).__name__}")


def _require(target: Any, guid: str) -> model.GameObjectNode:
    found = find_gameobject(target, guid)
    if found is None:
        raise EditError(f"GameObject not found: {guid}")
    return found[0]


def _resolve_parent_guid(target: Any, parent: str) -> str:
    if parent in ("root", "") and isinstance(target, model.Prefab):
        return target.root.guid
    return parent


def _find_chain(target: Any, guid: str) -> Optional[list[model.GameObjectNode]]:
    """Root-to-node path (inclusive) - used for world-transform composition."""
    def _walk(nodes: list[model.GameObjectNode], trail: list[model.GameObjectNode]):
        for n in nodes:
            here = trail + [n]
            if n.guid == guid:
                return here
            found = _walk(n.transform.children, here)
            if found is not None:
                return found
        return None

    if isinstance(target, model.Prefab):
        if target.root.guid == guid:
            return [target.root]
        sub = _walk(target.root.transform.children, [target.root])
        return sub
    return _walk(target.roots, [])


# ---------------------------------------------------------------------------
# TRS <-> model conversions
# ---------------------------------------------------------------------------
def _f(n: Any) -> float:
    return float(n.value) if isinstance(n, Num) else float(n)


def _vec3_floats(v: model.Vec3) -> tuple[float, float, float]:
    return (_f(v.x), _f(v.y), _f(v.z))


def _quat_floats(q: model.Quat) -> tuple[float, float, float, float]:
    return (_f(q.x), _f(q.y), _f(q.z), _f(q.w))


def _vec3_from_floats(t: tuple[float, float, float]) -> model.Vec3:
    return model.Vec3(Num.of_float(float(t[0])), Num.of_float(float(t[1])), Num.of_float(float(t[2])))


def _quat_from_floats(t: tuple[float, float, float, float]) -> model.Quat:
    return model.Quat(Num.of_float(float(t[0])), Num.of_float(float(t[1])),
                      Num.of_float(float(t[2])), Num.of_float(float(t[3])))


def _node_local_trs(node: model.GameObjectNode) -> mathutil.Trs:
    t = node.transform
    return mathutil.Trs(_vec3_floats(t.local_pos), _quat_floats(t.local_rot),
                        _vec3_floats(t.local_scale))


def _world_trs_of_chain(chain: list[model.GameObjectNode]) -> mathutil.Trs:
    world = mathutil.IDENTITY
    for node in chain:
        world = world.then(_node_local_trs(node))
    return world


def identity_world_matrix_blob() -> OrderedObj:
    """A freshly-constructed glm::mat4(1.0) identity, matching the shape a new
    Transform's ``worldMatrix_`` is written with (the engine recomputes the
    real value at load, but the field must hold *something* well-shaped)."""
    rows = [(1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0),
            (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0)]
    obj = OrderedObj()
    for i, row in enumerate(rows):
        obj[f"value{i}"] = OrderedObj(
            (f"value{j}", Num.of_float(v)) for j, v in enumerate(row)
        )
    return obj


# ---------------------------------------------------------------------------
# structural edits
# ---------------------------------------------------------------------------
def new_gameobject(name: str, *, kind: str = model.KIND_SCENE,
                   pos: tuple[float, float, float] = (0.0, 0.0, 0.0),
                   rot: tuple[float, float, float, float] = (0.0, 0.0, 0.0, 1.0),
                   scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
                   is_active: bool = True, guid: Optional[str] = None) -> model.GameObjectNode:
    return model.GameObjectNode(
        kind=kind,
        guid=guid or mint_guid(),
        name=name,
        is_active=is_active,
        components=[],
        transform=model.Transform(
            local_pos=_vec3_from_floats(pos),
            local_rot=_quat_from_floats(rot),
            local_scale=_vec3_from_floats(scale),
            world_matrix=identity_world_matrix_blob(),
            children=[],
        ),
    )


def add_gameobject(target: Any, *, parent: Optional[str], name: str,
                   pos: tuple[float, float, float] = (0.0, 0.0, 0.0),
                   rot: tuple[float, float, float, float] = (0.0, 0.0, 0.0, 1.0),
                   scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
                   is_active: bool = True, guid: Optional[str] = None) -> model.GameObjectNode:
    node = new_gameobject(name, pos=pos, rot=rot, scale=scale, is_active=is_active, guid=guid)
    if parent is None:
        if isinstance(target, model.Prefab):
            raise EditError("a Prefab has a single implicit root - pass "
                            "parent='root' (or the root's guid) to add a child")
        target.roots.append(node)
        return node
    parent_guid = _resolve_parent_guid(target, parent)
    parent_node = _require(target, parent_guid)
    parent_node.transform.children.append(node)
    return node


def remove_gameobject(target: Any, guid: str) -> model.GameObjectNode:
    found = find_gameobject(target, guid)
    if found is None:
        raise EditError(f"GameObject not found: {guid}")
    node, container, index = found
    if container is None:
        raise EditError("cannot remove a Prefab's own root GameObject")
    return container.pop(index)


def move_gameobject(target: Any, *, guid: str, new_parent: Optional[str],
                    preserve_world_transform: bool = True) -> None:
    found = find_gameobject(target, guid)
    if found is None:
        raise EditError(f"GameObject not found: {guid}")
    node, container, index = found
    if container is None:
        raise EditError("cannot move a Prefab's own root GameObject")

    new_parent_guid = _resolve_parent_guid(target, new_parent) if new_parent is not None else None
    if new_parent_guid == guid:
        raise EditError("cannot reparent a GameObject under itself")

    world_before = None
    if preserve_world_transform:
        chain = _find_chain(target, guid)
        if chain is None:
            raise EditError(f"GameObject not found while resolving its chain: {guid}")
        world_before = _world_trs_of_chain(chain)

    container.pop(index)
    if new_parent_guid is None:
        if isinstance(target, model.Prefab):
            raise EditError("a Prefab has a single implicit root - pass new_parent")
        target.roots.append(node)
    else:
        new_parent_chain = _find_chain(target, new_parent_guid)
        if new_parent_chain is None:
            # put it back where it was before failing, so the edit is atomic
            container.insert(index, node)
            raise EditError(f"new parent not found: {new_parent}")
        new_parent_node = new_parent_chain[-1]
        new_parent_node.transform.children.append(node)
        if preserve_world_transform:
            new_parent_world = _world_trs_of_chain(new_parent_chain)
            new_local = new_parent_world.local_of(world_before)
            node.transform.local_pos = _vec3_from_floats(new_local.pos)
            node.transform.local_rot = _quat_from_floats(new_local.rot)
            node.transform.local_scale = _vec3_from_floats(new_local.scale)


def set_transform(target: Any, guid: str, *,
                  pos: Optional[tuple[float, float, float]] = None,
                  rot: Optional[tuple[float, float, float, float]] = None,
                  scale: Optional[tuple[float, float, float]] = None) -> None:
    node = _require(target, guid)
    if pos is not None:
        node.transform.local_pos = _vec3_from_floats(pos)
    if rot is not None:
        node.transform.local_rot = _quat_from_floats(rot)
    if scale is not None:
        node.transform.local_scale = _vec3_from_floats(scale)


def set_active(target: Any, guid: str, is_active: bool) -> None:
    node = _require(target, guid)
    node.is_active = bool(is_active)


def rename_gameobject(target: Any, guid: str, new_name: str) -> None:
    node = _require(target, guid)
    node.name = new_name


# ---------------------------------------------------------------------------
# component edits
# ---------------------------------------------------------------------------
def _leaf_of(name: str) -> str:
    return name.split("<", 1)[0].rsplit("::", 1)[-1]


def _default_scalar(shape: str, default: Any) -> Any:
    if shape == "int":
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
    """A brand-new ``Field<T>`` param blob (mirrors ``tools.bt.edits._field_blob``
    - same shape, since this is the generic cereal FieldContext<T> layout, not
    anything Scene/BT-specific)."""
    guid_ver = Ver(("type", "Guid"), 0, OrderedObj([("value_", guid)]))
    holder = Ver(("type", f"FieldHolder<{field_type_leaf}>"), 0,
                OrderedObj([("value0", guid_ver)]))
    ptr = Ptr(exact=True, null=False, fqn=None, wrapper="shared", data=holder)
    return Ver(("type", f"Field<{field_type_leaf}>"), 0, OrderedObj([("value0", ptr)]))


def _param_blob(pinfo: dict) -> Any:
    shape = pinfo.get("shape")
    if shape in ("int", "float", "bool", "string"):
        return _default_scalar(shape, pinfo.get("default"))
    if shape == "vec2":
        return _vec(2)
    if shape == "vec3":
        return _vec(3)
    if shape == "field":
        return _field_blob(_leaf_of(pinfo.get("type", "?")))
    if shape == "vector":
        return []
    # nested/unknown -> a harmless placeholder; validate() flags it, and a
    # human finishes it in the editor (mirrors tools.bt's own limitation).
    return Num.of_int(0)


def component_body_blob(entry: dict, guid: str) -> OrderedObj:
    """A brand-new component's ``data`` blob (everything after its own class
    version): ComponentBase's ``value0`` (``guid_``/``isEnable_``) followed by
    each catalog param's default value, in catalog order."""
    out = OrderedObj()
    guid_ver = Ver(("type", "Guid"), 0, OrderedObj([("value_", guid)]))
    base_body = OrderedObj([("guid_", guid_ver), ("isEnable_", True)])
    out.append("value0", Ver(("type", "ComponentBase"), 0, base_body))
    for p in entry.get("params", []):
        out.append(p["key"], _param_blob(p))
    return out


def new_component(entry: dict, *, guid: Optional[str] = None) -> model.Component:
    if entry.get("immediate_base") not in (None, "ComponentBase"):
        raise EditError(
            f"{entry['fqn']}: has an intermediate base ({entry['immediate_base']}) with its "
            f"own fields this toolkit does not model yet - refusing to construct a new "
            f"instance from scratch (it would be missing required data). Copy an existing "
            f"GameObject/prefab that already has one, or add it via the in-engine editor."
        )
    cguid = guid or mint_guid()
    return model.Component(fqn=entry["fqn"], class_version=int(entry.get("version", 0)),
                           data=component_body_blob(entry, cguid))


def _find_param(entry: dict, key: str) -> Optional[dict]:
    for p in entry.get("params", []):
        if p["key"] == key or p["member"] == key:
            return p
    return None


def _coerce(shape: str, raw: str) -> Any:
    if shape == "int":
        return Num.of_int(int(raw, 0))
    if shape == "float":
        return Num.of_float(float(raw))
    if shape == "bool":
        return raw.strip().lower() in ("1", "true", "yes", "on")
    if shape == "string":
        return raw
    if shape in ("vec2", "vec3"):
        parts = [p.strip() for p in raw.replace(" ", ",").split(",") if p.strip()]
        return OrderedObj([(f"value{i}", Num.of_float(float(x))) for i, x in enumerate(parts)])
    if shape == "field":
        return raw.strip().upper()
    raise EditError(f"cannot set a param of shape {shape!r}")


def _set_field_guid(node: Any, guid: str) -> None:
    # cereal only emits `cereal_class_version` the first time a given type is
    # serialised in an archive - so a Field<T>/FieldHolder<T> at the first
    # occurrence round-trips as a Ver, but every later occurrence of the same
    # type (the common case) has no version key and round-trips as a plain
    # OrderedObj instead. Accept both at each level.
    if isinstance(node, Ver):
        ptr = node.body["value0"]
    elif isinstance(node, OrderedObj):
        ptr = node["value0"]
    else:
        ptr = None
    if not isinstance(ptr, Ptr):
        raise EditError("field param does not have the expected Field<T> shape")
    holder = ptr.data
    if isinstance(holder, Ver):
        guid_node = holder.body["value0"]
    elif isinstance(holder, OrderedObj):
        guid_node = holder["value0"]
    else:
        guid_node = None
    if isinstance(guid_node, Ver):
        guid_node.body["value_"] = guid
    elif isinstance(guid_node, OrderedObj):
        guid_node["value_"] = guid
    else:
        raise EditError("field param does not have the expected FieldHolder<T> shape")


def _set_one_param(comp: model.Component, entry: dict, key: str, raw: str) -> str:
    pinfo = _find_param(entry, key)
    if pinfo is None:
        raise EditError(f"{entry['fqn']} has no param named {key!r}")
    shape = pinfo.get("shape")
    if shape not in catalog_mod.SETTABLE_SHAPES:
        raise EditError(
            f"{entry['fqn']}.{pinfo['member']} has shape {shape!r}; tools/scene v1 cannot "
            f"set it - edit the .scene/.prefab by hand or in the editor"
        )
    jkey = pinfo["key"]
    if not isinstance(comp.data, OrderedObj) or jkey not in comp.data:
        raise EditError(f"param {jkey!r} missing from the stored blob (regen-catalog?)")
    if shape == "field":
        node = comp.data[jkey]
        if not isinstance(node, (Ver, OrderedObj)):
            raise EditError(f"{jkey}: expected a Field<T> blob")
        _set_field_guid(node, _coerce(shape, raw))
    else:
        comp.data[jkey] = _coerce(shape, raw)
    return pinfo["member"]


def add_component(target: Any, guid: str, component_type: str, *,
                  cat: Optional[catalog_mod.Catalog] = None,
                  component_guid: Optional[str] = None,
                  params: Optional[dict[str, str]] = None) -> model.Component:
    cat = cat or catalog_mod.load()
    try:
        entry = cat.resolve_component(component_type)
    except catalog_mod.CatalogError as e:
        raise EditError(str(e)) from e
    node = _require(target, guid)
    comp = new_component(entry, guid=component_guid)
    if params:
        for k, v in params.items():
            _set_one_param(comp, entry, k, v)
    node.components.append(comp)
    return comp


def remove_component(target: Any, guid: str, index: int) -> model.Component:
    node = _require(target, guid)
    if not (0 <= index < len(node.components)):
        raise EditError(f"component index {index} out of range "
                        f"(GameObject has {len(node.components)})")
    return node.components.pop(index)


def set_component_params(target: Any, guid: str, index: int, assignments: dict[str, str],
                         cat: Optional[catalog_mod.Catalog] = None) -> list[str]:
    cat = cat or catalog_mod.load()
    node = _require(target, guid)
    if not (0 <= index < len(node.components)):
        raise EditError(f"component index {index} out of range "
                        f"(GameObject has {len(node.components)})")
    comp = node.components[index]
    entry = cat.component_by_fqn(comp.fqn)
    if entry is None:
        raise EditError(f"{comp.fqn} is not in the catalog (regen-catalog?) - "
                        f"cannot set params by name")
    return [_set_one_param(comp, entry, k, v) for k, v in assignments.items()]


# ---------------------------------------------------------------------------
# instantiate-prefab
# ---------------------------------------------------------------------------
def _remint_guids(node: model.GameObjectNode) -> None:
    node.guid = mint_guid()
    for comp in node.components:
        try:
            model.set_component_guid(comp, mint_guid())
        except ValueError:
            pass  # component has no locatable ComponentBase body - leave it be
    for child in node.transform.children:
        _remint_guids(child)


def instantiate_prefab(target: Any, prefab: model.Prefab, *,
                       parent: Optional[str] = None) -> model.GameObjectNode:
    """Deep-copy ``prefab``'s tree into ``target`` (a Scene, or another
    Prefab's tree), minting a fresh GUID for every GameObject/Component in the
    copy and re-tagging the root as ``CopiedPrefabGameObject`` - mirroring the
    engine's own ``Scene::OnDrawFileDropGui`` ->
    ``PrefabGameObject::CopyForInstantiate()`` behaviour (a scene's copy of a
    prefab is always a fully independent baked snapshot, never a live link
    back to the source ``.prefab``)."""
    import copy as _copy

    new_root = _copy.deepcopy(prefab.root)
    new_root.kind = model.KIND_COPIED_PREFAB
    _remint_guids(new_root)
    if parent is None:
        if isinstance(target, model.Prefab):
            raise EditError("a Prefab has a single implicit root - pass "
                            "parent='root' (or the root's guid)")
        target.roots.append(new_root)
    else:
        parent_guid = _resolve_parent_guid(target, parent)
        parent_node = _require(target, parent_guid)
        parent_node.transform.children.append(new_root)
    return new_root


# ---------------------------------------------------------------------------
# copy-prefab
# ---------------------------------------------------------------------------
def copy_prefab(prefab: model.Prefab) -> model.Prefab:
    """Deep-copy a whole ``Prefab`` for saving as a brand-new standalone
    ``.prefab`` file: fresh GUID for every GameObject/Component in the tree
    (same ``_remint_guids`` rule as ``instantiate_prefab``) and an empty
    ``copied_object_guids`` list, since this new file has no scene instances
    of its own yet. Unlike ``instantiate_prefab``, the root's ``kind`` stays
    ``KIND_PREFAB_ROOT`` - the result is still a real prefab asset, not a
    scene-embedded instance."""
    import copy as _copy

    new_root = _copy.deepcopy(prefab.root)
    _remint_guids(new_root)
    return model.Prefab(root=new_root, copied_object_guids=[])


# ---------------------------------------------------------------------------
# batch apply - the primary agent-facing interface
# ---------------------------------------------------------------------------
def apply(target: Any, ops: list[dict]) -> list[str]:
    """Apply a JSON-shaped list of ops in order; each is
    ``{"op": "<name>", ...kwargs}``. Raises :class:`EditError` naming the
    failing op's index on the first failure (nothing after it is applied)."""
    log: list[str] = []
    for i, op in enumerate(ops):
        kind = op.get("op")
        try:
            if kind == "add-gameobject":
                node = add_gameobject(
                    target, parent=op.get("parent"), name=op["name"],
                    pos=tuple(op.get("pos", (0.0, 0.0, 0.0))),
                    rot=tuple(op.get("rot", (0.0, 0.0, 0.0, 1.0))),
                    scale=tuple(op.get("scale", (1.0, 1.0, 1.0))),
                    is_active=bool(op.get("is_active", True)),
                    guid=op.get("guid"),
                )
                log.append(f"add-gameobject: {node.name} [{node.guid}]")
            elif kind == "remove-gameobject":
                node = remove_gameobject(target, op["guid"])
                log.append(f"remove-gameobject: {node.name} [{node.guid}]")
            elif kind == "move-gameobject":
                move_gameobject(target, guid=op["guid"], new_parent=op.get("new_parent"),
                                preserve_world_transform=bool(op.get("preserve_world_transform", True)))
                log.append(f"move-gameobject: {op['guid']} -> {op.get('new_parent')}")
            elif kind == "set-transform":
                pos = tuple(op["pos"]) if "pos" in op else None
                rot = tuple(op["rot"]) if "rot" in op else None
                scale = tuple(op["scale"]) if "scale" in op else None
                set_transform(target, op["guid"], pos=pos, rot=rot, scale=scale)
                log.append(f"set-transform: {op['guid']}")
            elif kind == "set-active":
                set_active(target, op["guid"], bool(op["is_active"]))
                log.append(f"set-active: {op['guid']} -> {op['is_active']}")
            elif kind == "rename-gameobject":
                rename_gameobject(target, op["guid"], op["name"])
                log.append(f"rename-gameobject: {op['guid']} -> {op['name']}")
            elif kind == "add-component":
                comp = add_component(target, op["guid"], op["type"],
                                     component_guid=op.get("component_guid"),
                                     params=op.get("params"))
                log.append(f"add-component: {op['guid']} += {comp.fqn}")
            elif kind == "remove-component":
                comp = remove_component(target, op["guid"], int(op["index"]))
                log.append(f"remove-component: {op['guid']} -= {comp.fqn} (#{op['index']})")
            elif kind == "set-component-params":
                touched = set_component_params(target, op["guid"], int(op["index"]), op["params"])
                log.append(f"set-component-params: {op['guid']}#{op['index']} -> {touched}")
            elif kind == "instantiate-prefab":
                from . import reader as _reader
                prefab = _reader.read_prefab_file(op["prefab_path"])
                node = instantiate_prefab(target, prefab, parent=op.get("parent"))
                log.append(f"instantiate-prefab: {op['prefab_path']} -> {node.name} [{node.guid}]")
            else:
                raise EditError(f"unknown op: {kind!r}")
        except Exception as e:  # noqa: BLE001
            raise EditError(f"op #{i} ({kind}): {e}") from e
    return log
