"""CLI edit verbs: add-clip-node, remove-node, set-node-params, move-node,
add-transition, remove-transition, set-transition-params, add-condition,
remove-condition, add-param, remove-param, set-param, apply, validate."""

from __future__ import annotations

import argparse
import difflib
import json
import sys
from pathlib import Path

from . import edits, meta
from .cereal_json import read_text, to_file_bytes
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


def _commit(path: Path, old_text: str, tree, *, dry_run: bool) -> int:
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


def _pos(arg: str | None) -> tuple[float, float] | None:
    if arg is None:
        return None
    parts = [p.strip() for p in arg.replace(" ", ",").split(",") if p.strip()]
    if len(parts) != 2:
        raise SystemExit(f"error: --pos expects X,Y - got {arg!r}")
    return (float(parts[0]), float(parts[1]))


def _transition_addr(a: argparse.Namespace) -> dict:
    if a.index is None and (a.from_ is None or a.next is None):
        raise SystemExit("error: give --index, or both --from and --next")
    return dict(any_state=a.any_state, index=a.index, from_guid=a.from_, next_guid=a.next)


# ---------------------------------------------------------------------------
def cmd_add_clip_node(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    clip_guid = edits.resolve_clip_arg(a.clip, _REPO)
    node = edits.add_clip_node(tree, name=a.name, clip_guid=clip_guid, speed=a.speed,
                               blend_offset_secs=a.blend_offset, model_anim_index=a.model_anim_index,
                               pos=_pos(a.pos))
    print(f"new AnimationClipNode: {node.guid}")
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_remove_node(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.remove_node(tree, a.node, cascade=a.cascade)
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_set_node_params(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    assignments = {}
    for kv in a.assignments:
        if "=" not in kv:
            raise SystemExit(f"error: expected KEY=VALUE, got {kv!r}")
        k, v = kv.split("=", 1)
        assignments[k] = v
    touched = edits.set_node_params(tree, a.node, assignments)
    print(f"set {touched}")
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_move_node(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.move_node(tree, a.node, _pos(a.pos))
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_add_transition(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    t = edits.add_transition(tree, from_guid=a.from_, next_guid=a.next, any_state=a.any_state,
                             duration_secs=a.duration, visual_from_guid=a.visual_from)
    print(f"new transition: {t.from_guid} -> {t.next_guid}")
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_remove_transition(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.remove_transition(tree, **_transition_addr(a))
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_set_transition_params(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.set_transition_params(tree, duration_secs=a.duration, visual_from_guid=a.visual_from,
                                **_transition_addr(a))
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_add_condition(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.add_condition(tree, name=a.name, kind=a.kind, value=a.value, **_transition_addr(a))
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_remove_condition(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.remove_condition(tree, condition_index=a.condition_index, **_transition_addr(a))
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_add_param(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.add_param(tree, a.name, a.kind, a.value)
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_remove_param(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.remove_param(tree, a.name)
    return _commit(path, text, tree, dry_run=a.dry_run)


def cmd_set_param(a: argparse.Namespace) -> int:
    path = _path(a.file)
    text, tree = _load(path)
    edits.set_param(tree, a.name, a.value)
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
    return _commit(path, text, tree, dry_run=a.dry_run)


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


def _add_transition_addr_args(p: argparse.ArgumentParser) -> None:
    p.add_argument("--any-state", action="store_true",
                   help="address the any-state transition list instead of the direct one")
    p.add_argument("--index", type=int, help="transition index (see: show)")
    p.add_argument("--from", dest="from_", help="source node guid (with --next)")
    p.add_argument("--next", help="target node guid (with --from)")


def register(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("add-clip-node", help="add an AnimationClipNode")
    p.add_argument("file")
    p.add_argument("--name", required=True)
    p.add_argument("--clip", required=True, help="Mv1File asset guid, or a path to a .mv1/.mv1.meta")
    p.add_argument("--speed", type=float, default=1.0)
    p.add_argument("--blend-offset", type=float, default=0.0, dest="blend_offset")
    p.add_argument("--model-anim-index", type=int, default=0, dest="model_anim_index")
    p.add_argument("--pos", help="editor position X,Y (default: an auto-placed grid slot)")
    _add_dry(p)
    p.set_defaults(func=cmd_add_clip_node)

    p = sub.add_parser("remove-node", help="remove a node (refuses Entry/AnyState)")
    p.add_argument("file")
    p.add_argument("--node", required=True)
    p.add_argument("--cascade", action="store_true",
                   help="also remove any transition referencing this node")
    _add_dry(p)
    p.set_defaults(func=cmd_remove_node)

    p = sub.add_parser("set-node-params", help="set a node's parameters")
    p.add_argument("file")
    p.add_argument("--node", required=True)
    p.add_argument("assignments", nargs="+", metavar="KEY=VALUE")
    _add_dry(p)
    p.set_defaults(func=cmd_set_node_params)

    p = sub.add_parser("move-node", help="reposition a node on the editor canvas")
    p.add_argument("file")
    p.add_argument("--node", required=True)
    p.add_argument("--pos", required=True, help="X,Y")
    _add_dry(p)
    p.set_defaults(func=cmd_move_node)

    p = sub.add_parser("add-transition", help="add a transition between two nodes")
    p.add_argument("file")
    p.add_argument("--from", dest="from_", required=True)
    p.add_argument("--next", required=True)
    p.add_argument("--any-state", action="store_true",
                   help="an any-state transition (--from must be the AnyState node's guid)")
    p.add_argument("--duration", type=float, default=0.0, dest="duration")
    p.add_argument("--visual-from", dest="visual_from", help="editor-only; defaults to --from")
    _add_dry(p)
    p.set_defaults(func=cmd_add_transition)

    p = sub.add_parser("remove-transition", help="remove a transition")
    p.add_argument("file")
    _add_transition_addr_args(p)
    _add_dry(p)
    p.set_defaults(func=cmd_remove_transition)

    p = sub.add_parser("set-transition-params", help="set a transition's duration/visual-from")
    p.add_argument("file")
    _add_transition_addr_args(p)
    p.add_argument("--duration", type=float, dest="duration")
    p.add_argument("--visual-from", dest="visual_from")
    _add_dry(p)
    p.set_defaults(func=cmd_set_transition_params)

    p = sub.add_parser("add-condition", help="add an equality condition to a transition")
    p.add_argument("file")
    _add_transition_addr_args(p)
    p.add_argument("--name", required=True, help="parameter name to check")
    p.add_argument("--kind", required=True, choices=["bool", "int", "float"])
    p.add_argument("--value", required=True)
    _add_dry(p)
    p.set_defaults(func=cmd_add_condition)

    p = sub.add_parser("remove-condition", help="remove a condition from a transition")
    p.add_argument("file")
    _add_transition_addr_args(p)
    p.add_argument("--condition-index", type=int, required=True, dest="condition_index")
    _add_dry(p)
    p.set_defaults(func=cmd_remove_condition)

    p = sub.add_parser("add-param", help="add an additionParameters_ entry")
    p.add_argument("file")
    p.add_argument("--name", required=True)
    p.add_argument("--kind", required=True, choices=["bool", "int", "float"])
    p.add_argument("--value", required=True)
    _add_dry(p)
    p.set_defaults(func=cmd_add_param)

    p = sub.add_parser("remove-param", help="remove an additionParameters_ entry")
    p.add_argument("file")
    p.add_argument("--name", required=True)
    _add_dry(p)
    p.set_defaults(func=cmd_remove_param)

    p = sub.add_parser("set-param", help="set an additionParameters_ entry's value")
    p.add_argument("file")
    p.add_argument("--name", required=True)
    p.add_argument("--value", required=True)
    _add_dry(p)
    p.set_defaults(func=cmd_set_param)

    p = sub.add_parser("apply", help="apply a JSON batch of edit ops atomically")
    p.add_argument("file")
    p.add_argument("ops", help="path to a JSON array of {op: ...}")
    _add_dry(p)
    p.set_defaults(func=cmd_apply)

    p = sub.add_parser("validate", help="check a tree against the catalog")
    p.add_argument("file")
    p.set_defaults(func=cmd_validate)
