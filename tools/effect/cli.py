"""CLI subcommands for tools.effect: new-project, show, validate, add-node,
set-params, apply, compile, install.

Nodes have no stable id in the ``.efkproj`` format (unlike ``tools/bt``'s
per-node GUIDs), so ``--parent``/``--path`` address a node by a dot-separated
0-based child-index path from the root, e.g. ``"1.0"`` = the root's 2nd child
node's 1st child node. ``""`` (or ``"root"``) means the root itself - use it
as ``--parent`` to attach a new top-level node. ``show`` prints these paths.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from . import meta as meta_mod
from . import presets as p
from . import xmlio
from .model import Elem

_REPO = Path(__file__).resolve().parents[2]

# Pinned CUI: verified this session that its .efkefc output's INFO-chunk
# version matches assets already shipped in Assets/Art/Effect/ byte-for-byte;
# the newer 1.80.2 build also present under Downloads/ was NOT verified and
# should not be used for assets that ship in this repo.
DEFAULT_CUI_PATH = r"C:\Users\e29sw\Downloads\Effekseer1.7.3.0Win\Tool\Effekseer.exe"

_KIND_BUILDERS = {
    "sprite": lambda: p.drawing_values("sprite", p.sprite()),
    "ring": lambda: p.drawing_values("ring", p.ring()),
    "ribbon": lambda: p.drawing_values("ribbon", p.ribbon()),
}


class CliError(RuntimeError):
    pass


def _resolve(arg: str) -> Path:
    path = Path(arg)
    if path.is_absolute():
        return path
    if path.exists():
        return path.resolve()
    return _REPO / path


# ---------------------------------------------------------------------------
# node-path addressing
def resolve_node(project: Elem, path: str) -> Elem:
    root = project.require("Root")
    if path in ("", "root", "."):
        return root
    children = root.require("Children")
    node = None
    for part in path.split("."):
        try:
            idx = int(part)
        except ValueError as e:
            raise CliError(f"bad node path {path!r}: {part!r} is not an index") from e
        if idx < 0 or idx >= len(children.children):
            raise CliError(f"no node at index {idx} in path {path!r} "
                            f"(only {len(children.children)} children there)")
        node = children.children[idx]
        kids = node.child("Children")
        children = kids if kids is not None else Elem("Children")
    return node


def _drawing_kind(node: Elem) -> str | None:
    dv = node.child("DrawingValues")
    if dv is None:
        return None
    t = dv.child("Type")
    type_val = t.text if t is not None else "0"
    for kind, num in p.DRAWING_TYPE.items():
        if str(num) == type_val:
            return kind
    return f"type={type_val}"


# ---------------------------------------------------------------------------
def cmd_new_project(args: argparse.Namespace) -> int:
    target_dir = Path(args.dir)
    if not target_dir.is_absolute():
        target_dir = Path.cwd() / target_dir
    target_dir.mkdir(parents=True, exist_ok=True)
    out_path = target_dir / f"{args.name}.efkproj"
    if out_path.exists() and not args.force:
        print(f"error: {out_path} already exists (use --force)", file=sys.stderr)
        return 1

    proj = p.new_project(start_frame=args.start, end_frame=args.end, is_loop=args.loop)
    xmlio.write(out_path, proj)
    print(f"created {out_path}")
    return 0


def cmd_show(args: argparse.Namespace) -> int:
    proj = xmlio.read(_resolve(args.file))
    root = proj.require("Root")
    print(f"Root  (StartFrame={proj.child('StartFrame').text} "
          f"EndFrame={proj.child('EndFrame').text} IsLoop={proj.child('IsLoop').text})")
    kids = root.child("Children")
    if kids:
        _print_children(kids, 1, "")
    return 0


def _print_children(children: Elem, depth: int, prefix: str) -> None:
    pad = "  " * depth
    for i, node in enumerate(children.children):
        path = f"{prefix}{i}" if prefix == "" else f"{prefix}.{i}"
        name = node.child("Name")
        name_text = name.text if name is not None else "?"
        kind = _drawing_kind(node)
        kind_str = f"  [{kind}]" if kind else ""
        print(f"{pad}[{path}] {name_text}{kind_str}")
        sub = node.child("Children")
        if sub and sub.children:
            _print_children(sub, depth + 1, path)


def cmd_validate(args: argparse.Namespace) -> int:
    path = _resolve(args.file)
    try:
        proj = xmlio.read(path)
    except Exception as e:  # noqa: BLE001
        print(f"FAIL  not well-formed XML: {e}", file=sys.stderr)
        return 1

    problems: list[str] = []
    if proj.tag != "EffekseerProject":
        problems.append(f"root element is <{proj.tag}>, expected <EffekseerProject>")
    for required in ("Root", "ToolVersion", "Version", "StartFrame", "EndFrame", "IsLoop"):
        if proj.child(required) is None:
            problems.append(f"missing top-level <{required}>")

    root = proj.child("Root")
    node_count = 0
    unknown_kinds: set[str] = set()
    if root is not None:
        kids = root.child("Children")
        if kids is not None:
            def walk(children: Elem) -> None:
                nonlocal node_count
                for node in children.children:
                    node_count += 1
                    kind = _drawing_kind(node)
                    if kind and kind.startswith("type="):
                        unknown_kinds.add(kind)
                    sub = node.child("Children")
                    if sub:
                        walk(sub)
            walk(kids)

    for k in sorted(unknown_kinds):
        problems.append(f"unsupported DrawingValues {k} (v1 only models sprite/ring/ribbon; "
                         "this may still compile fine via the CUI, it just wasn't built by this toolkit)")

    if problems:
        print(f"{len(problems)} problem(s) in {path.name}:")
        for msg in problems:
            print(f"  - {msg}")
        return 1
    print(f"OK  {path.name}  ({node_count} node(s))")
    return 0


def cmd_add_node(args: argparse.Namespace) -> int:
    path = _resolve(args.file)
    proj = xmlio.read(path)
    parent = resolve_node(proj, args.parent)
    parent_children = parent.child_or_add("Children")

    if args.kind == "group":
        new_node = p.group_node(args.name)
    elif args.kind in _KIND_BUILDERS:
        new_node = p.node(args.name, drawing=_KIND_BUILDERS[args.kind]())
    else:
        raise CliError(f"unknown --kind {args.kind!r}")

    for assignment in args.set or []:
        key, _, value = assignment.partition("=")
        new_node.set_path(key, value)

    parent_children.children.append(new_node)
    xmlio.write(path, proj)
    idx = len(parent_children.children) - 1
    new_path = idx if args.parent in ("", "root", ".") else f"{args.parent}.{idx}"
    print(f"added [{new_path}] {args.name} ({args.kind}) under [{args.parent or 'root'}]")
    return 0


def cmd_set_params(args: argparse.Namespace) -> int:
    path = _resolve(args.file)
    proj = xmlio.read(path)
    node = resolve_node(proj, args.path)
    for assignment in args.set or []:
        key, _, value = assignment.partition("=")
        node.set_path(key, value)
    xmlio.write(path, proj)
    print(f"updated [{args.path}] ({len(args.set)} field(s))")
    return 0


def cmd_apply(args: argparse.Namespace) -> int:
    path = _resolve(args.file)
    ops = json.loads(Path(args.ops).read_text(encoding="utf-8"))
    proj = xmlio.read(path)
    for i, op in enumerate(ops):
        kind = op.get("op")
        if kind == "add-node":
            parent = resolve_node(proj, op.get("parent", ""))
            parent_children = parent.child_or_add("Children")
            node_kind = op["kind"]
            if node_kind == "group":
                new_node = p.group_node(op.get("name", "Node"))
            elif node_kind in _KIND_BUILDERS:
                new_node = p.node(op.get("name", "Node"), drawing=_KIND_BUILDERS[node_kind]())
            else:
                raise CliError(f"op {i}: unknown kind {node_kind!r}")
            for key, value in (op.get("set") or {}).items():
                new_node.set_path(key, str(value))
            parent_children.children.append(new_node)
        elif kind == "set-params":
            node = resolve_node(proj, op["path"])
            for key, value in (op.get("set") or {}).items():
                node.set_path(key, str(value))
        else:
            raise CliError(f"op {i}: unknown op {kind!r}")
    xmlio.write(path, proj)
    print(f"applied {len(ops)} op(s) to {path.name}")
    return 0


def _find_cui_path(args: argparse.Namespace) -> Path:
    for candidate in (args.cui_path, os.environ.get("EFFEKSEER_CUI"), DEFAULT_CUI_PATH):
        if candidate and Path(candidate).exists():
            return Path(candidate)
    raise CliError(
        "Effekseer CUI not found. Tried --cui-path, $EFFEKSEER_CUI, and the pinned default "
        f"({DEFAULT_CUI_PATH}). Pass --cui-path to point at a local Effekseer 1.7.3.0 "
        "Tool/Effekseer.exe (this toolkit is pinned to 1.7.3.0 - its .efkefc output was "
        "verified to match assets already shipped in Assets/Art/Effect/ byte-for-byte)."
    )


def cmd_compile(args: argparse.Namespace) -> int:
    in_path = _resolve(args.file)
    out_path = _resolve(args.out) if args.out else in_path.with_suffix(".efkefc")
    cui = _find_cui_path(args)

    result = subprocess.run(
        [str(cui), "-cui", "-in", str(in_path), "-o", str(out_path)],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"CUI exited {result.returncode}", file=sys.stderr)
        print(result.stdout, file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        return 1
    if not out_path.exists():
        print(f"CUI exited 0 but {out_path} was not created", file=sys.stderr)
        return 1
    header = out_path.read_bytes()[:16]
    if not header.startswith(b"EFKE") or b"INFO" not in header:
        print(f"{out_path} does not look like a valid .efkefc (header {header!r})", file=sys.stderr)
        return 1
    print(f"compiled {out_path}")
    return 0


def cmd_install(args: argparse.Namespace) -> int:
    efkefc_src = _resolve(args.efkefc)
    dest = _resolve(args.dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(efkefc_src, dest)

    default_dir = (_REPO / meta_mod.DEFAULT_DIR).resolve()
    try:
        rel = dest.resolve().relative_to(default_dir)
    except ValueError:
        rel = Path(dest.name)

    if args.project:
        project_src = _resolve(args.project)
        source_dest = default_dir / "_Source" / rel.with_suffix(".efkproj")
        source_dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(project_src, source_dest)
        print(f"copied source {source_dest}")

    name = dest.stem
    guid = meta_mod.mint_guid()
    content_path = meta_mod.content_path_for(name, dest.parent, _REPO)
    meta_path = dest.with_suffix(dest.suffix + ".meta")
    meta_mod.write_meta(meta_path, name, guid, content_path)

    print(f"installed {dest}")
    print(f"          {meta_path.name}")
    print(f"GUID:     {guid}")
    return 0


# ---------------------------------------------------------------------------
def _wrap(fn):
    def run(args: argparse.Namespace) -> int:
        try:
            return fn(args)
        except CliError as e:
            print(f"error: {e}", file=sys.stderr)
            return 1
    return run


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("new-project", help="create a new .efkproj skeleton")
    sp.add_argument("name")
    sp.add_argument("--dir", default=".", help="target directory (default: cwd)")
    sp.add_argument("--start", type=int, default=0)
    sp.add_argument("--end", type=int, default=60)
    sp.add_argument("--loop", type=lambda s: s.lower() == "true", default=True, metavar="true|false")
    sp.add_argument("--force", action="store_true")
    sp.set_defaults(func=_wrap(cmd_new_project))

    sp = sub.add_parser("show", help="print a .efkproj node tree as an outline")
    sp.add_argument("file")
    sp.set_defaults(func=_wrap(cmd_show))

    sp = sub.add_parser("validate", help="structural sanity checks")
    sp.add_argument("file")
    sp.set_defaults(func=_wrap(cmd_validate))

    sp = sub.add_parser("add-node", help="add a node under an existing node/root")
    sp.add_argument("file")
    sp.add_argument("--parent", default="", help='node path, e.g. "1.0" (default: root)')
    sp.add_argument("--kind", required=True, choices=["sprite", "ring", "ribbon", "group"])
    sp.add_argument("--name", default="Node")
    sp.add_argument("--set", action="append", metavar="dotted.path=value",
                     help="may be repeated")
    sp.set_defaults(func=_wrap(cmd_add_node))

    sp = sub.add_parser("set-params", help="set fields on an existing node")
    sp.add_argument("file")
    sp.add_argument("--path", required=True, help='node path, e.g. "1.0"')
    sp.add_argument("--set", action="append", required=True, metavar="dotted.path=value")
    sp.set_defaults(func=_wrap(cmd_set_params))

    sp = sub.add_parser("apply", help="apply a batch of ops from a JSON file")
    sp.add_argument("file")
    sp.add_argument("ops", help="path to a JSON list of {op, ...} objects")
    sp.set_defaults(func=_wrap(cmd_apply))

    sp = sub.add_parser("compile", help="compile .efkproj -> .efkefc via the Effekseer CUI")
    sp.add_argument("file")
    sp.add_argument("--out", default=None, help="output path (default: FILE with .efkefc)")
    sp.add_argument("--cui-path", default=None, help="override the pinned Effekseer CUI path")
    sp.set_defaults(func=_wrap(cmd_compile))

    sp = sub.add_parser("install", help="copy a compiled .efkefc into Assets/ as a ParticleFile asset")
    sp.add_argument("efkefc", help="compiled .efkefc to install")
    sp.add_argument("--project", default=None,
                     help="source .efkproj to also commit under Assets/Art/Effect/_Source/")
    sp.add_argument("--dest", required=True, help="e.g. Assets/Art/Effect/MyPack/Spark.efkefc")
    sp.set_defaults(func=_wrap(cmd_install))
