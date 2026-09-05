"""``show`` / ``validate`` CLI subcommands - read-only."""

from __future__ import annotations

import argparse
from pathlib import Path

from tools.common.cereal_json import read_text

from . import model, reader, validate


def _short(guid: str) -> str:
    return guid.split("-")[0]


def _fmt_vec3(v: model.Vec3) -> str:
    return f"({v.x.render()}, {v.y.render()}, {v.z.render()})"


def _fmt_quat(q: model.Quat) -> str:
    return f"({q.x.render()}, {q.y.render()}, {q.z.render()}, {q.w.render()})"


def _print_component(comp: model.Component, indent: str) -> None:
    guid = model.find_component_guid(comp)
    enabled = model.find_component_enabled(comp)
    leaf = comp.fqn.rsplit("::", 1)[-1]
    flags = []
    if guid is not None:
        flags.append(_short(guid))
    flags.append(f"v{comp.class_version}")
    if enabled is False:
        flags.append("disabled")
    print(f"{indent}- {leaf}  [{', '.join(flags)}]")


def _print_gameobject(node: model.GameObjectNode, indent: str) -> None:
    active = "" if node.is_active else " (inactive)"
    kind_tag = "" if node.kind == "scene" else f" <{node.kind}>"
    t = node.transform
    print(f"{indent}{node.name}  [{_short(node.guid)}]{active}{kind_tag}  "
          f"pos={_fmt_vec3(t.local_pos)}")
    for comp in node.components:
        _print_component(comp, indent + "    ")
    for child in t.children:
        _print_gameobject(child, indent + "  ")


def cmd_show(args: argparse.Namespace) -> int:
    path = Path(args.file)
    text = read_text(path)
    if path.suffix == ".scene":
        scene = reader.read_scene(text)
        print(f"Scene \"{scene.name}\"  ({len(scene.roots)} root object(s))")
        for root in scene.roots:
            _print_gameobject(root, "  ")
    elif path.suffix == ".prefab":
        prefab = reader.read_prefab(text)
        print("Prefab")
        _print_gameobject(prefab.root, "  ")
        if prefab.copied_object_guids:
            print(f"  copied instances: {len(prefab.copied_object_guids)} "
                  f"({', '.join(_short(g) for g in prefab.copied_object_guids)})")
    else:
        print(f"error: unrecognised extension {path.suffix!r} (expected .scene or .prefab)")
        return 1
    return 0


def cmd_validate(args: argparse.Namespace) -> int:
    path = Path(args.file)
    text = read_text(path)
    if path.suffix == ".scene":
        problems = validate.validate_scene(reader.read_scene(text))
    elif path.suffix == ".prefab":
        problems = validate.validate_prefab(reader.read_prefab(text))
    else:
        print(f"error: unrecognised extension {path.suffix!r} (expected .scene or .prefab)")
        return 1
    if not problems:
        print(f"OK: {path.name} - no problems found")
        return 0
    hard = [p for p in problems if not p.startswith("note:")]
    for p in problems:
        print(("NOTE  " if p.startswith("note:") else "FAIL  ") + p)
    return 1 if hard else 0


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("show", help="print a .scene/.prefab GameObject tree (read-only)")
    sp.add_argument("file")
    sp.set_defaults(func=cmd_show)

    sp = sub.add_parser("validate", help="static checks on a .scene/.prefab (read-only)")
    sp.add_argument("file")
    sp.set_defaults(func=cmd_validate)
