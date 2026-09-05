"""Load and query ``tools/animtree/catalog.json`` - the machine-readable
description of every ``IAnimationNode`` subtype (scraped from the C++ headers
by :mod:`tools.animtree.catalog_scan`), plus the fixed bool/int/float
condition-predicate and parameter kinds (hand-written - see
``catalog_scan.scan`` for why those two are not scanned).

The catalog is committed and regenerable (``python -m tools.animtree
regen-catalog``). It drives: version-key resolution during round-trip,
``validate``, ``show`` and ``set-node-params`` coercion.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

CATALOG_PATH = Path(__file__).with_name("catalog.json")

# param shapes that `set-node-params` can coerce a CLI string into
SETTABLE_SHAPES = {"int", "float", "bool", "string", "field"}


class CatalogError(RuntimeError):
    pass


class Catalog:
    def __init__(self, data: dict[str, Any]) -> None:
        self.data = data
        self.node_types: dict[str, dict] = data.get("node_types", {})
        self.by_leaf: dict[str, str] = data.get("by_leaf", {})
        self.conditions: dict[str, dict] = data.get("conditions", {})
        self.params: dict[str, dict] = data.get("params", {})
        self.generated_from: str = data.get("generated_from", "")

    # -- node-type lookups -------------------------------------------------
    def node_by_fqn(self, fqn: str) -> Optional[dict]:
        return self.node_types.get(fqn)

    def node_by_leaf(self, leaf: str) -> Optional[dict]:
        fqn = self.by_leaf.get(leaf)
        return self.node_types.get(fqn) if fqn else None

    def resolve_node_type(self, spec: str) -> Optional[dict]:
        """Accept an fqn or a bare leaf class name (e.g. ``AnimationClipNode``)."""
        return self.node_types.get(spec) or self.node_by_leaf(spec)

    def addable_node_types(self) -> dict[str, dict]:
        """Node types a user can add via ``add-clip-node``-style verbs -
        excludes the two fixed singletons (Entry/AnyState)."""
        return {fqn: e for fqn, e in self.node_types.items() if not e.get("singleton")}

    def params_of(self, entry: Optional[dict]) -> list[dict]:
        return list(entry.get("params", [])) if entry else []

    def param_by_key(self, entry: Optional[dict], json_key: str) -> Optional[dict]:
        for p in self.params_of(entry):
            if p.get("key") == json_key:
                return p
        return None

    def guid_key(self, entry: Optional[dict]) -> Optional[str]:
        for p in self.params_of(entry):
            if p.get("shape") == "self_guid":
                return p["key"]
        return None

    def pos_key(self, entry: Optional[dict]) -> Optional[str]:
        for p in self.params_of(entry):
            if p.get("shape") == "self_pos":
                return p["key"]
        return None

    # -- condition / param kind lookups -------------------------------------
    def condition_fqn(self, kind: str) -> str:
        e = self.conditions.get(kind)
        if e is None:
            raise CatalogError(f"unknown condition kind: {kind!r} (want one of {list(self.conditions)})")
        return e["fqn"]

    def condition_version(self, kind: str) -> int:
        return int(self.conditions.get(kind, {}).get("version", 0))

    def kind_by_condition_fqn(self, fqn: str) -> Optional[str]:
        for k, e in self.conditions.items():
            if e["fqn"] == fqn:
                return k
        return None

    def param_fqn(self, kind: str) -> str:
        e = self.params.get(kind)
        if e is None:
            raise CatalogError(f"unknown parameter kind: {kind!r} (want one of {list(self.params)})")
        return e["fqn"]

    def param_version(self, kind: str) -> int:
        return int(self.params.get(kind, {}).get("version", 0))

    def kind_by_param_fqn(self, fqn: str) -> Optional[str]:
        for k, e in self.params.items():
            if e["fqn"] == fqn:
                return k
        return None


_cached: Optional[Catalog] = None


def load(path: Path | None = None, *, force: bool = False) -> Catalog:
    global _cached
    if _cached is not None and not force and path is None:
        return _cached
    p = path or CATALOG_PATH
    if not p.exists():
        cat = Catalog({})
    else:
        cat = Catalog(json.loads(p.read_text(encoding="utf-8")))
    if path is None:
        _cached = cat
    return cat
