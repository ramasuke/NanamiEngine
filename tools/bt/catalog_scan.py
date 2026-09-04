"""Build ``tools/bt/catalog.json`` by scraping the enemy behaviour Action headers.

Regex line/blob scanner - no libclang. Anything it cannot classify is recorded
with shape ``"unknown"``; the reader then falls back to a structural fingerprint
for that slot's version key and ``set-params`` refuses to touch it.
"""

from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path
from typing import Any, Optional

_REPO = Path(__file__).resolve().parents[2]
ACTION_ROOT = _REPO / "Assets" / "Scripts" / "Core" / "Game" / "Npc" / "Enemy" / "Behaviour" / "Action"
CONTENT_ROOT = ACTION_ROOT / "Content"
CATALOG_PATH = Path(__file__).with_name("catalog.json")

# extra helper structs (not registered actions) that appear as action members
EXTRA_STRUCT_FILES = {
    "PhysicsPower": _REPO / "Assets/Scripts/Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h",
    "Position": ACTION_ROOT / "Position/Enemy_Behaviour_Action_Position.h",
}

_SKIP_DIRS = {"TickContext", "FieldGameObject", "Position"}

# -- regexes --------------------------------------------------------------------
RE_CLASS = re.compile(r"\bclass\s+(\w+)\s+final\s*:\s*public\s+[\w:]*ActionBase\b")
RE_STRUCT = re.compile(r"\b(?:class|struct)\s+(\w+)\s+final\b")
RE_REGISTER_TYPE = re.compile(r"CEREAL_REGISTER_TYPE\s*\(\s*([\w:]+)\s*\)")
RE_CLASS_VERSION = re.compile(r"CEREAL_CLASS_VERSION\s*\(\s*([\w:]+)\s*,\s*(\d+)\s*\)")
RE_REG_ACTION_NAMED = re.compile(
    r"REGISTER_ENEMY_ACTION_WITH_NAME\s*\(\s*(\w+)\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*\)"
)
RE_REG_ACTION_BARE = re.compile(r"REGISTER_ENEMY_ACTION\s*\(\s*(\w+)\s*\)")
RE_SAVE = re.compile(r"\bvoid\s+save\s*\(\s*Archive\s*&\s*\w+\s*,")
RE_ARCHIVE_CALL = re.compile(
    r"archive\s*\(\s*(?:CEREAL_NVP\s*\(\s*(\w+)\s*\)|([a-zA-Z_]\w*))\s*\)"
)
RE_BASE_CALL = re.compile(r"cereal::base_class\s*<")
RE_FIELD_MACRO = re.compile(r"FIELD\s*\(\s*([\w:]+)\s*\)")
RE_FIELD_TMPL = re.compile(r"\bField\s*<\s*([\w:]+)\s*>")
RE_VECTOR = re.compile(r"std::vector\s*<\s*([\w:<>\s]+?)\s*>")


def _leaf(name: str) -> str:
    return name.split("<", 1)[0].strip().rsplit("::", 1)[-1]


def _git_head() -> str:
    try:
        return subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=_REPO, capture_output=True, text=True, check=True
        ).stdout.strip()
    except Exception:  # noqa: BLE001
        return "unknown"


def _read(path: Path) -> str:
    raw = path.read_bytes()
    if raw[:3] == b"\xef\xbb\xbf":
        raw = raw[3:]
    for enc in ("utf-8", "cp932", "latin-1"):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", "replace")


def _balanced_block(text: str, open_idx: int) -> str:
    """Return the ``{...}`` block starting at/after ``open_idx`` (inclusive braces)."""
    i = text.find("{", open_idx)
    if i < 0:
        return ""
    depth = 0
    for j in range(i, len(text)):
        c = text[j]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[i : j + 1]
    return text[i:]


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def _classify_member(decl_type: str, enums: set[str], known_types: set[str]) -> dict:
    t = decl_type.strip()
    m = RE_FIELD_MACRO.search(t) or RE_FIELD_TMPL.search(t)
    if m:
        return {"shape": "field", "type": _leaf(m.group(1))}
    if "glm::vec3" in t:
        return {"shape": "vec3"}
    if "glm::vec2" in t:
        return {"shape": "vec2"}
    if "glm::quat" in t:
        return {"shape": "quat"}
    if "std::string" in t:
        return {"shape": "string"}
    mv = RE_VECTOR.search(t)
    if mv:
        return {"shape": "vector", "elem": _leaf(mv.group(1))}
    if re.search(r"\bbool\b", t):
        return {"shape": "bool"}
    if re.search(r"\b(float|double)\b", t):
        return {"shape": "float"}
    if re.search(r"\b(int|short|long|unsigned)\b", t) or re.search(
        r"std::u?int\d+_t|std::size_t|size_t|std::uint32_t", t
    ):
        return {"shape": "int"}
    leaf = _leaf(t)
    if leaf in enums:
        return {"shape": "enum", "type": leaf}
    if leaf in known_types:
        return {"shape": "nested", "type": leaf}
    return {"shape": "unknown", "type": leaf or t}


def _parse_serializable(body: str, known_types: set[str]) -> tuple[list[dict], list[str]]:
    """Return (params, member_names) from a class/struct body."""
    body = _strip_comments(body)
    enums = set(re.findall(r"enum\s+class\s+(\w+)", body))

    m = RE_SAVE.search(body)
    if not m:
        return [], []
    save_block = _balanced_block(body, m.end())

    order: list[tuple[str, bool]] = []  # (member, named)
    for call in RE_ARCHIVE_CALL.finditer(save_block):
        named, bare = call.group(1), call.group(2)
        member = named or bare
        if member in ("this",):
            continue
        # skip base_class<...>(this)
        seg = save_block[max(0, call.start() - 30):call.start()]
        if RE_BASE_CALL.search(save_block[call.start():call.end()]):
            continue
        order.append((member, bool(named)))
    # also drop an initial base_class call captured as bare "archive(cereal::...)"
    order = [(mm, nn) for (mm, nn) in order if mm and not mm.startswith("cereal")]

    _KEYWORDS = {"return", "const", "static", "virtual", "auto", "explicit",
                 "constexpr", "inline", "mutable", "public", "private"}
    type_pat = re.compile(
        r"(\[\[serialize\(\d+\)\]\]\s*)?"
        r"((?:FIELD\s*\([^)]*\)|Field\s*<[^>]*>|[\w:<>\s\*&])+?)\s+"
        + r"MEMBER" + r"\s*(?:=|;|\{|\()"
    )

    def _decl_type(member: str) -> str:
        pat = re.compile(type_pat.pattern.replace("MEMBER", re.escape(member)))
        best, best_score = "", -1
        for m in pat.finditer(body):
            t = m.group(2).strip()
            leaf = t.split()[-1] if t.split() else t
            score = 0
            if m.group(1):
                score += 3
            if t and leaf not in _KEYWORDS and t not in _KEYWORDS:
                score += 2
            if "(" in t or "<" in t:  # FIELD(...) / Field<...>
                score += 1
            if score > best_score:
                best, best_score = t, score
        return best

    params: list[dict] = []
    positional = 0
    for member, named in order:
        info = _classify_member(_decl_type(member), enums, known_types)
        if named:
            key = member
        else:
            positional += 1
            key = f"value{positional}"
        params.append({"key": key, "member": member, "named": named, **info})
    return params, [mm for mm, _ in order]


def scan() -> dict[str, Any]:
    headers = [p for p in CONTENT_ROOT.rglob("*.h")
               if not any(part in _SKIP_DIRS for part in p.relative_to(CONTENT_ROOT).parts[:-1])]

    # first pass: know every action class leaf name (needed to classify members)
    raw: list[tuple[Path, str]] = [(p, _read(p)) for p in headers]
    known_leaves: set[str] = set(EXTRA_STRUCT_FILES)
    for _p, text in raw:
        for cm in RE_CLASS.finditer(text):
            known_leaves.add(cm.group(1))

    actions: dict[str, dict] = {}
    by_fqn: dict[str, str] = {}
    by_leaf: dict[str, str] = {}

    for path, text in raw:
        cm = RE_CLASS.search(text)
        if not cm:
            continue
        cls = cm.group(1)
        body = _balanced_block(text, cm.end())

        vt = RE_REGISTER_TYPE.search(text)
        fqn = vt.group(1) if vt else f"GameCore::Npc::Enemy::Behaviour::Action::{cls}"
        cv = RE_CLASS_VERSION.search(text)
        version = int(cv.group(2)) if cv else 0
        rn = RE_REG_ACTION_NAMED.search(text)
        rb = RE_REG_ACTION_BARE.search(text)
        display = rn.group(2) if rn else (rb.group(1) if rb else cls)

        params, _members = _parse_serializable(body, known_leaves)
        rel = path.relative_to(CONTENT_ROOT).as_posix()

        actions[display] = {
            "class": cls,
            "fqn": fqn,
            "leaf": _leaf(fqn),
            "version": version,
            "header": "Assets/Scripts/Core/Game/Npc/Enemy/Behaviour/Action/Content/" + rel,
            "params": params,
        }
        by_fqn[fqn] = display
        by_leaf.setdefault(_leaf(fqn), display)
        by_leaf.setdefault(cls, display)

    structs: dict[str, dict] = {}
    for leaf, path in EXTRA_STRUCT_FILES.items():
        if not path.exists():
            continue
        text = _read(path)
        sm = RE_STRUCT.search(text) or RE_CLASS.search(text)
        if not sm:
            continue
        body = _balanced_block(text, sm.end())
        cv = RE_CLASS_VERSION.search(text)
        vt = RE_REGISTER_TYPE.search(text)
        params, _m = _parse_serializable(body, known_leaves)
        structs[leaf] = {
            "fqn": (vt.group(1) if vt else leaf),
            "version": int(cv.group(2)) if cv else 0,
            "params": params,
        }

    nodes = {
        "Editor::Npc::Behaviour::EntryNode": {"leaf": "EntryNode", "version": 0},
        "Editor::Npc::Behaviour::SelectorNode": {"leaf": "SelectorNode", "version": 0},
        "Editor::Npc::Behaviour::SequenceNode": {"leaf": "SequenceNode", "version": 0},
        "Editor::Npc::Behaviour::RandomSelectorNode": {"leaf": "RandomSelectorNode", "version": 1},
        "Editor::Npc::Behaviour::OnceExecute": {"leaf": "OnceExecute", "version": 0},
        "Editor::Npc::Behaviour::OnceSuccessNode": {"leaf": "OnceSuccessNode", "version": 0},
        "Editor::Npc::Enemy::Behaviour::ActionNode": {"leaf": "ActionNode", "version": 1},
    }

    return {
        "generated_from": _git_head(),
        "actions": dict(sorted(actions.items())),
        "structs": dict(sorted(structs.items())),
        "by_fqn": dict(sorted(by_fqn.items())),
        "by_leaf": dict(sorted(by_leaf.items())),
        "nodes": nodes,
    }


def write_catalog(data: dict[str, Any], path: Path | None = None) -> Path:
    p = path or CATALOG_PATH
    p.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return p


def check_fresh() -> tuple[bool, str]:
    if not CATALOG_PATH.exists():
        return False, "catalog.json missing"
    current = json.dumps(scan().get("actions"), sort_keys=True)
    on_disk = json.dumps(json.loads(CATALOG_PATH.read_text(encoding="utf-8")).get("actions"),
                         sort_keys=True)
    if current != on_disk:
        return False, "catalog.json is stale - run: python -m tools.bt regen-catalog"
    return True, "ok"


if __name__ == "__main__":
    write_catalog(scan())
    print(f"wrote {CATALOG_PATH}")
