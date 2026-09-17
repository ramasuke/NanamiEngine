"""Edit component fields inside a ``gameobject.get_json`` document.

The engine has no runtime reflection, so parameter edits go through cereal JSON:
``gameobject.get_json`` -> patch here -> ``gameobject.set_json`` (which rebuilds the
GameObject). Layout of that document::

    {"gameObject": {"polymorphic_id": .., "polymorphic_name": "NanamiEngine::Scene::SceneGameObject",
                    "ptr_wrapper": {"id": .., "data": {
                        "value0": {...IGameObject base...}, "isActive_": .., "name_": .., "guid_": {"value_": ..},
                        "components_": {"componentCount": N,
                                        "component_0": {"polymorphic_id": .., "polymorphic_name": ..,
                                                        "ptr_wrapper": {"id": .., "data": {
                                                            "value0": {"guid_": {"value_": ..}, "isEnable_": ..},
                                                            "someField_": ..}}}},
                        "transform_": {...}}}}}

Base classes are positional (``value0``, ``value1``...), so a component's guid lives in
whichever base object holds both ``guid_`` and ``isEnable_`` (``ComponentBase``), and a
field may belong to a base. glm vectors are objects keyed ``value0..valueN``; a JSON list
of the same length is accepted as a shorthand for them.
"""

from __future__ import annotations

import re
from typing import Any

from tools.common import cereal_json as cj

_BASE_KEY = re.compile(r"value\d+$")
_META_KEYS = {"cereal_class_version", "polymorphic_id", "polymorphic_name"}
# ComponentBase::guid_ identifies the component for FIELD references and later edits
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
    """Find ``ComponentBase::guid_`` by walking the positional base objects."""
    pending = [data]
    while pending:
        node = pending.pop(0)
        guid = node.get("guid_")
        if isinstance(guid, cj.OrderedObj) and "isEnable_" in node and isinstance(guid.get("value_"), str):
            return guid["value_"]
        pending.extend(v for k, v in node.items() if _BASE_KEY.match(k) and isinstance(v, cj.OrderedObj))
    return None


def iter_components(data: cj.OrderedObj):
    """Yield ``(index, polymorphic_name or None, component data)`` for a GameObject's own components."""
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
    """Return ``(patched json text, changed field names)``. Unknown fields and type mismatches raise."""
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
    """Pretty cereal JSON of one component's data object (its fields, bases included)."""
    return cj.dumps(find_component(cj.loads(json_text), component_guid))
