"""Load/query ``tools/scene/catalog.json`` (see ``catalog_scan.py``)."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

CATALOG_PATH = Path(__file__).with_name("catalog.json")

SETTABLE_SHAPES = {"int", "float", "bool", "string", "vec2", "vec3", "field"}


class CatalogError(RuntimeError):
    pass


class Catalog:
    def __init__(self, data: dict[str, Any]) -> None:
        self.data = data
        self.components: dict[str, dict] = data.get("components", {})
        self.by_leaf: dict[str, list[str]] = data.get("by_leaf", {})
        self.gameobject_shapes: dict[str, dict] = data.get("gameobject_shapes", {})
        # base leaf -> {"fqn", "header", "version", "empty", ["ambiguous"]}
        # (see catalog_scan._scan_bases); empty for a catalog predating it.
        self.bases: dict[str, dict] = data.get("bases", {})

    def component_by_fqn(self, fqn: str) -> Optional[dict]:
        return self.components.get(fqn)

    def base_info(self, leaf: str) -> Optional[dict]:
        """What the scanner recorded about a component base class (see
        ``catalog_scan._scan_bases``), or ``None`` if it never found its
        definition."""
        return self.bases.get(leaf)

    def resolve_component(self, spec: str) -> dict:
        """Resolve a component ``--type`` argument: an exact FQN, or a bare
        leaf name when it names exactly one FQN. Fails closed (lists every
        candidate) on ambiguity or when nothing matches."""
        if spec in self.components:
            return self.components[spec]
        candidates = self.by_leaf.get(spec, [])
        if len(candidates) == 1:
            return self.components[candidates[0]]
        if len(candidates) > 1:
            raise CatalogError(
                f"{spec!r} is ambiguous - matches: {', '.join(candidates)} "
                f"(pass the full FQN)"
            )
        raise CatalogError(f"unknown component type: {spec!r}")

    def param_by_key(self, entry: Optional[dict], key: str) -> Optional[dict]:
        if entry is None:
            return None
        for p in entry.get("params", []):
            if p["key"] == key:
                return p
        return None


_CACHE: Optional[Catalog] = None


def load(path: Path | None = None) -> Catalog:
    global _CACHE
    if path is None and _CACHE is not None:
        return _CACHE
    p = path or CATALOG_PATH
    data = json.loads(p.read_text(encoding="utf-8")) if p.exists() else {}
    cat = Catalog(data)
    if path is None:
        _CACHE = cat
    return cat
