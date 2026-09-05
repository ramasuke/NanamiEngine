""":mod:`tools.scene.model` (:class:`Scene` / :class:`Prefab`) -> cereal-JSON text.

Regenerates every polymorphic id, ptr_wrapper id and cereal_class_version from
global counters, in the exact depth-first order ``cereal::JSONOutputArchive``
uses (mirrors ``tools.bt.writer``'s approach), so the output loads back
identically. A ``.prefab`` root is written as a bare object (no polymorphic
wrapper/version/base-chain - see :func:`write_prefab`); a ``.scene``'s root
array and every nested GameObject/Component go through the normal
``shared_ptr`` bookkeeping.
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver
from tools.common.cereal_json import Num, OrderedObj, dumps, to_file_bytes

from . import model

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


class _W:
    def __init__(self) -> None:
        self.k = 0
        self.poly_ctr = 0
        self.poly: dict[str, int] = {}
        self.emitted: set[tuple] = set()

    # -- bookkeeping counters ------------------------------------------------
    def new_k(self) -> int:
        self.k += 1
        return FIRST_BIT | self.k

    def poly_id(self, fqn: str) -> tuple[int, bool]:
        if fqn in self.poly:
            return self.poly[fqn], False
        self.poly_ctr += 1
        self.poly[fqn] = self.poly_ctr
        return self.poly_ctr, True

    def emit_ver(self, key: tuple, version: int, obj: OrderedObj) -> None:
        if key not in self.emitted:
            self.emitted.add(key)
            obj.insert(0, "cereal_class_version", Num.of_int(int(version)))

    def poly_slot(self, fqn: str, exact: bool) -> OrderedObj:
        o = OrderedObj()
        if exact:
            o["polymorphic_id"] = Num.of_int(EXACT_PID)
            return o
        pid, first = self.poly_id(fqn)
        o["polymorphic_id"] = Num.of_int((FIRST_BIT | pid) if first else pid)
        if first:
            o["polymorphic_name"] = fqn
        return o

    # -- tagged blob (component params) --------------------------------------
    def blob(self, n: Any) -> Any:
        if isinstance(n, Ptr):
            if n.null:
                return OrderedObj([("polymorphic_id", Num.of_int(0))])
            o = self.poly_slot(n.fqn or "", exact=n.exact)
            if n.wrapper == "shared":
                kid = self.new_k()
                o["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)),
                                               ("data", self.blob(n.data))])
            else:
                o["ptr_wrapper"] = OrderedObj([("valid", Num.of_int(1)),
                                               ("data", self.blob(n.data))])
            return o
        if isinstance(n, Ver):
            o = OrderedObj()
            if n.literal_presence is None:
                self.emit_ver(n.key, n.version, o)
            elif n.literal_presence:
                o.insert(0, "cereal_class_version", Num.of_int(int(n.version)))
            # literal_presence is False: never emit for this occurrence.
            for k, v in n.body.items():
                o[k] = self.blob(v)
            return o
        if isinstance(n, OrderedObj):
            return OrderedObj((k, self.blob(v)) for k, v in n.items())
        if isinstance(n, list):
            return [self.blob(x) for x in n]
        return n

    # -- small fixed-shape leaves --------------------------------------------
    def guid_obj(self, guid: str) -> OrderedObj:
        g = OrderedObj()
        self.emit_ver(("type", "Guid"), 0, g)
        g["value_"] = guid
        return g

    def vec3_obj(self, v: model.Vec3) -> OrderedObj:
        return OrderedObj([("value0", v.x), ("value1", v.y), ("value2", v.z)])

    def quat_obj(self, q: model.Quat) -> OrderedObj:
        return OrderedObj([("value0", q.x), ("value1", q.y), ("value2", q.z), ("value3", q.w)])

    def base_chain_obj(self) -> OrderedObj:
        """The IGameObject -> IObject base-class chain every GameObject kind
        shares (``archive(cereal::base_class<IGameObject>(this))`` -> IGameObject's
        own ``archive(cereal::base_class<IObject>(this))`` - both bodies are
        otherwise empty)."""
        io = OrderedObj()
        self.emit_ver(("type", "IObject"), 0, io)
        ig = OrderedObj()
        self.emit_ver(("type", "IGameObject"), 0, ig)
        ig["value0"] = io
        return ig

    # -- Transform / ComponentGroup / Component ------------------------------
    def transform_obj(self, t: model.Transform) -> OrderedObj:
        obj = OrderedObj()
        self.emit_ver(("type", model.FQN_TRANSFORM), 0, obj)
        obj["localPos_"] = self.vec3_obj(t.local_pos)
        obj["localRot_"] = self.quat_obj(t.local_rot)
        obj["localScale_"] = self.vec3_obj(t.local_scale)
        obj["worldMatrix_"] = self.blob(t.world_matrix)
        obj["childCount"] = Num.of_int(len(t.children))
        for child in t.children:
            obj.append("child", self.gameobject_slot(child))
        return obj

    def component_slot(self, c: model.Component) -> OrderedObj:
        slot = self.poly_slot(c.fqn, exact=False)
        kid = self.new_k()
        data = OrderedObj()
        self.emit_ver(("comp", c.fqn), int(c.class_version), data)
        src = c.data if isinstance(c.data, OrderedObj) else OrderedObj()
        for k, v in src.items():
            data[k] = self.blob(v)
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot

    def components_obj(self, comps: list[model.Component]) -> OrderedObj:
        obj = OrderedObj()
        self.emit_ver(("type", model.FQN_COMPONENT_GROUP), 0, obj)
        obj["componentCount"] = Num.of_int(len(comps))
        for i, c in enumerate(comps):
            obj[f"component_{i}"] = self.component_slot(c)
        return obj

    # -- GameObject -----------------------------------------------------
    def gameobject_body(self, node: model.GameObjectNode) -> OrderedObj:
        """``isActive_``/``name_``/``guid_``/``components_``/``transform_``,
        unwrapped - used directly for a ``.prefab`` root, and nested under a
        base-chain wrapper for a ``gameObject_N``/``"child"`` polymorphic slot.
        """
        body = OrderedObj()
        body["isActive_"] = bool(node.is_active)
        body["name_"] = node.name
        body["guid_"] = self.guid_obj(node.guid)
        body["components_"] = self.components_obj(node.components)
        body["transform_"] = self.transform_obj(node.transform)
        return body

    def gameobject_slot(self, node: Optional[model.GameObjectNode]) -> OrderedObj:
        if node is None:
            return OrderedObj([("polymorphic_id", Num.of_int(0))])
        fqn = model.GAMEOBJECT_FQN_BY_KIND[node.kind]
        slot = self.poly_slot(fqn, exact=False)
        kid = self.new_k()
        data = OrderedObj()
        self.emit_ver(("go", fqn), model.GAMEOBJECT_CLASS_VERSION[fqn], data)
        data["value0"] = self.base_chain_obj()
        for k, v in self.gameobject_body(node).items():
            data[k] = v
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot


def write_scene(scene: model.Scene) -> str:
    w = _W()
    root = OrderedObj()
    root["name"] = scene.name
    root["gameObjectCount"] = Num.of_int(len(scene.roots))
    for i, node in enumerate(scene.roots):
        root[f"gameObject_{i}"] = w.gameobject_slot(node)
    return dumps(root)


def write_scene_file(path, scene: model.Scene) -> None:
    open(path, "wb").write(to_file_bytes(write_scene(scene)))


def write_prefab(prefab: model.Prefab) -> str:
    """A ``.prefab`` root is a bare object (see ``PrefabGameObject::OnSave()``):
    no polymorphic wrapper, no outer class version, no IGameObject/IObject base
    chain - just the five body fields, then the ``copiedObjectGuidList_`` tail
    as a bare count (``value0``) followed by that many bare Guids
    (``value1..N``)."""
    w = _W()
    root = w.gameobject_body(prefab.root)
    root["value0"] = Num.of_int(len(prefab.copied_object_guids))
    for i, guid in enumerate(prefab.copied_object_guids, 1):
        root[f"value{i}"] = w.guid_obj(guid)
    return dumps(root)


def write_prefab_file(path, prefab: model.Prefab) -> None:
    open(path, "wb").write(to_file_bytes(write_prefab(prefab)))
