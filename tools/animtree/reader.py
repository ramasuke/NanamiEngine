"""cereal-JSON テキスト  ->  :class:`tools.animtree.model.Tree`。

アーカイブの管理情報（polymorphic id、ptr_wrapper id、cereal_class_version）に
寛容: ノード/遷移/条件/パラメータの構造はモデルに復号し、ノードの guid/position 以外の
フィールドはタグ付きの :mod:`tools.common.blob` にして、writer が管理情報を正確に
再構築できるようにする。

「純粋な木」のアーカイブ（すべての ``ptr_wrapper`` が新しいデータを書く）のみ対応し、
後方参照があれば :class:`PureTreeError` を送出する。``nodes_``/遷移リストは
新規所有オブジェクトの単純な配列なので（遷移はノードを GUID の *値* で参照し、
cereal のポインタ同一性は使わない - model.py 参照）、エンジンが書く実際の
``.animTree`` はすべてこれに当てはまる。
"""

from __future__ import annotations

from typing import Any, Optional

from tools.common.blob import Ptr, Ver, fingerprint
from tools.common.cereal_json import Num, OrderedObj, loads, read_text

from . import catalog as catalog_mod
from . import model

EXACT_PID = 0x40000000
FIRST_BIT = 0x80000000


class PureTreeError(RuntimeError):
    pass


class _Ctx:
    """パースごとの状態: アーカイブのポリモーフィック型テーブル。"""

    def __init__(self, cat: catalog_mod.Catalog) -> None:
        self.cat = cat
        self.poly: dict[int, str] = {}
        # cereal は型の cereal_class_version をアーカイブ内で最初に現れたときだけ書く。
        # 以降の同じ型のインスタンスは同じバージョンで保存されている
        self.node_versions: dict[str, int] = {}

    def ptr_slot(self, slot: OrderedObj):
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
                    "- this file is a DAG, which tools/animtree does not support"
                )
        elif "valid" in pw:
            wrapper = "unique"
        else:
            raise PureTreeError(f"ptr_wrapper without id/valid: {pw!r}")
        return dict(null=False, exact=exact, fqn=fqn, wrapper=wrapper, data=pw["data"])


def _num(v: Any) -> int:
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


def _guid_value(obj: OrderedObj) -> str:
    _v, body = _strip_ccv(obj)
    return body["value_"]


def _vec2(obj: OrderedObj) -> tuple[Any, Any]:
    # loads() が作った生の Num オブジェクトのまま保持する（素の Python float に
    # 変換しない）ので、未編集の値は *元の* リテラル表記のまま往復する。
    # RapidJSON の Grisu2 浮動小数点出力と Python の repr() は同じ double の
    # 最下位桁で食い違うことがあり（tools/common/cereal_json.py のモジュール
    # docstring 参照）、そうしないと派生した（整数でない）座標の
    # バイト一致の往復が壊れる。
    return (obj["value0"], obj["value1"])


def _typed(kind: str, raw: Any) -> Any:
    # int/float: `raw` はすでに loads() が作った Num なのでそのまま保持する
    # （_vec2 参照）。素の Python 数値には戻さない。
    if kind == "bool":
        return bool(raw)
    if kind in ("int", "float"):
        return raw
    raise ValueError(f"unknown kind {kind!r}")


# ---------------------------------------------------------------------------
# 汎用の値タグ付け（tools.bt.reader._tag_value / _tag_field と同じ）
# ---------------------------------------------------------------------------
def _tag_value(ctx: _Ctx, val: Any, pinfo: Optional[dict]) -> Any:
    if isinstance(val, (Num, str, bool)) or val is None:
        return val
    if isinstance(val, list):
        return [_tag_value(ctx, x, None) for x in val]
    if not isinstance(val, OrderedObj):
        return val

    keys = val.keys()

    if pinfo and pinfo.get("shape") == "field":
        return _tag_field(ctx, val, pinfo.get("type", "?"))

    if "polymorphic_id" in keys:
        s = ctx.ptr_slot(val)
        if s["null"]:
            return Ptr(exact=False, null=True, fqn=None, wrapper="", data=None)
        return Ptr(
            exact=s["exact"], null=False, fqn=s["fqn"], wrapper=s["wrapper"],
            data=_tag_value(ctx, s["data"], None),
        )

    if keys and keys[0] == "cereal_class_version":
        v, body = _strip_ccv(val)
        if _is_guid_obj(val):
            return Ver(("type", "Guid"), v or 0, OrderedObj([("value_", body["value_"])]))
        return Ver(("fp", fingerprint(val)), v or 0, _tag_plain(ctx, body))

    if _is_guid_obj(val):
        return OrderedObj([("value_", val["value_"])])

    return _tag_plain(ctx, val)


def _tag_plain(ctx: _Ctx, obj: OrderedObj) -> OrderedObj:
    return OrderedObj((k, _tag_value(ctx, v, None)) for k, v in obj.items())


def _tag_field(ctx: _Ctx, val: OrderedObj, ftype: str) -> Ver:
    outer_v, outer_body = _strip_ccv(val)
    inner = outer_body["value0"]                       # ptr スロット（exact） - FieldContext<T>
    s = ctx.ptr_slot(inner)
    holder_data = s["data"]
    holder_v, holder_body = _strip_ccv(holder_data)
    guid_obj = holder_body["value0"]
    if isinstance(guid_obj, OrderedObj) and _is_guid_obj(guid_obj):
        guid_v, _gb = _strip_ccv(guid_obj)
        guid_tagged = Ver(("type", "Guid"), guid_v or 0, OrderedObj([("value_", guid_obj["value_"])]))
    else:
        guid_tagged = _tag_value(ctx, guid_obj, None)
    holder = Ver(("type", f"FieldHolder<{ftype}>"), holder_v or 0,
                 OrderedObj([("value0", guid_tagged)]))
    ptr = Ptr(exact=True, null=False, fqn=None, wrapper="shared", data=holder)
    return Ver(("type", f"Field<{ftype}>"), outer_v or 0, OrderedObj([("value0", ptr)]))


# ---------------------------------------------------------------------------
# ノード
# ---------------------------------------------------------------------------
def _is_base_stub(key: str, val: Any) -> bool:
    """各ノードの ``save()`` が最初に行う名前なしの
    ``archive(cereal::base_class<IAnimationNode>(this))`` 呼び出し。キーは常に "value0" で
    カタログのパラメータではなく、中身は常に ``{}`` か ``{"cereal_class_version": N}``。"""
    return key == "value0" and isinstance(val, OrderedObj) and \
        all(kk == "cereal_class_version" for kk in val.keys())


def _read_node(ctx: _Ctx, slot: OrderedObj, *, expected_fqn: Optional[str] = None) -> model.Node:
    s = ctx.ptr_slot(slot)
    fqn = s["fqn"] or expected_fqn
    if fqn is None:
        raise ValueError("node slot has no resolvable type")
    entry = ctx.cat.node_by_fqn(fqn)
    if entry is None:
        raise ValueError(f"unknown node type: {fqn!r} (regen-catalog?)")

    data = s["data"]
    v, data = _strip_ccv(data)
    if v is not None:
        ctx.node_versions[fqn] = v
    class_version = ctx.node_versions.get(fqn, int(entry.get("version", 0)))

    guid: Optional[str] = None
    pos: tuple[float, float] = (0.0, 0.0)
    params = OrderedObj()
    for key, val in data.items():
        if _is_base_stub(key, val):
            vv, _ = _strip_ccv(val)
            params.append(key, Ver(("type", "IAnimationNode"), vv if vv is not None else 0, OrderedObj()))
            continue
        pinfo = ctx.cat.param_by_key(entry, key)
        shape = pinfo.get("shape") if pinfo else None
        if shape == "self_guid":
            guid = _guid_value(val)
            continue
        if shape == "self_pos":
            pos = _vec2(val)
            continue
        params.append(key, _tag_value(ctx, val, pinfo))

    if guid is None:
        raise ValueError(f"node of type {fqn!r} has no guid_ field (regen-catalog?)")
    return model.Node(guid=guid, pos=pos, type_fqn=fqn, class_version=class_version, params=params)


# ---------------------------------------------------------------------------
# 条件 / 遷移
# ---------------------------------------------------------------------------
def _read_conditions(ctx: _Ctx, slot: OrderedObj) -> list[model.Condition]:
    data = slot["ptr_wrapper"]["data"]              # unique_ptr の形: {"valid":1, "data":...}
    _v, data = _strip_ccv(data)
    count = int(_num(data["value0"]))
    out: list[model.Condition] = []
    for i in range(1, count + 1):
        s = ctx.ptr_slot(data[f"value{i}"])
        kind = ctx.cat.kind_by_condition_fqn(s["fqn"])
        if kind is None:
            raise ValueError(f"unknown condition type: {s['fqn']!r}")
        _v2, body = _strip_ccv(s["data"])
        # body["value0"] は IAnimationNodePathAdditionCondition 基底のスタブなので読み飛ばす
        out.append(model.Condition(name=body["name_"], kind=kind, value=_typed(kind, body["equalValue_"])))
    return out


def _read_transition(ctx: _Ctx, slot: OrderedObj) -> model.Transition:
    s = ctx.ptr_slot(slot)
    data = s["data"]
    _v, data = _strip_ccv(data)
    conditions = _read_conditions(ctx, data["additionConditionGroup_"])
    duration = data["transitionDuration_secs_"]  # 生の Num - _vec2 参照
    from_guid = _guid_value(data["fromNodeGuid_"])
    next_guid = _guid_value(data["nextNodeGuid_"])
    visual_from_guid = _guid_value(data["visualFromNodeGuid_"]) if "visualFromNodeGuid_" in data else from_guid
    return model.Transition(from_guid=from_guid, next_guid=next_guid, visual_from_guid=visual_from_guid,
                            duration_secs=duration, conditions=conditions)


def _read_params(ctx: _Ctx, slot: OrderedObj) -> list[model.Param]:
    # ParameterGroup は IObject 派生ではない（仮想基底なし）。save()/load() は
    # バージョン引数を取らないので、このファイルの他の shared_ptr と違い、
    # この階層には polymorphic_id のラッパーも取り除くべき
    # cereal_class_version も無い。
    data = slot["ptr_wrapper"]["data"]
    count = int(_num(data["value0"]))
    out: list[model.Param] = []
    for i in range(1, count + 1):
        s = ctx.ptr_slot(data[f"value{i}"])
        kind = ctx.cat.kind_by_param_fqn(s["fqn"])
        if kind is None:
            raise ValueError(f"unknown parameter type: {s['fqn']!r}")
        _v, body = _strip_ccv(s["data"])
        out.append(model.Param(name=body["name_"], kind=kind, value=_typed(kind, body["value_"])))
    return out


# ---------------------------------------------------------------------------
def read_tree(text: str, cat: catalog_mod.Catalog | None = None) -> model.Tree:
    cat = cat or catalog_mod.load()
    ctx = _Ctx(cat)
    root = loads(text)
    if "entryNode" not in root or "nodesCount" not in root:
        raise ValueError("not a .animTree file: missing entryNode/nodesCount")

    params = _read_params(ctx, root["additionParameters_"]) if "additionParameters_" in root else []
    entry = _read_node(ctx, root["entryNode"], expected_fqn=model.FQN_ENTRY_NODE)
    any_state = _read_node(ctx, root["visualAnyStateNode"], expected_fqn=model.FQN_ANYSTATE_NODE)

    count = int(_num(root["nodesCount"]))
    nodes = [_read_node(ctx, root[f"nodes_{i}"]) for i in range(count)]

    path_count = int(_num(root["fromNodeNodePathCount"])) if "fromNodeNodePathCount" in root else 0
    transitions = [_read_transition(ctx, root[f"fromNodeNodePath_{i}"]) for i in range(path_count)]

    any_path_count = (int(_num(root["fromAnyStateNodeNodePathCount"]))
                      if "fromAnyStateNodeNodePathCount" in root else 0)
    any_state_transitions = [_read_transition(ctx, root[f"fromAnyStateNodeNodePath_{i}"])
                             for i in range(any_path_count)]

    return model.Tree(entry=entry, any_state=any_state, nodes=nodes, transitions=transitions,
                      any_state_transitions=any_state_transitions, params=params)


def read_tree_file(path) -> model.Tree:
    return read_tree(read_text(path))
