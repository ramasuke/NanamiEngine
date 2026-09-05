"""Build ``tools/animtree/catalog.json`` by scraping the ``IAnimationNode``
subclass headers under ``Engine/Module/AnimationTree/Node/``.

Regex/brace-matching scanner - no libclang - mirroring
:mod:`tools.bt.catalog_scan`. Anything it cannot classify is recorded with
shape ``"unknown"``; the reader then falls back to a structural fingerprint
for that slot's version key and ``set-node-params`` refuses to touch it.

The condition-predicate and parameter kinds (``AnimationNodePathAdditionCondition<T>``
/ ``AnimationParameter<T>``, T in bool/int/float) are **not** scanned - they are
3 fixed explicit instantiations of one template each, in a single small file;
there is no realistic "a 4th kind appears" path (it would require touching the
engine's equality-only ``Check()``, the byte-buffer netcode plumbing, and a
fixed 3-entry ImGui combo box), and nothing downstream would consume a scanned
catalog for them anyway (no add-condition-type scaffold exists, mirroring
tools/bt's own hand-written ``nodes`` dict for its fixed structural node
types). They are written as a literal dict directly in :func:`scan`.
"""

from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path
from typing import Any, Optional

_REPO = Path(__file__).resolve().parents[2]
NODE_ROOT = _REPO / "Engine" / "Module" / "AnimationTree" / "Node"
CATALOG_PATH = Path(__file__).with_name("catalog.json")

# these two leaves are dedicated named members on AnimationTree itself
# (entryNode_/visualAnyStateNode_), not part of the polymorphic nodes_ map -
# not recoverable from each node's own header, so hand-listed here exactly as
# tools/bt/catalog_scan.py hardcodes its own fixed structural node-type dict.
SINGLETON_LEAVES = {"AnimatorEntryNode", "AnimationVisualAnyStateNode"}

# -- regexes ------------------------------------------------------------------
RE_CLASS = re.compile(r"\bclass\s+(\w+)\s+final\s*:\s*public\s+IAnimationNode\b")
RE_REGISTER_TYPE = re.compile(r"CEREAL_REGISTER_TYPE\s*\(\s*([\w:]+)\s*\)")
RE_CLASS_VERSION = re.compile(r"CEREAL_CLASS_VERSION\s*\(\s*([\w:]+)\s*,\s*(\d+)\s*\)")
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


def _classify_member(decl_type: str, member: str) -> dict:
    # name-based special case, checked before the type-based dispatch: this
    # naming idiom (guid_/position_ as the node's identity/canvas-position
    # fields) is consistent and load-bearing across every IAnimationNode
    # subtype in the engine - a type-based rule (e.g. "any bare Guid-typed
    # member") would be needless generalisation for a one-convention reality.
    if member == "guid_":
        return {"shape": "self_guid"}
    if member == "position_":
        return {"shape": "self_pos"}

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
    return {"shape": "unknown", "type": _leaf(t) or t}


def _parse_serializable(body: str) -> list[dict]:
    """Return the ordered param list from a class body's ``save()`` method."""
    body = _strip_comments(body)

    m = RE_SAVE.search(body)
    if not m:
        return []
    save_block = _balanced_block(body, m.end())

    order: list[tuple[str, bool]] = []  # (member, named)
    for call in RE_ARCHIVE_CALL.finditer(save_block):
        named, bare = call.group(1), call.group(2)
        member = named or bare
        if member in ("this",):
            continue
        if RE_BASE_CALL.search(save_block[call.start():call.end()]):
            continue
        order.append((member, bool(named)))
    order = [(mm, nn) for (mm, nn) in order if mm and not mm.startswith("cereal")]

    _KEYWORDS = {"return", "const", "static", "virtual", "auto", "explicit",
                 "constexpr", "inline", "mutable", "public", "private"}
    type_pat_tpl = (
        r"(\[\[serialize\(\d+\)\]\]\s*)?"
        r"((?:FIELD\s*\([^)]*\)|Field\s*<[^>]*>|[\w:<>\s\*&])+?)\s+"
        r"MEMBER" + r"\s*(?:=|;|\{|\()"
    )

    def _decl_type(member: str) -> str:
        pat = re.compile(type_pat_tpl.replace("MEMBER", re.escape(member)))
        best, best_score = "", -1
        for mm in pat.finditer(body):
            t = mm.group(2).strip()
            leaf = t.split()[-1] if t.split() else t
            score = 0
            if mm.group(1):
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
        info = _classify_member(_decl_type(member), member)
        if named:
            key = member
        else:
            positional += 1
            key = f"value{positional}"
        params.append({"key": key, "member": member, "named": named, **info})
    return params


def scan() -> dict[str, Any]:
    headers = list(NODE_ROOT.rglob("*.h")) if NODE_ROOT.exists() else []

    node_types: dict[str, dict] = {}
    by_leaf: dict[str, str] = {}

    for path in sorted(headers):
        text = _read(path)
        cm = RE_CLASS.search(text)
        if not cm:
            continue
        cls = cm.group(1)
        body = _balanced_block(text, cm.end())

        vt = RE_REGISTER_TYPE.search(text)
        fqn = vt.group(1) if vt else f"NanamiEngine::Module::AnimationTree::{cls}"
        cv = RE_CLASS_VERSION.search(text)
        version = int(cv.group(2)) if cv else 0

        params = _parse_serializable(body)
        rel = path.relative_to(_REPO).as_posix()

        node_types[fqn] = {
            "class": cls,
            "fqn": fqn,
            "leaf": _leaf(fqn),
            "version": version,
            "singleton": cls in SINGLETON_LEAVES,
            "header": rel,
            "params": params,
        }
        by_leaf.setdefault(_leaf(fqn), fqn)
        by_leaf.setdefault(cls, fqn)

    conditions = {
        k: {"fqn": f"NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<{k}>",
            "version": 0}
        for k in ("bool", "int", "float")
    }
    params_by_kind = {
        k: {"fqn": f"NanamiEngine::Module::AnimationTree::AnimationParameter<{k}>", "version": 0}
        for k in ("bool", "int", "float")
    }

    return {
        "generated_from": _git_head(),
        "node_types": dict(sorted(node_types.items())),
        "by_leaf": dict(sorted(by_leaf.items())),
        "conditions": conditions,
        "params": params_by_kind,
    }


def write_catalog(data: dict[str, Any], path: Path | None = None) -> Path:
    p = path or CATALOG_PATH
    p.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return p


def check_fresh() -> tuple[bool, str]:
    if not CATALOG_PATH.exists():
        return False, "catalog.json missing"
    current = json.dumps(scan().get("node_types"), sort_keys=True)
    on_disk = json.dumps(
        json.loads(CATALOG_PATH.read_text(encoding="utf-8")).get("node_types"), sort_keys=True
    )
    if current != on_disk:
        return False, "catalog.json is stale - run: python -m tools.animtree regen-catalog"
    return True, "ok"


if __name__ == "__main__":
    write_catalog(scan())
    print(f"wrote {CATALOG_PATH}")
