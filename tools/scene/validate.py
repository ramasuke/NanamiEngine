""":class:`tools.scene.model.Scene` / :class:`Prefab` の静的チェック。

``tools.bt.validate`` と同じ考え方: 問題は素の文字列として報告する。
``note:`` で始まる行は情報（書き込みを妨げない）で、それ以外はハードな失敗。
ここではモデルを変更しない。

:func:`validate_source_bytes` と :func:`validate_class_versions` はモデルではなく
ディスク上のファイルそのものを対象にする。チェック対象の 2 点はモデルではすでに
正規化されて消えているため。
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

#: クラスバージョンを持つがコンポーネントではない値型。cereal が書き出す
#: メンバー名の集合そのものをキーにする。
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
# ディスク上でのチェック
# ---------------------------------------------------------------------------
def validate_source_bytes(raw: bytes) -> list[str]:
    """ディスク上の正確なバイト列でしか成り立たないチェック。

    ``cereal_json.read_text`` は先頭の BOM を取り除くので、BOM 付きファイルはここでは
    問題なくパースできてもエンジンでは落ちる: rapidjson は BOM を読み飛ばさないため
    ドキュメントのルートがオブジェクトにならず、cereal が
    ``rapidjson internal assertion failure: IsObject()`` を報告する。
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
    """FIELD ブロック内で参照されている保持元の ``ptr_wrapper.data``。"""
    slot = block.get("value0") if isinstance(block, OrderedObj) else None
    wrapper = slot.get("ptr_wrapper") if isinstance(slot, OrderedObj) else None
    data = wrapper.get("data") if isinstance(wrapper, OrderedObj) else None
    return data if isinstance(data, OrderedObj) else None


class _ClassVersionAudit:
    """cereal の型ごと 1 回の ``cereal_class_version`` 管理を再現する。

    cereal はアーカイブ（= 1 ファイル）内で型が *最初に* 出現したときにこのキーを書き、
    以降は二度と書かず、読み込みの残りではその値をキャッシュする。ここを誤るのは
    見た目だけの問題ではない:

    * 最初の出現で欠けている -> 名前付きの検索がそのまま
      ``provided NVP (cereal_class_version) not found`` で失敗する。
    * 繰り返しの出現にある -> cereal はそれを消費しないため、次の *位置による*
      読み込み（``base_class<>`` スロット、``Field<T>`` のコンテキストポインタ）がその数値に
      当たり、rapidjson が ``IsObject()`` でアサートする。

    コンポーネントの繰り返しの出現は数値の ``polymorphic_id`` しか持たないので、
    型はエンジンと同じく cereal の id 表から解決する。名前だけで走査すると繰り返しを
    すべて読み飛ばしてしまい、まさにそこにこうしたバグが潜む。
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
        """この型の最初のメンバー読み込みが名前なしの ``valueN`` スロットなら True。

        その場合に限り、余分なバージョンキーは致命的になる: cereal はそのキーを読み飛ばし
        （バージョンはすでに分かっている）、位置による読み込みがその数値に当たる。
        名前付きの読み込みから始まる型では、余分なキーが読まれずに残るだけ。
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
            # GameObject もコンポーネントと同様にバージョン付きのポリモーフィックノードだが、
            # その基底チェーン（IGameObject -> IObject）は他のどこにも現れないので、
            # 一貫して読み飛ばしても最初の出現に関する情報は失われない。
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
                # std::vector<FIELD(T)>: cereal は各要素をそれぞれ独立した Field<T> として順に読む
                for i, element in enumerate(block):
                    if isinstance(element, OrderedObj):
                        self._check_field(param.get("type") or "?", element,
                                          f"{where}.{param['key']}[{i}]")

    def _check_bases(self, entry: dict, data: OrderedObj, where: str) -> None:
        bases = entry.get("bases", [])
        present = [b for b in bases if b["key"] in data]
        # 一部のコンポーネントは base_class<> スロット全体をバージョンで切り替える
        # （ShakeCameraBehaviour）ため、各 valueN スロットの中身がずれる。
        # 葉の名前が揃うよう、全スロットがそろっているときだけ監査する。
        if len(present) != len(bases):
            return
        for base in bases:
            node = data.get(base["key"])
            if isinstance(node, OrderedObj):
                self._check_base(base["leaf"], node, f"{where}.{base['key']}")

    def _check_base(self, leaf: str, node: OrderedObj, where: str) -> None:
        # 葉をキーにする: カタログに fqn のない基底がいくつかあり、
        # そうしないと 1 つの型にまとまってしまう。
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
    """ファイル全体で ``cereal_class_version`` の配置を監査する。"""
    audit = _ClassVersionAudit(cat)
    audit.run(loads(text), "")
    problems: list[str] = []
    # カタログがモデル化していないコンポーネントは基底を隠すので、後方の出現が
    # 実際にはそうでないのに最初の出現に見えることがある。余分なキーの指摘は依然として
    # 確か（報告には実際の出現が必要）だが、"missing" の指摘は推測になるので、
    # 理由を添えて note として報告する。
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
