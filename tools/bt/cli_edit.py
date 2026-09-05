"""CLI edit verbs: add-node, remove-node, move-node, set-params, set-weight,
add-bb-param, remove-bb-param, apply, validate."""

from __future__ import annotations

import argparse
import difflib
import json
import sys
from pathlib import Path

from . import catalog as catalog_mod
from . import edits, meta
from .cereal_json import read_text, to_file_bytes
from .layout import DX, DY, auto_layout
from .reader import read_tree
from .validate import validate
from .writer import write_tree

_REPO = Path(__file__).resolve().parents[2]


def _path(arg: str) -> Path:
    p = Path(arg)
    if not str(p).endswith(meta.DATA_EXT):
        p = Path(str(p) + meta.DATA_EXT)
    if not p.is_absolute() and not p.exists():
        p = _REPO / p
    if not p.exists():
        raise SystemExit(f"error: {p} not found")
    return p


def _load(path: Path):
    text = read_text(path)
    return text, read_tree(text)


def _commit(path: Path, old_text: str, tree, *, dry_run: bool, layout: bool = False) -> int:
    if layout:
        auto_layout(tree)
    problems = validate(tree)
    hard = [p for p in problems if not p.startswith("note:")]
    for p in problems:
        print(("  " if p.startswith("note:") else "  ! ") + p, file=sys.stderr)
    if hard:
        print("aborted: validation failed (no changes written)", file=sys.stderr)
        return 1
    new_text = write_tree(tree)
    if new_text == old_text:
        print("no change")
        return 0
    if dry_run:
        diff = difflib.unified_diff(old_text.splitlines(True), new_text.splitlines(True),
                                    fromfile=str(path.name), tofile=str(path.name) + " (new)")
        sys.stdout.writelines(diff)
        return 0
    path.write_bytes(to_file_bytes(new_text))
    print(f"wrote {path.name}")
    return 0


# ---------------------------------------------------------------------------
def cmd_add_node(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    pos = tuple(float(x) for x in a.pos.split(",")) if a.pos else None
    node = edits.add_node(tree, parent_guid=a.parent, kind=a.kind, name=a.name,
                          action_type=a.type, index=a.index, weight=a.weight, pos=pos)
    print(f"new {a.kind} node: {node.guid}")
    return _commit(path, text, tree, dry_run=a.dry_run,
                   layout=not a.no_layout and pos is None)


def cmd_copy_node(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    pos = tuple(float(x) for x in a.pos.split(",")) if a.pos else None
    node = edits.copy_node(tree, src_guid=a.node, parent_guid=a.parent, index=a.index,
                           weight=a.weight, pos=pos)
    print(f"copy of {a.node} -> {node.guid}  (run `show` to see the copied subtree's guids)")
    return _commit(path, text, tree, dry_run=a.dry_run,
                   layout=not a.no_layout and pos is None)


def cmd_remove_node(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.remove_node(tree, a.node)
    return _commit(path, text, tree, dry_run=a.dry_run, layout=not a.no_layout)


def cmd_move_node(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.move_node(tree, guid=a.node, parent_guid=a.parent, index=a.index, weight=a.weight)
    return _commit(path, text, tree, dry_run=a.dry_run, layout=not a.no_layout)


def cmd_set_params(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    assignments = {}
    for kv in a.assignments:
        if "=" not in kv:
            raise SystemExit(f"error: expected KEY=VALUE, got {kv!r}")
        k, v = kv.split("=", 1)
        assignments[k] = v
    touched = edits.set_params(tree, a.node, assignments)
    print(f"set {touched}")
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_set_weight(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.set_weight(tree, child_guid=a.node, parent_guid=a.parent, index=a.index, weight=a.weight)
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_bb_add(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.add_bb_param(tree, a.name, a.int)
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_bb_remove(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.remove_bb_param(tree, a.name)
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_apply(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    ops = json.loads(Path(a.ops).read_text(encoding="utf-8"))
    if not isinstance(ops, list):
        raise SystemExit("error: ops file must be a JSON array of {op: ...} objects")
    log = edits.apply(tree, ops)
    for line in log:
        print("  " + line)
    return _commit(path, text, tree, dry_run=a.dry_run, layout=not a.no_layout)


def cmd_layout(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    auto_layout(tree, dx=a.dx, dy=a.dy)
    new_text = write_tree(tree)
    if new_text == text:
        print("no change")
        return 0
    if a.dry_run:
        sys.stdout.writelines(difflib.unified_diff(
            text.splitlines(True), new_text.splitlines(True),
            fromfile=path.name, tofile=path.name + " (new)"))
        return 0
    path.write_bytes(to_file_bytes(new_text))
    print(f"laid out {path.name}")
    return 0


def cmd_validate(a: argparse.Namespace) -> int:
    path = _path(a.file)
    _text, tree = _load(path)
    problems = validate(tree)
    if not problems:
        print("ok")
        return 0
    hard = 0
    for p in problems:
        if p.startswith("note:"):
            print("  " + p)
        else:
            hard += 1
            print("  ! " + p)
    return 1 if hard else 0


# ---------------------------------------------------------------------------
def _add_dry(p: argparse.ArgumentParser) -> None:
    p.add_argument("--dry-run", action="store_true", help="print a diff, write nothing")


def _add_layout(p: argparse.ArgumentParser) -> None:
    p.add_argument("--no-layout", action="store_true",
                   help="keep existing node positions instead of auto-arranging the tree")


def register(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("add-node", help="add a node under a parent")
    p.add_argument("file")
    p.add_argument("--parent", required=True, help='parent guid, or "entry"')
    p.add_argument("--kind", required=True,
                   choices=["selector", "sequence", "random", "once-exec", "once-success", "action"])
    p.add_argument("--name", help="ActionNode label (kind=action)")
    p.add_argument("--type", help="action type: display name / fqn / leaf (kind=action)")
    p.add_argument("--index", type=int, help="insert position among siblings")
    p.add_argument("--weight", type=int, default=100, help="RandomSelector weight")
    p.add_argument("--pos", help="editor position X,Y (implies --no-layout)")
    _add_dry(p)
    _add_layout(p)
    p.set_defaults(func=cmd_add_node)

    p = sub.add_parser("copy-node", help="deep-copy a node (and its subtree) under a new parent")
    p.add_argument("file")
    p.add_argument("--node", required=True, help="guid of the subtree to copy")
    p.add_argument("--parent", required=True, help='destination parent guid, or "entry"')
    p.add_argument("--index", type=int, help="insert position among siblings")
    p.add_argument("--weight", type=int, default=100, help="RandomSelector weight")
    p.add_argument("--pos", help="editor position X,Y for the copy's root (implies --no-layout)")
    _add_dry(p)
    _add_layout(p)
    p.set_defaults(func=cmd_copy_node)

    p = sub.add_parser("remove-node", help="detach a node (and its subtree)")
    p.add_argument("file")
    p.add_argument("--node", required=True)
    _add_dry(p)
    _add_layout(p)
    p.set_defaults(func=cmd_remove_node)

    p = sub.add_parser("move-node", help="reparent a node")
    p.add_argument("file")
    p.add_argument("--node", required=True)
    p.add_argument("--parent", required=True)
    p.add_argument("--index", type=int)
    p.add_argument("--weight", type=int, default=100)
    _add_dry(p)
    _add_layout(p)
    p.set_defaults(func=cmd_move_node)

    p = sub.add_parser("set-params", help="set an action's parameters")
    p.add_argument("file")
    p.add_argument("--node", required=True)
    p.add_argument("assignments", nargs="+", metavar="KEY=VALUE")
    _add_dry(p)
    p.set_defaults(func=cmd_set_params)

    p = sub.add_parser("set-weight", help="set a RandomSelector child weight")
    p.add_argument("file")
    p.add_argument("--node", help="child guid")
    p.add_argument("--parent", help="RandomSelector guid (with --index)")
    p.add_argument("--index", type=int)
    p.add_argument("--weight", type=int, required=True)
    _add_dry(p)
    p.set_defaults(func=cmd_set_weight)

    p = sub.add_parser("add-bb-param", help="add a blackboard int parameter")
    p.add_argument("file")
    p.add_argument("--name", required=True)
    p.add_argument("--int", type=int, required=True, dest="int")
    _add_dry(p)
    p.set_defaults(func=cmd_bb_add)

    p = sub.add_parser("remove-bb-param", help="remove a blackboard parameter")
    p.add_argument("file")
    p.add_argument("--name", required=True)
    _add_dry(p)
    p.set_defaults(func=cmd_bb_remove)

    p = sub.add_parser("apply", help="apply a JSON batch of edit ops atomically")
    p.add_argument("file")
    p.add_argument("ops", help="path to a JSON array of {op: ...}")
    _add_dry(p)
    _add_layout(p)
    p.set_defaults(func=cmd_apply)

    p = sub.add_parser("layout",
                       help="re-arrange nodes (selectors branch across, sequences stack down)")
    p.add_argument("file")
    p.add_argument("--dx", type=float, default=DX, help=f"leaf column width (default {DX:g})")
    p.add_argument("--dy", type=float, default=DY, help=f"row / level step (default {DY:g})")
    _add_dry(p)
    p.set_defaults(func=cmd_layout)

    p = sub.add_parser("validate", help="check a tree against the catalog")
    p.add_argument("file")
    p.set_defaults(func=cmd_validate)
