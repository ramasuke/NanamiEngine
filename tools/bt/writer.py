""":class:`tools.bt.model.Tree`  ->  cereal-JSON text.

Regenerates every polymorphic id, ptr_wrapper id and cereal_class_version from
global counters, in the exact depth-first order ``cereal::JSONOutputArchive``
uses, so the output loads back identically.
"""

from __future__ import annotations

from typing import Any

from . import model
from .blob import Ptr, Ver
from .cereal_json import Num, OrderedObj, dumps

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000

ANIM_PARAM_INT_FQN = "NanamiEngine::Module::AnimationTree::AnimationParameter<int>"

_FQN_BY_KIND = {
    model.Selector: model.FQN_SELECTOR,
    model.Sequence: model.FQN_SEQUENCE,
    model.RandomSelector: model.FQN_RANDOM,
    model.OnceExecute: model.FQN_ONCE_EXEC,
    model.OnceSuccess: model.FQN_ONCE_SUCCESS,
    model.Action: model.FQN_ACTION_NODE,
}
_LEAF = {
    model.FQN_SELECTOR: "SelectorNode",
    model.FQN_SEQUENCE: "SequenceNode",
    model.FQN_RANDOM: "RandomSelectorNode",
    model.FQN_ONCE_EXEC: "OnceExecute",
    model.FQN_ONCE_SUCCESS: "OnceSuccessNode",
    model.FQN_ACTION_NODE: "ActionNode",
}


class _W:
    def __init__(self) -> None:
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

    # -- node structure ---------------------------------------------------
    def nodebase_header(self, guid: str, pos: tuple[float, float]) -> OrderedObj:
        h = OrderedObj()
        self.emit_ver(("type", "NodeBase"), 0, h)
        io = OrderedObj()
        self.emit_ver(("type", "IObject"), 0, io)
        h["value0"] = io
        g = OrderedObj()
        self.emit_ver(("type", "Guid"), 0, g)
        g["value_"] = guid
        h["guid_"] = g
        h["position_"] = OrderedObj(
            [("value0", Num.of_float(float(pos[0]))), ("value1", Num.of_float(float(pos[1])))]
        )
        return h

    def node_slot(self, node) -> OrderedObj:
        if node is None:
            return OrderedObj([("polymorphic_id", Num.of_int(0))])
        fqn = _FQN_BY_KIND[type(node)]
        slot = self.poly_slot(fqn, exact=False)
        kid = self.new_k()
        data = OrderedObj()
        self.emit_ver(("type", _LEAF[fqn]), model.NODE_CLASS_VERSION[fqn], data)
        data["value0"] = self.nodebase_header(node.guid, node.pos)

        if isinstance(node, (model.Selector, model.Sequence)):
            data["children_"] = [self.node_slot(c) for c in node.children]
        elif isinstance(node, model.RandomSelector):
            data["children_"] = [self.node_slot(c) for c in node.children]
            data["weights_"] = [Num.of_int(int(w)) for w in node.weights]
        elif isinstance(node, model.OnceExecute):
            data["child_"] = self.node_slot(node.child)
            data["state_"] = Num.of_int(int(node.state))
        elif isinstance(node, model.OnceSuccess):
            data["child_"] = self.node_slot(node.child)
        elif isinstance(node, model.Action):
            data["name_"] = node.name
            data["action_"] = self.action_slot(node)
        else:  # pragma: no cover
            raise TypeError(type(node).__name__)

        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot

    def action_slot(self, action: model.Action) -> OrderedObj:
        slot = self.poly_slot(action.type_fqn, exact=False)
        data = OrderedObj()
        self.emit_ver(("type", action.type_name.rsplit("::", 1)[-1]),
                      int(action.action_version), data)
        params = action.params if action.params is not None else OrderedObj()
        for key, val in params.items():
            data[key] = self.blob(val)
        slot["ptr_wrapper"] = OrderedObj([("valid", Num.of_int(1)), ("data", data)])
        return slot

    # -- tagged blob ----------------------------------------------------
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
            self.emit_ver(n.key, n.version, o)
            for k, v in n.body.items():
                o[k] = self.blob(v)
            return o
        if isinstance(n, OrderedObj):
            return OrderedObj((k, self.blob(v)) for k, v in n.items())
        if isinstance(n, list):
            return [self.blob(x) for x in n]
        return n

    # -- blackboard ----------------------------------------------------
    def params_block(self, params: list[model.BbParam]) -> OrderedObj:
        data = OrderedObj([("value0", Num.of_int(len(params)))])
        for i, p in enumerate(params, 1):
            slot = self.poly_slot(ANIM_PARAM_INT_FQN, exact=False)
            kid = self.new_k()
            pd = OrderedObj()
            self.emit_ver(("type", "AnimationParameter<int>"), 0, pd)
            base = OrderedObj()
            self.emit_ver(("type", "IAnimationParameter"), 0, base)
            pd["value0"] = base
            pd["name_"] = p.name
            pd["value_"] = Num.of_int(int(p.value))
            slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", pd)])
            data[f"value{i}"] = slot
        return OrderedObj([("ptr_wrapper",
                            OrderedObj([("valid", Num.of_int(1)), ("data", data)]))])

    # -- entry -------------------------------------------------------------
    def entry_slot(self, entry: model.Entry) -> OrderedObj:
        slot = OrderedObj([("polymorphic_id", Num.of_int(EXACT_PID))])
        kid = self.new_k()
        data = OrderedObj()
        self.emit_ver(("type", "EntryNode"), 0, data)
        data["value0"] = self.nodebase_header(entry.guid, entry.pos)
        data["nextNode_"] = self.node_slot(entry.child)
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot


def write_tree(tree: model.Tree) -> str:
    w = _W()
    root = OrderedObj()
    root["entryNode_"] = w.entry_slot(tree.entry)
    root["parameters_"] = w.params_block(tree.params)
    return dumps(root)


def write_tree_file(path, tree: model.Tree) -> None:
    from .cereal_json import to_file_bytes
    open(path, "wb").write(to_file_bytes(write_tree(tree)))
