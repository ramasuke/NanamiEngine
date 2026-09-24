""":class:`tools.bt.model.Tree` に対する構造編集とパラメータ編集。

どの関数もツリーをその場で変更し、変更の後にその逆操作を行うと元のバイト列に
戻るように作られている（selftest ステージ 6 参照）。``apply`` は編集操作の一括を
アトミックに実行する（構築 -> 全適用 -> 呼び出し側で検証 -> 1回書き込み）。
"""

from __future__ import annotations

import copy
from typing import Any, Optional

from . import catalog as catalog_mod
from . import model
from .blob import Ptr, Ver
from .cereal_json import Num, OrderedObj

EMPTY_GUID = "00000000-0000-0000-0000-000000000000"

_KIND_ALIASES = {
    "selector": model.Selector,
    "sequence": model.Sequence,
    "random": model.RandomSelector,
    "random-selector": model.RandomSelector,
    "once-exec": model.OnceExecute,
    "once-execute": model.OnceExecute,
    "once-success": model.OnceSuccess,
    "action": model.Action,
}


class EditError(RuntimeError):
    pass


# ---------------------------------------------------------------------------
# blob ビルダー（action を一から生成）
# ---------------------------------------------------------------------------
def _leaf(name: str) -> str:
    return name.split("<", 1)[0].rsplit("::", 1)[-1]


def _default_scalar(shape: str, default: Any):
    if shape in ("int", "enum"):
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


def _field_blob(field_type_leaf: str, guid: str = EMPTY_GUID) -> Ver:
    guid_ver = Ver(("type", "Guid"), 0, OrderedObj([("value_", guid)]))
    holder = Ver(("type", f"FieldHolder<{field_type_leaf}>"), 0,
                 OrderedObj([("value0", guid_ver)]))
    ptr = Ptr(exact=True, null=False, fqn=None, wrapper="shared", data=holder)
    return Ver(("type", f"Field<{field_type_leaf}>"), 0, OrderedObj([("value0", ptr)]))


def _param_blob(cat: catalog_mod.Catalog, pinfo: dict) -> Any:
    shape = pinfo.get("shape")
    if shape in ("int", "float", "bool", "string", "enum"):
        return _default_scalar(shape, pinfo.get("default"))
    if shape == "vec2":
        return _vec(2)
    if shape == "vec3":
        return _vec(3)
    if shape == "quat":
        return _vec(4)
    if shape == "field":
        return _field_blob(_leaf(pinfo.get("type", "?")))
    if shape == "vector":
        return []
    if shape == "nested":
        leaf = _leaf(pinfo.get("type", "?"))
        sub = cat.type_by_leaf(leaf)
        body = OrderedObj()
        if cat.action_by_leaf(leaf):  # ネストした型が ActionBase を継承
            body.append("value0", Ver(("type", "ActionBase"), 0, OrderedObj()))
        for sp in cat.params_of(sub):
            body.append(sp["key"], _param_blob(cat, sp))
        return Ver(("type", leaf), int((sub or {}).get("version", 0)), body)
    # 不明 -> ファイルがロードできるよう 0 を出力する。validate が指摘する
    return Num.of_int(0)


def action_blob(cat: catalog_mod.Catalog, entry: dict) -> OrderedObj:
    out = OrderedObj()
    out.append("value0", Ver(("type", "ActionBase"), 0, OrderedObj()))
    for p in cat.params_of(entry):
        out.append(p["key"], _param_blob(cat, p))
    return out


# ---------------------------------------------------------------------------
# 探索
# ---------------------------------------------------------------------------
def _find_parent(tree: model.Tree, guid: str):
    """ノード `guid` の (parent_node, container_list_or_None, index) を返す。"""
    if tree.entry.child is not None and tree.entry.child.guid == guid:
        return tree.entry, None, 0
    for node, parent, _c, _i in tree.walk():
        kids = model.children_of(parent) if parent is not tree.entry else []
        for i, k in enumerate(kids):
            if k.guid == guid:
                return parent, kids, i
    raise EditError(f"node not found: {guid}")


def _resolve_parent(tree: model.Tree, guid: str):
    if guid in ("entry", tree.entry.guid):
        return tree.entry
    n = tree.find(guid)
    if n is None:
        raise EditError(f"parent not found: {guid}")
    return n


# ---------------------------------------------------------------------------
# 構造編集
# ---------------------------------------------------------------------------
def _make_node(kind: str, *, name: Optional[str], action_type: Optional[str],
               pos, node_guid: str, cat: catalog_mod.Catalog):
    cls = _KIND_ALIASES.get(kind)
    if cls is None:
        raise EditError(f"unknown node kind: {kind!r}")
    pos = tuple(pos) if pos else (0.0, 0.0)
    if cls is model.Action:
        if not action_type:
            raise EditError("an action node needs --type")
        entry = cat.resolve_action(action_type)
        if entry is None:
            raise EditError(f"unknown action type: {action_type!r} (see: regen-catalog)")
        return model.Action(guid=node_guid, pos=pos, name=name or entry["class"],
                            type_fqn=entry["fqn"], action_version=int(entry.get("version", 0)),
                            params=action_blob(cat, entry))
    if cls is model.RandomSelector:
        return model.RandomSelector(guid=node_guid, pos=pos)
    if cls in (model.Selector, model.Sequence):
        return cls(guid=node_guid, pos=pos)
    if cls is model.OnceExecute:
        return model.OnceExecute(guid=node_guid, pos=pos)
    return model.OnceSuccess(guid=node_guid, pos=pos)


def _attach(parent, node, index: Optional[int], weight: int) -> None:
    if isinstance(parent, model.Entry):
        if parent.child is not None:
            raise EditError("entry already has a child; remove or move it first")
        parent.child = node
    elif isinstance(parent, (model.Selector, model.Sequence)):
        i = len(parent.children) if index is None else index
        parent.children.insert(i, node)
    elif isinstance(parent, model.RandomSelector):
        i = len(parent.children) if index is None else index
        parent.children.insert(i, node)
        parent.weights.insert(i, int(weight))
    elif isinstance(parent, (model.OnceExecute, model.OnceSuccess)):
        if parent.child is not None:
            raise EditError(f"{type(parent).__name__} already has a child")
        parent.child = node
    else:
        raise EditError(f"{type(parent).__name__} cannot take children")


def add_node(tree: model.Tree, *, parent_guid: str, kind: str, name: str | None = None,
             action_type: str | None = None, index: int | None = None, weight: int = 100,
             pos=None, node_guid: str | None = None,
             cat: catalog_mod.Catalog | None = None):
    cat = cat or catalog_mod.load()
    from . import meta as _meta
    node = _make_node(kind, name=name, action_type=action_type, pos=pos,
                      node_guid=node_guid or _meta.mint_guid(), cat=cat)
    _attach(_resolve_parent(tree, parent_guid), node, index, weight)
    return node


def _clone_node(node, mint):
    """ノードのサブツリーをディープコピーし、各階層で新しい guid を発行する
    （"pure tree" 形式では2つのノードが guid を共有できない）。各 Action の
    パラメータ blob もディープコピーするので、複製を編集しても元は変わらない。"""
    if isinstance(node, model.Action):
        return model.Action(guid=mint(), pos=node.pos, name=node.name,
                            type_fqn=node.type_fqn, action_version=node.action_version,
                            params=copy.deepcopy(node.params))
    if isinstance(node, model.RandomSelector):
        return model.RandomSelector(guid=mint(), pos=node.pos,
                                    children=[_clone_node(c, mint) for c in node.children],
                                    weights=list(node.weights))
    if isinstance(node, (model.Selector, model.Sequence)):
        return type(node)(guid=mint(), pos=node.pos,
                          children=[_clone_node(c, mint) for c in node.children])
    if isinstance(node, model.OnceExecute):
        return model.OnceExecute(guid=mint(), pos=node.pos, state=node.state,
                                 child=_clone_node(node.child, mint) if node.child else None)
    if isinstance(node, model.OnceSuccess):
        return model.OnceSuccess(guid=mint(), pos=node.pos,
                                 child=_clone_node(node.child, mint) if node.child else None)
    raise EditError(f"cannot copy a node of type {type(node).__name__}")


def copy_node(tree: model.Tree, *, src_guid: str, parent_guid: str,
             index: int | None = None, weight: int = 100, pos=None):
    """``src_guid`` のサブツリーをディープコピーし、複製を ``parent_guid`` の下に付ける。
    主な用途は2つ: `RandomSelector` の重み付き分岐を1つ複製して一部フィールドだけ
    変える（`add-node` でノードごとに作り直す代わりに）ことと、既存の分岐から
    別の分岐（例: "enraged" 時の攻撃群）を丸ごと複製して、差分だけを後から手で書くこと。
    """
    src = tree.find(src_guid)
    if src is None:
        raise EditError(f"node not found: {src_guid}")
    if isinstance(src, model.Entry):
        raise EditError("cannot copy the entry node")
    from . import meta as _meta
    clone = _clone_node(src, _meta.mint_guid)
    if pos is not None:
        clone.pos = tuple(pos)
    _attach(_resolve_parent(tree, parent_guid), clone, index, weight)
    return clone


def _detach(tree: model.Tree, guid: str):
    parent, kids, idx = _find_parent(tree, guid)
    if kids is None:  # エントリーの子
        node = parent.child
        parent.child = None
        return node
    node = kids[idx]
    kids.pop(idx)
    if isinstance(parent, model.RandomSelector) and idx < len(parent.weights):
        parent.weights.pop(idx)
    if isinstance(parent, (model.OnceExecute, model.OnceSuccess)):
        parent.child = None
    return node


def remove_node(tree: model.Tree, guid: str):
    return _detach(tree, guid)


def move_node(tree: model.Tree, *, guid: str, parent_guid: str,
              index: int | None = None, weight: int = 100):
    node = _detach(tree, guid)
    _attach(_resolve_parent(tree, parent_guid), node, index, weight)
    return node


# ---------------------------------------------------------------------------
# パラメータ編集
# ---------------------------------------------------------------------------
def _coerce(shape: str, raw: str):
    if shape in ("int", "enum"):
        return Num.of_int(int(raw, 0))
    if shape == "float":
        return Num.of_float(float(raw))
    if shape == "bool":
        return raw.strip().lower() in ("1", "true", "yes", "on")
    if shape == "string":
        return raw
    if shape in ("vec2", "vec3", "quat"):
        parts = [p.strip() for p in raw.replace(" ", ",").split(",") if p.strip()]
        return OrderedObj([(f"value{i}", Num.of_float(float(x))) for i, x in enumerate(parts)])
    if shape == "field":
        return raw.strip().upper()
    raise EditError(f"cannot set a param of shape {shape!r}")


def _unwrap_nested(val):
    """shape='nested' パラメータの現在値の、書き込み可能なフィールドコンテナ。
    この出現がどちらの表現でも扱う: ``Ver`` でラップされた形（このスロット自体が
    ``cereal_class_version`` を明示的に持つ。:func:`add_node` で新しく雛形生成した
    構造体は常にこれ）か、素の ``OrderedObj``（このファイルでバージョン追跡されない
    構造体型の既存の出現。reader.py の ``_tag_value`` 参照）。どちらも同一に
    ラウンドトリップし、ここでは次に添字アクセスする dict を選ぶだけ。
    """
    return val.body if isinstance(val, Ver) else val


def _set_field_guid(val, guid: str) -> None:
    """shape='field' パラメータの guid を、このスロットがどちらの表現でも書き込む:
    トップレベルのカタログ型メンバーが持つ完全な ``Ver`` タグ付き形
    （``Ver -> Ptr -> Ver -> Ver``）か、素のタグ付け（pinfo を失った）既存構造体の
    2階層以上内側に埋もれた FIELD(T) が退避する、平坦化された Ptr のみの形
    （``Ptr -> OrderedObj -> OrderedObj``）。2つの形がある理由は reader.py の
    ``_tag_value``/``_tag_plain`` を参照。
    """
    outer = val.body if isinstance(val, Ver) else val
    holder = outer["value0"].data
    inner = holder.body if isinstance(holder, Ver) else holder
    guid_slot = inner["value0"]
    target = guid_slot.body if isinstance(guid_slot, Ver) else guid_slot
    target["value_"] = guid


def _leaf_param(cat: catalog_mod.Catalog, entry: Optional[dict], key: str) -> dict:
    for p in cat.params_of(entry):
        if key in (p["key"], p["member"]):
            return p
    type_name = (entry or {}).get("class", entry)
    raise EditError(f"{type_name}: no param {key!r} (see: show / validate)")


def _set_nested_param(cat: catalog_mod.Catalog, node: model.Action, entry: Optional[dict],
                      dotted_key: str, raw: str) -> str:
    """ドット区切りのパス（例: ``attackPower_.value_`` や
    ``spawnPosition_.targetObject_``）を1つ以上の shape='nested' 構造体メンバー越しに
    解決してリーフに書き込む。新規追加ノードと既存ノードの両方で動く
    （:func:`_unwrap_nested` / :func:`_set_field_guid` 参照）。
    """
    parts = dotted_key.split(".")
    cur_entry = entry
    container = node.params
    for i, part in enumerate(parts):
        pinfo = _leaf_param(cat, cur_entry, part)
        jkey = pinfo["key"]
        if jkey not in container:
            raise EditError(f"param {jkey!r} missing from the stored blob; regen-catalog?")
        val = container[jkey]
        if i == len(parts) - 1:
            shape = pinfo["shape"]
            if shape == "field":
                _set_field_guid(val, _coerce("field", raw))
            elif shape in catalog_mod.SETTABLE_SHAPES:
                container[jkey] = _coerce(shape, raw)
            else:
                raise EditError(
                    f"{node.type_name}: {dotted_key} has shape {shape!r}; not settable"
                )
            return pinfo["member"]
        if pinfo["shape"] != "nested":
            raise EditError(
                f"{node.type_name}: {'.'.join(parts[:i + 1])} is not a nested struct "
                f"(shape {pinfo['shape']!r}); cannot descend into it"
            )
        container = _unwrap_nested(val)
        cur_entry = cat.type_by_leaf(pinfo.get("type", "?"))
        if cur_entry is None:
            raise EditError(f"unknown nested struct type {pinfo.get('type')!r}")
    raise EditError("empty dotted key")  # 到達不能: dotted_key.split は常に1要素以上


def set_params(tree: model.Tree, guid: str, assignments: dict[str, str],
               cat: catalog_mod.Catalog | None = None) -> list[str]:
    """action のパラメータを1つ以上設定する。

    素のキー（``rate_``）は従来どおり、直接設定可能なトップレベルのパラメータを設定する。
    ドット区切りのキー（``attackPower_.value_``、``spawnPosition_.targetObject_``、
    ``spawnPosition_.offset_``）は shape='nested' の構造体メンバーの内側に届く。
    例えば ``PhysicsAttack``/``RadiateProjectile``/``PlayAnimation`` に埋め込まれた
    ``PhysicsPower``/``Position``/``WriteBlackBoard``/``WaitSeconds``/``PlaySE``
    構造体で、素のキーでは指定できない（nested 自体の shape は
    ``SETTABLE_SHAPES`` に含まれない）。
    """
    cat = cat or catalog_mod.load()
    node = tree.find(guid)
    if not isinstance(node, model.Action):
        raise EditError(f"{guid} is not an action node")
    entry = cat.action_by_fqn(node.type_fqn)
    touched: list[str] = []
    for key, raw in assignments.items():
        if "." in key:
            touched.append(_set_nested_param(cat, node, entry, key, raw))
            continue
        pinfo = _leaf_param(cat, entry, key)
        shape = pinfo["shape"]
        if shape not in catalog_mod.SETTABLE_SHAPES:
            raise EditError(
                f"{node.type_name}.{pinfo['member']} has shape {shape!r}; tools/bt v1 "
                f"cannot set it directly - reach inside it with a dotted key "
                f"(e.g. {key}.<field>_) or edit the .enemyBehaviourData by hand"
            )
        jkey = pinfo["key"]
        if jkey not in node.params:
            raise EditError(f"param {jkey!r} missing from the stored blob; regen-catalog?")
        if shape == "field":
            _set_field_guid(node.params[jkey], _coerce(shape, raw))
        else:
            node.params[jkey] = _coerce(shape, raw)
        touched.append(pinfo["member"])
    return touched


def set_weight(tree: model.Tree, *, child_guid: str | None = None,
               parent_guid: str | None = None, index: int | None = None,
               weight: int = 100) -> None:
    if child_guid is not None:
        parent, kids, idx = _find_parent(tree, child_guid)
    else:
        parent = _resolve_parent(tree, parent_guid)
        idx = index
    if not isinstance(parent, model.RandomSelector):
        raise EditError("set-weight only applies to a RandomSelector's children")
    if idx is None or not (0 <= idx < len(parent.weights)):
        raise EditError(f"weight index out of range: {idx}")
    parent.weights[idx] = int(weight)


def add_bb_param(tree: model.Tree, name: str, value: int) -> None:
    if any(p.name == name for p in tree.params):
        raise EditError(f"blackboard param {name!r} already exists")
    tree.params.append(model.BbParam(name=name, value=int(value), kind="int"))


def remove_bb_param(tree: model.Tree, name: str) -> None:
    before = len(tree.params)
    tree.params[:] = [p for p in tree.params if p.name != name]
    if len(tree.params) == before:
        raise EditError(f"no blackboard param named {name!r}")


# ---------------------------------------------------------------------------
# 一括処理
# ---------------------------------------------------------------------------
def apply(tree: model.Tree, ops: list[dict], cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    log: list[str] = []
    for i, op in enumerate(ops):
        kind = op.get("op")
        try:
            if kind == "add-node":
                n = add_node(tree, parent_guid=op["parent"], kind=op["kind"],
                             name=op.get("name"), action_type=op.get("type"),
                             index=op.get("index"), weight=int(op.get("weight", 100)),
                             pos=op.get("pos"), node_guid=op.get("guid"), cat=cat)
                log.append(f"add-node {op['kind']} -> {n.guid}")
            elif kind == "copy-node":
                n = copy_node(tree, src_guid=op["node"], parent_guid=op["parent"],
                             index=op.get("index"), weight=int(op.get("weight", 100)),
                             pos=op.get("pos"))
                log.append(f"copy-node {op['node']} -> {n.guid} (under {op['parent']})")
            elif kind == "remove-node":
                remove_node(tree, op["node"])
                log.append(f"remove-node {op['node']}")
            elif kind == "move-node":
                move_node(tree, guid=op["node"], parent_guid=op["parent"],
                          index=op.get("index"), weight=int(op.get("weight", 100)))
                log.append(f"move-node {op['node']} -> {op['parent']}")
            elif kind == "set-params":
                touched = set_params(tree, op["node"], op.get("set", {}), cat=cat)
                log.append(f"set-params {op['node']} {touched}")
            elif kind == "set-weight":
                set_weight(tree, child_guid=op.get("node"), parent_guid=op.get("parent"),
                           index=op.get("index"), weight=int(op["weight"]))
                log.append(f"set-weight {op.get('node') or op.get('parent')}")
            elif kind == "add-bb-param":
                add_bb_param(tree, op["name"], int(op["value"]))
                log.append(f"add-bb-param {op['name']}")
            elif kind == "remove-bb-param":
                remove_bb_param(tree, op["name"])
                log.append(f"remove-bb-param {op['name']}")
            else:
                raise EditError(f"unknown op {kind!r}")
        except Exception as e:  # noqa: BLE001
            raise EditError(f"op #{i} ({kind}): {e}") from e
    return log
