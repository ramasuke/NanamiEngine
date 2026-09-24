"""cereal オブジェクトのサブツリーをタグ付きで表現する。

リーダーはアクションの ``data`` オブジェクトを blob に変換する。素の値
(``OrderedObj`` / ``list`` / :class:`~tools.bt.cereal_json.Num` / ``str`` /
``bool`` / ``None``) はそのまま通し、cereal の管理用の2つの構造だけをラップする:

* :class:`Ptr`  - ``polymorphic_id`` を持っていたオブジェクト (シリアライズされたポインタ)。
* :class:`Ver`  - 先頭キーが ``cereal_class_version`` だったオブジェクト
                  (シリアライズされたバージョン付き型)。

ライターは blob を走査し、すべての ``polymorphic_id`` / ``ptr_wrapper.id`` /
``cereal_class_version`` をグローバルカウンタから再生成する。そのため blob は
位置に依存せず、ツリーの他の場所での構造編集にも耐える。
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Optional

from .cereal_json import Num, OrderedObj

Obj = OrderedObj  # シグネチャの可読性のための別名


@dataclass
class Ptr:
    """シリアライズされたポインタのスロット (``polymorphic_id`` を持っていた)。"""

    exact: bool                 # polymorphic_id == 0x40000000 (動的型 == 静的型)
    null: bool                  # polymorphic_id == 0
    fqn: str | None             # polymorphic_name / アーカイブの型テーブルから取得
    wrapper: str                # "shared" (ptr_wrapper.id) | "unique" (valid) | ""
    data: Any                   # ptr_wrapper.data の blob。null なら None


@dataclass
class Ver:
    """シリアライズされたバージョン付き型 (先頭キーが ``cereal_class_version``)。"""

    key: tuple                  # 安定した型の識別子。例: ("type", "Guid")
    version: int
    body: Obj                   # 残りのフィールド (blob タグ付き)、元の順序
    literal_presence: Optional[bool] = None
    """Opt out of the writer's global once-per-key ``cereal_class_version``
    tracking for this node and reproduce this *specific* occurrence's presence
    exactly as parsed instead: ``True`` -> always re-emit ``version``, ``False``
    -> never emit it, ``None`` (default) -> the normal global-key behaviour.

    Needed for opaque/unmodeled sub-structures where two genuinely different
    C++ types can serialise identically when empty (e.g. several distinct
    marker-interface bases each contributing a bare, version-0, field-less
    ``archive(cereal::base_class<Mixin>(this))`` as sibling ``valueN`` members) -
    a shared synthetic ("fp", ...) key would wrongly conflate them and suppress
    a version that the source file actually repeats. A consumer that cannot
    resolve real per-member type identity (no catalog) should set this instead
    of relying on the structural-fingerprint key for such nodes.
    """


def fingerprint(node: Any) -> str:
    """管理用の値を無視した、パース済み生ノードの構造シグネチャ。

    同じ C++ 型の2つのシリアライズ済みインスタンスは同じ fingerprint を持つ。
    カタログがモデル化していないバージョン付き構造体の、代替の型識別子として使う。
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
