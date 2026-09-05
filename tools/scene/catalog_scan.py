"""Build ``tools/scene/catalog.json`` by scraping registered Component headers.

Regex/brace-matching scanner - no libclang - mirroring ``tools.bt.catalog_scan``'s
approach. Anything it cannot classify is recorded with shape ``"unknown"``; this
only affects whether ``add-component``/``set-component-params`` can touch that
field by name - it never affects round-trip fidelity (``reader.py``/``writer.py``
tag every component's data as an opaque blob independently of this catalog).

Scope (v1): only components registered via the ``ENGINE_REGISTER_COMPONENT``
macro - a **direct** ``ComponentBase`` subclass - are catalogued as addable via
``add-component`` (confirmed ~66 such headers, spanning ``Engine/Module/**``,
``Packages/**``, and ``Assets/Scripts/**``). Components with an intermediate
base (e.g. ``GamePlay::Npc::Enemy::Hyena : EnemyBase : ComponentBase``, which
hand-registers the three cereal macros against ``EnemyBase`` directly rather
than using the macro) still round-trip losslessly through the tagged-blob
reader/writer - they are just not offered as an ``add-component --type``
target, since these gameplay-script components are normally introduced by
copying a whole prefab (``instantiate-prefab``), not attached bare to an
arbitrary GameObject.

Separately, ``ENGINE_REGISTER_COMPONENT``'s polymorphic *registration* always
targets ``ComponentBase`` (that's hardcoded into the macro), but several
components still have a real, distinct C++ **immediate base** with its own
serialised fields (every Collider goes through ``ColliderBase``, which owns
``mass_``/``isGravity_``/``friction_``/etc.) - each entry's ``immediate_base``
records that class's leaf name (``None``/``"ComponentBase"`` when there isn't
one). Those intermediate fields are not (yet) flattened into ``params``, so
they round-trip but aren't individually settable; ``edits.add_component``
refuses to construct a brand-new instance of such a type from scratch, to
avoid ever emitting a struct missing fields the engine requires.
"""

from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path
from typing import Any, Optional

_REPO = Path(__file__).resolve().parents[2]
CATALOG_PATH = Path(__file__).with_name("catalog.json")

COMPONENT_BASE_FQN = "NanamiEngine::Module::Component::ComponentBase"

# Broad scan roots - the macro-name pre-filter (see scan()) keeps this cheap and
# naturally excludes vendored third-party headers (cereal, ImGui, DxLib, Jolt),
# none of which reference this engine-specific macro.
SCAN_ROOTS = ["Engine", "Packages", "Assets/Scripts"]

# This file only *defines* the macro (`#define ENGINE_REGISTER_COMPONENT(TYPE,
# VERSION)`) - its parameter list `(TYPE, VERSION)` would otherwise parse as a
# spurious registration for a literal type named "TYPE".
_DEFINITION_FILE = "Engine/Module/Component/ComponentBase.h"

RE_ENGINE_REGISTER = re.compile(
    r"ENGINE_REGISTER_COMPONENT\s*\(\s*([\w:]+)\s*,\s*(\d+)\s*\)"
)
RE_SAVE = re.compile(r"\bvoid\s+save\s*\(\s*Archive\s*&\s*\w+\s*,")
RE_ARCHIVE_CALL = re.compile(
    r"archive\s*\(\s*(?:CEREAL_NVP\s*\(\s*(\w+)\s*\)|([a-zA-Z_]\w*))\s*\)"
)
RE_BASE_CALL = re.compile(r"cereal::base_class\s*<\s*([\w:]+)\s*>")
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


def _find_class_body(text: str, leaf: str) -> str:
    for m in re.finditer(r"\bclass\s+(\w+)\b", text):
        if m.group(1) == leaf:
            return _balanced_block(text, m.end())
    return ""


def _classify_member(decl_type: str, known_types: set[str]) -> dict:
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
    if leaf in known_types:
        return {"shape": "nested", "type": leaf}
    return {"shape": "unknown", "type": leaf or t}


def _parse_serializable(body: str, known_types: set[str]) -> tuple[list[dict], Optional[str]]:
    """Return ``(params, immediate_base)`` from a class body's ``save()`` method.

    ``immediate_base`` is the leaf type named by the first
    ``archive(cereal::base_class<Base>(this))`` call - almost always
    ``ComponentBase`` directly, but several components (e.g. every Collider,
    via ``ColliderBase``) have a real intermediate C++ base with its *own*
    serialised fields that this scanner does not (yet) flatten in. Those
    fields still round-trip losslessly (``reader.py`` tags them as an opaque
    nested blob); they are just not individually offered as settable params,
    and ``edits.add_component`` refuses to construct a brand-new instance of
    such a type from scratch (see that module) rather than risk building a
    struct the engine can't load.
    """
    body = _strip_comments(body)
    m = RE_SAVE.search(body)
    if not m:
        return [], None
    save_block = _balanced_block(body, m.end())

    # archive(cereal::base_class<Base>(this)) never matches RE_ARCHIVE_CALL at
    # all (its argument is a call expression, not CEREAL_NVP(x) or a bare
    # identifier) - it's simply skipped by the loop below on its own. Find the
    # base type name with a dedicated, independent search instead of trying to
    # recover it as a side effect of the archive-call scan.
    bm = RE_BASE_CALL.search(save_block)
    immediate_base = _leaf(bm.group(1)) if bm else None

    order: list[tuple[str, bool]] = []
    for call in RE_ARCHIVE_CALL.finditer(save_block):
        named, bare = call.group(1), call.group(2)
        member = named or bare
        if not member or member == "this":
            continue
        order.append((member, bool(named)))
    order = [(mm, nn) for (mm, nn) in order if not mm.startswith("cereal")]

    _KEYWORDS = {"return", "const", "static", "virtual", "auto", "explicit",
                "constexpr", "inline", "mutable", "public", "private"}
    type_pat_tmpl = (
        r"(\[\[serialize\(\d+\)\]\]\s*)?"
        r"((?:FIELD\s*\([^)]*\)|Field\s*<[^>]*>|[\w:<>\s\*&])+?)\s+"
        r"MEMBER" + r"\s*(?:=|;|\{|\()"
    )

    def _decl_type(member: str) -> str:
        pat = re.compile(type_pat_tmpl.replace("MEMBER", re.escape(member)))
        best, best_score = "", -1
        for mm in pat.finditer(body):
            t = mm.group(2).strip()
            leaf = t.split()[-1] if t.split() else t
            score = 0
            if mm.group(1):
                score += 3
            if t and leaf not in _KEYWORDS and t not in _KEYWORDS:
                score += 2
            if "(" in t or "<" in t:
                score += 1
            if score > best_score:
                best, best_score = t, score
        return best

    params: list[dict] = []
    positional = 0
    for member, named in order:
        info = _classify_member(_decl_type(member), known_types)
        if named:
            key = member
        else:
            positional += 1
            key = f"value{positional}"
        params.append({"key": key, "member": member, "named": named, **info})
    return params, immediate_base


def _iter_header_files() -> list[Path]:
    out: list[Path] = []
    for root in SCAN_ROOTS:
        root_path = _REPO / root
        if not root_path.exists():
            continue
        out.extend(root_path.rglob("*.h"))
    return out


def scan() -> dict[str, Any]:
    # Keyed by FQN (always unique) - unlike tools.bt's actions, components have
    # no separate "editor display name" macro, and two real leaf names collide
    # here (SceneContextBase, StatusPresenter each have two distinct FQNs), so
    # bare leaf cannot be the primary key. `by_leaf` below maps a leaf to every
    # FQN that uses it, for name resolution to fail closed on ambiguity.
    components: dict[str, dict] = {}
    by_leaf: dict[str, list[str]] = {}
    known_leaves: set[str] = set()

    matches: list[tuple[Path, str, str, str, int]] = []  # path, rel, text, fqn, version
    for path in _iter_header_files():
        rel = path.relative_to(_REPO).as_posix()
        if rel == _DEFINITION_FILE:
            continue
        text = _read(path)
        if "ENGINE_REGISTER_COMPONENT" not in text:
            continue
        for m in RE_ENGINE_REGISTER.finditer(text):
            fqn, version = m.group(1), int(m.group(2))
            matches.append((path, rel, text, fqn, version))
            known_leaves.add(_leaf(fqn))

    for path, rel, text, fqn, version in matches:
        leaf = _leaf(fqn)
        body = _find_class_body(text, leaf)
        params, immediate_base = _parse_serializable(body, known_leaves) if body else ([], None)
        components[fqn] = {
            "class": leaf,
            "fqn": fqn,
            "leaf": leaf,
            "version": version,
            "base_fqn": COMPONENT_BASE_FQN,
            "immediate_base": immediate_base,
            "header": rel,
            "params": params,
        }
        by_leaf.setdefault(leaf, []).append(fqn)

    gameobject_shapes = {
        "NanamiEngine::Scene::SceneGameObject": {"leaf": "SceneGameObject", "version": 0},
        "NanamiEngine::Module::GameObject::PrefabGameObject": {"leaf": "PrefabGameObject", "version": 1},
        "NanamiEngine::Scene::CopiedPrefabGameObject": {"leaf": "CopiedPrefabGameObject", "version": 0},
    }

    return {
        "generated_from": _git_head(),
        "components": dict(sorted(components.items())),
        "by_leaf": {k: sorted(v) for k, v in sorted(by_leaf.items())},
        "gameobject_shapes": gameobject_shapes,
    }


def write_catalog(data: dict[str, Any], path: Path | None = None) -> Path:
    p = path or CATALOG_PATH
    p.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return p


def check_fresh() -> tuple[bool, str]:
    if not CATALOG_PATH.exists():
        return False, "catalog.json missing"
    current = json.dumps(scan().get("components"), sort_keys=True)
    on_disk = json.dumps(json.loads(CATALOG_PATH.read_text(encoding="utf-8")).get("components"),
                         sort_keys=True)
    if current != on_disk:
        return False, "catalog.json is stale - run: python -m tools.scene regen-catalog"
    return True, "ok"


if __name__ == "__main__":
    write_catalog(scan())
    print(f"wrote {CATALOG_PATH}")
