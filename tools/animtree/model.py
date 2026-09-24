"""AnimationTree のメモリ上のモデル。cereal-JSON の管理情報
（polymorphic id、ptr_wrapper id、cereal_class_version）から切り離してある。

``reader.read_tree`` が :class:`Tree` を構築し、``writer.write_tree`` がそれを
cereal-JSON に書き戻す（id / バージョンはすべて一から振り直す）。

敵の BehaviourTree と違い AnimationTree は木ではなく **グラフ** である:
ノード同士は :class:`Transition` の端点を通してのみ参照し合い（GUID による。
cereal のポインタ同一性は使わない）、親子関係は無い。すべてのノード
（固定シングルトン ``Entry`` と ``AnyState`` を含む）はノード型カタログ
（:mod:`tools.animtree.catalog`）で記述され、汎用の :class:`Node` として保持される。
guid/position 以外のフィールドは *タグ付き blob*（:mod:`tools.common.blob`）で持ち、
:class:`tools.bt.model.Action` がカタログ記述のアクションを包むのと同じ方式。
これにより今後 ``IAnimationNode`` サブクラスが増えてもモデルを書き直す必要がなく、
カタログを増やすだけで済む。
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Optional

from tools.common.cereal_json import Num


def numval(x: Any) -> Any:
    """生の :class:`~tools.common.cereal_json.Num`（``Node.pos``、``Transition.duration_secs``、
    int/float の ``Condition``/``Param`` の値が、無変更の往復で元の数値リテラル
    表記を保つために保持しているもの）を素の Python 数値に戻す。
    それ以外（bool、新規作成した値の素の float/int）はそのまま返す。"""
    return x.value if isinstance(x, Num) else x

# polymorphic_name に現れる完全修飾 C++ 型名（型が固定の
# シングルトンスロット 2 つでは一切出力されない - reader.py 参照）
FQN_ENTRY_NODE = "NanamiEngine::Module::AnimationTree::AnimatorEntryNode"
FQN_ANYSTATE_NODE = "NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode"
FQN_CLIP_NODE = "NanamiEngine::Module::AnimationTree::AnimationClipNode"
FQN_NODE_PATH = "NanamiEngine::Module::AnimationTree::AnimationNodePath"

# 全 *構造* 型の CEREAL_CLASS_VERSION（ソースで確認済み）。
# 追加可能なノード型そのもの（AnimationClipNode, ...）は tools/animtree/catalog.json が
# 正本。ここはこのツールキットが常に手で出力する型（遷移、条件グループ、
# 基底クラスのスタブ）の固定定数で、tools/bt が NODE_CLASS_VERSION を
# ハードコードしているのと同じ。
NODE_PATH_CLASS_VERSION = 1
COND_GROUP_CLASS_VERSION = 0
COND_CLASS_VERSION = 0
PARAM_CLASS_VERSION = 0
IANIMATIONNODE_CLASS_VERSION = 0
IOBJECT_CLASS_VERSION = 0
ICONDITION_CLASS_VERSION = 0
IPARAMETER_CLASS_VERSION = 0

# AnimationParameter<T> と AnimationNodePathAdditionCondition<T> の両方について
# 現在存在する 3 つの明示的実体化
KINDS = ("bool", "int", "float")


@dataclass
class Node:
    """1 つの ``IAnimationNode`` インスタンス: シングルトン（Entry/AnyState）か
    追加可能なノード（現状は ``AnimationClipNode`` -
    :meth:`tools.animtree.catalog.Catalog.addable_node_types` 参照）。``guid``/``pos``
    はツールキット全体（遷移、validate、CLI）で一様に指定できるよう型のフィールド
    blob から取り出してあり、writer が型自身のフィールド順のカタログ宣言位置に
    戻す。
    """

    guid: str
    pos: tuple[float, float] = (0.0, 0.0)
    type_fqn: str = FQN_CLIP_NODE
    class_version: int = 0
    #: guid_/position_ *以外* の全フィールドのタグ付き blob（blob.Ver / blob.Ptr / ...）を
    #: 型の save() 順で持つ（例: ClipNode なら animationFile_, name_, speed_,
    #: blendAnimationOffset_secs_, modelAnimationIndex_。AnyState は空、
    #: Entry は {speed_}）。
    params: Any = None

    @property
    def is_singleton(self) -> bool:
        return self.type_fqn in (FQN_ENTRY_NODE, FQN_ANYSTATE_NODE)


@dataclass
class Condition:
    """遷移の AND グループ内の等値ガード 1 つ。エンジンの ``Check()`` は
    *常に* 等値比較で、``<``/``>``/``!=`` 演算子は無い。"""

    name: str            # 判定対象の additionParameters_ のパラメータ名
    kind: str             # "bool" | "int" | "float"
    value: Any


@dataclass
class Transition:
    """1 つの ``AnimationNodePath``。**自身の識別 GUID を持たない**
    （``AnimationNodePath::GetGuid()`` は常に空の GUID を返すスタブのバグ）ので、
    遷移は位置（``transitions``/``any_state_transitions`` 内のインデックス）か
    ``(from_guid, next_guid)`` の組で指定し、guid 引数では指定しない。"""

    from_guid: str
    next_guid: str
    visual_from_guid: str          # エディタ専用の「描画元」ノード。新規作成した遷移では == from_guid
    duration_secs: float = 0.0
    conditions: list[Condition] = field(default_factory=list)   # AND グループ。[] == 無条件/「常に」


@dataclass
class Param:
    """``additionParameters_`` の 1 項目（``ParameterGroup.conditionParameters_``）。
    ``tools.bt`` のブラックボード（エンジンの制約ではなく v1 の制限として ``int`` のみ）と
    違い、最初から bool/int/float に対応する。エンジンでは ``AnimationParameter<T>`` が
    3 つとも同じように実体化されている。"""

    name: str
    kind: str             # "bool" | "int" | "float"
    value: Any


@dataclass
class Tree:
    entry: Node                        # type_fqn == FQN_ENTRY_NODE、"entryNode" から
    any_state: Node                     # type_fqn == FQN_ANYSTATE_NODE、"visualAnyStateNode" から
    nodes: list[Node] = field(default_factory=list)                         # "nodes_N"
    transitions: list[Transition] = field(default_factory=list)             # "fromNodeNodePath_N"
    any_state_transitions: list[Transition] = field(default_factory=list)   # "fromAnyStateNodeNodePath_N"
    params: list[Param] = field(default_factory=list)                       # "additionParameters_"

    def find_node(self, guid: str) -> Optional[Node]:
        if self.entry.guid == guid:
            return self.entry
        if self.any_state.guid == guid:
            return self.any_state
        return next((n for n in self.nodes if n.guid == guid), None)

    def all_node_guids(self) -> set[str]:
        return {self.entry.guid, self.any_state.guid, *(n.guid for n in self.nodes)}

    def find_param(self, name: str) -> Optional[Param]:
        return next((p for p in self.params if p.name == name), None)
