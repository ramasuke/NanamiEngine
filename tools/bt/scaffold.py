"""Scaffold a new enemy behaviour Action (C++) and wire it into the build.

Creates ``Enemy_Behaviour_Action_<Name>.{h,cpp}`` under
``Assets/Scripts/Core/Game/Npc/Enemy/Behaviour/Action/Content/<category>/<Name>/``
and patches the three wiring points:

  1. ``Enemy_Behaviour_ActionHeaders.h``   (editor sees the new type)
  2. ``NanamiEngine.vcxproj``              (the .cpp/.h get compiled)   [mandatory]
  3. ``NanamiEngine.vcxproj.filters``      (Solution Explorer grouping) [optional]

Generated C++ is ASCII-only and written as Shift-JIS (CP932) - the repo's source
encoding - so it is byte-identical under cp932/ascii and needs no post-processing.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from . import vcxproj

_REPO = Path(__file__).resolve().parents[2]
ACTION_DIR_REL = "Assets/Scripts/Core/Game/Npc/Enemy/Behaviour/Action"
CONTENT_DIR = _REPO / ACTION_DIR_REL / "Content"
HEADERS_AGG = _REPO / "Assets/Scripts/Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionHeaders.h"
VCXPROJ = _REPO / "NanamiEngine.vcxproj"
FILTERS = _REPO / "NanamiEngine.vcxproj.filters"

_SCALAR = {
    "int": ("int", "0"),
    "float": ("float", "0.0f"),
    "bool": ("bool", "false"),
    "string": ("std::string", None),
}


class ScaffoldError(RuntimeError):
    pass


@dataclass
class Param:
    name: str
    shape: str
    default: str | None = None

    @property
    def cpp_type(self) -> str:
        return _SCALAR[self.shape][0]

    @property
    def cpp_default(self) -> str | None:
        if self.default is None:
            return _SCALAR[self.shape][1]
        d = self.default.strip()
        if self.shape == "float" and re.fullmatch(r"-?\d+(\.\d+)?", d):
            return d + "f"
        if self.shape == "string" and not (d.startswith('"') and d.endswith('"')):
            return f'"{d}"'
        return d


def parse_param(spec: str) -> Param:
    m = re.fullmatch(r"(\w+)\s*:\s*(\w+)\s*(?:=\s*(.+))?", spec)
    if not m:
        raise ScaffoldError(f"bad --param {spec!r}; expected name:type[=default]")
    nm, shape, dfl = m.group(1), m.group(2), m.group(3)
    if shape not in _SCALAR:
        raise ScaffoldError(
            f"--param {nm}: shape {shape!r} not supported by add-action "
            f"(use one of {sorted(_SCALAR)}); add vec3/FIELD members by hand"
        )
    member = nm if nm.endswith("_") else nm + "_"
    return Param(name=member, shape=shape, default=dfl)


# ---------------------------------------------------------------------------
def _rel_ups(n: int) -> str:
    return "../" * n


def _paths(name: str, category: str, subdir: str | None):
    cat_path = (subdir or category.replace("::", "/")).strip("/")
    rel_dir = f"Content/{cat_path}/{name}"
    fs_dir = CONTENT_DIR / Path(cat_path) / name
    # depth from the header's directory up to .../Action/
    depth_to_action = len(Path(rel_dir).parts)          # Content/<...>/<name>
    # depth from the header's directory up to Assets/Scripts/
    depth_to_scripts = len(("Core", "Game", "Npc", "Enemy", "Behaviour", "Action")) + depth_to_action
    return fs_dir, rel_dir, depth_to_action, depth_to_scripts


def _render_h(name: str, category: str, version: int, params: list[Param],
              depth_action: int, depth_scripts: int) -> str:
    fqn = f"GameCore::Npc::Enemy::Behaviour::Action::{name}"
    inc_base = _rel_ups(depth_action) + "Enemy_Behaviour_ActionBase.h"
    inc_factory = _rel_ups(depth_scripts) + "Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

    members, saves, loads, guis = [], [], [], []
    for p in params:
        dfl = f" = {p.cpp_default}" if p.cpp_default is not None else ""
        members.append(f"        [[serialize(0)]] {p.cpp_type} {p.name}{dfl};")
        saves.append(f"            archive(CEREAL_NVP({p.name}));")
        loads.append(f"            if (version >= 0) archive(CEREAL_NVP({p.name}));")
        guis.append(f'        ImGuiHelper::OnDrawInputField("{p.name}", {p.name});')

    L = []
    L.append("#pragma once")
    L.append(f'#include "{inc_base}"')
    L.append(f'#include "{inc_factory}"')
    L.append('#include "cereal/types/base_class.hpp"')
    L.append('#include "cereal/types/polymorphic.hpp"')
    L.append("")
    L.append("namespace GameCore::Npc::Enemy::Behaviour::Action")
    L.append("{")
    L.append(f"    class {name} final : public ActionBase")
    L.append("    {")
    L.append("        TickStatus DoTick(const TickContext& context) override;")
    if params:
        L.append("        void       DoDrawGui() override;")
        L.append("")
        L.extend(members)
    L.append("")
    L.append("    public:")
    L.append("        template<class Archive>")
    L.append("        void save(Archive& archive, const std::uint32_t version) const")
    L.append("        {")
    L.append("            archive(cereal::base_class<ActionBase>(this));")
    L.extend(saves)
    L.append("        }")
    L.append("        template<class Archive>")
    L.append("        void load(Archive& archive, const std::uint32_t version)")
    L.append("        {")
    L.append("            archive(cereal::base_class<ActionBase>(this));")
    L.extend(loads)
    L.append("        }")
    L.append("    };")
    L.append("")
    L.append(f'    REGISTER_ENEMY_ACTION_WITH_NAME({name}, "{category}::{name}")')
    L.append("}")
    L.append("")
    if version and version > 0:
        L.append(f"CEREAL_CLASS_VERSION({fqn}, {version})")
    L.append(f"CEREAL_REGISTER_TYPE({fqn})")
    L.append(f"CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, {fqn})")
    L.append("")
    return "\r\n".join(L)


def _render_cpp(name: str, params: list[Param]) -> str:
    L = []
    L.append(f'#include "Enemy_Behaviour_Action_{name}.h"')
    L.append("")
    L.append("namespace GameCore::Npc::Enemy::Behaviour")
    L.append("{")
    L.append(f"    TickStatus Action::{name}::DoTick(const TickContext& context)")
    L.append("    {")
    L.append("        // TODO: implement. Return Running while working, Success when done,")
    L.append("        //       Failure to let the parent node try a sibling.")
    L.append("        return TickStatus::Success;")
    L.append("    }")
    if params:
        L.append("")
        L.append(f"    void Action::{name}::DoDrawGui()")
        L.append("    {")
        for p in params:
            L.append(f'        ImGuiHelper::OnDrawInputField("{p.name}", {p.name});')
        L.append("    }")
    L.append("}")
    L.append("")
    return "\r\n".join(L)


def _win(rel_posix: str) -> str:
    return rel_posix.replace("/", "\\")


# ---------------------------------------------------------------------------
def add_action(name: str, category: str, *, params: list[Param] | None = None,
               version: int = 0, subdir: str | None = None, filters: bool = True,
               encoding: str = "cp932", dry_run: bool = False) -> list[str]:
    if not re.fullmatch(r"[A-Za-z_]\w*", name):
        raise ScaffoldError(f"invalid action name: {name!r}")
    params = params or []
    fs_dir, rel_dir, d_action, d_scripts = _paths(name, category, subdir)
    h_path = fs_dir / f"Enemy_Behaviour_Action_{name}.h"
    cpp_path = fs_dir / f"Enemy_Behaviour_Action_{name}.cpp"
    if h_path.exists() or cpp_path.exists():
        raise ScaffoldError(f"{h_path.name} already exists at {fs_dir}")

    h_text = _render_h(name, category, version, params, d_action, d_scripts)
    cpp_text = _render_cpp(name, params)
    log: list[str] = []

    if dry_run:
        log.append(f"[dry-run] create {h_path.relative_to(_REPO)}")
        log.append(f"[dry-run] create {cpp_path.relative_to(_REPO)}")
    else:
        fs_dir.mkdir(parents=True, exist_ok=True)
        h_path.write_bytes(h_text.encode(encoding))
        cpp_path.write_bytes(cpp_text.encode(encoding))
        log.append(f"create {h_path.relative_to(_REPO)}")
        log.append(f"create {cpp_path.relative_to(_REPO)}")

    # 1. ActionHeaders.h
    inc = f"../../../../../Core/Game/Npc/Enemy/Behaviour/Action/{rel_dir}/Enemy_Behaviour_Action_{name}.h"
    log += _patch_headers_agg(inc, dry_run=dry_run)

    # 2 + 3. vcxproj (+ filters)
    win_h = _win(f"{ACTION_DIR_REL}/{rel_dir}/Enemy_Behaviour_Action_{name}.h")
    win_cpp = _win(f"{ACTION_DIR_REL}/{rel_dir}/Enemy_Behaviour_Action_{name}.cpp")
    splices = [vcxproj.Splice("ClCompile", win_cpp), vcxproj.Splice("ClInclude", win_h)]
    log += vcxproj.apply_splices(VCXPROJ, FILTERS if filters else None, splices,
                                 anchor=vcxproj.CONTENT_ANCHOR, dry_run=dry_run)
    return log


def _patch_headers_agg(include_rel: str, *, dry_run: bool) -> list[str]:
    raw = HEADERS_AGG.read_bytes()
    bom = raw[:3] == b"\xef\xbb\xbf"
    body = raw[3:] if bom else raw
    text = body.decode("cp932")
    line = f'#include "{include_rel}"'
    if line in text:
        return [f"ActionHeaders.h: include already present - skipped"]
    lines = text.split("\r\n")
    last = max(i for i, ln in enumerate(lines) if ln.startswith('#include "'))
    lines.insert(last + 1, line)
    new = "\r\n".join(lines)
    if not dry_run:
        HEADERS_AGG.write_bytes((b"\xef\xbb\xbf" if bom else b"") + new.encode("cp932"))
    return [f"ActionHeaders.h: + {line}"]


def remove_action(name: str, category: str, *, subdir: str | None = None,
                  filters: bool = True, dry_run: bool = False) -> list[str]:
    fs_dir, rel_dir, _da, _ds = _paths(name, category, subdir)
    h_path = fs_dir / f"Enemy_Behaviour_Action_{name}.h"
    cpp_path = fs_dir / f"Enemy_Behaviour_Action_{name}.cpp"
    log: list[str] = []

    win_h = _win(f"{ACTION_DIR_REL}/{rel_dir}/Enemy_Behaviour_Action_{name}.h")
    win_cpp = _win(f"{ACTION_DIR_REL}/{rel_dir}/Enemy_Behaviour_Action_{name}.cpp")
    log += vcxproj.remove_splices(VCXPROJ, FILTERS if filters else None,
                                 [win_cpp, win_h], dry_run=dry_run)

    inc = f"../../../../../Core/Game/Npc/Enemy/Behaviour/Action/{rel_dir}/Enemy_Behaviour_Action_{name}.h"
    raw = HEADERS_AGG.read_bytes()
    bom = raw[:3] == b"\xef\xbb\xbf"
    text = (raw[3:] if bom else raw).decode("cp932")
    if f'#include "{inc}"' in text:
        text = text.replace(f'#include "{inc}"\r\n', "").replace(f'\r\n#include "{inc}"', "")
        if not dry_run:
            HEADERS_AGG.write_bytes((b"\xef\xbb\xbf" if bom else b"") + text.encode("cp932"))
        log.append(f"ActionHeaders.h: - {inc}")

    for p in (h_path, cpp_path):
        if p.exists():
            if not dry_run:
                p.unlink()
            log.append(f"delete {p.relative_to(_REPO)}")
    if not dry_run:
        d = fs_dir
        while d != CONTENT_DIR and d.exists():
            try:
                if any(d.iterdir()):
                    break
                d.rmdir()
                log.append(f"rmdir {d.relative_to(_REPO)}")
            except OSError:
                break
            d = d.parent
    return log
