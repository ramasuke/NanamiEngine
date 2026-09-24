""":class:`tools.scene.model.Scene` または :class:`tools.scene.model.Prefab` に対する
構造 + Transform 編集プリミティブと、一括適用の :func:`apply` エントリポイント。

``tools.bt.edits`` と同様: どの関数も対象をその場で変更する。呼び出し側は
メモリ上にモデルを構築し、N 個の編集を適用し、``tools.scene.validate`` を1回呼び、
1回書き込む (``cli_edit.py`` の CLI を参照)。
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver
from tools.common.cereal_json import Num, OrderedObj
from tools.common.meta_base import mint_guid

from . import catalog as catalog_mod
from . import mathutil, model

EMPTY_GUID = "00000000-0000-0000-0000-000000000000"


class EditError(RuntimeError):
    pass


# ---------------------------------------------------------------------------
# 検索
# ---------------------------------------------------------------------------
def _search(nodes: list[model.GameObjectNode], guid: str):
    for i, n in enumerate(nodes):
        if n.guid == guid:
            return n, nodes, i
        found = _search(n.transform.children, guid)
        if found is not None:
            return found
    return None


def find_gameobject(target: Any, guid: str):
    """``container[index] is node`` となる ``(node, container, index)``。``guid`` が
    Prefab 自身のルートなら ``(node, None, None)`` (所属するリストがない - 大半の編集は
    それを直接操作するのを拒否する)。``target`` のどこにも見つからなければ ``None``。"""
    if isinstance(target, model.Prefab):
        if target.root.guid == guid:
            return target.root, None, None
        return _search(target.root.transform.children, guid)
    if isinstance(target, model.Scene):
        return _search(target.roots, guid)
    raise TypeError(f"expected Scene or Prefab, got {type(target).__name__}")


def _require(target: Any, guid: str) -> model.GameObjectNode:
    found = find_gameobject(target, guid)
    if found is None:
        raise EditError(f"GameObject not found: {guid}")
    return found[0]


def _resolve_parent_guid(target: Any, parent: str) -> str:
    if parent in ("root", "") and isinstance(target, model.Prefab):
        return target.root.guid
    return parent


def _find_chain(target: Any, guid: str) -> Optional[list[model.GameObjectNode]]:
    """ルートからノードまでのパス (両端含む) - ワールド変換の合成に使う。"""
    def _walk(nodes: list[model.GameObjectNode], trail: list[model.GameObjectNode]):
        for n in nodes:
            here = trail + [n]
            if n.guid == guid:
                return here
            found = _walk(n.transform.children, here)
            if found is not None:
                return found
        return None

    if isinstance(target, model.Prefab):
        if target.root.guid == guid:
            return [target.root]
        sub = _walk(target.root.transform.children, [target.root])
        return sub
    return _walk(target.roots, [])


# ---------------------------------------------------------------------------
# TRS <-> モデルの変換
# ---------------------------------------------------------------------------
def _f(n: Any) -> float:
    return float(n.value) if isinstance(n, Num) else float(n)


def _vec3_floats(v: model.Vec3) -> tuple[float, float, float]:
    return (_f(v.x), _f(v.y), _f(v.z))


def _quat_floats(q: model.Quat) -> tuple[float, float, float, float]:
    return (_f(q.x), _f(q.y), _f(q.z), _f(q.w))


def _vec3_from_floats(t: tuple[float, float, float]) -> model.Vec3:
    return model.Vec3(Num.of_float(float(t[0])), Num.of_float(float(t[1])), Num.of_float(float(t[2])))


def _quat_from_floats(t: tuple[float, float, float, float]) -> model.Quat:
    return model.Quat(Num.of_float(float(t[0])), Num.of_float(float(t[1])),
                      Num.of_float(float(t[2])), Num.of_float(float(t[3])))


def _node_local_trs(node: model.GameObjectNode) -> mathutil.Trs:
    t = node.transform
    return mathutil.Trs(_vec3_floats(t.local_pos), _quat_floats(t.local_rot),
                        _vec3_floats(t.local_scale))


def _world_trs_of_chain(chain: list[model.GameObjectNode]) -> mathutil.Trs:
    world = mathutil.IDENTITY
    for node in chain:
        world = world.then(_node_local_trs(node))
    return world


def identity_world_matrix_blob() -> OrderedObj:
    """新規構築した glm::mat4(1.0) の単位行列。新しい Transform の ``worldMatrix_`` が
    書かれる形に合わせる (エンジンはロード時に実際の値を再計算するが、フィールドには
    正しい形の *何か* が入っている必要がある)。"""
    rows = [(1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0),
            (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0)]
    obj = OrderedObj()
    for i, row in enumerate(rows):
        obj[f"value{i}"] = OrderedObj(
            (f"value{j}", Num.of_float(v)) for j, v in enumerate(row)
        )
    return obj


# ---------------------------------------------------------------------------
# 構造編集
# ---------------------------------------------------------------------------
def new_gameobject(name: str, *, kind: str = model.KIND_SCENE,
                   pos: tuple[float, float, float] = (0.0, 0.0, 0.0),
                   rot: tuple[float, float, float, float] = (0.0, 0.0, 0.0, 1.0),
                   scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
                   is_active: bool = True, guid: Optional[str] = None) -> model.GameObjectNode:
    return model.GameObjectNode(
        kind=kind,
        guid=guid or mint_guid(),
        name=name,
        is_active=is_active,
        components=[],
        transform=model.Transform(
            local_pos=_vec3_from_floats(pos),
            local_rot=_quat_from_floats(rot),
            local_scale=_vec3_from_floats(scale),
            world_matrix=identity_world_matrix_blob(),
            children=[],
        ),
    )


def add_gameobject(target: Any, *, parent: Optional[str], name: str,
                   pos: tuple[float, float, float] = (0.0, 0.0, 0.0),
                   rot: tuple[float, float, float, float] = (0.0, 0.0, 0.0, 1.0),
                   scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
                   is_active: bool = True, guid: Optional[str] = None) -> model.GameObjectNode:
    node = new_gameobject(name, pos=pos, rot=rot, scale=scale, is_active=is_active, guid=guid)
    if parent is None:
        if isinstance(target, model.Prefab):
            raise EditError("a Prefab has a single implicit root - pass "
                            "parent='root' (or the root's guid) to add a child")
        target.roots.append(node)
        return node
    parent_guid = _resolve_parent_guid(target, parent)
    parent_node = _require(target, parent_guid)
    parent_node.transform.children.append(node)
    return node


def remove_gameobject(target: Any, guid: str) -> model.GameObjectNode:
    found = find_gameobject(target, guid)
    if found is None:
        raise EditError(f"GameObject not found: {guid}")
    node, container, index = found
    if container is None:
        raise EditError("cannot remove a Prefab's own root GameObject")
    return container.pop(index)


def move_gameobject(target: Any, *, guid: str, new_parent: Optional[str],
                    preserve_world_transform: bool = True) -> None:
    found = find_gameobject(target, guid)
    if found is None:
        raise EditError(f"GameObject not found: {guid}")
    node, container, index = found
    if container is None:
        raise EditError("cannot move a Prefab's own root GameObject")

    new_parent_guid = _resolve_parent_guid(target, new_parent) if new_parent is not None else None
    if new_parent_guid == guid:
        raise EditError("cannot reparent a GameObject under itself")

    world_before = None
    if preserve_world_transform:
        chain = _find_chain(target, guid)
        if chain is None:
            raise EditError(f"GameObject not found while resolving its chain: {guid}")
        world_before = _world_trs_of_chain(chain)

    container.pop(index)
    if new_parent_guid is None:
        if isinstance(target, model.Prefab):
            raise EditError("a Prefab has a single implicit root - pass new_parent")
        target.roots.append(node)
    else:
        new_parent_chain = _find_chain(target, new_parent_guid)
        if new_parent_chain is None:
            # 失敗する前に元の位置へ戻し、編集をアトミックにする
            container.insert(index, node)
            raise EditError(f"new parent not found: {new_parent}")
        new_parent_node = new_parent_chain[-1]
        new_parent_node.transform.children.append(node)
        if preserve_world_transform:
            new_parent_world = _world_trs_of_chain(new_parent_chain)
            new_local = new_parent_world.local_of(world_before)
            node.transform.local_pos = _vec3_from_floats(new_local.pos)
            node.transform.local_rot = _quat_from_floats(new_local.rot)
            node.transform.local_scale = _vec3_from_floats(new_local.scale)


def set_transform(target: Any, guid: str, *,
                  pos: Optional[tuple[float, float, float]] = None,
                  rot: Optional[tuple[float, float, float, float]] = None,
                  scale: Optional[tuple[float, float, float]] = None) -> None:
    node = _require(target, guid)
    if pos is not None:
        node.transform.local_pos = _vec3_from_floats(pos)
    if rot is not None:
        node.transform.local_rot = _quat_from_floats(rot)
    if scale is not None:
        node.transform.local_scale = _vec3_from_floats(scale)


def set_active(target: Any, guid: str, is_active: bool) -> None:
    node = _require(target, guid)
    node.is_active = bool(is_active)


def rename_gameobject(target: Any, guid: str, new_name: str) -> None:
    node = _require(target, guid)
    node.name = new_name


# ---------------------------------------------------------------------------
# コンポーネント編集
# ---------------------------------------------------------------------------
def _leaf_of(name: str) -> str:
    return name.split("<", 1)[0].rsplit("::", 1)[-1]


def _default_scalar(shape: str, default: Any) -> Any:
    if shape == "int":
        try:
            return Num.of_int(int(default))
        except (TypeError, ValueError):
            return Num.of_int(0)
    if shape == "float":
        try:
            return Num.of_float(float(default))
        except (TypeError, ValueError):
            return Num.of_float(0.0)
    if shape == "bool":
        return bool(default) if isinstance(default, bool) else str(default).lower() == "true"
    if shape == "string":
        return "" if default is None else str(default)
    return Num.of_int(0)


def _vec(n: int) -> OrderedObj:
    return OrderedObj([(f"value{i}", Num.of_float(0.0)) for i in range(n)])


def field_blob(field_type_leaf: str, guid: str = EMPTY_GUID) -> Ver:
    """新規の ``Field<T>`` パラメータ blob (``tools.bt.edits._field_blob`` と同じ
    - これは Scene/BT 固有ではなく、汎用の cereal FieldContext<T> レイアウトなので同じ形)。

    ``Field<T>``/``FieldContext<T>`` は ``T`` ごとの実際の C++ テンプレート
    インスタンス化で、それぞれ cereal が独立にバージョン管理する - しかし新しく
    追加したフィールドはツリーの *どこにでも* 置かれうる一方、ファイル内の別の場所に
    ある同じ ``T`` の実際の先行出現 (例: まったく別のコンポーネント上の
    ``FIELD(Asset::SpriteFile)``) は、この合成の ``("type", ...)`` キーではなく
    構造 fingerprint でタグ付けされて読み戻される - そのためライターのグローバルな
    キーごと1回の追跡では両者が同じ実型だと分からず、こちらを誤って最初の出現と
    みなし、cereal がそこで期待しない `cereal_class_version` を出力してしまうことがある。
    ここで余分なバージョンキーがあるのは無害ではない: cereal の JSON アーカイブが
    名前付きノードを *検索* するのは、その型のバージョンをまだ知らないときだけで、
    実際の2回目以降の出現ではその検索を丸ごと飛ばして次の値を位置で読む。そのため
    迷い込んだバージョンキーは後続の読み込みをすべて黙ってずらす - 具体的には、
    Field<T>::load() の直後の呼び出し (名前なしの `archive(context_)`) が整数を
    オブジェクトとして扱い、rapidjson::GenericValue::MemberEnd() で未定義動作になる。
    代わりに `literal_presence=False` (出力しない) を強制する: これが本当にその型の
    ファイル内初出というまれなケースでは、cereal は黙ってロードを壊すのではなく、
    捕捉可能な "NVP (cereal_class_version) not found" 例外で明示的に失敗する。
    """
    guid_ver = Ver(("type", "Guid"), 0, OrderedObj([("value_", guid)]))
    holder = Ver(("type", f"FieldHolder<{field_type_leaf}>"), 0,
                OrderedObj([("value0", guid_ver)]), literal_presence=False)
    ptr = Ptr(exact=True, null=False, fqn=None, wrapper="shared", data=holder)
    return Ver(("type", f"Field<{field_type_leaf}>"), 0, OrderedObj([("value0", ptr)]),
               literal_presence=False)


def color32_blob(r: int, g: int, b: int) -> Ver:
    """``Color32`` パラメータ blob。

    :func:`field_blob` と違い、こちらは常に ``cereal_class_version`` を書く
    (``literal_presence=True``)。Color32::load は *名前付き* の読み込み (``r_``) から
    始まるので、cereal の名前検索は不要なバージョンキーを読み飛ばし、迷い込んだキーは
    無害 - 一方、ファイル内最初の Color32 でキーが欠けていると致命的。Field<T> は
    最初の読み込みが位置によるので、逆のトレードオフを取らざるを得ない。
    """
    return Ver(("type", "Color32"), 0,
               OrderedObj([("r_", Num.of_int(r)), ("g_", Num.of_int(g)), ("b_", Num.of_int(b))]),
               literal_presence=True)


def _reactive_wrap(pinfo: dict, value: Any) -> Any:
    # NOTE: SerializableReactiveProperty<T> は {"value": T} で保存される
    return OrderedObj([("value", value)]) if pinfo.get("reactive") else value


def _param_blob(pinfo: dict) -> Any:
    return _reactive_wrap(pinfo, _param_blob_inner(pinfo))


def _param_blob_inner(pinfo: dict) -> Any:
    shape = pinfo.get("shape")
    if shape in ("int", "float", "bool", "string"):
        return _default_scalar(shape, pinfo.get("default"))
    if shape == "vec2":
        return _vec(2)
    if shape == "vec3":
        return _vec(3)
    if shape == "field":
        return field_blob(_leaf_of(pinfo.get("type", "?")))
    if shape == "color32":
        return color32_blob(255, 255, 255)
    if shape == "vector":
        return []
    # ネスト/不明 -> 無害なプレースホルダー。validate() が指摘し、
    # 人がエディタで仕上げる (tools.bt 自身の制限と同じ)。
    return Num.of_int(0)


def _component_bases(entry: dict) -> list[dict]:
    """新規インスタンスが書くべき順序付きの ``base_class<>`` スロット
    (``[{"leaf", "key"}]`` - ``catalog_scan._parse_serializable`` を参照)。``bases``
    フィールド以前のカタログは最初の基底しか知らないので、それにフォールバックし、
    古いカタログではクラッシュせず従来の単一スロットの動作に落ちるようにする。"""
    bases = entry.get("bases")
    if bases is None:
        return [{"leaf": entry.get("immediate_base") or "ComponentBase", "key": "value0"}]
    return bases


def _unmodeled_bases(entry: dict, cat: catalog_mod.Catalog) -> list[str]:
    """このツールキットがゼロから構築できない基底 leaf: ``ComponentBase`` 以外で、
    自身の ``save()`` 本体が空だと分かっていないもの
    (``ColliderBase``、``NetworkComponent``、不明/曖昧な leaf など)。"""
    out: list[str] = []
    for b in _component_bases(entry):
        leaf = b["leaf"]
        if leaf == "ComponentBase":
            continue
        info = cat.base_info(leaf)
        if info is None or info.get("ambiguous") or not info.get("empty"):
            out.append(leaf)
    return out


def _empty_base_blob(leaf: str, version: int) -> Ver:
    """フィールドを持たないマーカー基底 (``IInitRenderable``、
    ``IUserInterfaceRenderable``、``IAwakable`` など) 用の新規スロット: *常に*
    ``cereal_class_version`` を持つ空オブジェクト。

    エンジンはそのキーをファイル内でその型が最初に出現したときだけ書き、以降は
    素の ``{}`` を書くが、このライターは先行する出現があるかどうかを判別できない
    (リーダーは既存スロットを実型ではなく構造 fingerprint でキー付けする -
    ``field_blob`` が述べているのと同じ盲点) ので、どちらの場合でも正しくロードされる
    形を1つ選ぶ必要がある。``Field<T>`` と違い、ここでは常に出力しても安全: 型の
    最初の出現では cereal がキーを探して読み、以降の出現では ``loadClassVersion`` が
    飛ばされ、基底の ``load()`` 本体は空なのでノード内は何も読まれず、
    ``finishNode()`` が親のイテレータをオブジェクト全体の先へ進めるだけ - 余分な
    キーには一切触れない。(``Field<T>`` が壊れたのはまさに、迷い込んだキーの後に
    *名前なし* の位置による読み込みが続いたから。空の基底の中では読み込み自体がない。)
    逆にキーを省くと、最初の出現のケースで "NVP (cereal_class_version) not found" で
    失敗する。エンジンは次の保存時にファイルを正規化し直す。
    """
    return Ver(("type", leaf), int(version), OrderedObj(), literal_presence=True)


def component_body_blob(entry: dict, guid: str,
                        cat: Optional[catalog_mod.Catalog] = None) -> OrderedObj:
    """新規コンポーネントの ``data`` blob (自身のクラスバージョンより後のすべて):
    コンポーネントがアーカイブする ``base_class<>`` ごとに名前なしの ``valueN`` スロットを
    アーカイブ順に1つ - ComponentBase のもの (``guid_``/``isEnable_``) と、各マーカー
    mixin 用の空オブジェクト - その後に各カタログパラメータの既定値をカタログ順に並べる。
    cereal は基底スロットを位置で読むので、すべて揃っている必要がある: mixin の
    スロットを落としたら、エンジンが ``spriteFile_`` を基底クラスとして読んでしまった。"""
    cat = cat or catalog_mod.load()
    out = OrderedObj()
    for b in _component_bases(entry):
        leaf = b["leaf"]
        if leaf == "ComponentBase":
            guid_ver = Ver(("type", "Guid"), 0, OrderedObj([("value_", guid)]))
            base_body = OrderedObj([("guid_", guid_ver), ("isEnable_", True)])
            out.append(b["key"], Ver(("type", "ComponentBase"), 0, base_body))
        else:
            info = cat.base_info(leaf) or {}
            out.append(b["key"], _empty_base_blob(leaf, info.get("version", 0)))
    for p in entry.get("params", []):
        out.append(p["key"], _param_blob(p))
    return out


def new_component(entry: dict, *, guid: Optional[str] = None,
                  cat: Optional[catalog_mod.Catalog] = None) -> model.Component:
    cat = cat or catalog_mod.load()
    bases = _component_bases(entry)
    missing = _unmodeled_bases(entry, cat)
    if missing:
        raise EditError(
            f"{entry['fqn']}: has base class(es) with their own serialised fields this "
            f"toolkit does not model yet ({', '.join(missing)}) - refusing to construct a "
            f"new instance from scratch (it would be missing required data). Copy an "
            f"existing GameObject/prefab that already has one, or add it via the in-engine "
            f"editor."
        )
    if not any(b["leaf"] == "ComponentBase" for b in bases):
        # モデル化したどの基底も空の mixin なのに、guid_/isEnable_ を持つものがない。
        raise EditError(
            f"{entry['fqn']}: does not archive ComponentBase directly (bases: "
            f"{', '.join(b['leaf'] for b in bases) or 'none'}) - this toolkit cannot construct "
            f"a brand-new instance from scratch. Copy an existing GameObject/prefab that "
            f"already has one, or add it via the in-engine editor."
        )
    if entry.get("interleaved_bases"):
        raise EditError(
            f"{entry['fqn']}: archives a base class after one of its own fields - this "
            f"toolkit only models the bases-first layout; refusing to guess the slot order."
        )
    cguid = guid or mint_guid()
    return model.Component(fqn=entry["fqn"], class_version=int(entry.get("version", 0)),
                           data=component_body_blob(entry, cguid, cat))


def _find_param(entry: dict, key: str) -> Optional[dict]:
    for p in entry.get("params", []):
        if p["key"] == key or p["member"] == key:
            return p
    return None


def _coerce(shape: str, raw: str) -> Any:
    if shape == "int":
        return Num.of_int(int(raw, 0))
    if shape == "float":
        return Num.of_float(float(raw))
    if shape == "bool":
        return raw.strip().lower() in ("1", "true", "yes", "on")
    if shape == "string":
        return raw
    if shape in ("vec2", "vec3"):
        parts = [p.strip() for p in raw.replace(" ", ",").split(",") if p.strip()]
        return OrderedObj([(f"value{i}", Num.of_float(float(x))) for i, x in enumerate(parts)])
    if shape == "field":
        return raw.strip().upper()
    if shape == "color32":
        parts = [p.strip() for p in raw.replace(" ", ",").split(",") if p.strip()]
        if len(parts) != 3:
            raise EditError(f"color32 takes three components (r,g,b), got {raw!r}")
        return [int(x, 0) for x in parts]
    raise EditError(f"cannot set a param of shape {shape!r}")


def _is_flag_combination(values: dict, number: int) -> bool:
    # NOTE: 全値が 0 か 2 の累乗の enum はビットフラグとみなし、OR した値も許す
    flags = [int(v) for v in values.values()]
    if not all(v == 0 or (v > 0 and v & (v - 1) == 0) for v in flags):
        return False
    mask = 0
    for v in flags:
        mask |= v
    return number >= 0 and number & ~mask == 0


def _enum_value(pinfo: dict, raw: str) -> int:
    """enum パラメータは列挙子名 (``Hyena`` / ``EnemyKind::Hyena``) またはその整数を
    受け付ける。名前はカタログの ``values`` と照合する。"""
    values: Optional[dict] = pinfo.get("values")
    name = raw.strip().rsplit("::", 1)[-1]
    if values and name in values:
        return int(values[name])
    try:
        number = int(raw, 0)
    except ValueError:
        known = ", ".join(values) if values else "unknown (use the integer value)"
        raise EditError(f"{pinfo['member']}: {raw!r} is not a {pinfo['enum']} value ({known})")
    if values and number not in values.values() and not _is_flag_combination(values, number):
        raise EditError(f"{pinfo['member']}: {number} is not a {pinfo['enum']} value "
                        f"({', '.join(f'{k}={v}' for k, v in values.items())})")
    return number


def _set_field_guid(node: Any, guid: str) -> None:
    # cereal は `cereal_class_version` を、アーカイブ内でその型が最初にシリアライズ
    # されたときだけ出力する - そのため最初の出現の Field<T>/FieldHolder<T> は Ver として
    # 往復するが、同じ型のそれ以降の出現 (よくあるケース) はバージョンキーを持たず
    # 素の OrderedObj として往復する。各階層で両方を受け付ける。
    if isinstance(node, Ver):
        ptr = node.body["value0"]
    elif isinstance(node, OrderedObj):
        ptr = node["value0"]
    else:
        ptr = None
    if not isinstance(ptr, Ptr):
        raise EditError("field param does not have the expected Field<T> shape")
    holder = ptr.data
    if isinstance(holder, Ver):
        guid_node = holder.body["value0"]
    elif isinstance(holder, OrderedObj):
        guid_node = holder["value0"]
    else:
        guid_node = None
    if isinstance(guid_node, Ver):
        guid_node.body["value_"] = guid
    elif isinstance(guid_node, OrderedObj):
        guid_node["value_"] = guid
    else:
        raise EditError("field param does not have the expected FieldHolder<T> shape")


def _set_color32(node: Any, rgb: list[int]) -> None:
    # その場で変更する: ファイル内の既存の cereal_class_version の配置は、この blob の
    # 位置に対して正しく、ノードを置き換えるとそれを書き換えてしまう。
    body = node.body if isinstance(node, Ver) else node
    if not isinstance(body, OrderedObj) or not all(k in body for k in ("r_", "g_", "b_")):
        raise EditError("color32 param does not have the expected r_/g_/b_ shape")
    for key, value in zip(("r_", "g_", "b_"), rgb):
        body[key] = Num.of_int(value)


def _set_one_param(comp: model.Component, entry: dict, key: str, raw: str) -> str:
    pinfo = _find_param(entry, key)
    if pinfo is None:
        raise EditError(f"{entry['fqn']} has no param named {key!r}")
    shape = pinfo.get("shape")
    if shape not in catalog_mod.SETTABLE_SHAPES:
        raise EditError(
            f"{entry['fqn']}.{pinfo['member']} has shape {shape!r}; tools/scene v1 cannot "
            f"set it - edit the .scene/.prefab by hand or in the editor"
        )
    jkey = pinfo["key"]
    if not isinstance(comp.data, OrderedObj) or jkey not in comp.data:
        raise EditError(f"param {jkey!r} missing from the stored blob (regen-catalog?)")
    if shape == "field":
        node = comp.data[jkey]
        if not isinstance(node, (Ver, OrderedObj)):
            raise EditError(f"{jkey}: expected a Field<T> blob")
        _set_field_guid(node, _coerce(shape, raw))
    elif shape == "color32":
        _set_color32(comp.data[jkey], _coerce(shape, raw))
    elif shape == "int" and pinfo.get("enum"):
        comp.data[jkey] = _reactive_wrap(pinfo, Num.of_int(_enum_value(pinfo, raw)))
    else:
        comp.data[jkey] = _reactive_wrap(pinfo, _coerce(shape, raw))
    return pinfo["member"]


def add_component(target: Any, guid: str, component_type: str, *,
                  cat: Optional[catalog_mod.Catalog] = None,
                  component_guid: Optional[str] = None,
                  params: Optional[dict[str, str]] = None) -> model.Component:
    cat = cat or catalog_mod.load()
    try:
        entry = cat.resolve_component(component_type)
    except catalog_mod.CatalogError as e:
        raise EditError(str(e)) from e
    node = _require(target, guid)
    comp = new_component(entry, guid=component_guid, cat=cat)
    if params:
        for k, v in params.items():
            _set_one_param(comp, entry, k, v)
    node.components.append(comp)
    return comp


def remove_component(target: Any, guid: str, index: int) -> model.Component:
    node = _require(target, guid)
    if not (0 <= index < len(node.components)):
        raise EditError(f"component index {index} out of range "
                        f"(GameObject has {len(node.components)})")
    return node.components.pop(index)


def set_component_params(target: Any, guid: str, index: int, assignments: dict[str, str],
                         cat: Optional[catalog_mod.Catalog] = None) -> list[str]:
    cat = cat or catalog_mod.load()
    node = _require(target, guid)
    if not (0 <= index < len(node.components)):
        raise EditError(f"component index {index} out of range "
                        f"(GameObject has {len(node.components)})")
    comp = node.components[index]
    entry = cat.component_by_fqn(comp.fqn)
    if entry is None:
        raise EditError(f"{comp.fqn} is not in the catalog (regen-catalog?) - "
                        f"cannot set params by name")
    return [_set_one_param(comp, entry, k, v) for k, v in assignments.items()]


# ---------------------------------------------------------------------------
# instantiate-prefab
# ---------------------------------------------------------------------------
def _remint_guids(node: model.GameObjectNode, guid_remap: dict[str, str]) -> None:
    """``node`` とその配下すべてに新しい GUID を割り当て、``old -> new`` を
    ``guid_remap`` に記録する。"""
    new_guid = mint_guid()
    guid_remap[node.guid] = new_guid
    node.guid = new_guid
    for comp in node.components:
        old_comp_guid = model.find_component_guid(comp)
        try:
            new_comp_guid = mint_guid()
            model.set_component_guid(comp, new_comp_guid)
        except ValueError:
            continue  # コンポーネントに特定可能な ComponentBase 本体がない - そのままにする
        if old_comp_guid is not None:
            guid_remap[old_comp_guid] = new_comp_guid
    for child in node.transform.children:
        _remint_guids(child, guid_remap)


def _remap_guid_blob(blob: Any, guid_remap: dict[str, str]) -> None:
    if isinstance(blob, Ptr):
        _remap_guid_blob(blob.data, guid_remap)
    elif isinstance(blob, Ver):
        _remap_guid_blob(blob.body, guid_remap)
    elif isinstance(blob, OrderedObj):
        for key, value in blob.items():
            if key == "value_" and isinstance(value, str) and value in guid_remap:
                blob[key] = guid_remap[value]
            else:
                _remap_guid_blob(value, guid_remap)
    elif isinstance(blob, list):
        for item in blob:
            _remap_guid_blob(item, guid_remap)


def _remap_guid_references(node: model.GameObjectNode, guid_remap: dict[str, str]) -> None:
    """コピーしたツリー内のオブジェクトを指す Guid 参照 (例: ``FIELD(IGameObject)``) を、
    コピー側のオブジェクトを指すように付け替える - Instantiate 時のエンジンの
    ``GuidRemap::FromCopiedHierarchy`` + ``OnUpdateCopiedFieldInittables`` と同じ。
    アセット参照やツリー外への参照は ``guid_remap`` になく、そのまま残る。"""
    for comp in node.components:
        _remap_guid_blob(comp.data, guid_remap)
    for child in node.transform.children:
        _remap_guid_references(child, guid_remap)


def instantiate_prefab(target: Any, prefab: model.Prefab, *,
                       parent: Optional[str] = None) -> model.GameObjectNode:
    """``prefab`` のツリーを ``target`` (Scene、または別の Prefab のツリー) にディープコピーする。
    コピー内のすべての GameObject/Component に新しい GUID を割り当て (それらの間の参照も
    コピー側に付け替える)、ルートを ``CopiedPrefabGameObject`` として付け直す - エンジン自身の
    ``Scene::OnDrawFileDropGui`` -> ``PrefabGameObject::CopyForInstantiate()`` の動作と同じ
    (シーン内のプレハブのコピーは常に完全に独立した焼き込み済みスナップショットで、
    元の ``.prefab`` へのライブリンクには決してならない)。"""
    import copy as _copy

    new_root = _copy.deepcopy(prefab.root)
    new_root.kind = model.KIND_COPIED_PREFAB
    guid_remap: dict[str, str] = {}
    _remint_guids(new_root, guid_remap)
    _remap_guid_references(new_root, guid_remap)
    if parent is None:
        if isinstance(target, model.Prefab):
            raise EditError("a Prefab has a single implicit root - pass "
                            "parent='root' (or the root's guid)")
        target.roots.append(new_root)
    else:
        parent_guid = _resolve_parent_guid(target, parent)
        parent_node = _require(target, parent_guid)
        parent_node.transform.children.append(new_root)
    return new_root


# ---------------------------------------------------------------------------
# copy-prefab
# ---------------------------------------------------------------------------
def copy_prefab(prefab: model.Prefab) -> model.Prefab:
    """``Prefab`` 全体を、新しい独立した ``.prefab`` ファイルとして保存するために
    ディープコピーする: ツリー内のすべての GameObject/Component に新しい GUID を割り当て
    (``instantiate_prefab`` と同じ ``_remint_guids`` の規則)、``copied_object_guids`` は
    空リストにする (この新しいファイルにはまだ自身のシーンインスタンスがないため)。
    ``instantiate_prefab`` と違い、ルートの ``kind`` は ``KIND_PREFAB_ROOT`` のまま -
    結果はシーンに埋め込まれたインスタンスではなく、本物のプレハブアセットである。"""
    import copy as _copy

    new_root = _copy.deepcopy(prefab.root)
    guid_remap: dict[str, str] = {}
    _remint_guids(new_root, guid_remap)
    _remap_guid_references(new_root, guid_remap)
    return model.Prefab(root=new_root, copied_object_guids=[])


# ---------------------------------------------------------------------------
# 一括適用 - エージェント向けの主要インターフェース
# ---------------------------------------------------------------------------
def apply(target: Any, ops: list[dict]) -> list[str]:
    """JSON 形式の操作リストを順に適用する。各要素は ``{"op": "<name>", ...kwargs}``。
    最初の失敗で、失敗した操作のインデックスを示す :class:`EditError` を送出する
    (それ以降は適用されない)。"""
    log: list[str] = []
    for i, op in enumerate(ops):
        kind = op.get("op")
        try:
            if kind == "add-gameobject":
                node = add_gameobject(
                    target, parent=op.get("parent"), name=op["name"],
                    pos=tuple(op.get("pos", (0.0, 0.0, 0.0))),
                    rot=tuple(op.get("rot", (0.0, 0.0, 0.0, 1.0))),
                    scale=tuple(op.get("scale", (1.0, 1.0, 1.0))),
                    is_active=bool(op.get("is_active", True)),
                    guid=op.get("guid"),
                )
                log.append(f"add-gameobject: {node.name} [{node.guid}]")
            elif kind == "remove-gameobject":
                node = remove_gameobject(target, op["guid"])
                log.append(f"remove-gameobject: {node.name} [{node.guid}]")
            elif kind == "move-gameobject":
                move_gameobject(target, guid=op["guid"], new_parent=op.get("new_parent"),
                                preserve_world_transform=bool(op.get("preserve_world_transform", True)))
                log.append(f"move-gameobject: {op['guid']} -> {op.get('new_parent')}")
            elif kind == "set-transform":
                pos = tuple(op["pos"]) if "pos" in op else None
                rot = tuple(op["rot"]) if "rot" in op else None
                scale = tuple(op["scale"]) if "scale" in op else None
                set_transform(target, op["guid"], pos=pos, rot=rot, scale=scale)
                log.append(f"set-transform: {op['guid']}")
            elif kind == "set-active":
                set_active(target, op["guid"], bool(op["is_active"]))
                log.append(f"set-active: {op['guid']} -> {op['is_active']}")
            elif kind == "rename-gameobject":
                rename_gameobject(target, op["guid"], op["name"])
                log.append(f"rename-gameobject: {op['guid']} -> {op['name']}")
            elif kind == "add-component":
                comp = add_component(target, op["guid"], op["type"],
                                     component_guid=op.get("component_guid"),
                                     params=op.get("params"))
                log.append(f"add-component: {op['guid']} += {comp.fqn}")
            elif kind == "remove-component":
                comp = remove_component(target, op["guid"], int(op["index"]))
                log.append(f"remove-component: {op['guid']} -= {comp.fqn} (#{op['index']})")
            elif kind == "set-component-params":
                touched = set_component_params(target, op["guid"], int(op["index"]), op["params"])
                log.append(f"set-component-params: {op['guid']}#{op['index']} -> {touched}")
            elif kind == "instantiate-prefab":
                from . import reader as _reader
                prefab = _reader.read_prefab_file(op["prefab_path"])
                node = instantiate_prefab(target, prefab, parent=op.get("parent"))
                log.append(f"instantiate-prefab: {op['prefab_path']} -> {node.name} [{node.guid}]")
            else:
                raise EditError(f"unknown op: {kind!r}")
        except Exception as e:  # noqa: BLE001
            raise EditError(f"op #{i} ({kind}): {e}") from e
    return log
