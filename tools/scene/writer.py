""":mod:`tools.scene.model`（:class:`Scene` / :class:`Prefab`）-> cereal-JSON テキスト。

ポリモーフィック id、ptr_wrapper id、cereal_class_version をすべてグローバルカウンタから
``cereal::JSONOutputArchive`` と全く同じ深さ優先の順序で再生成する
（``tools.bt.writer`` と同じ方針）ので、出力は同一に読み戻せる。``.prefab`` のルートは
素のオブジェクトとして書く（ポリモーフィックラッパー/バージョン/基底チェーンなし。
:func:`write_prefab` 参照）。``.scene`` のルート配列と、入れ子のすべての
GameObject/Component は通常の ``shared_ptr`` 管理を通る。
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver
from tools.common.cereal_json import Num, OrderedObj, dumps, to_file_bytes

from . import model

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


def _marked_gameobject_fqns(roots: list[Optional[model.GameObjectNode]]) -> set[str]:
    """ファイル内のどこかで ``mark_`` を持つノードが 1 つ以上ある GameObject 型（fqn）。
    cereal は型のクラスバージョンを最初の出現でしか出力せず、エンジンはそれを以降の
    すべての出現に適用するので、``mark_`` は型ごとに全部付けるか全く付けないかのどちらか。"""
    marked: set[str] = set()
    stack = [n for n in roots if n is not None]
    while stack:
        node = stack.pop()
        if node.mark is not None:
            marked.add(model.GAMEOBJECT_FQN_BY_KIND[node.kind])
        stack.extend(c for c in node.transform.children if c is not None)
    return marked


class _W:
    def __init__(self, marked_fqns: set[str]) -> None:
        self.k = 0
        self.poly_ctr = 0
        self.poly: dict[str, int] = {}
        self.emitted: set[tuple] = set()
        self.marked_fqns = marked_fqns

    # -- 管理用カウンタ ------------------------------------------------------
    def new_k(self) -> int:
        self.k += 1
        return FIRST_BIT | self.k

    def poly_id(self, fqn: str) -> tuple[int, bool]:
        if fqn in self.poly:
            return self.poly[fqn], False
        self.poly_ctr += 1
        self.poly[fqn] = self.poly_ctr
        return self.poly_ctr, True

    def emit_ver(self, key: tuple, version: int, obj: OrderedObj) -> None:
        if key not in self.emitted:
            self.emitted.add(key)
            obj.insert(0, "cereal_class_version", Num.of_int(int(version)))

    def poly_slot(self, fqn: str, exact: bool) -> OrderedObj:
        o = OrderedObj()
        if exact:
            o["polymorphic_id"] = Num.of_int(EXACT_PID)
            return o
        pid, first = self.poly_id(fqn)
        o["polymorphic_id"] = Num.of_int((FIRST_BIT | pid) if first else pid)
        if first:
            o["polymorphic_name"] = fqn
        return o

    # -- タグ付き blob（コンポーネントのパラメータ）--------------------------
    def blob(self, n: Any) -> Any:
        if isinstance(n, Ptr):
            if n.null:
                return OrderedObj([("polymorphic_id", Num.of_int(0))])
            o = self.poly_slot(n.fqn or "", exact=n.exact)
            if n.wrapper == "shared":
                kid = self.new_k()
                o["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)),
                                               ("data", self.blob(n.data))])
            else:
                o["ptr_wrapper"] = OrderedObj([("valid", Num.of_int(1)),
                                               ("data", self.blob(n.data))])
            return o
        if isinstance(n, Ver):
            o = OrderedObj()
            if n.literal_presence is None:
                self.emit_ver(n.key, n.version, o)
            elif n.literal_presence:
                o.insert(0, "cereal_class_version", Num.of_int(int(n.version)))
            # literal_presence が False: この出現では出力しない。
            for k, v in n.body.items():
                # o[k]= ではなく append: バージョン付き構造体はキーを繰り返すことがあり
                # （Transform が "child" でそうするように、BoneSync はエントリごとに "sync" メンバーを
                # 1 つ書く）、__setitem__ だとそれらが 1 つにまとまってしまう。
                o.append(k, self.blob(v))
            return o
        if isinstance(n, OrderedObj):
            return OrderedObj((k, self.blob(v)) for k, v in n.items())
        if isinstance(n, list):
            return [self.blob(x) for x in n]
        return n

    # -- 小さな固定形状の葉 --------------------------------------------------
    def guid_obj(self, guid: str) -> OrderedObj:
        g = OrderedObj()
        self.emit_ver(("type", "Guid"), 0, g)
        g["value_"] = guid
        return g

    def vec3_obj(self, v: model.Vec3) -> OrderedObj:
        return OrderedObj([("value0", v.x), ("value1", v.y), ("value2", v.z)])

    def quat_obj(self, q: model.Quat) -> OrderedObj:
        return OrderedObj([("value0", q.x), ("value1", q.y), ("value2", q.z), ("value3", q.w)])

    def base_chain_obj(self) -> OrderedObj:
        """どの GameObject 種別にも共通の IGameObject -> IObject 基底クラスチェーン
        （``archive(cereal::base_class<IGameObject>(this))`` -> IGameObject 自身の
        ``archive(cereal::base_class<IObject>(this))``。どちらの本体もそれ以外は空）。"""
        io = OrderedObj()
        self.emit_ver(("type", "IObject"), 0, io)
        ig = OrderedObj()
        self.emit_ver(("type", "IGameObject"), 0, ig)
        ig["value0"] = io
        return ig

    # -- Transform / ComponentGroup / Component ------------------------------
    def transform_obj(self, t: model.Transform) -> OrderedObj:
        obj = OrderedObj()
        self.emit_ver(("type", model.FQN_TRANSFORM), 0, obj)
        obj["localPos_"] = self.vec3_obj(t.local_pos)
        obj["localRot_"] = self.quat_obj(t.local_rot)
        obj["localScale_"] = self.vec3_obj(t.local_scale)
        obj["worldMatrix_"] = self.blob(t.world_matrix)
        obj["childCount"] = Num.of_int(len(t.children))
        for child in t.children:
            obj.append("child", self.gameobject_slot(child))
        return obj

    def component_slot(self, c: model.Component) -> OrderedObj:
        slot = self.poly_slot(c.fqn, exact=False)
        kid = self.new_k()
        data = OrderedObj()
        self.emit_ver(("comp", c.fqn), int(c.class_version), data)
        src = c.data if isinstance(c.data, OrderedObj) else OrderedObj()
        for k, v in src.items():
            # data[k]= ではなく append: コンポーネントはキーを繰り返すことがあり
            # （Transform が "child" でそうするように、BoneSync はエントリごとに "sync" メンバーを
            # 1 つ書く）、__setitem__ だとそれらが 1 つにまとまってしまう。
            data.append(k, self.blob(v))
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot

    def components_obj(self, comps: list[model.Component]) -> OrderedObj:
        obj = OrderedObj()
        self.emit_ver(("type", model.FQN_COMPONENT_GROUP), 0, obj)
        obj["componentCount"] = Num.of_int(len(comps))
        for i, c in enumerate(comps):
            obj[f"component_{i}"] = self.component_slot(c)
        return obj

    # -- GameObject -----------------------------------------------------
    def gameobject_body(self, node: model.GameObjectNode, write_mark: bool) -> OrderedObj:
        """``isActive_``/``name_``/``guid_``/``components_``/``transform_``
        （``write_mark`` のときは + ``mark_``）を展開したもの。``.prefab`` のルートにはそのまま使い、
        ``gameObject_N``/``"child"`` のポリモーフィックスロットでは基底チェーンのラッパーの下に入れる。
        """
        body = OrderedObj()
        body["isActive_"] = bool(node.is_active)
        body["name_"] = node.name
        body["guid_"] = self.guid_obj(node.guid)
        body["components_"] = self.components_obj(node.components)
        body["transform_"] = self.transform_obj(node.transform)
        if write_mark:
            body["mark_"] = Num.of_int(int(node.mark or 0))
        return body

    def gameobject_slot(self, node: Optional[model.GameObjectNode]) -> OrderedObj:
        if node is None:
            return OrderedObj([("polymorphic_id", Num.of_int(0))])
        fqn = model.GAMEOBJECT_FQN_BY_KIND[node.kind]
        slot = self.poly_slot(fqn, exact=False)
        kid = self.new_k()
        data = OrderedObj()
        write_mark = fqn in self.marked_fqns
        version_table = (model.GAMEOBJECT_CLASS_VERSION if write_mark
                         else model.GAMEOBJECT_CLASS_VERSION_WITHOUT_MARK)
        self.emit_ver(("go", fqn), version_table[fqn], data)
        data["value0"] = self.base_chain_obj()
        for k, v in self.gameobject_body(node, write_mark).items():
            data[k] = v
        slot["ptr_wrapper"] = OrderedObj([("id", Num.of_int(kid)), ("data", data)])
        return slot


def write_scene(scene: model.Scene) -> str:
    w = _W(_marked_gameobject_fqns(scene.roots))
    root = OrderedObj()
    root["name"] = scene.name
    root["gameObjectCount"] = Num.of_int(len(scene.roots))
    for i, node in enumerate(scene.roots):
        root[f"gameObject_{i}"] = w.gameobject_slot(node)
    return dumps(root)


def write_scene_file(path, scene: model.Scene) -> None:
    open(path, "wb").write(to_file_bytes(write_scene(scene)))


def write_prefab(prefab: model.Prefab) -> str:
    """``.prefab`` のルートは素のオブジェクト（``PrefabGameObject::OnSave()`` 参照）:
    ポリモーフィックラッパーも外側のクラスバージョンも IGameObject/IObject 基底チェーンもない。
    本体の 5 フィールド（ルートが持っていれば + ``mark_``。クラスバージョンがないので
    エンジンはキーの有無で判定する）の後に、``copiedObjectGuidList_`` の末尾部分が
    素の個数（``value0``）とその数だけの素の Guid（``value1..N``）として続くだけ。"""
    w = _W(_marked_gameobject_fqns(prefab.root.transform.children))
    root = w.gameobject_body(prefab.root, prefab.root.mark is not None)
    root["value0"] = Num.of_int(len(prefab.copied_object_guids))
    for i, guid in enumerate(prefab.copied_object_guids, 1):
        root[f"value{i}"] = w.guid_obj(guid)
    return dumps(root)


def write_prefab_file(path, prefab: model.Prefab) -> None:
    open(path, "wb").write(to_file_bytes(write_prefab(prefab)))
