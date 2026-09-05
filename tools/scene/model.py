"""In-memory model for ``.scene`` / ``.prefab`` files.

Structure (confirmed against the engine source and real fixtures - see
``docs/Scene.md``):

* A ``.scene`` is a flat, indexed array of root :class:`GameObjectNode`
  (``gameObjectCount`` / ``gameObject_0..N``) - only parentless objects are
  listed; children live inside their parent's :class:`Transform`.
* A ``.prefab`` is a single root :class:`GameObjectNode` of kind
  ``"prefab_root"`` plus a trailing ``copied_object_guids`` list - the engine's
  bookkeeping of which scene instances were copied from this prefab
  (``PrefabGameObject::copiedObjectGuidList_``). Round-tripped losslessly;
  no v1 CLI verb edits it.
* Every :class:`GameObjectNode` (regardless of kind) carries the same fields
  (``isActive_``/``name_``/``guid_``/``components_``/``transform_``) - a prefab's
  *children* are always plain ``"scene"``-kind nodes, exactly like an ordinary
  Scene's; only a prefab file's *root* is ``"prefab_root"``.
* :class:`Transform`'s children are **not** an indexed array - the engine
  writes the same JSON key ``"child"`` once per child, as repeated siblings
  (see ``tools.common.cereal_json`` module docstring). The model exposes them
  as an ordinary ordered Python list; bridging that repeated-key idiom is the
  reader/writer's job.
* A :class:`Component`'s own ``cereal_class_version`` and everything from its
  ``value0`` base-class chain onward is kept as one opaque tagged blob
  (``tools.common.blob.Ptr``/``Ver`` - see that module) rather than being
  unwrapped into separate fields. This is deliberate: a component's immediate
  base is *not* always ``ComponentBase`` directly (e.g. ``Hyena : EnemyBase :
  ComponentBase``), so the base-chain depth varies per component type and
  isn't safe to assume. ``guid``/``is_enabled`` are read/written by walking the
  blob to find ComponentBase's own body (see :func:`find_component_guid` /
  :func:`find_component_enabled` / :func:`set_component_enabled`) - which,
  being the base of every component's base chain, is unambiguous regardless of
  how deep that chain is.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Optional

from tools.common.blob import Ver
from tools.common.cereal_json import OrderedObj

# ---------------------------------------------------------------------------
# fixed structural FQNs / class versions (confirmed against engine source -
# these are hardcoded C++ types, not scanned; see docs/Scene.md)
# ---------------------------------------------------------------------------
FQN_SCENE_GAMEOBJECT = "NanamiEngine::Scene::SceneGameObject"
FQN_PREFAB_GAMEOBJECT = "NanamiEngine::Module::GameObject::PrefabGameObject"
FQN_COPIED_PREFAB_GO = "NanamiEngine::Scene::CopiedPrefabGameObject"
FQN_IGAMEOBJECT = "NanamiEngine::Module::GameObject::IGameObject"
FQN_IOBJECT = "NanamiEngine::Module::Object::IObject"
FQN_GUID = "Guid"
FQN_COMPONENT_GROUP = "NanamiEngine::Module::GameObject::ComponentGroup"
FQN_TRANSFORM = "NanamiEngine::Module::GameObject::Transform"
FQN_COMPONENT_BASE = "NanamiEngine::Module::Component::ComponentBase"

# kind <-> fqn for the three GameObject "root object" types that can appear in
# a gameObject_N / "child" polymorphic slot.
KIND_SCENE = "scene"
KIND_PREFAB_ROOT = "prefab_root"
KIND_COPIED_PREFAB = "copied_prefab"

GAMEOBJECT_FQN_BY_KIND = {
    KIND_SCENE: FQN_SCENE_GAMEOBJECT,
    KIND_PREFAB_ROOT: FQN_PREFAB_GAMEOBJECT,
    KIND_COPIED_PREFAB: FQN_COPIED_PREFAB_GO,
}
GAMEOBJECT_KIND_BY_FQN = {v: k for k, v in GAMEOBJECT_FQN_BY_KIND.items()}

GAMEOBJECT_CLASS_VERSION = {
    FQN_SCENE_GAMEOBJECT: 0,
    FQN_PREFAB_GAMEOBJECT: 1,
    FQN_COPIED_PREFAB_GO: 0,
}


# ---------------------------------------------------------------------------
# small numeric leaves - kept as tools.common.cereal_json.Num (not plain float)
# so an untouched file round-trips its original literal text exactly; only an
# edit synthesises a fresh Num via Num.of_float().
# ---------------------------------------------------------------------------
@dataclass
class Vec3:
    x: Any
    y: Any
    z: Any


@dataclass
class Quat:
    x: Any
    y: Any
    z: Any
    w: Any


@dataclass
class Transform:
    local_pos: Vec3
    local_rot: Quat
    local_scale: Vec3
    world_matrix: Any  # opaque blob (engine-recomputed at load; never edited by the CLI)
    children: list["GameObjectNode"] = field(default_factory=list)


@dataclass
class Component:
    fqn: str
    class_version: int  # always known, regardless of whether *this* occurrence printed it
    data: Any  # tagged blob (Ptr/Ver/plain) of the component's `data` object, ccv stripped


@dataclass
class GameObjectNode:
    kind: str  # "scene" | "prefab_root" | "copied_prefab"
    guid: str
    name: str
    is_active: bool
    components: list[Component]
    transform: Transform


@dataclass
class Scene:
    name: str
    roots: list[GameObjectNode] = field(default_factory=list)


@dataclass
class Prefab:
    root: GameObjectNode
    copied_object_guids: list[str] = field(default_factory=list)


# ---------------------------------------------------------------------------
# ComponentBase accessors - find/mutate guid_/isEnable_ inside the base-class
# blob chain, regardless of how many intermediate bases sit above ComponentBase.
# ---------------------------------------------------------------------------
def _unwrap(node: Any) -> Optional[OrderedObj]:
    """The OrderedObj body of a blob node, whether Ver-wrapped (this occurrence
    printed cereal_class_version) or plain (a later, un-versioned occurrence)."""
    if isinstance(node, Ver):
        return node.body
    if isinstance(node, OrderedObj):
        return node
    return None


def _find_component_base_body(comp: Component) -> Optional[OrderedObj]:
    """Walk nested ``value0`` base-class wrappers to find the body holding
    ComponentBase's own ``guid_``/``isEnable_`` - unambiguous since ComponentBase
    is always the innermost base and always serialises exactly those two keys,
    regardless of how many intermediate bases (``EnemyBase``, ``ColliderBase``,
    ...) sit above it for a given component type.
    """
    node = comp.data.get("value0") if isinstance(comp.data, OrderedObj) else None
    while node is not None:
        body = _unwrap(node)
        if body is None:
            return None
        if "guid_" in body.keys() and "isEnable_" in body.keys():
            return body
        node = body.get("value0")
    return None


def find_component_guid(comp: Component) -> Optional[str]:
    body = _find_component_base_body(comp)
    if body is None:
        return None
    guid_obj = _unwrap(body["guid_"])
    return guid_obj["value_"] if guid_obj is not None else None


def find_component_enabled(comp: Component) -> Optional[bool]:
    body = _find_component_base_body(comp)
    if body is None:
        return None
    return bool(body["isEnable_"])


def set_component_enabled(comp: Component, enabled: bool) -> None:
    body = _find_component_base_body(comp)
    if body is None:
        raise ValueError(f"component {comp.fqn}: could not locate ComponentBase body")
    body["isEnable_"] = bool(enabled)


def set_component_guid(comp: Component, new_guid: str) -> None:
    body = _find_component_base_body(comp)
    if body is None:
        raise ValueError(f"component {comp.fqn}: could not locate ComponentBase body")
    guid_node = _unwrap(body["guid_"])
    if guid_node is None:
        raise ValueError(f"component {comp.fqn}: guid_ has an unexpected shape")
    guid_node["value_"] = new_guid
