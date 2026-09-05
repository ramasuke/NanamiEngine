""":class:`tools.animtree.model.Tree`  ->  cereal-JSON text.

Regenerates every polymorphic id, ptr_wrapper id and cereal_class_version from
global counters, in the exact depth-first order ``AnimationTree::OnSave()``
uses, so the output loads back identically.
"""

from __future__ import annotations

from typing import Any

from tools.common.blob import Ptr, Ver
from tools.common.cereal_json import Num, OrderedObj, dumps, to_file_bytes

from . import catalog as catalog_mod
from . import model

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


class _W:
    def __init__(self, cat: catalog_mod.Catalog) -> None:
        self.cat = cat
        self.k = 0
        self.poly_ctr = 0
        self.poly: dict[str, int] = {}
        self.emitted: set[tuple] = set()

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

    # -- pointer slots -------------------------------------------------------
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

    # -- shared leaf helpers -------------------------------------------------
    def guid_obj(self, guid: str) -> OrderedObj:
        g = OrderedObj()
        self.emit_ver(("type", "Guid"), 0, g)
        g["value_"] = guid
        return g

    @staticmethod
    def _num_float(v: Any) -> Num:
        # preserve an already-Num value's original literal text (see
        # reader._vec2's docstring); only synthesise a fresh literal for a
        # plain Python number (a value an edit just set).
        return v if isinstance(v, Num) else Num.of_float(float(v))

    @staticmethod
    def _num_int(v: Any) -> Num:
        return v if isinstance(v, Num) else Num.of_int(int(v))

    def pos_obj(self, pos: tuple[Any, Any]) -> OrderedObj:
        return OrderedObj([("value0", self._num_float(pos[0])),
                           ("value1", self._num_float(pos[1]))])

    @classmethod
    def _scalar(cls, kind: str, value: Any) -> Any:
        if kind == "bool":
            return bool(value)
        if kind == "int":
            return cls._num_int(value)
        if kind == "float":
            return cls._num_float(value)
        raise ValueError(f"unknown kind {kind!r}")

    # -- tagged blob (mirrors tools.bt.writer._W.blob) -----------------------
    def blob(self, n: Any) -> Any:
        if isinstance(n, Ptr):
            if n.null:
                return OrderedObj([("polymorphic_id", Num.of_int(0))])
            o = self.poly_slot(n.fqn or "", exact=n.exact)
            if n.wrapper == "shared":
                kid = self.new_k()
                o["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", self.blob(n.data))])
            else:
                o["ptr_wrapper"] = OrderedObj([("valid", Num.of_int(1)), ("data", self.blob(n.data))])
            return o
        if isinstance(n, Ver):
            o = OrderedObj()
            self.emit_ver(n.key, n.version, o)
            for k, v in n.body.items():
                o[k] = self.blob(v)
            return o
        if isinstance(n, OrderedObj):
            return OrderedObj((k, self.blob(v)) for k, v in n.items())
        if isinstance(n, list):
            return [self.blob(x) for x in n]
        return n

    # -- node ------------------------------------------------------------
    def node_base_slot(self) -> OrderedObj:
        # the leading, unnamed archive(cereal::base_class<IAnimationNode>(this))
        # every node's save() makes - always {} or {"cereal_class_version": 0}
        base = OrderedObj()
        self.emit_ver(("type", "IAnimationNode"), 0, base)
        return base

    def node_data(self, node: model.Node) -> OrderedObj:
        entry = self.cat.node_by_fqn(node.type_fqn)
        if entry is None:
            raise ValueError(f"unknown node type: {node.type_fqn!r} (regen-catalog?)")
        data = OrderedObj()
        self.emit_ver(("type", entry["leaf"]), int(node.class_version), data)
        data["value0"] = self.node_base_slot()
        params = node.params if node.params is not None else OrderedObj()
        for pinfo in self.cat.params_of(entry):
            key = pinfo["key"]
            shape = pinfo.get("shape")
            if shape == "self_guid":
                data[key] = self.guid_obj(node.guid)
            elif shape == "self_pos":
                data[key] = self.pos_obj(node.pos)
            else:
                if key not in params:
                    raise ValueError(f"{node.type_fqn}: param {key!r} missing from the stored blob")
                data[key] = self.blob(params[key])
        return data

    def node_slot(self, node: model.Node, *, exact: bool) -> OrderedObj:
        slot = self.poly_slot(node.type_fqn, exact=exact)
        kid = self.new_k()
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", self.node_data(node))])
        return slot

    # -- conditions / transitions ------------------------------------------
    def condition_slot(self, c: model.Condition) -> OrderedObj:
        fqn = self.cat.condition_fqn(c.kind)
        slot = self.poly_slot(fqn, exact=False)
        kid = self.new_k()
        data = OrderedObj()
        self.emit_ver(("type", f"AnimationNodePathAdditionCondition<{c.kind}>"),
                      self.cat.condition_version(c.kind), data)
        data["value0"] = self._condition_base_slot()
        data["name_"] = c.name
        data["equalValue_"] = self._scalar(c.kind, c.value)
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot

    def _condition_base_slot(self) -> OrderedObj:
        base = OrderedObj()
        self.emit_ver(("type", "IAnimationNodePathAdditionCondition"), 0, base)
        return base

    def condition_group_slot(self, conditions: list[model.Condition]) -> OrderedObj:
        data = OrderedObj()
        self.emit_ver(("type", "AnimationNodePathAdditionConditionGroup"),
                      model.COND_GROUP_CLASS_VERSION, data)
        data["value0"] = Num.of_int(len(conditions))
        for i, c in enumerate(conditions, 1):
            data[f"value{i}"] = self.condition_slot(c)
        return OrderedObj([("ptr_wrapper", OrderedObj([("valid", Num.of_int(1)), ("data", data)]))])

    def transition_slot(self, t: model.Transition) -> OrderedObj:
        slot = OrderedObj([("polymorphic_id", Num.of_int(EXACT_PID))])
        kid = self.new_k()
        data = OrderedObj()
        self.emit_ver(("type", "AnimationNodePath"), model.NODE_PATH_CLASS_VERSION, data)
        iobj = OrderedObj()
        self.emit_ver(("type", "IObject"), model.IOBJECT_CLASS_VERSION, iobj)
        data["value0"] = iobj
        data["additionConditionGroup_"] = self.condition_group_slot(t.conditions)
        data["transitionDuration_secs_"] = self._num_float(t.duration_secs)
        data["fromNodeGuid_"] = self.guid_obj(t.from_guid)
        data["nextNodeGuid_"] = self.guid_obj(t.next_guid)
        data["visualFromNodeGuid_"] = self.guid_obj(t.visual_from_guid)
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot

    # -- additionParameters_ ------------------------------------------------
    def params_block(self, params: list[model.Param]) -> OrderedObj:
        # ParameterGroup is not IObject-derived and its save()/load() take no
        # version arg - no polymorphic_id wrapper, no cereal_class_version at
        # this level (unlike every other shared_ptr in the file).
        outer_kid = self.new_k()
        data = OrderedObj([("value0", Num.of_int(len(params)))])
        for i, p in enumerate(params, 1):
            fqn = self.cat.param_fqn(p.kind)
            slot = self.poly_slot(fqn, exact=False)
            kid = self.new_k()
            pd = OrderedObj()
            self.emit_ver(("type", f"AnimationParameter<{p.kind}>"), self.cat.param_version(p.kind), pd)
            pd["value0"] = self._param_base_slot()
            pd["name_"] = p.name
            pd["value_"] = self._scalar(p.kind, p.value)
            slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", pd)])
            data[f"value{i}"] = slot
        return OrderedObj([("ptr_wrapper", OrderedObj([("id", Num.of_int(outer_kid)), ("data", data)]))])

    def _param_base_slot(self) -> OrderedObj:
        base = OrderedObj()
        self.emit_ver(("type", "IAnimationParameter"), 0, base)
        return base


def write_tree(tree: model.Tree, cat: catalog_mod.Catalog | None = None) -> str:
    cat = cat or catalog_mod.load()
    w = _W(cat)
    root = OrderedObj()
    root["additionParameters_"] = w.params_block(tree.params)
    root["entryNode"] = w.node_slot(tree.entry, exact=True)
    root["visualAnyStateNode"] = w.node_slot(tree.any_state, exact=True)

    root["nodesCount"] = Num.of_int(len(tree.nodes))
    for i, n in enumerate(tree.nodes):
        root[f"nodes_{i}"] = w.node_slot(n, exact=False)

    root["fromNodeNodePathCount"] = Num.of_int(len(tree.transitions))
    for i, t in enumerate(tree.transitions):
        root[f"fromNodeNodePath_{i}"] = w.transition_slot(t)

    root["fromAnyStateNodeNodePathCount"] = Num.of_int(len(tree.any_state_transitions))
    for i, t in enumerate(tree.any_state_transitions):
        root[f"fromAnyStateNodeNodePath_{i}"] = w.transition_slot(t)

    return dumps(root)


def write_tree_file(path, tree: model.Tree, cat: catalog_mod.Catalog | None = None) -> None:
    open(path, "wb").write(to_file_bytes(write_tree(tree, cat)))
