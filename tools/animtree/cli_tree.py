"""Tree-level CLI subcommands: new-tree, show. Delegates the edit verbs
(including validate/apply) to :mod:`tools.animtree.cli_edit`."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import catalog as catalog_mod
from . import edits as edits_mod
from . import meta as meta_mod
from . import model
from .cereal_json import read_text, to_file_bytes
from .reader import read_tree
from .writer import write_tree

_REPO = Path(__file__).resolve().parents[2]


def _resolve_data_path(arg: str) -> Path:
    p = Path(arg)
    if not str(p).endswith(meta_mod.DATA_EXT):
        p = Path(str(p) + meta_mod.DATA_EXT)
    if not p.is_absolute() and not p.exists():
        p = _REPO / p
    return p


# ---------------------------------------------------------------------------
def cmd_new_tree(args: argparse.Namespace) -> int:
    target_dir = Path(args.dir)
    if not target_dir.is_absolute():
        target_dir = _REPO / target_dir
    target_dir.mkdir(parents=True, exist_ok=True)
    data_path = target_dir / f"{args.name}{meta_mod.DATA_EXT}"
    meta_path = target_dir / f"{args.name}{meta_mod.META_EXT}"
    if (data_path.exists() or meta_path.exists()) and not args.force:
        print(f"error: {data_path.name} already exists (use --force)", file=sys.stderr)
        return 1

    guid = meta_mod.mint_guid()
    entry = edits_mod.new_singleton_node(model.FQN_ENTRY_NODE, meta_mod.mint_guid(), (120.0, 40.0),
                                         {"speed_": 1.0})
    any_state = edits_mod.new_singleton_node(model.FQN_ANYSTATE_NODE, meta_mod.mint_guid(), (120.0, 160.0))
    tree = model.Tree(entry=entry, any_state=any_state)
    data_path.write_bytes(to_file_bytes(write_tree(tree)))

    content_path = meta_mod.content_path_for(args.name, target_dir, _REPO)
    meta_mod.write_meta(meta_path, args.name, guid, content_path)

    rel = data_path.relative_to(_REPO) if str(data_path).startswith(str(_REPO)) else data_path
    print(f"created {rel}")
    print(f"        {meta_path.name}")
    print(f"GUID:   {guid}")
    print()
    print("bind it to a GameObject: tools.scene add-component --type Animator "
          f"--param animationTreeFile_={guid}")
    return 0


def cmd_show(args: argparse.Namespace) -> int:
    path = _resolve_data_path(args.file)
    tree = read_tree(read_text(path))
    cat = catalog_mod.load()
    print(f"Entry     {tree.entry.guid}  pos={_p(tree.entry.pos)}")
    print(f"AnyState  {tree.any_state.guid}  pos={_p(tree.any_state.pos)}")
    if tree.nodes:
        print("\nnodes:")
        for n in tree.nodes:
            entry = cat.node_by_fqn(n.type_fqn)
            leaf = entry.get("leaf") if entry else n.type_fqn
            extra = ""
            if entry and n.params is not None:
                nm = _string_param(n, entry, "name_")
                if nm is not None:
                    extra = f'  "{nm}"'
            print(f"  {leaf}  {n.guid}  pos={_p(n.pos)}{extra}")
    else:
        print("\nnodes: (empty)")

    _print_transitions("transitions", tree.transitions)
    _print_transitions("any-state transitions", tree.any_state_transitions)

    if tree.params:
        print("\nparameters:")
        for p in tree.params:
            print(f"  {p.name} : {p.kind} = {model.numval(p.value)}")
    else:
        print("\nparameters: (empty)")
    return 0


def _string_param(node: model.Node, entry: dict, member: str):
    for pinfo in entry.get("params", []):
        if pinfo.get("member") == member and pinfo["key"] in node.params:
            return node.params[pinfo["key"]]
    return None


def _print_transitions(title: str, transitions: list) -> None:
    if not transitions:
        print(f"\n{title}: (empty)")
        return
    print(f"\n{title}:")
    for i, t in enumerate(transitions):
        cond = "; ".join(f"{c.name}=={model.numval(c.value)}" for c in t.conditions) or "always"
        print(f"  [{i}] {t.from_guid} -> {t.next_guid}  duration={model.numval(t.duration_secs):g}  ({cond})")


def _p(pos) -> str:
    x, y = model.numval(pos[0]), model.numval(pos[1])
    return f"({x:g},{y:g})"


def register(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("new-tree", help="create an empty .animTree + .meta")
    p.add_argument("name")
    p.add_argument("--dir", default=meta_mod.DEFAULT_DIR,
                   help=f"target directory (default: {meta_mod.DEFAULT_DIR})")
    p.add_argument("--force", action="store_true", help="overwrite existing files")
    p.set_defaults(func=cmd_new_tree)

    p = sub.add_parser("show", help="print an AnimationTree as a readable outline")
    p.add_argument("file")
    p.set_defaults(func=cmd_show)

    try:
        from . import cli_edit
        cli_edit.register(sub)
    except Exception:  # noqa: BLE001
        pass
