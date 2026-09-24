"""``gameobject.get_json`` のドキュメント内でコンポーネントのフィールドを編集する。

エンジンには実行時リフレクションが無いので、パラメータの編集は cereal JSON を通す:
``gameobject.get_json`` -> ここで書き換え -> ``gameobject.set_json`` (GameObject を
作り直す)。そのドキュメントのレイアウト::

    {"gameObject": {"polymorphic_id": .., "polymorphic_name": "NanamiEngine::Scene::SceneGameObject",
                    "ptr_wrapper": {"id": .., "data": {
                        "value0": {...IGameObject base...}, "isActive_": .., "name_": .., "guid_": {"value_": ..},
                        "components_": {"componentCount": N,
                                        "component_0": {"polymorphic_id": .., "polymorphic_name": ..,
                                                        "ptr_wrapper": {"id": .., "data": {
                                                            "value0": {"guid_": {"value_": ..}, "isEnable_": ..},
                                                            "someField_": ..}}}},
                        "transform_": {...}}}}}

基底クラスは位置で決まる (``value0``、``value1``...) ので、コンポーネントの guid は
``guid_`` と ``isEnable_`` の両方を持つ基底オブジェクト (``ComponentBase``) にあり、
フィールドが基底に属することもある。glm のベクトルは ``value0..valueN`` をキーとする
オブジェクトで、同じ長さの JSON リストも省略形として受け付ける。
"""

from __future__ import annotations

import re
from typing import Any

from tools.common import cereal_json as cj

_BASE_KEY = re.compile(r"value\d+$")
_META_KEYS = {"cereal_class_version", "polymorphic_id", "polymorphic_name"}
# ComponentBase::guid_ は FIELD 参照や後の編集でコンポーネントを識別する
_READ_ONLY_FIELDS = {"guid_"}


class PatchError(ValueError):
    pass


def _obj(value: Any, where: str) -> cj.OrderedObj:
    if not isinstance(value, cj.OrderedObj):
        raise PatchError(f"{where}: expected a JSON object")
    return value


def game_object_data(doc: Any) -> cj.OrderedObj:
    root = _obj(doc, "document").get("gameObject")
    if root is None:
        raise PatchError('document has no "gameObject" (pass the json from gameobject.get_json)')
    return _obj(_obj(_obj(root, "gameObject").get("ptr_wrapper"), "gameObject.ptr_wrapper").get("data"),
                "gameObject.ptr_wrapper.data")


def component_base_guid(data: cj.OrderedObj) -> str | None:
    """位置で決まる基底オブジェクトをたどって ``ComponentBase::guid_`` を探す。"""
    pending = [data]
    while pending:
        node = pending.pop(0)
        guid = node.get("guid_")
        if isinstance(guid, cj.OrderedObj) and "isEnable_" in node and isinstance(guid.get("value_"), str):
            return guid["value_"]
        pending.extend(v for k, v in node.items() if _BASE_KEY.match(k) and isinstance(v, cj.OrderedObj))
    return None


def iter_components(data: cj.OrderedObj):
    """GameObject 自身のコンポーネントごとに ``(index, polymorphic_name または None, コンポーネントのデータ)`` を yield する。"""
    group = _obj(data.get("components_"), "components_")
    count = group.get("componentCount")
    total = count.value if isinstance(count, cj.Num) else 0
    for i in range(int(total)):
        entry = _obj(group.get(f"component_{i}"), f"components_.component_{i}")
        wrapper = _obj(entry.get("ptr_wrapper"), f"component_{i}.ptr_wrapper")
        yield i, entry.get("polymorphic_name"), _obj(wrapper.get("data"), f"component_{i}.ptr_wrapper.data")


def find_component(doc: Any, component_guid: str) -> cj.OrderedObj:
    wanted = component_guid.strip().upper()
    seen = []
    for _i, _name, data in iter_components(game_object_data(doc)):
        guid = component_base_guid(data)
        seen.append(guid)
        if guid is not None and guid.upper() == wanted:
            return data
    raise PatchError(f"component {component_guid} is not on this GameObject (components: {seen})")


def _field_owner(data: cj.OrderedObj, key: str) -> cj.OrderedObj | None:
    if key in data and not _BASE_KEY.match(key):
        return data
    for k, v in data.items():
        if _BASE_KEY.match(k) and isinstance(v, cj.OrderedObj):
            owner = _field_owner(v, key)
            if owner is not None:
                return owner
    return None


def field_names(data: cj.OrderedObj) -> list[str]:
    names: list[str] = []
    for k, v in data.items():
        if k in _META_KEYS or k in _READ_ONLY_FIELDS:
            continue
        if _BASE_KEY.match(k) and isinstance(v, cj.OrderedObj):
            names.extend(n for n in field_names(v) if n not in names)
        elif k not in names:
            names.append(k)
    return names


def _positional_keys(obj: cj.OrderedObj) -> list[str] | None:
    keys = [k for k in obj.keys() if k not in _META_KEYS]
    if keys and keys == [f"value{i}" for i in range(len(keys))]:
        return keys
    return None


def _from_python(value: Any) -> Any:
    if isinstance(value, bool) or value is None or isinstance(value, str):
        return value
    if isinstance(value, int):
        return cj.Num.of_int(value)
    if isinstance(value, float):
        return cj.Num.of_float(value)
    if isinstance(value, dict):
        return cj.OrderedObj((k, _from_python(v)) for k, v in value.items())
    if isinstance(value, list):
        return [_from_python(v) for v in value]
    raise PatchError(f"unsupported value {value!r}")


def _coerce(existing: Any, new: Any, path: str) -> Any:
    if isinstance(existing, bool):
        if not isinstance(new, bool):
            raise PatchError(f"{path}: expected a bool, got {new!r}")
        return new
    if isinstance(existing, cj.Num):
        if isinstance(new, bool) or not isinstance(new, (int, float)):
            raise PatchError(f"{path}: expected a number, got {new!r}")
        if existing.is_int:
            if isinstance(new, float) and not new.is_integer():
                raise PatchError(f"{path}: expected an integer, got {new!r}")
            return cj.Num.of_int(int(new))
        return cj.Num.of_float(float(new))
    if isinstance(existing, str):
        if not isinstance(new, str):
            raise PatchError(f"{path}: expected a string, got {new!r}")
        return new
    if isinstance(existing, cj.OrderedObj):
        positional = _positional_keys(existing)
        if isinstance(new, list) and positional is not None:
            if len(new) != len(positional):
                raise PatchError(f"{path}: expected {len(positional)} values, got {len(new)}")
            new = dict(zip(positional, new))
        if not isinstance(new, dict):
            raise PatchError(f"{path}: expected an object with keys {[k for k in existing.keys() if k not in _META_KEYS]}, got {new!r}")
        for key, value in new.items():
            if key in _META_KEYS:
                raise PatchError(f"{path}.{key}: cereal metadata can't be edited")
            if key not in existing:
                raise PatchError(f"{path}.{key}: no such key (keys: {[k for k in existing.keys() if k not in _META_KEYS]})")
            existing[key] = _coerce(existing[key], value, f"{path}.{key}")
        return existing
    if isinstance(existing, list):
        if not isinstance(new, list) or len(new) != len(existing):
            raise PatchError(f"{path}: expected a list of {len(existing)} values")
        return [_coerce(e, n, f"{path}[{i}]") for i, (e, n) in enumerate(zip(existing, new))]
    if existing is None:
        return _from_python(new)
    raise PatchError(f"{path}: can't edit a value of type {type(existing).__name__}")


def set_component_params(json_text: str, component_guid: str, params: dict[str, Any]) -> tuple[str, list[str]]:
    """``(書き換え後の json テキスト, 変更したフィールド名)`` を返す。未知のフィールドや型の不一致は例外を投げる。"""
    if not isinstance(params, dict) or not params:
        raise PatchError("params must be a non-empty object of field name -> value")
    doc = cj.loads(json_text)
    data = find_component(doc, component_guid)
    for key, value in params.items():
        if key in _META_KEYS or _BASE_KEY.match(key):
            raise PatchError(f"{key}: not a field name (fields: {field_names(data)})")
        if key in _READ_ONLY_FIELDS:
            raise PatchError(f"{key}: the component guid can't be changed")
        owner = _field_owner(data, key)
        if owner is None:
            raise PatchError(f"{key}: no such field (fields: {field_names(data)})")
        owner[key] = _coerce(owner[key], value, key)
    return cj.dumps(doc), list(params)


def component_fields_json(json_text: str, component_guid: str) -> str:
    """1 つのコンポーネントのデータオブジェクト (フィールド、基底を含む) を整形した cereal JSON。"""
    return cj.dumps(find_component(cj.loads(json_text), component_guid))
