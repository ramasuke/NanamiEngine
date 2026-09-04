"""Load and query ``tools/bt/catalog.json`` - the machine-readable description of
every enemy behaviour Action (and the handful of helper structs used as action
members), scraped from the C++ headers by :mod:`tools.bt.catalog_scan`.

The catalog is committed and regenerable (``python -m tools.bt regen-catalog``).
It drives: version-key resolution during round-trip, ``validate``, ``show`` and
``set-params`` coercion, and ``add-action`` scaffolding hints.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

CATALOG_PATH = Path(__file__).with_name("catalog.json")

# param shapes that `set-params` can coerce a CLI string into
SETTABLE_SHAPES = {"int", "float", "bool", "string", "enum", "vec2", "vec3", "field"}


class Catalog:
    def __init__(self, data: dict[str, Any]) -> None:
        self.data = data
        self.actions: dict[str, dict] = data.get("actions", {})
        self.structs: dict[str, dict] = data.get("structs", {})
        self.by_fqn: dict[str, str] = data.get("by_fqn", {})
        self.by_leaf: dict[str, str] = data.get("by_leaf", {})
        self.nodes: dict[str, dict] = data.get("nodes", {})
        self.generated_from: str = data.get("generated_from", "")

    # -- lookups ---------------------------------------------------------
    def action_by_name(self, display_name: str) -> Optional[dict]:
        return self.actions.get(display_name)

    def action_by_fqn(self, fqn: str) -> Optional[dict]:
        name = self.by_fqn.get(fqn)
        return self.actions.get(name) if name else None

    def action_by_leaf(self, leaf: str) -> Optional[dict]:
        name = self.by_leaf.get(leaf)
        return self.actions.get(name) if name else None

    def type_by_leaf(self, leaf: str) -> Optional[dict]:
        """A struct/action param description addressable by its C++ leaf name."""
        if leaf in self.structs:
            return self.structs[leaf]
        return self.action_by_leaf(leaf)

    def resolve_action(self, spec: str) -> Optional[dict]:
        """Accept a display name ('Basic::ToPlayerDistance'), an fqn, or a leaf."""
        return (
            self.actions.get(spec)
            or self.action_by_fqn(spec)
            or self.action_by_leaf(spec)
        )

    # -- helpers -------------------------------------------------------------
    def params_of(self, entry: Optional[dict]) -> list[dict]:
        return list(entry.get("params", [])) if entry else []

    def param_by_key(self, entry: Optional[dict], json_key: str) -> Optional[dict]:
        for p in self.params_of(entry):
            if p.get("key") == json_key:
                return p
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
