"""``tools/scene/catalog.json`` の読み込み/照会 (``catalog_scan.py`` を参照)。"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

CATALOG_PATH = Path(__file__).with_name("catalog.json")

SETTABLE_SHAPES = {"int", "float", "bool", "string", "vec2", "vec3", "field", "color32"}


class CatalogError(RuntimeError):
    pass


class Catalog:
    def __init__(self, data: dict[str, Any]) -> None:
        self.data = data
        self.components: dict[str, dict] = data.get("components", {})
        self.by_leaf: dict[str, list[str]] = data.get("by_leaf", {})
        self.gameobject_shapes: dict[str, dict] = data.get("gameobject_shapes", {})
        # 基底の leaf -> {"fqn", "header", "version", "empty", ["ambiguous"]}
        # (catalog_scan._scan_bases を参照)。それ以前のカタログでは空。
        self.bases: dict[str, dict] = data.get("bases", {})

    def component_by_fqn(self, fqn: str) -> Optional[dict]:
        return self.components.get(fqn)

    def base_info(self, leaf: str) -> Optional[dict]:
        """スキャナがコンポーネントの基底クラスについて記録した内容
        (``catalog_scan._scan_bases`` を参照)。定義が見つからなかった場合は ``None``。"""
        return self.bases.get(leaf)

    def resolve_component(self, spec: str) -> dict:
        """コンポーネントの ``--type`` 引数を解決する: 完全な FQN、または FQN を1つだけ
        指す素の leaf 名。曖昧なときや何も一致しないときは安全側に失敗する
        (候補をすべて列挙する)。"""
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
