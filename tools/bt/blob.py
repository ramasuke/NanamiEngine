"""Tagged representation of a cereal object sub-tree.

The reader converts an action's ``data`` object into a blob: plain values pass
through (``OrderedObj`` / ``list`` / :class:`~tools.bt.cereal_json.Num` / ``str`` /
``bool`` / ``None``), while the two cereal bookkeeping constructs are wrapped:

* :class:`Ptr`  - an object that carried ``polymorphic_id`` (a serialised pointer).
* :class:`Ver`  - an object whose first key was ``cereal_class_version``
                  (a serialised versioned type).

The writer walks the blob and regenerates every ``polymorphic_id`` /
``ptr_wrapper.id`` / ``cereal_class_version`` from its global counters, so blobs
are position-independent and survive structural edits elsewhere in the tree.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from .cereal_json import Num, OrderedObj

Obj = OrderedObj  # alias for readability in signatures


@dataclass
class Ptr:
    """A serialised pointer slot (had ``polymorphic_id``)."""

    exact: bool                 # polymorphic_id == 0x40000000 (dynamic == static)
    null: bool                  # polymorphic_id == 0
    fqn: str | None             # from polymorphic_name / the archive type table
    wrapper: str                # "shared" (ptr_wrapper.id) | "unique" (valid) | ""
    data: Any                   # blob of ptr_wrapper.data, or None when null


@dataclass
class Ver:
    """A serialised versioned type (first key was ``cereal_class_version``)."""

    key: tuple                  # stable type identity, e.g. ("type", "Guid")
    version: int
    body: Obj                   # remaining fields (blob-tagged), original order


def fingerprint(node: Any) -> str:
    """Structural signature of a raw parsed node, ignoring bookkeeping values.

    Two serialised instances of the same C++ type share a fingerprint; it is the
    fallback type identity for versioned structs the catalog does not model.
    """
    if isinstance(node, OrderedObj):
        parts = []
        for k, v in node.items():
            if k == "cereal_class_version":
                continue
            if k == "polymorphic_name":
                continue
            if k == "polymorphic_id":
                parts.append("@pid")
                continue
            if k in ("id", "valid"):
                parts.append("@" + k)
                continue
            parts.append(k + ":" + fingerprint(v))
        return "{" + ",".join(parts) + "}"
    if isinstance(node, list):
        head = fingerprint(node[0]) if node else ""
        return "[" + head + ("..." if len(node) > 1 else "") + "]"
    if isinstance(node, bool):
        return "b"
    if isinstance(node, Num):
        return "i" if node.is_int else "f"
    if isinstance(node, str):
        return "s"
    if node is None:
        return "n"
    return "?"
