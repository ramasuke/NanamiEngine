"""Static checks for a :class:`tools.scene.model.Scene` / :class:`Prefab`.

Mirrors ``tools.bt.validate``'s spirit: problems are reported as plain strings;
a ``note:``-prefixed line is informational (does not block a write), anything
else is a hard failure. Nothing here mutates the model.
"""

from __future__ import annotations

from . import model


def _walk(node: model.GameObjectNode, guids: dict[str, list[str]], path: str) -> None:
    guids.setdefault(node.guid, []).append(f"{path}/{node.name} (GameObject)")
    for comp in node.components:
        cguid = model.find_component_guid(comp)
        if cguid is not None:
            guids.setdefault(cguid, []).append(f"{path}/{node.name}:{comp.fqn}")
    for child in node.transform.children:
        _walk(child, guids, f"{path}/{node.name}")


def validate_scene(scene: model.Scene) -> list[str]:
    problems: list[str] = []
    guids: dict[str, list[str]] = {}
    for root in scene.roots:
        _walk(root, guids, "")
    for guid, owners in guids.items():
        if len(owners) > 1:
            problems.append(f"duplicate GUID {guid} used by: {', '.join(owners)}")
    return problems


def validate_prefab(prefab: model.Prefab) -> list[str]:
    problems: list[str] = []
    guids: dict[str, list[str]] = {}
    _walk(prefab.root, guids, "")
    for guid, owners in guids.items():
        if len(owners) > 1:
            problems.append(f"duplicate GUID {guid} used by: {', '.join(owners)}")
    for guid in prefab.copied_object_guids:
        if guid in guids:
            problems.append(
                f"copiedObjectGuidList_ entry {guid} collides with a GameObject/Component "
                f"GUID inside this same prefab"
            )
    return problems
