"""In-memory model of an AnimationTree, decoupled from the cereal-JSON
bookkeeping (polymorphic ids, ptr_wrapper ids, cereal_class_version).

``reader.read_tree`` builds a :class:`Tree`; ``writer.write_tree`` renders one
back to cereal-JSON, regenerating every id / version from scratch.

Unlike an enemy BehaviourTree, an AnimationTree is a **graph**, not a tree:
nodes reference each other only through :class:`Transition` endpoints (by
GUID, never by cereal pointer identity), and there is no parent/child
relation. Every node - including the two fixed singletons (``Entry`` and
``AnyState``) - is described by the node-type catalog
(:mod:`tools.animtree.catalog`) and stored as a generic :class:`Node` whose
non-guid/non-position fields are kept as a *tagged blob*
(:mod:`tools.common.blob`), the same way :class:`tools.bt.model.Action` wraps
a catalog-described action. This keeps the format open to a future
``IAnimationNode`` subclass without a model rewrite - only the catalog needs
to grow.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Optional

from tools.common.cereal_json import Num


def numval(x: Any) -> Any:
    """Unwrap a raw :class:`~tools.common.cereal_json.Num` (as kept by
    ``Node.pos``, ``Transition.duration_secs`` and int/float
    ``Condition``/``Param`` values, to preserve their original numeric literal
    text through a no-op round trip) to a plain Python number. Pass-through
    for anything else (bool, already-plain float/int on a freshly-created
    value)."""
    return x.value if isinstance(x, Num) else x

# fully-qualified C++ type names, as they appear in polymorphic_name (or, for
# the two exact-typed singleton slots, never printed at all - see reader.py)
FQN_ENTRY_NODE = "NanamiEngine::Module::AnimationTree::AnimatorEntryNode"
FQN_ANYSTATE_NODE = "NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode"
FQN_CLIP_NODE = "NanamiEngine::Module::AnimationTree::AnimationClipNode"
FQN_NODE_PATH = "NanamiEngine::Module::AnimationTree::AnimationNodePath"

# CEREAL_CLASS_VERSION of every *structural* type (verified against source).
# tools/animtree/catalog.json is the source of truth for the addable node
# types themselves (AnimationClipNode, ...); these are the fixed constants for
# types this toolkit always emits by hand (transitions, condition groups,
# base-class stubs) - exactly how tools/bt hardcodes its own NODE_CLASS_VERSION.
NODE_PATH_CLASS_VERSION = 1
COND_GROUP_CLASS_VERSION = 0
COND_CLASS_VERSION = 0
PARAM_CLASS_VERSION = 0
IANIMATIONNODE_CLASS_VERSION = 0
IOBJECT_CLASS_VERSION = 0
ICONDITION_CLASS_VERSION = 0
IPARAMETER_CLASS_VERSION = 0

# the 3 explicit instantiations that exist today for both AnimationParameter<T>
# and AnimationNodePathAdditionCondition<T>
KINDS = ("bool", "int", "float")


@dataclass
class Node:
    """One ``IAnimationNode`` instance: a singleton (Entry/AnyState) or an
    addable node (``AnimationClipNode`` today - see
    :meth:`tools.animtree.catalog.Catalog.addable_node_types`). ``guid``/``pos``
    are hoisted out of the type's field blob for uniform addressing across the
    toolkit (transitions, validate, the CLI); the writer re-inserts them at
    the catalog-declared position in the type's own field order.
    """

    guid: str
    pos: tuple[float, float] = (0.0, 0.0)
    type_fqn: str = FQN_CLIP_NODE
    class_version: int = 0
    #: tagged blob (blob.Ver / blob.Ptr / ...) of every field *except*
    #: guid_/position_, in the type's save()-order (e.g. animationFile_,
    #: name_, speed_, blendAnimationOffset_secs_, modelAnimationIndex_ for a
    #: ClipNode; empty for AnyState; {speed_} for Entry).
    params: Any = None

    @property
    def is_singleton(self) -> bool:
        return self.type_fqn in (FQN_ENTRY_NODE, FQN_ANYSTATE_NODE)


@dataclass
class Condition:
    """One equality guard inside a transition's AND-group. ``Check()`` in the
    engine is *always* equality - there is no ``<``/``>``/``!=`` operator."""

    name: str            # additionParameters_ param name this checks
    kind: str             # "bool" | "int" | "float"
    value: Any


@dataclass
class Transition:
    """One ``AnimationNodePath``. Has **no identity GUID of its own** -
    ``AnimationNodePath::GetGuid()`` is a stub bug that always returns an
    empty GUID - so a transition is addressed positionally (its index in
    ``transitions``/``any_state_transitions``) or by its
    ``(from_guid, next_guid)`` pair, never by a guid argument."""

    from_guid: str
    next_guid: str
    visual_from_guid: str          # editor-only "drawn from" node; == from_guid for a freshly-authored transition
    duration_secs: float = 0.0
    conditions: list[Condition] = field(default_factory=list)   # AND-group; [] == unconditional/"always"


@dataclass
class Param:
    """One ``additionParameters_`` entry (``ParameterGroup.conditionParameters_``).
    Unlike ``tools.bt``'s blackboard (self-restricted to ``int`` as a stated v1
    limitation, not an engine constraint), this supports bool/int/float from
    day one - ``AnimationParameter<T>`` is identically instantiated for all
    three in the engine."""

    name: str
    kind: str             # "bool" | "int" | "float"
    value: Any


@dataclass
class Tree:
    entry: Node                        # type_fqn == FQN_ENTRY_NODE, from "entryNode"
    any_state: Node                     # type_fqn == FQN_ANYSTATE_NODE, from "visualAnyStateNode"
    nodes: list[Node] = field(default_factory=list)                         # "nodes_N"
    transitions: list[Transition] = field(default_factory=list)             # "fromNodeNodePath_N"
    any_state_transitions: list[Transition] = field(default_factory=list)   # "fromAnyStateNodeNodePath_N"
    params: list[Param] = field(default_factory=list)                       # "additionParameters_"

    def find_node(self, guid: str) -> Optional[Node]:
        if self.entry.guid == guid:
            return self.entry
        if self.any_state.guid == guid:
            return self.any_state
        return next((n for n in self.nodes if n.guid == guid), None)

    def all_node_guids(self) -> set[str]:
        return {self.entry.guid, self.any_state.guid, *(n.guid for n in self.nodes)}

    def find_param(self, name: str) -> Optional[Param]:
        return next((p for p in self.params if p.name == name), None)
