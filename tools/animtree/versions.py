"""Keep every node of one type in a tree at a single cereal class version.

cereal writes a type's ``cereal_class_version`` only once per archive (on its first
occurrence), so an older and a newer node of the same type cannot coexist in one
``.animTree``: the engine would load the newer node at the older version and silently
skip its newer members. When versions are mixed, the older nodes are upgraded to the
newest version present, filling the members they lack with the engine's in-class
initializer (catalog ``default``) - the same result as re-saving the tree in the editor.
"""

from __future__ import annotations

from typing import Any

from tools.common.cereal_json import Num

from . import catalog as catalog_mod
from . import model


class VersionError(ValueError):
    pass


def _scalar(shape: str, value: Any) -> Any:
    if shape == "float":
        return Num.of_float(float(value))
    if shape == "int":
        return Num.of_int(int(value))
    if shape == "bool":
        return bool(value)
    raise VersionError(f"cannot synthesise a default for a param of shape {shape!r}")


def unify_node_versions(tree: model.Tree, cat: catalog_mod.Catalog) -> list[str]:
    """Upgrade in place; returns the guids of the nodes that were upgraded."""
    newest: dict[str, int] = {}
    for node in tree.nodes:
        newest[node.type_fqn] = max(newest.get(node.type_fqn, node.class_version), node.class_version)

    upgraded: list[str] = []
    for node in tree.nodes:
        target = newest[node.type_fqn]
        if node.class_version == target:
            continue
        entry = cat.node_by_fqn(node.type_fqn)
        if entry is None:
            raise VersionError(f"unknown node type: {node.type_fqn!r} (regen-catalog?)")
        for pinfo in cat.params_for_version(entry, target):
            if int(pinfo.get("since", 0)) <= node.class_version:
                continue
            if "default" not in pinfo:
                raise VersionError(
                    f"{node.type_fqn}.{pinfo['member']}: node {node.guid} (version "
                    f"{node.class_version}) must be upgraded to version {target}, but the catalog "
                    f"has no in-class default for this member"
                )
            node.params.append(pinfo["key"], _scalar(pinfo["shape"], pinfo["default"]))
        node.class_version = target
        upgraded.append(node.guid)
    return upgraded
