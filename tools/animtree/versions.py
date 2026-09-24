"""ツリー内の同じ型のノードをすべて単一の cereal クラスバージョンに揃える。

cereal は型の ``cereal_class_version`` をアーカイブごとに 1 回（最初に現れたとき）しか
書かないので、同じ型の古いノードと新しいノードは 1 つの ``.animTree`` に共存できない。
エンジンは新しいノードを古いバージョンで読み込み、新しいメンバーを黙って読み飛ばしてしまう。
バージョンが混在する場合は、古いノードを存在する最新バージョンに引き上げ、欠けている
メンバーをエンジンのクラス内初期化子（カタログの ``default``）で埋める。
エディタでツリーを保存し直したのと同じ結果になる。
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
    """その場で引き上げ、引き上げたノードの guid を返す。"""
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
