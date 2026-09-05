"""Structural/Transform edit CLI subcommands: add/remove/move-gameobject,
set-transform, set-active, rename-gameobject, apply. Every mutating subcommand
supports ``--dry-run`` (print a diff, write nothing) and validates before
writing (see ``tools.scene.validate``)."""

from __future__ import annotations

import argparse
import difflib
import json
from pathlib import Path
from typing import Any

from tools.common.cereal_json import read_text, to_file_bytes

from . import edits, model, reader, validate, writer


def _load(path: Path):
    text = read_text(path)
    if path.suffix == ".scene":
        return reader.read_scene(text), "scene"
    if path.suffix == ".prefab":
        return reader.read_prefab(text), "prefab"
    raise SystemExit(f"error: unrecognised extension {path.suffix!r} (expected .scene or .prefab)")


def _dump(target: Any, kind: str) -> str:
    return writer.write_scene(target) if kind == "scene" else writer.write_prefab(target)


def _validate(target: Any, kind: str) -> list[str]:
    return validate.validate_scene(target) if kind == "scene" else validate.validate_prefab(target)


def _commit(path: Path, target: Any, kind: str, orig_text: str, *, dry_run: bool) -> int:
    problems = _validate(target, kind)
    hard = [p for p in problems if not p.startswith("note:")]
    for p in problems:
        print(("note: " if p.startswith("note:") else "FAIL: ") + p)
    if hard:
        print("validation failed - nothing written")
        return 1
    new_text = _dump(target, kind)
    if dry_run:
        diff = difflib.unified_diff(
            orig_text.splitlines(keepends=True), new_text.splitlines(keepends=True),
            fromfile=str(path), tofile=str(path) + " (after)",
        )
        sys_out = "".join(diff)
        print(sys_out if sys_out else "(no changes)")
        print("[dry-run] nothing written")
        return 0
    path.write_bytes(to_file_bytes(new_text))
    print(f"wrote {path}")
    return 0


def _parse_vec3(s: str) -> tuple[float, float, float]:
    parts = [float(x) for x in s.split(",")]
    if len(parts) != 3:
        raise argparse.ArgumentTypeError(f"expected x,y,z - got {s!r}")
    return (parts[0], parts[1], parts[2])


def _parse_quat(s: str) -> tuple[float, float, float, float]:
    parts = [float(x) for x in s.split(",")]
    if len(parts) != 4:
        raise argparse.ArgumentTypeError(f"expected x,y,z,w - got {s!r}")
    return (parts[0], parts[1], parts[2], parts[3])


# ---------------------------------------------------------------------------
# subcommands
# ---------------------------------------------------------------------------
def cmd_add_gameobject(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        node = edits.add_gameobject(
            target, parent=args.parent, name=args.name,
            pos=_parse_vec3(args.pos) if args.pos else (0.0, 0.0, 0.0),
            rot=_parse_quat(args.rot) if args.rot else (0.0, 0.0, 0.0, 1.0),
            scale=_parse_vec3(args.scale) if args.scale else (1.0, 1.0, 1.0),
            is_active=not args.inactive,
        )
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"added {node.name} [{node.guid}]")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_remove_gameobject(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        node = edits.remove_gameobject(target, args.guid)
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"removed {node.name} [{node.guid}]")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_move_gameobject(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        edits.move_gameobject(target, guid=args.guid, new_parent=args.new_parent,
                              preserve_world_transform=not args.no_preserve_world)
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"moved {args.guid} -> parent {args.new_parent or '(scene root)'}")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_set_transform(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        edits.set_transform(
            target, args.guid,
            pos=_parse_vec3(args.pos) if args.pos else None,
            rot=_parse_quat(args.rot) if args.rot else None,
            scale=_parse_vec3(args.scale) if args.scale else None,
        )
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"set-transform: {args.guid}")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_set_active(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        edits.set_active(target, args.guid, args.active == "true")
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"set-active: {args.guid} -> {args.active}")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_rename_gameobject(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        edits.rename_gameobject(target, args.guid, args.name)
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"renamed {args.guid} -> {args.name}")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def _parse_kv_list(pairs: list[str] | None) -> dict[str, str]:
    out: dict[str, str] = {}
    for kv in pairs or []:
        if "=" not in kv:
            raise argparse.ArgumentTypeError(f"expected key=value - got {kv!r}")
        k, v = kv.split("=", 1)
        out[k] = v
    return out


def cmd_add_component(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        comp = edits.add_component(target, args.guid, args.type,
                                   params=_parse_kv_list(args.param))
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"added component {comp.fqn} to {args.guid}")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_remove_component(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        comp = edits.remove_component(target, args.guid, args.index)
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"removed component {comp.fqn} (#{args.index}) from {args.guid}")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_set_component_params(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    try:
        touched = edits.set_component_params(target, args.guid, args.index, _parse_kv_list(args.set))
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"set-component-params: {args.guid}#{args.index} -> {touched}")
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_instantiate_prefab(args: argparse.Namespace) -> int:
    into_path = Path(args.into)
    target, kind = _load(into_path)
    orig_text = _dump(target, kind)
    prefab = reader.read_prefab_file(Path(args.prefab_path))
    try:
        node = edits.instantiate_prefab(target, prefab, parent=args.parent)
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    print(f"instantiated {args.prefab_path} -> {node.name} [{node.guid}]")
    return _commit(into_path, target, kind, orig_text, dry_run=args.dry_run)


def cmd_apply(args: argparse.Namespace) -> int:
    path = Path(args.file)
    target, kind = _load(path)
    orig_text = _dump(target, kind)
    ops = json.loads(Path(args.ops_file).read_text(encoding="utf-8"))
    if not isinstance(ops, list):
        print("error: ops file must contain a JSON array of {\"op\": ...} objects")
        return 1
    try:
        log = edits.apply(target, ops)
    except edits.EditError as e:
        print(f"error: {e}")
        return 1
    for line in log:
        print(line)
    return _commit(path, target, kind, orig_text, dry_run=args.dry_run)


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("add-gameobject", help="add a new GameObject")
    sp.add_argument("file")
    sp.add_argument("--parent", help="guid of the parent (omit for a scene root; "
                                     "use 'root' for a prefab's own root)")
    sp.add_argument("--name", required=True)
    sp.add_argument("--pos", help="x,y,z")
    sp.add_argument("--rot", help="x,y,z,w (quaternion)")
    sp.add_argument("--scale", help="x,y,z")
    sp.add_argument("--inactive", action="store_true")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_add_gameobject)

    sp = sub.add_parser("remove-gameobject", help="remove a GameObject by guid")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True)
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_remove_gameobject)

    sp = sub.add_parser("move-gameobject", help="reparent a GameObject")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True)
    sp.add_argument("--new-parent", dest="new_parent",
                    help="guid of the new parent (omit for a scene root; 'root' for a prefab's own root)")
    sp.add_argument("--no-preserve-world", action="store_true",
                    help="keep local pos/rot/scale unchanged instead of preserving world transform")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_move_gameobject)

    sp = sub.add_parser("set-transform", help="set local pos/rot/scale")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True)
    sp.add_argument("--pos", help="x,y,z")
    sp.add_argument("--rot", help="x,y,z,w (quaternion)")
    sp.add_argument("--scale", help="x,y,z")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_set_transform)

    sp = sub.add_parser("set-active", help="enable/disable a GameObject")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True)
    sp.add_argument("--active", choices=["true", "false"], required=True)
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_set_active)

    sp = sub.add_parser("rename-gameobject", help="rename a GameObject")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True)
    sp.add_argument("--name", required=True)
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_rename_gameobject)

    sp = sub.add_parser("add-component", help="attach a new component instance to a GameObject")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True, help="the GameObject's guid")
    sp.add_argument("--type", required=True, help="catalog component name (FQN or unambiguous leaf)")
    sp.add_argument("--param", action="append", help="key=value, repeatable")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_add_component)

    sp = sub.add_parser("remove-component", help="remove a component by index")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True, help="the GameObject's guid")
    sp.add_argument("--index", type=int, required=True, help="index into that GameObject's component list")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_remove_component)

    sp = sub.add_parser("set-component-params", help="set one or more param values on an existing component")
    sp.add_argument("file")
    sp.add_argument("--guid", required=True, help="the GameObject's guid")
    sp.add_argument("--index", type=int, required=True, help="index into that GameObject's component list")
    sp.add_argument("--set", action="append", required=True, help="key=value, repeatable")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_set_component_params)

    sp = sub.add_parser("instantiate-prefab",
                        help="deep-copy a .prefab's tree into a .scene/.prefab, with fresh guids")
    sp.add_argument("prefab_path")
    sp.add_argument("--into", required=True, help="the target .scene/.prefab file")
    sp.add_argument("--parent", help="guid of the parent (omit for a scene root; "
                                     "'root' for the target prefab's own root)")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_instantiate_prefab)

    sp = sub.add_parser("apply", help="apply a batch of edits from a JSON ops file")
    sp.add_argument("file")
    sp.add_argument("ops_file")
    sp.add_argument("--dry-run", action="store_true")
    sp.set_defaults(func=cmd_apply)
