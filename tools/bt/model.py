"""In-memory model of an enemy BehaviourTree, decoupled from the cereal-JSON
bookkeeping (polymorphic ids, ptr_wrapper ids, cereal_class_version).

`reader.read_tree` builds a :class:`Tree`; `writer.write_tree` renders one back to
cereal-JSON, regenerating every id / version from scratch.

Action parameters are kept as a *tagged blob* (:mod:`tools.bt.blob`) - a near-raw
copy of the action ``data`` sub-object with pointer slots and versioned slots
marked so the writer can rebuild their bookkeeping. This makes edits to unrelated
fields lossless even for actions the catalog does not fully model.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional, Union

# fully-qualified C++ type names as they appear in `polymorphic_name`.
# The six composite/control node types are shared verbatim between the Enemy
# and FriendlyNpc BehaviourTree flavors; only the ActionNode leaf differs (each
# flavor wraps its own ActionBase hierarchy) - see tools/bt/npc_kind.py.
FQN_ENTRY = "Editor::Npc::Behaviour::EntryNode"
FQN_SELECTOR = "Editor::Npc::Behaviour::SelectorNode"
FQN_SEQUENCE = "Editor::Npc::Behaviour::SequenceNode"
FQN_RANDOM = "Editor::Npc::Behaviour::RandomSelectorNode"
FQN_ONCE_EXEC = "Editor::Npc::Behaviour::OnceExecute"
FQN_ONCE_SUCCESS = "Editor::Npc::Behaviour::OnceSuccessNode"

FQN_ACTION_NODE_ENEMY = "Editor::Npc::Enemy::Behaviour::ActionNode"
FQN_ACTION_NODE_FRIENDLY = "Editor::Npc::Friendly::Behaviour::ActionNode"
FQN_ACTION_NODE = FQN_ACTION_NODE_ENEMY  # backward-compat alias (enemy default)

ACTION_FQN_PREFIX_ENEMY = "GameCore::Npc::Enemy::Behaviour::Action::"
ACTION_FQN_PREFIX_FRIENDLY = "GameCore::Npc::Friendly::Behaviour::Action::"
ACTION_FQN_PREFIX = ACTION_FQN_PREFIX_ENEMY  # backward-compat alias (enemy default)
ACTION_FQN_PREFIXES = (ACTION_FQN_PREFIX_ENEMY, ACTION_FQN_PREFIX_FRIENDLY)

# CEREAL_CLASS_VERSION of the editor node types (verified against source)
NODE_CLASS_VERSION = {
    FQN_ENTRY: 0,
    FQN_SELECTOR: 0,
    FQN_SEQUENCE: 0,
    FQN_RANDOM: 1,
    FQN_ONCE_EXEC: 0,
    FQN_ONCE_SUCCESS: 0,
    FQN_ACTION_NODE_ENEMY: 1,
    FQN_ACTION_NODE_FRIENDLY: 1,
}


@dataclass
class Node:
    """Base for every graph node: a GUID and an editor-canvas position."""

    guid: str
    pos: tuple[float, float] = (0.0, 0.0)


@dataclass
class Entry(Node):
    child: Optional["AnyNode"] = None


@dataclass
class Selector(Node):
    children: list["AnyNode"] = field(default_factory=list)


@dataclass
class Sequence(Node):
    children: list["AnyNode"] = field(default_factory=list)


@dataclass
class RandomSelector(Node):
    children: list["AnyNode"] = field(default_factory=list)
    weights: list[int] = field(default_factory=list)


@dataclass
class OnceExecute(Node):
    child: Optional["AnyNode"] = None
    state: int = 0


@dataclass
class OnceSuccess(Node):
    child: Optional["AnyNode"] = None


@dataclass
class Action(Node):
    """An ActionNode: an editor label (`name`) wrapping one concrete ActionBase."""

    name: str = ""
    type_fqn: str = ""          # GameCore::Npc::Enemy::Behaviour::Action::<X>
    action_version: int = 0     # CEREAL_CLASS_VERSION of the action class
    #: tagged blob of the action's serialised members (blob.Ver / blob.Ptr / ...),
    #: excluding the leading cereal_class_version and the ActionBase `value0` slot.
    params: "object" = None     # blob.Obj

    @property
    def type_name(self) -> str:
        for prefix in ACTION_FQN_PREFIXES:
            if self.type_fqn.startswith(prefix):
                return self.type_fqn[len(prefix):]
        return self.type_fqn.rsplit("::", 1)[-1]


CompositeNode = Union[Selector, Sequence, RandomSelector]
AnyNode = Union[Selector, Sequence, RandomSelector, OnceExecute, OnceSuccess, Action]

CHILDLESS = (Action,)
SINGLE_CHILD = (OnceExecute, OnceSuccess)
MULTI_CHILD = (Selector, Sequence, RandomSelector)


@dataclass
class BbParam:
    """A blackboard parameter. v1 supports the int variant only."""

    name: str
    value: int
    kind: str = "int"


@dataclass
class Tree:
    entry: Entry
    params: list[BbParam] = field(default_factory=list)
    #: which BehaviourTree flavor this file is - "enemy" | "friendly" (see
    #: tools/bt/npc_kind.py). Selects which ActionNode FQN the writer wraps
    #: every Action leaf in; irrelevant (and untested) for any other value.
    kind: str = "enemy"

    # -- navigation helpers ------------------------------------------------
    def walk(self):
        """Yield (node, parent, container, index) depth-first (entry first)."""
        stack: list[tuple[AnyNode, object, object, int]] = []
        if self.entry.child is not None:
            stack.append((self.entry.child, self.entry, self.entry, 0))
        while stack:
            node, parent, container, idx = stack.pop()
            yield node, parent, container, idx
            kids = children_of(node)
            for i in range(len(kids) - 1, -1, -1):
                stack.append((kids[i], node, node, i))

    def find(self, guid: str) -> Optional[AnyNode]:
        if guid in ("entry", self.entry.guid):
            return self.entry
        for node, *_ in self.walk():
            if node.guid == guid:
                return node
        return None


def children_of(node) -> list["AnyNode"]:
    if isinstance(node, MULTI_CHILD):
        return node.children
    if isinstance(node, SINGLE_CHILD):
        return [node.child] if node.child is not None else []
    return []


def set_children(node, kids: list["AnyNode"]) -> None:
    if isinstance(node, MULTI_CHILD):
        node.children = list(kids)
    elif isinstance(node, SINGLE_CHILD):
        node.child = kids[0] if kids else None
    elif isinstance(node, Entry):
        node.child = kids[0] if kids else None
    else:
        raise TypeError(f"{type(node).__name__} cannot hold children")
