"""cereal-JSON text -> :mod:`tools.scene.model` (:class:`Scene` / :class:`Prefab`).

Node/Component structure is decoded into the model; everything inside a
Component's own ``data`` (after its outer ``cereal_class_version``) becomes a
tagged :mod:`tools.common.blob` so the writer can rebuild bookkeeping from
scratch without needing to understand every component type's internal shape
(mirrors ``tools.bt.reader``'s approach to action parameters).

Only "pure tree" archives are supported (every ``ptr_wrapper`` writes fresh
data - true of every real ``.scene``/``.prefab`` in this engine, since a scene's
copy of a prefab is always a fully independent baked snapshot with fresh GUIDs,
never a live shared reference); a back-reference raises :class:`PureTreeError`.
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver, fingerprint
from tools.common.cereal_json import Num, OrderedObj, loads, read_text

from . import model

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


class PureTreeError(RuntimeError):
    pass


class _Ctx:
    """Per-parse state: the archive's polymorphic type table and per-type
    class-version memory (learned from whichever occurrence first shows
    ``cereal_class_version`` - see module docstring in ``model.py``)."""

    def __init__(self) -> None:
        self.poly: dict[int, str] = {}
        self.component_versions: dict[str, int] = {}

    def ptr_slot(self, slot: OrderedObj) -> dict:
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
                    "- this file is a DAG, which tools/scene does not support"
                )
        elif "valid" in pw:
            wrapper = "unique"
        else:
            raise PureTreeError(f"ptr_wrapper without id/valid: {pw!r}")
        return dict(null=False, exact=exact, fqn=fqn, wrapper=wrapper, data=pw["data"])


def _num(v: Any) -> Any:
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


# ---------------------------------------------------------------------------
# generic blob tagging (catalog-independent - see model.py docstring)
# ---------------------------------------------------------------------------
def _tag_value(ctx: _Ctx, val: Any) -> Any:
    if isinstance(val, (Num, str, bool)) or val is None:
        return val
    if isinstance(val, list):
        return [_tag_value(ctx, x) for x in val]
    if not isinstance(val, OrderedObj):
        return val

    keys = val.keys()
    if "polymorphic_id" in keys:
        s = ctx.ptr_slot(val)
        if s["null"]:
            return Ptr(exact=False, null=True, fqn=None, wrapper="", data=None)
        return Ptr(exact=s["exact"], null=False, fqn=s["fqn"], wrapper=s["wrapper"],
                  data=_tag_value(ctx, s["data"]))

    if keys and keys[0] == "cereal_class_version":
        v, body = _strip_ccv(val)
        if _is_guid_obj(val):
            return Ver(("type", "Guid"), v or 0, OrderedObj([("value_", body["value_"])]))
        # Fallback for unmodeled versioned structs (no catalog to name the real
        # C++ type here): key by structural fingerprint AND pin literal_presence
        # so this occurrence's version is reproduced unconditionally. Without
        # this, several genuinely-different, structurally-identical marker
        # types (e.g. multiple empty mixin base classes serialised as bare
        # sibling "value1"/"value2"/... members) would collide on the same
        # synthetic key and wrongly suppress each other's version after the
        # first - see tools.common.blob.Ver.literal_presence.
        return Ver(("fp", fingerprint(val)), v or 0, _tag_plain(ctx, body), literal_presence=True)

    if _is_guid_obj(val):
        return OrderedObj([("value_", val["value_"])])

    return _tag_plain(ctx, val)


def _tag_plain(ctx: _Ctx, obj: OrderedObj) -> OrderedObj:
    return OrderedObj((k, _tag_value(ctx, v)) for k, v in obj.items())


# ---------------------------------------------------------------------------
# Transform
# ---------------------------------------------------------------------------
def _read_vec3(obj: OrderedObj) -> model.Vec3:
    return model.Vec3(obj["value0"], obj["value1"], obj["value2"])


def _read_quat(obj: OrderedObj) -> model.Quat:
    return model.Quat(obj["value0"], obj["value1"], obj["value2"], obj["value3"])


def _read_transform(ctx: _Ctx, obj: OrderedObj) -> model.Transform:
    _v, obj = _strip_ccv(obj)  # Transform's own version (0) - re-derived by the writer
    local_pos = _read_vec3(obj["localPos_"])
    local_rot = _read_quat(obj["localRot_"])
    local_scale = _read_vec3(obj["localScale_"])
    world_matrix = obj["worldMatrix_"]  # opaque - never edited, passed through untouched
    count = int(_num(obj["childCount"]))
    children = []
    for slot in obj.values_for("child"):
        node = _read_gameobject_slot(ctx, slot)
        if node is not None:
            children.append(node)
    if len(children) != count:
        raise ValueError(f"childCount={count} but found {len(children)} \"child\" entries")
    return model.Transform(local_pos=local_pos, local_rot=local_rot, local_scale=local_scale,
                           world_matrix=world_matrix, children=children)


# ---------------------------------------------------------------------------
# Components
# ---------------------------------------------------------------------------
def _read_component_slot(ctx: _Ctx, slot: OrderedObj) -> model.Component:
    s = ctx.ptr_slot(slot)
    if s["null"]:
        raise ValueError("null component pointer encountered")
    fqn = s["fqn"]
    if fqn is None:
        raise ValueError("component slot without a resolvable type")
    v, rest = _strip_ccv(s["data"])
    if v is not None:
        ctx.component_versions[fqn] = v
    class_version = ctx.component_versions.get(fqn)
    if class_version is None:
        raise ValueError(
            f"component {fqn}: version never seen (its first occurrence in the "
            f"file should carry cereal_class_version)"
        )
    return model.Component(fqn=fqn, class_version=class_version, data=_tag_plain(ctx, rest))


def _read_components(ctx: _Ctx, obj: OrderedObj) -> list[model.Component]:
    _v, obj = _strip_ccv(obj)  # ComponentGroup's own version (0)
    count = int(_num(obj["componentCount"]))
    return [_read_component_slot(ctx, obj[f"component_{i}"]) for i in range(count)]


# ---------------------------------------------------------------------------
# GameObject
# ---------------------------------------------------------------------------
def _read_gameobject_body(ctx: _Ctx, kind: str, obj: OrderedObj) -> model.GameObjectNode:
    is_active = bool(obj["isActive_"])
    name = obj["name_"]
    guid = obj["guid_"]["value_"]
    components = _read_components(ctx, obj["components_"])
    transform = _read_transform(ctx, obj["transform_"])
    return model.GameObjectNode(kind=kind, guid=guid, name=name, is_active=is_active,
                                components=components, transform=transform)


def _read_gameobject_slot(ctx: _Ctx, slot: OrderedObj) -> Optional[model.GameObjectNode]:
    """Read a ``shared_ptr<IGameObject>`` slot (a ``gameObject_N`` entry, or a
    Transform's repeated ``"child"`` entry)."""
    s = ctx.ptr_slot(slot)
    if s["null"]:
        return None
    fqn = s["fqn"]
    kind = model.GAMEOBJECT_KIND_BY_FQN.get(fqn)
    if kind is None:
        raise ValueError(f"unknown GameObject type in slot: {fqn!r}")
    _v, data = _strip_ccv(s["data"])  # the concrete type's own version (SceneGameObject=0, ...)
    base = data["value0"]  # IGameObject -> IObject base chain: no fields to extract, just skip
    if not isinstance(base, OrderedObj):
        raise ValueError("GameObject base-class block (value0) has an unexpected shape")
    body = OrderedObj(data.items()[1:])  # everything after value0: isActive_.. transform_
    return _read_gameobject_body(ctx, kind, body)


# ---------------------------------------------------------------------------
# Scene
# ---------------------------------------------------------------------------
def read_scene(text: str) -> model.Scene:
    ctx = _Ctx()
    root = loads(text)
    if "gameObjectCount" not in root:
        raise ValueError("not a .scene file: missing gameObjectCount")
    name = root["name"]
    count = int(_num(root["gameObjectCount"]))
    roots = []
    for i in range(count):
        node = _read_gameobject_slot(ctx, root[f"gameObject_{i}"])
        if node is not None:
            roots.append(node)
    return model.Scene(name=name, roots=roots)


def read_scene_file(path) -> model.Scene:
    return read_scene(read_text(path))


# ---------------------------------------------------------------------------
# Prefab
# ---------------------------------------------------------------------------
def read_prefab(text: str) -> model.Prefab:
    """A ``.prefab`` root is written by hand-rolled ``archive(CEREAL_NVP(x))``
    calls in ``PrefabGameObject::OnSave()`` - NOT through the class's own
    ``save``/``load`` template (that pair only fires for the in-memory
    portable-binary round-trip used by ``CopyForInstantiate``). So, unlike a
    ``gameObject_N`` slot, the root here is a bare object: no polymorphic
    wrapper, no outer ``cereal_class_version``, no IGameObject/IObject base
    chain - just the five named fields, then the ``copiedObjectGuidList_`` tail
    as a bare count (``value0``) followed by that many bare ``Guid``s
    (``value1..N``).
    """
    ctx = _Ctx()
    root = loads(text)
    if "components_" not in root or "transform_" not in root:
        raise ValueError("not a .prefab file: missing components_/transform_")
    go = _read_gameobject_body(ctx, model.KIND_PREFAB_ROOT, root)

    guids: list[str] = []
    if "value0" in root:
        count = int(_num(root["value0"]))
        for i in range(1, count + 1):
            g = root[f"value{i}"]
            _v, g = _strip_ccv(g)
            guids.append(g["value_"])
    return model.Prefab(root=go, copied_object_guids=guids)


def read_prefab_file(path) -> model.Prefab:
    return read_prefab(read_text(path))
