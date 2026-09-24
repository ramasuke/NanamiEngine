"""``tools/bt/catalog.json`` の読み込みと問い合わせ。これは敵の behaviour Action
（と、action のメンバーとして使われる少数の補助構造体）すべてを機械可読に記述したもので、
:mod:`tools.bt.catalog_scan` が C++ ヘッダーから抽出する。

カタログはコミット対象で再生成可能（``python -m tools.bt regen-catalog``）。
用途: ラウンドトリップ時のバージョンキー解決、``validate``、``show`` と
``set-params`` の型変換、``add-action`` の雛形生成のヒント。
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

from . import npc_kind

CATALOG_PATH = npc_kind.ENEMY.catalog_path  # 後方互換用エイリアス

# `set-params` が CLI 文字列から変換できるパラメータ形状
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

    # -- 検索 ---------------------------------------------------------
    def action_by_name(self, display_name: str) -> Optional[dict]:
        return self.actions.get(display_name)

    def action_by_fqn(self, fqn: str) -> Optional[dict]:
        name = self.by_fqn.get(fqn)
        return self.actions.get(name) if name else None

    def action_by_leaf(self, leaf: str) -> Optional[dict]:
        name = self.by_leaf.get(leaf)
        return self.actions.get(name) if name else None

    def type_by_leaf(self, leaf: str) -> Optional[dict]:
        """C++ のリーフ名で引ける構造体/action のパラメータ記述。"""
        if leaf in self.structs:
            return self.structs[leaf]
        return self.action_by_leaf(leaf)

    def struct_by_fqn(self, fqn: str) -> Optional[tuple[str, dict]]:
        """この多相 fqn で登録された補助構造体の (leaf, entry)。"""
        for leaf, entry in self.structs.items():
            if entry.get("fqn") == fqn:
                return leaf, entry
        return None

    def resolve_action(self, spec: str) -> Optional[dict]:
        """表示名（'Basic::ToPlayerDistance'）、fqn、リーフ名のいずれも受け付ける。"""
        return (
            self.actions.get(spec)
            or self.action_by_fqn(spec)
            or self.action_by_leaf(spec)
        )

    # -- ヘルパー -------------------------------------------------------------
    def params_of(self, entry: Optional[dict]) -> list[dict]:
        return list(entry.get("params", [])) if entry else []

    def param_by_key(self, entry: Optional[dict], json_key: str) -> Optional[dict]:
        for p in self.params_of(entry):
            if p.get("key") == json_key:
                return p
        return None


_cached: dict[str, Catalog] = {}


def load(kind: str = "enemy", path: Path | None = None, *, force: bool = False) -> Catalog:
    global _cached
    if path is None and not force and kind in _cached:
        return _cached[kind]
    p = path or npc_kind.by_name(kind).catalog_path
    if not p.exists():
        cat = Catalog({})
    else:
        cat = Catalog(json.loads(p.read_text(encoding="utf-8")))
    if path is None:
        _cached[kind] = cat
    return cat
