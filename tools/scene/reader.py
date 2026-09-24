"""cereal-JSON テキスト -> :mod:`tools.scene.model`（:class:`Scene` / :class:`Prefab`）。

Node/Component の構造はモデルにデコードする。Component 自身の ``data`` の中身
（外側の ``cereal_class_version`` 以降）はすべてタグ付きの :mod:`tools.common.blob` にし、
writer が各コンポーネント型の内部形状を理解しなくても管理情報を一から組み直せるようにする
（``tools.bt.reader`` のアクションパラメータの扱いと同じ方針）。

対応するのは「純粋なツリー」のアーカイブだけ（どの ``ptr_wrapper`` も新しいデータを書く。
このエンジンの実際の ``.scene``/``.prefab`` はすべてそうで、シーン内のプレハブのコピーは
常に新しい GUID を持つ完全に独立した焼き込みスナップショットであり、共有参照ではない）。
後方参照があれば :class:`PureTreeError` を送出する。
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver, fingerprint
from tools.common.cereal_json import Num, OrderedObj, loads, read_text

from . import model

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


class PureTreeError(RuntimeError):
    pass


class _Ctx:
    """パースごとの状態: アーカイブのポリモーフィック型テーブルと、型ごとの
    クラスバージョンの記憶（``cereal_class_version`` を最初に出力した出現から学習する。
    ``model.py`` のモジュール docstring 参照）。"""

    def __init__(self) -> None:
        self.poly: dict[int, str] = {}
        self.component_versions: dict[str, int] = {}

    def ptr_slot(self, slot: OrderedObj) -> dict:
        pid = _num(slot["polymorphic_id"])
        if pid == 0:
            return dict(null=True, exact=False, fqn=None, wrapper="", data=None)
        exact = pid == EXACT_PID
        masked = pid & ~FIRST_BIT
        fqn = None
        if "polymorphic_name" in slot:
            fqn = slot["polymorphic_name"]
            if not exact:
                self.poly[masked] = fqn
        elif not exact:
            fqn = self.poly.get(masked)
        pw = slot["ptr_wrapper"]
        if "id" in pw:
            wrapper = "shared"
            if not (_num(pw["id"]) & FIRST_BIT):
                raise PureTreeError(
                    "shared object re-reference (ptr_wrapper.id without the 0x80000000 bit) "
                    "- this file is a DAG, which tools/scene does not support"
                )
        elif "valid" in pw:
            wrapper = "unique"
        else:
            raise PureTreeError(f"ptr_wrapper without id/valid: {pw!r}")
        return dict(null=False, exact=exact, fqn=fqn, wrapper=wrapper, data=pw["data"])


def _num(v: Any) -> Any:
    return v.value if isinstance(v, Num) else v


def _strip_ccv(obj: OrderedObj) -> tuple[Optional[int], OrderedObj]:
    """オブジェクト先頭の ``cereal_class_version`` を切り離す。"""
    if len(obj) and obj.keys()[0] == "cereal_class_version":
        v = _num(obj.values()[0])
        rest = OrderedObj(obj.items()[1:])
        return v, rest
    return None, obj


def _is_guid_obj(obj: OrderedObj) -> bool:
    keys = [k for k in obj.keys() if k != "cereal_class_version"]
    return keys == ["value_"] and isinstance(obj["value_"], str)


# ---------------------------------------------------------------------------
# 汎用 blob タグ付け（カタログ非依存。model.py の docstring 参照）
# ---------------------------------------------------------------------------
def _tag_value(ctx: _Ctx, val: Any) -> Any:
    if isinstance(val, (Num, str, bool)) or val is None:
        return val
    if isinstance(val, list):
        return [_tag_value(ctx, x) for x in val]
    if not isinstance(val, OrderedObj):
        return val

    keys = val.keys()
    if "polymorphic_id" in keys:
        s = ctx.ptr_slot(val)
        if s["null"]:
            return Ptr(exact=False, null=True, fqn=None, wrapper="", data=None)
        return Ptr(exact=s["exact"], null=False, fqn=s["fqn"], wrapper=s["wrapper"],
                  data=_tag_value(ctx, s["data"]))

    if keys and keys[0] == "cereal_class_version":
        v, body = _strip_ccv(val)
        if _is_guid_obj(val):
            return Ver(("type", "Guid"), v or 0, OrderedObj([("value_", body["value_"])]))
        # モデル化されていないバージョン付き構造体のフォールバック（ここには実際の
        # C++ 型名を与えるカタログがない）: 構造フィンガープリントをキーにし、さらに
        # literal_presence を固定して、この出現のバージョンを無条件に再現する。
        # こうしないと、本当は別物なのに構造が同一のマーカー型（例: 兄弟の "value1"/"value2"/...
        # メンバーとしてシリアライズされる複数の空の mixin 基底クラス）が同じ合成キーで衝突し、
        # 最初の 1 つ以降のバージョンを誤って抑制してしまう。tools.common.blob.Ver.literal_presence 参照。
        return Ver(("fp", fingerprint(val)), v or 0, _tag_plain(ctx, body), literal_presence=True)

    if _is_guid_obj(val):
        return OrderedObj([("value_", val["value_"])])

    return _tag_plain(ctx, val)


def _tag_plain(ctx: _Ctx, obj: OrderedObj) -> OrderedObj:
    return OrderedObj((k, _tag_value(ctx, v)) for k, v in obj.items())


# ---------------------------------------------------------------------------
# Transform
# ---------------------------------------------------------------------------
def _read_vec3(obj: OrderedObj) -> model.Vec3:
    return model.Vec3(obj["value0"], obj["value1"], obj["value2"])


def _read_quat(obj: OrderedObj) -> model.Quat:
    return model.Quat(obj["value0"], obj["value1"], obj["value2"], obj["value3"])


def _read_transform(ctx: _Ctx, obj: OrderedObj) -> model.Transform:
    _v, obj = _strip_ccv(obj)  # Transform 自身のバージョン（0）- writer が再導出する
    local_pos = _read_vec3(obj["localPos_"])
    local_rot = _read_quat(obj["localRot_"])
    local_scale = _read_vec3(obj["localScale_"])
    world_matrix = obj["worldMatrix_"]  # 不透明 - 編集せずそのまま通す
    count = int(_num(obj["childCount"]))
    children = []
    for slot in obj.values_for("child"):
        node = _read_gameobject_slot(ctx, slot)
        if node is not None:
            children.append(node)
    if len(children) != count:
        raise ValueError(f"childCount={count} but found {len(children)} \"child\" entries")
    return model.Transform(local_pos=local_pos, local_rot=local_rot, local_scale=local_scale,
                           world_matrix=world_matrix, children=children)


# ---------------------------------------------------------------------------
# コンポーネント
# ---------------------------------------------------------------------------
def _read_component_slot(ctx: _Ctx, slot: OrderedObj) -> model.Component:
    s = ctx.ptr_slot(slot)
    if s["null"]:
        raise ValueError("null component pointer encountered")
    fqn = s["fqn"]
    if fqn is None:
        raise ValueError("component slot without a resolvable type")
    v, rest = _strip_ccv(s["data"])
    if v is not None:
        ctx.component_versions[fqn] = v
    class_version = ctx.component_versions.get(fqn)
    if class_version is None:
        raise ValueError(
            f"component {fqn}: version never seen (its first occurrence in the "
            f"file should carry cereal_class_version)"
        )
    return model.Component(fqn=fqn, class_version=class_version, data=_tag_plain(ctx, rest))


def _read_components(ctx: _Ctx, obj: OrderedObj) -> list[model.Component]:
    _v, obj = _strip_ccv(obj)  # ComponentGroup 自身のバージョン（0）
    count = int(_num(obj["componentCount"]))
    return [_read_component_slot(ctx, obj[f"component_{i}"]) for i in range(count)]


# ---------------------------------------------------------------------------
# GameObject
# ---------------------------------------------------------------------------
def _read_gameobject_body(ctx: _Ctx, kind: str, obj: OrderedObj) -> model.GameObjectNode:
    is_active = bool(obj["isActive_"])
    name = obj["name_"]
    guid = obj["guid_"]["value_"]
    components = _read_components(ctx, obj["components_"])
    transform = _read_transform(ctx, obj["transform_"])
    mark = int(_num(obj["mark_"])) if "mark_" in obj else None
    return model.GameObjectNode(kind=kind, guid=guid, name=name, is_active=is_active,
                                components=components, transform=transform, mark=mark)


def _read_gameobject_slot(ctx: _Ctx, slot: OrderedObj) -> Optional[model.GameObjectNode]:
    """``shared_ptr<IGameObject>`` スロット（``gameObject_N`` エントリ、または
    Transform の繰り返される ``"child"`` エントリ）を読む。"""
    s = ctx.ptr_slot(slot)
    if s["null"]:
        return None
    fqn = s["fqn"]
    kind = model.GAMEOBJECT_KIND_BY_FQN.get(fqn)
    if kind is None:
        raise ValueError(f"unknown GameObject type in slot: {fqn!r}")
    _v, data = _strip_ccv(s["data"])  # 具象型自身のバージョン（SceneGameObject=0, ...）
    base = data["value0"]  # IGameObject -> IObject の基底チェーン: 取り出すフィールドはないので読み飛ばす
    if not isinstance(base, OrderedObj):
        raise ValueError("GameObject base-class block (value0) has an unexpected shape")
    body = OrderedObj(data.items()[1:])  # value0 以降すべて: isActive_.. transform_
    return _read_gameobject_body(ctx, kind, body)


# ---------------------------------------------------------------------------
# Scene
# ---------------------------------------------------------------------------
def read_scene(text: str) -> model.Scene:
    ctx = _Ctx()
    root = loads(text)
    if "gameObjectCount" not in root:
        raise ValueError("not a .scene file: missing gameObjectCount")
    name = root["name"]
    count = int(_num(root["gameObjectCount"]))
    roots = []
    for i in range(count):
        node = _read_gameobject_slot(ctx, root[f"gameObject_{i}"])
        if node is not None:
            roots.append(node)
    return model.Scene(name=name, roots=roots)


def read_scene_file(path) -> model.Scene:
    return read_scene(read_text(path))


# ---------------------------------------------------------------------------
# Prefab
# ---------------------------------------------------------------------------
def read_prefab(text: str) -> model.Prefab:
    """``.prefab`` のルートは ``PrefabGameObject::OnSave()`` 内で手書きの
    ``archive(CEREAL_NVP(x))`` 呼び出しによって書かれる。クラス自身の ``save``/``load``
    テンプレート経由ではない（そちらは ``CopyForInstantiate`` が使うメモリ上の
    portable-binary 往復でのみ呼ばれる）。そのため ``gameObject_N`` スロットと違い、
    ここのルートは素のオブジェクトで、ポリモーフィックラッパーも外側の ``cereal_class_version`` も
    IGameObject/IObject 基底チェーンもない。名前付きの 5 フィールドの後に、
    ``copiedObjectGuidList_`` の末尾部分が素の個数（``value0``）とその数だけの素の ``Guid``
    （``value1..N``）として続くだけ。
    """
    ctx = _Ctx()
    root = loads(text)
    if "components_" not in root or "transform_" not in root:
        raise ValueError("not a .prefab file: missing components_/transform_")
    go = _read_gameobject_body(ctx, model.KIND_PREFAB_ROOT, root)

    guids: list[str] = []
    if "value0" in root:
        count = int(_num(root["value0"]))
        for i in range(1, count + 1):
            g = root[f"value{i}"]
            _v, g = _strip_ccv(g)
            guids.append(g["value_"])
    return model.Prefab(root=go, copied_object_guids=guids)


def read_prefab_file(path) -> model.Prefab:
    return read_prefab(read_text(path))
