"""``.scene`` / ``.prefab`` ファイルのインメモリモデル。

構造（エンジンのソースと実際のフィクスチャで確認済み。``docs/Scene.md`` 参照）:

* ``.scene`` はルート :class:`GameObjectNode` のフラットな添字付き配列
  （``gameObjectCount`` / ``gameObject_0..N``）。並ぶのは親を持たないオブジェクトだけで、
  子は親の :class:`Transform` の中にある。
* ``.prefab`` は種別 ``"prefab_root"`` の単一ルート :class:`GameObjectNode` と、
  末尾の ``copied_object_guids`` リストからなる。後者はどのシーンインスタンスがこの
  プレハブからコピーされたかというエンジン側の管理情報
  （``PrefabGameObject::copiedObjectGuidList_``）。欠落なく往復するが、
  v1 の CLI 動詞で編集するものはない。
* どの :class:`GameObjectNode` も（種別に関係なく）同じフィールド
  （``isActive_``/``name_``/``guid_``/``components_``/``transform_`` と、省略可能な末尾の
  ``mark_``。:data:`GAMEOBJECT_CLASS_VERSION_WITHOUT_MARK` 参照）を持つ。プレハブの
  *子* は常に普通のシーンと同じ ``"scene"`` 種別のノードで、``"prefab_root"`` なのは
  プレハブファイルの *ルート* だけ。
* :class:`Transform` の子は添字付き配列 **ではない**。エンジンは子ごとに同じ JSON キー
  ``"child"`` を 1 回ずつ、兄弟として繰り返し書く（``tools.common.cereal_json`` の
  モジュール docstring 参照）。モデルでは普通の順序付き Python リストとして公開し、
  重複キーの慣習との橋渡しは reader/writer の仕事。
* :class:`Component` 自身の ``cereal_class_version`` と、その ``value0`` 基底クラス
  チェーン以降はすべて、個別フィールドに展開せず 1 つの不透明なタグ付き blob
  （``tools.common.blob.Ptr``/``Ver``。同モジュール参照）として保持する。これは意図的で、
  コンポーネントの直接の基底は常に ``ComponentBase`` とは限らない（例: ``Hyena : EnemyBase :
  ComponentBase``）ため、基底チェーンの深さは型ごとに異なり決め打ちできない。
  ``guid``/``is_enabled`` は blob をたどって ComponentBase 自身の本体を見つけて読み書き
  する（:func:`find_component_guid` / :func:`find_component_enabled` /
  :func:`set_component_enabled` 参照）。ComponentBase はどのコンポーネントの基底チェーンでも
  最も根元にあるので、チェーンの深さに関係なく一意に定まる。
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Optional

from tools.common.blob import Ver
from tools.common.cereal_json import OrderedObj

# ---------------------------------------------------------------------------
# 固定の構造用 FQN / クラスバージョン（エンジンのソースで確認済み。
# スキャンではなくハードコードされた C++ 型。docs/Scene.md 参照）
# ---------------------------------------------------------------------------
FQN_SCENE_GAMEOBJECT = "NanamiEngine::Scene::SceneGameObject"
FQN_PREFAB_GAMEOBJECT = "NanamiEngine::Module::GameObject::PrefabGameObject"
FQN_COPIED_PREFAB_GO = "NanamiEngine::Scene::CopiedPrefabGameObject"
FQN_IGAMEOBJECT = "NanamiEngine::Module::GameObject::IGameObject"
FQN_IOBJECT = "NanamiEngine::Module::Object::IObject"
FQN_GUID = "Guid"
FQN_COMPONENT_GROUP = "NanamiEngine::Module::GameObject::ComponentGroup"
FQN_TRANSFORM = "NanamiEngine::Module::GameObject::Transform"
FQN_COMPONENT_BASE = "NanamiEngine::Module::Component::ComponentBase"

# gameObject_N / "child" のポリモーフィックスロットに現れうる
# 3 種の GameObject「ルートオブジェクト」型の kind <-> fqn 対応。
KIND_SCENE = "scene"
KIND_PREFAB_ROOT = "prefab_root"
KIND_COPIED_PREFAB = "copied_prefab"

GAMEOBJECT_FQN_BY_KIND = {
    KIND_SCENE: FQN_SCENE_GAMEOBJECT,
    KIND_PREFAB_ROOT: FQN_PREFAB_GAMEOBJECT,
    KIND_COPIED_PREFAB: FQN_COPIED_PREFAB_GO,
}
GAMEOBJECT_KIND_BY_FQN = {v: k for k, v in GAMEOBJECT_FQN_BY_KIND.items()}

GAMEOBJECT_CLASS_VERSION = {
    FQN_SCENE_GAMEOBJECT: 1,
    FQN_PREFAB_GAMEOBJECT: 2,
    FQN_COPIED_PREFAB_GO: 1,
}
# ``transform_`` の後に ``mark_`` が追加される前のバージョン。ファイル内でその型の
# ノードがどれも mark を持たない間、writer はその GameObject 型をこのバージョン（``mark_``
# キーなし）のままにするので、手を付けていない mark 導入前のファイルはバイト単位で同一に往復する。
GAMEOBJECT_CLASS_VERSION_WITHOUT_MARK = {
    FQN_SCENE_GAMEOBJECT: 0,
    FQN_PREFAB_GAMEOBJECT: 1,
    FQN_COPIED_PREFAB_GO: 0,
}

# Engine/Module/GameObject/Mark/GameObjectMark.h の ``GAMEOBJECT_MARK_NAMES`` と対応
# （添字 == ``mark_`` に格納される uint8_t 値）。
MARK_NAMES = [
    "None",
    "LabelGray", "LabelBlue", "LabelTeal", "LabelGreen", "LabelYellow", "LabelOrange", "LabelRed", "LabelPurple",
    "CircleGray", "CircleBlue", "CircleTeal", "CircleGreen", "CircleYellow", "CircleOrange", "CircleRed", "CirclePurple",
    "DiamondGray", "DiamondBlue", "DiamondTeal", "DiamondGreen", "DiamondYellow", "DiamondOrange", "DiamondRed", "DiamondPurple",
]


# ---------------------------------------------------------------------------
# 小さな数値の葉 - 素の float ではなく tools.common.cereal_json.Num のまま保持し、
# 手を付けていないファイルは元のリテラル文字列のまま往復させる。
# 新しい Num を Num.of_float() で作るのは編集時だけ。
# ---------------------------------------------------------------------------
@dataclass
class Vec3:
    x: Any
    y: Any
    z: Any


@dataclass
class Quat:
    x: Any
    y: Any
    z: Any
    w: Any


@dataclass
class Transform:
    local_pos: Vec3
    local_rot: Quat
    local_scale: Vec3
    world_matrix: Any  # 不透明な blob（ロード時にエンジンが再計算する。CLI では編集しない）
    children: list["GameObjectNode"] = field(default_factory=list)


@dataclass
class Component:
    fqn: str
    class_version: int  # *この* 出現で出力されたかどうかに関係なく常に判明している
    data: Any  # コンポーネントの `data` オブジェクトのタグ付き blob（Ptr/Ver/plain）、ccv は除去済み


@dataclass
class GameObjectNode:
    kind: str  # "scene" | "prefab_root" | "copied_prefab"
    guid: str
    name: str
    is_active: bool
    components: list[Component]
    transform: Transform
    # ``mark_``（MARK_NAMES への添字）。None = ファイルがこのフィールド導入前のもの。
    mark: Optional[int] = None


@dataclass
class Scene:
    name: str
    roots: list[GameObjectNode] = field(default_factory=list)


@dataclass
class Prefab:
    root: GameObjectNode
    copied_object_guids: list[str] = field(default_factory=list)


# ---------------------------------------------------------------------------
# ComponentBase アクセサ - ComponentBase の上に中間基底がいくつあっても、
# 基底クラス blob チェーン内の guid_/isEnable_ を探索・変更する。
# ---------------------------------------------------------------------------
def _unwrap(node: Any) -> Optional[OrderedObj]:
    """blob ノードの OrderedObj 本体。Ver で包まれている（この出現で
    cereal_class_version を出力した）場合も、素のまま（後続のバージョンなし出現）の場合も返す。"""
    if isinstance(node, Ver):
        return node.body
    if isinstance(node, OrderedObj):
        return node
    return None


def _find_component_base_body(comp: Component) -> Optional[OrderedObj]:
    """入れ子の ``value0`` 基底クラスラッパーをたどり、ComponentBase 自身の
    ``guid_``/``isEnable_`` を持つ本体を見つける。ComponentBase は常に最も内側の基底で、
    必ずこの 2 キーだけをシリアライズするので、コンポーネント型ごとに中間基底
    （``EnemyBase``、``ColliderBase``、...）がいくつあっても一意に定まる。
    """
    node = comp.data.get("value0") if isinstance(comp.data, OrderedObj) else None
    while node is not None:
        body = _unwrap(node)
        if body is None:
            return None
        if "guid_" in body.keys() and "isEnable_" in body.keys():
            return body
        node = body.get("value0")
    return None


def find_component_guid(comp: Component) -> Optional[str]:
    body = _find_component_base_body(comp)
    if body is None:
        return None
    guid_obj = _unwrap(body["guid_"])
    return guid_obj["value_"] if guid_obj is not None else None


def find_component_enabled(comp: Component) -> Optional[bool]:
    body = _find_component_base_body(comp)
    if body is None:
        return None
    return bool(body["isEnable_"])


def set_component_enabled(comp: Component, enabled: bool) -> None:
    body = _find_component_base_body(comp)
    if body is None:
        raise ValueError(f"component {comp.fqn}: could not locate ComponentBase body")
    body["isEnable_"] = bool(enabled)


def set_component_guid(comp: Component, new_guid: str) -> None:
    body = _find_component_base_body(comp)
    if body is None:
        raise ValueError(f"component {comp.fqn}: could not locate ComponentBase body")
    guid_node = _unwrap(body["guid_"])
    if guid_node is None:
        raise ValueError(f"component {comp.fqn}: guid_ has an unexpected shape")
    guid_node["value_"] = new_guid
