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
targets ``ComponentBase`` (that's hardcoded into the macro), but a component's
``save()`` usually archives **several** base classes, each as one unnamed
positional slot (``value0``, ``value1``, ...) ahead of its own fields - e.g.
``ImageRenderer`` writes ``ComponentBase`` (``value0``), then the empty
lifecycle mixins ``IInitRenderable`` (``value1``) and
``IUserInterfaceRenderable`` (``value2``), then ``spriteFile_``. cereal reads
those slots *positionally*, so a from-scratch instance must reproduce every one
of them. Each entry's ``bases`` lists every ``cereal::base_class<X>(this)`` call
in archive order with the ``valueN`` key it occupies (``immediate_base`` is kept
as ``bases[0]`` for compatibility), and the top-level ``bases`` table records,
per base leaf, whether its own ``save()`` body is empty (a pure marker mixin
that ``edits.add_component`` can emit as an empty slot) and its
``CEREAL_CLASS_VERSION``. Bases with real serialised fields of their own
(every Collider via ``ColliderBase``, which owns ``mass_``/``isGravity_``/...,
plus ``NetworkComponent``/``EnemyBase``) are not (yet) flattened into
``params``: they round-trip but aren't individually settable, and
``edits.add_component`` refuses to construct a brand-new instance of such a
type from scratch, to avoid ever emitting a struct missing fields the engine
requires.
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
# One archive(...) call: a base_class<X>(this) wrapper, a CEREAL_NVP(member), or
# a bare (unnamed) member - all three matter for the positional "valueN" layout.
RE_ARCHIVE_CALL = re.compile(
    r"archive\s*\(\s*(?:"
    r"cereal::base_class\s*<\s*(?P<base>[\w:]+)\s*>\s*\(\s*this\s*\)"
    r"|CEREAL_NVP\s*\(\s*(?P<nvp>\w+)\s*\)"
    r"|(?P<bare>[a-zA-Z_]\w*)"
    r")\s*\)"
)
RE_CLASS_VERSION = re.compile(r"CEREAL_CLASS_VERSION\s*\(\s*([\w:]+)\s*,\s*(\d+)\s*\)")
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
    """The ``{...}`` body of ``class <leaf>``'s *definition* - a forward
    declaration (``class X;`` / ``friend class X;``) has a ``;`` before any
    ``{`` and is skipped, rather than grabbing whatever unrelated block follows
    it (which is what a base-type scan would otherwise pick up from a header
    that merely forward-declares ``ColliderBase``/``ComponentBase``)."""
    for m in re.finditer(r"\bclass\s+(\w+)\b", text):
        if m.group(1) != leaf:
            continue
        brace = text.find("{", m.end())
        if brace < 0:
            return ""
        if ";" in text[m.end():brace]:
            continue
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


def _parse_serializable(body: str, known_types: set[str]) -> tuple[list[dict], list[dict], bool]:
    """Return ``(params, bases, interleaved)`` from a class body's ``save()``.

    ``bases`` is every ``archive(cereal::base_class<X>(this))`` call, in archive
    order, as ``{"leaf": X, "key": "valueN"}``. cereal names each *unnamed* node
    ``value<N>`` from a per-object counter that only unnamed nodes advance
    (``JSONOutputArchive::writeName``), so the base slots occupy
    ``value0..value{n-1}`` and a member archived without ``CEREAL_NVP`` continues
    that same count after the last base - which is why ``params`` numbering
    starts at ``len(bases)``, not at a hardcoded 1. ``interleaved`` is True if a
    base_class call appears *after* a member (never the case today;
    ``edits.new_component`` refuses such a type rather than guess the layout).

    Only ``ComponentBase`` and bases whose own ``save()`` body is empty (see
    :func:`_scan_bases`) can be constructed from scratch by
    ``edits.add_component``; a base with real fields of its own (every Collider,
    via ``ColliderBase``) still round-trips losslessly (``reader.py`` tags it as
    an opaque nested blob) but is refused there rather than risk building a
    struct the engine can't load.
    """
    body = _strip_comments(body)
    m = RE_SAVE.search(body)
    if not m:
        return [], [], False
    save_block = _balanced_block(body, m.end())

    bases: list[dict] = []
    order: list[tuple[str, bool]] = []
    interleaved = False
    for call in RE_ARCHIVE_CALL.finditer(save_block):
        base = call.group("base")
        if base:
            if order:
                interleaved = True
            bases.append({"leaf": _leaf(base), "key": f"value{len(bases)}"})
            continue
        named, bare = call.group("nvp"), call.group("bare")
        member = named or bare
        if not member or member == "this" or member.startswith("cereal"):
            continue
        order.append((member, bool(named)))

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
    positional = len(bases)  # the valueN counter continues after the base slots
    for member, named in order:
        info = _classify_member(_decl_type(member), known_types)
        if named:
            key = member
        else:
            key = f"value{positional}"
            positional += 1
        params.append({"key": key, "member": member, "named": named, **info})
    return params, bases, interleaved


def _scan_bases(leaves: set[str], headers: list[tuple[str, str]]) -> dict[str, dict]:
    """Locate the defining header of every base leaf some component archives
    and record what ``edits.add_component`` needs to know to emit that slot
    from scratch: whether the base's own ``save()`` body is **empty** (a pure
    marker mixin such as ``IInitRenderable`` - safe to write as a bare
    ``{"cereal_class_version": N}`` object) and its ``CEREAL_CLASS_VERSION``
    (0 when the macro is absent - cereal's ``detail::Version<T>`` default).
    ``fqn`` is only known when that macro names it. A leaf defined with
    *different* ``save()`` bodies in more than one header is flagged
    ``ambiguous`` so the editor fails closed on it.
    """
    out: dict[str, dict] = {}
    sigs: dict[str, str] = {}
    for rel, text in headers:
        for leaf in leaves:
            if leaf not in text or not re.search(r"\bclass\s+" + re.escape(leaf) + r"\b", text):
                continue
            body = _find_class_body(text, leaf)
            if not body:
                continue
            stripped = _strip_comments(body)
            sm = RE_SAVE.search(stripped)
            if not sm:
                continue
            save_block = _balanced_block(stripped, sm.end())
            signature = re.sub(r"\s+", " ", save_block).strip()
            fqn, version = None, 0
            for vm in RE_CLASS_VERSION.finditer(text):
                if _leaf(vm.group(1)) == leaf:
                    fqn, version = vm.group(1), int(vm.group(2))
                    break
            if leaf in out:
                if sigs[leaf] != signature:
                    out[leaf]["ambiguous"] = True
                continue
            out[leaf] = {
                "fqn": fqn,
                "header": rel,
                "version": version,
                "empty": re.search(r"\barchive\s*\(", save_block) is None,
            }
            sigs[leaf] = signature
    return out


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
    all_headers: list[tuple[str, str]] = []  # rel, text - for the base-type scan
    for path in _iter_header_files():
        rel = path.relative_to(_REPO).as_posix()
        text = _read(path)
        all_headers.append((rel, text))
        if rel == _DEFINITION_FILE:
            continue
        if "ENGINE_REGISTER_COMPONENT" not in text:
            continue
        for m in RE_ENGINE_REGISTER.finditer(text):
            fqn, version = m.group(1), int(m.group(2))
            matches.append((path, rel, text, fqn, version))
            known_leaves.add(_leaf(fqn))

    base_leaves: set[str] = set()
    for path, rel, text, fqn, version in matches:
        leaf = _leaf(fqn)
        body = _find_class_body(text, leaf)
        params, bases, interleaved = (_parse_serializable(body, known_leaves)
                                      if body else ([], [], False))
        base_leaves.update(b["leaf"] for b in bases)
        components[fqn] = {
            "class": leaf,
            "fqn": fqn,
            "leaf": leaf,
            "version": version,
            "base_fqn": COMPONENT_BASE_FQN,
            "immediate_base": bases[0]["leaf"] if bases else None,
            "bases": bases,
            "interleaved_bases": interleaved,
            "header": rel,
            "params": params,
        }
        by_leaf.setdefault(leaf, []).append(fqn)
    bases_table = _scan_bases(base_leaves, all_headers)

    gameobject_shapes = {
        "NanamiEngine::Scene::SceneGameObject": {"leaf": "SceneGameObject", "version": 0},
        "NanamiEngine::Module::GameObject::PrefabGameObject": {"leaf": "PrefabGameObject", "version": 1},
        "NanamiEngine::Scene::CopiedPrefabGameObject": {"leaf": "CopiedPrefabGameObject", "version": 0},
    }

    return {
        "generated_from": _git_head(),
        "components": dict(sorted(components.items())),
        "by_leaf": {k: sorted(v) for k, v in sorted(by_leaf.items())},
        "bases": dict(sorted(bases_table.items())),
        "gameobject_shapes": gameobject_shapes,
    }


def write_catalog(data: dict[str, Any], path: Path | None = None) -> Path:
    p = path or CATALOG_PATH
    p.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return p


def _freshness_view(data: dict[str, Any]) -> str:
    # Everything derived from the C++ headers (not `generated_from`, which is
    # just the git HEAD the file was last written at).
    return json.dumps({"components": data.get("components"), "bases": data.get("bases")},
                      sort_keys=True)


def check_fresh() -> tuple[bool, str]:
    if not CATALOG_PATH.exists():
        return False, "catalog.json missing"
    current = _freshness_view(scan())
    on_disk = _freshness_view(json.loads(CATALOG_PATH.read_text(encoding="utf-8")))
    if current != on_disk:
        return False, "catalog.json is stale - run: python -m tools.scene regen-catalog"
    return True, "ok"


if __name__ == "__main__":
    write_catalog(scan())
    print(f"wrote {CATALOG_PATH}")
