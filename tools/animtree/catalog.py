"""``tools/animtree/catalog.json`` の読み込みと問い合わせ。これは全 ``IAnimationNode``
サブタイプの機械可読な記述（:mod:`tools.animtree.catalog_scan` が C++ ヘッダから抽出）と、
固定の bool/int/float 条件述語・パラメータ種別（手書き。この 2 つをスキャンしない理由は
``catalog_scan.scan`` を参照）から成る。

カタログはコミットされており再生成できる（``python -m tools.animtree
regen-catalog``）。往復時のバージョンキー解決、``validate``、``show``、
``set-node-params`` の型変換に使う。
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

CATALOG_PATH = Path(__file__).with_name("catalog.json")

# `set-node-params` が CLI 文字列から変換できるパラメータ形状
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

    # -- ノード型の検索 -------------------------------------------------
    def node_by_fqn(self, fqn: str) -> Optional[dict]:
        return self.node_types.get(fqn)

    def node_by_leaf(self, leaf: str) -> Optional[dict]:
        fqn = self.by_leaf.get(leaf)
        return self.node_types.get(fqn) if fqn else None

    def resolve_node_type(self, spec: str) -> Optional[dict]:
        """fqn または修飾なしのクラス名（例: ``AnimationClipNode``）を受け付ける。"""
        return self.node_types.get(spec) or self.node_by_leaf(spec)

    def addable_node_types(self) -> dict[str, dict]:
        """``add-clip-node`` 系のコマンドで追加できるノード型 -
        固定のシングルトン 2 つ（Entry/AnyState）は除く。"""
        return {fqn: e for fqn, e in self.node_types.items() if not e.get("singleton")}

    def params_of(self, entry: Optional[dict]) -> list[dict]:
        return list(entry.get("params", [])) if entry else []

    def params_for_version(self, entry: Optional[dict], class_version: int) -> list[dict]:
        """``class_version`` で保存されたノードが実際に持つパラメータ。エンジンの
        ``load()`` が ``if (version >= N)`` で読むメンバー（カタログの ``since``）は
        古い blob には無く cereal も読み飛ばすので、必須にしてはならない。"""
        return [p for p in self.params_of(entry) if int(p.get("since", 0)) <= class_version]

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

    # -- 条件 / パラメータ種別の検索 -------------------------------------
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
