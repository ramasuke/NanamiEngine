"""敵 BehaviourTree のメモリ上のモデル。cereal-JSON の管理情報
（polymorphic id、ptr_wrapper id、cereal_class_version）から切り離されている。

`reader.read_tree` が :class:`Tree` を構築し、`writer.write_tree` がそれを
cereal-JSON に書き戻す。id / version はすべて一から生成し直す。

Action のパラメータは *タグ付き blob*（:mod:`tools.bt.blob`）として保持する。
action の ``data`` サブオブジェクトをほぼそのまま写したもので、ポインタスロットと
バージョン付きスロットに印を付けて writer が管理情報を再構築できるようにしている。
これにより、カタログが完全にはモデル化していない action でも、無関係なフィールドの
編集が無損失になる。
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional, Union

# `polymorphic_name` に現れる完全修飾 C++ 型名。
# 6つの複合/制御ノード型は Enemy と FriendlyNpc の BehaviourTree 種別で
# そのまま共有され、ActionNode リーフだけが異なる（各種別が独自の ActionBase
# 階層をラップする）。tools/bt/npc_kind.py 参照。
FQN_ENTRY = "Editor::Npc::Behaviour::EntryNode"
FQN_SELECTOR = "Editor::Npc::Behaviour::SelectorNode"
FQN_SEQUENCE = "Editor::Npc::Behaviour::SequenceNode"
FQN_RANDOM = "Editor::Npc::Behaviour::RandomSelectorNode"
FQN_ONCE_EXEC = "Editor::Npc::Behaviour::OnceExecute"
FQN_ONCE_SUCCESS = "Editor::Npc::Behaviour::OnceSuccessNode"

FQN_ACTION_NODE_ENEMY = "Editor::Npc::Enemy::Behaviour::ActionNode"
FQN_ACTION_NODE_FRIENDLY = "Editor::Npc::Friendly::Behaviour::ActionNode"
FQN_ACTION_NODE = FQN_ACTION_NODE_ENEMY  # 後方互換用エイリアス（enemy がデフォルト）

ACTION_FQN_PREFIX_ENEMY = "GameCore::Npc::Enemy::Behaviour::Action::"
ACTION_FQN_PREFIX_FRIENDLY = "GameCore::Npc::Friendly::Behaviour::Action::"
ACTION_FQN_PREFIX = ACTION_FQN_PREFIX_ENEMY  # 後方互換用エイリアス（enemy がデフォルト）
ACTION_FQN_PREFIXES = (ACTION_FQN_PREFIX_ENEMY, ACTION_FQN_PREFIX_FRIENDLY)

# エディタのノード型の CEREAL_CLASS_VERSION（ソースと照合済み）
NODE_CLASS_VERSION = {
    FQN_ENTRY: 0,
    FQN_SELECTOR: 0,
    FQN_SEQUENCE: 0,
    FQN_RANDOM: 1,
    FQN_ONCE_EXEC: 0,
    FQN_ONCE_SUCCESS: 0,
    FQN_ACTION_NODE_ENEMY: 1,
    FQN_ACTION_NODE_FRIENDLY: 1,
}


@dataclass
class Node:
    """全グラフノードの基底: GUID とエディタキャンバス上の位置。"""

    guid: str
    pos: tuple[float, float] = (0.0, 0.0)


@dataclass
class Entry(Node):
    child: Optional["AnyNode"] = None


@dataclass
class Selector(Node):
    children: list["AnyNode"] = field(default_factory=list)


@dataclass
class Sequence(Node):
    children: list["AnyNode"] = field(default_factory=list)


@dataclass
class RandomSelector(Node):
    children: list["AnyNode"] = field(default_factory=list)
    weights: list[int] = field(default_factory=list)


@dataclass
class OnceExecute(Node):
    child: Optional["AnyNode"] = None
    state: int = 0


@dataclass
class OnceSuccess(Node):
    child: Optional["AnyNode"] = None


@dataclass
class Action(Node):
    """ActionNode: 具象 ActionBase を1つラップするエディタ上のラベル（`name`）。"""

    name: str = ""
    type_fqn: str = ""          # GameCore::Npc::Enemy::Behaviour::Action::<X>
    action_version: int = 0     # action クラスの CEREAL_CLASS_VERSION
    #: action のシリアライズ済みメンバーのタグ付き blob（blob.Ver / blob.Ptr / ...）。
    #: 先頭の cereal_class_version と ActionBase の `value0` スロットは除く。
    params: "object" = None     # blob.Obj

    @property
    def type_name(self) -> str:
        for prefix in ACTION_FQN_PREFIXES:
            if self.type_fqn.startswith(prefix):
                return self.type_fqn[len(prefix):]
        return self.type_fqn.rsplit("::", 1)[-1]


CompositeNode = Union[Selector, Sequence, RandomSelector]
AnyNode = Union[Selector, Sequence, RandomSelector, OnceExecute, OnceSuccess, Action]

CHILDLESS = (Action,)
SINGLE_CHILD = (OnceExecute, OnceSuccess)
MULTI_CHILD = (Selector, Sequence, RandomSelector)


@dataclass
class BbParam:
    """ブラックボードのパラメータ。v1 は int 型のみ対応。"""

    name: str
    value: int
    kind: str = "int"


@dataclass
class Tree:
    entry: Entry
    params: list[BbParam] = field(default_factory=list)
    #: このファイルの BehaviourTree 種別 - "enemy" | "friendly"
    #: （tools/bt/npc_kind.py 参照）。writer が各 Action リーフをどの ActionNode FQN で
    #: ラップするかを決める。それ以外の値は無関係（かつ未テスト）。
    kind: str = "enemy"

    # -- 探索ヘルパー ------------------------------------------------
    def walk(self):
        """(node, parent, container, index) を深さ優先で返す（エントリーが先）。"""
        stack: list[tuple[AnyNode, object, object, int]] = []
        if self.entry.child is not None:
            stack.append((self.entry.child, self.entry, self.entry, 0))
        while stack:
            node, parent, container, idx = stack.pop()
            yield node, parent, container, idx
            kids = children_of(node)
            for i in range(len(kids) - 1, -1, -1):
                stack.append((kids[i], node, node, i))

    def find(self, guid: str) -> Optional[AnyNode]:
        if guid in ("entry", self.entry.guid):
            return self.entry
        for node, *_ in self.walk():
            if node.guid == guid:
                return node
        return None


def children_of(node) -> list["AnyNode"]:
    if isinstance(node, MULTI_CHILD):
        return node.children
    if isinstance(node, SINGLE_CHILD):
        return [node.child] if node.child is not None else []
    return []


def set_children(node, kids: list["AnyNode"]) -> None:
    if isinstance(node, MULTI_CHILD):
        node.children = list(kids)
    elif isinstance(node, SINGLE_CHILD):
        node.child = kids[0] if kids else None
    elif isinstance(node, Entry):
        node.child = kids[0] if kids else None
    else:
        raise TypeError(f"{type(node).__name__} cannot hold children")
