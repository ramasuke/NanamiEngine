"""Static checks for a :class:`tools.scene.model.Scene` / :class:`Prefab`.

Mirrors ``tools.bt.validate``'s spirit: problems are reported as plain strings;
a ``note:``-prefixed line is informational (does not block a write), anything
else is a hard failure. Nothing here mutates the model.

:func:`validate_source_bytes` and :func:`validate_class_versions` work on the
file as it sits on disk rather than on the model, because the model has already
normalised away the two things they check.
"""

from __future__ import annotations

import json
from typing import Any, Optional

from tools.common.cereal_json import Num, OrderedObj, loads

from . import catalog as catalog_mod
from . import model

_VER = "cereal_class_version"
_MSB = 0x80000000
_EXACT = 0x40000000

#: Value types that carry a class version but are not components, keyed by the
#: exact member-name set cereal writes for them.
_VALUE_TYPE_SIGNATURES = {
    frozenset({"r_", "g_", "b_"}): "Color32",
}


def _walk(node: model.GameObjectNode, guids: dict[str, list[str]], path: str) -> None:
    guids.setdefault(node.guid, []).append(f"{path}/{node.name} (GameObject)")
    for comp in node.components:
        cguid = model.find_component_guid(comp)
        if cguid is not None:
            guids.setdefault(cguid, []).append(f"{path}/{node.name}:{comp.fqn}")
    for child in node.transform.children:
        _walk(child, guids, f"{path}/{node.name}")


def validate_scene(scene: model.Scene) -> list[str]:
    problems: list[str] = []
    guids: dict[str, list[str]] = {}
    for root in scene.roots:
        _walk(root, guids, "")
    for guid, owners in guids.items():
        if len(owners) > 1:
            problems.append(f"duplicate GUID {guid} used by: {', '.join(owners)}")
    return problems


def validate_prefab(prefab: model.Prefab) -> list[str]:
    problems: list[str] = []
    guids: dict[str, list[str]] = {}
    _walk(prefab.root, guids, "")
    for guid, owners in guids.items():
        if len(owners) > 1:
            problems.append(f"duplicate GUID {guid} used by: {', '.join(owners)}")
    for guid in prefab.copied_object_guids:
        if guid in guids:
            problems.append(
                f"copiedObjectGuidList_ entry {guid} collides with a GameObject/Component "
                f"GUID inside this same prefab"
            )
    return problems


# ---------------------------------------------------------------------------
# on-disk checks
# ---------------------------------------------------------------------------
def validate_source_bytes(raw: bytes) -> list[str]:
    """Checks that only hold on the exact bytes on disk.

    ``cereal_json.read_text`` strips a leading BOM, so a BOM'd file parses fine
    here and still dies in the engine: rapidjson does not skip one, so the
    document root never becomes an object and cereal reports
    ``rapidjson internal assertion failure: IsObject()``.
    """
    problems: list[str] = []
    body = raw
    if body[:3] == b"\xef\xbb\xbf":
        problems.append(
            "file starts with a UTF-8 BOM - the engine's rapidjson does not skip it and "
            "fails with \"IsObject()\"; write UTF-8 with no BOM (cereal_json.to_file_bytes)"
        )
        body = body[3:]
    try:
        text = body.decode("utf-8")
    except UnicodeDecodeError as exc:
        problems.append(f"not valid UTF-8: {exc}")
        return problems
    try:
        json.loads(text)
    except ValueError as exc:
        problems.append(f"not well-formed JSON: {exc}")
    return problems


def _as_int(value: Any) -> Optional[int]:
    if isinstance(value, Num):
        return value.value if value.is_int else None
    if isinstance(value, bool):
        return None
    return value if isinstance(value, int) else None


def _field_holder(block: Any) -> Optional[OrderedObj]:
    """The referenced holder's ``ptr_wrapper.data`` inside a FIELD block."""
    slot = block.get("value0") if isinstance(block, OrderedObj) else None
    wrapper = slot.get("ptr_wrapper") if isinstance(slot, OrderedObj) else None
    data = wrapper.get("data") if isinstance(wrapper, OrderedObj) else None
    return data if isinstance(data, OrderedObj) else None


class _ClassVersionAudit:
    """Replays cereal's once-per-type ``cereal_class_version`` bookkeeping.

    cereal writes the key on a type's FIRST occurrence in an archive (= one
    file) and never again, caching the value for the rest of the read. Getting
    that wrong is not cosmetic:

    * missing on a first occurrence -> the named lookup fails outright with
      ``provided NVP (cereal_class_version) not found``;
    * present on a repeat -> cereal never consumes it, so the next *positional*
      read (``base_class<>`` slots, ``Field<T>``'s context pointer) lands on
      that number and rapidjson asserts ``IsObject()``.

    Repeat occurrences of a component carry only a numeric ``polymorphic_id``,
    so types are resolved through cereal's id table exactly as the engine does -
    a name-only scan would skip every repeat, which is where these bugs hide.
    """

    def __init__(self, cat: catalog_mod.Catalog) -> None:
        self._cat = cat
        self._polymap: dict[int, str] = {}
        self._first: dict[str, str] = {}
        self._reads_positionally: dict[str, bool] = {}
        self.unmodelled: set[str] = set()
        self.problems: list[tuple[str, str]] = []

    @staticmethod
    def _first_read_is_positional(node: OrderedObj) -> bool:
        """True when this type's first member read is an unnamed ``valueN`` slot.

        Only then is a stray version key fatal: cereal skips the key (it already
        knows the version) and the positional read lands on that number.
        Types that start with a named read just leave the stray unvisited.
        """
        rest = [k for k in node.keys() if k != _VER]
        return bool(rest) and rest[0].startswith("value")

    def _check(self, type_key: str, node: OrderedObj, where: str) -> None:
        if type_key not in self._first:
            self._first[type_key] = where
            self._reads_positionally[type_key] = self._first_read_is_positional(node)
            if _VER not in node:
                self.problems.append((
                    "missing",
                    f"{where}: first occurrence of {type_key} has no {_VER} "
                    f"(the engine fails with \"provided NVP ({_VER}) not found\")",
                ))
        elif _VER in node:
            if self._reads_positionally.get(type_key):
                self.problems.append((
                    "stray",
                    f"{where}: repeat occurrence of {type_key} carries a stray {_VER} "
                    f"(first was {self._first[type_key]}); cereal leaves it unread and the "
                    f"next positional read descends into it -> \"IsObject()\"",
                ))
            else:
                self.problems.append((
                    "note",
                    f"note: {where}: repeat occurrence of {type_key} carries a stray "
                    f"{_VER} (first was {self._first[type_key]}); harmless here because "
                    f"{type_key} starts with a named read, but the engine never writes it",
                ))

    def _check_component(self, fqn: str, data: OrderedObj, where: str) -> None:
        self._check(fqn, data, where)
        if fqn in self._cat.gameobject_shapes:
            # GameObjects are versioned polymorphic nodes like components, but
            # their base chain (IGameObject -> IObject) appears nowhere else, so
            # skipping it consistently costs no first-occurrence information.
            return
        entry = self._cat.component_by_fqn(fqn)
        if entry is None:
            self.unmodelled.add(fqn)
            return
        self._check_bases(entry, data, where)
        for param in entry.get("params", []):
            if param.get("shape") != "field":
                continue
            block = data.get(param["key"])
            if isinstance(block, OrderedObj):
                self._check_field(param.get("type") or "?", block,
                                  f"{where}.{param['key']}")
            elif isinstance(block, list):
                # std::vector<FIELD(T)>: cereal reads each element as its own Field<T>, in order
                for i, element in enumerate(block):
                    if isinstance(element, OrderedObj):
                        self._check_field(param.get("type") or "?", element,
                                          f"{where}.{param['key']}[{i}]")

    def _check_bases(self, entry: dict, data: OrderedObj, where: str) -> None:
        bases = entry.get("bases", [])
        present = [b for b in bases if b["key"] in data]
        # Some components gate whole base_class<> slots on the version
        # (ShakeCameraBehaviour), which shifts what each valueN slot holds.
        # Only audit when every slot is present, so the leaf names line up.
        if len(present) != len(bases):
            return
        for base in bases:
            node = data.get(base["key"])
            if isinstance(node, OrderedObj):
                self._check_base(base["leaf"], node, f"{where}.{base['key']}")

    def _check_base(self, leaf: str, node: OrderedObj, where: str) -> None:
        # Key on the leaf: several bases have no fqn in the catalog and would
        # otherwise collapse into one type.
        info = self._cat.base_info(leaf)
        if info is None:
            return
        self._check(leaf, node, where)
        own = info.get("bases", [])
        if any(b["key"] not in node for b in own):
            return
        for base in own:
            child = node.get(base["key"])
            if isinstance(child, OrderedObj):
                self._check_base(base["leaf"], child, f"{where}.{base['key']}")

    def _check_field(self, field_type: str, block: OrderedObj, where: str) -> None:
        self._check(f"Field<{field_type}>", block, where)
        holder = _field_holder(block)
        if holder is not None:
            self._check(f"FieldHolder<{field_type}>", holder, f"{where}.value0")

    def run(self, node: Any, where: str) -> None:
        if isinstance(node, OrderedObj):
            self._visit_pointer(node, where)
            self._visit_value_type(node, where)
            counts: dict[str, int] = {}
            for key, value in node.items():
                counts[key] = counts.get(key, 0) + 1
                label = key if counts[key] == 1 else f"{key}#{counts[key] - 1}"
                self.run(value, f"{where}/{label}")
        elif isinstance(node, list):
            for index, value in enumerate(node):
                self.run(value, f"{where}/[{index}]")

    def _visit_pointer(self, node: OrderedObj, where: str) -> None:
        raw_id = _as_int(node.get("polymorphic_id"))
        if raw_id is None or raw_id in (0, _EXACT):
            return
        if raw_id & _MSB:
            fqn = node.get("polymorphic_name")
            if isinstance(fqn, str):
                self._polymap[raw_id & ~_MSB] = fqn
        else:
            fqn = self._polymap.get(raw_id)
        wrapper = node.get("ptr_wrapper")
        data = wrapper.get("data") if isinstance(wrapper, OrderedObj) else None
        if isinstance(fqn, str) and isinstance(data, OrderedObj):
            self._check_component(fqn, data, where)

    def _visit_value_type(self, node: OrderedObj, where: str) -> None:
        if any(isinstance(v, (OrderedObj, list)) for _k, v in node.items()):
            return
        signature = frozenset(k for k in node.keys() if k != _VER)
        leaf = _VALUE_TYPE_SIGNATURES.get(signature)
        if leaf is not None:
            self._check(leaf, node, where)


def validate_class_versions(text: str, cat: catalog_mod.Catalog) -> list[str]:
    """Audit ``cereal_class_version`` placement across a whole file."""
    audit = _ClassVersionAudit(cat)
    audit.run(loads(text), "")
    problems: list[str] = []
    # A component the catalog does not model hides its bases, so an occurrence
    # further down can look like the first one when it is not. Strays are still
    # sound (they need a sighting to report), but "missing" findings become
    # guesses, so report them as notes and say why.
    blind = bool(audit.unmodelled)
    for kind, message in audit.problems:
        if kind == "missing" and blind:
            problems.append("note: " + message + " - unverified, see the note below")
        else:
            problems.append(message)
    if blind:
        problems.append(
            "note: these types are not in the catalog, so their base slots were not "
            "audited (run regen-catalog; some are unreachable by the v1 scanner): "
            + ", ".join(sorted(audit.unmodelled))
        )
    return problems
