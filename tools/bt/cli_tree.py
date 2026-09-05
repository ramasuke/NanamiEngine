"""Tree-level CLI subcommands: new-tree, show, validate, and the edit verbs."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import catalog as catalog_mod
from . import meta as meta_mod
from . import model
from . import npc_kind
from .cereal_json import read_text, to_file_bytes
from .reader import read_tree
from .writer import write_tree

_REPO = Path(__file__).resolve().parents[2]


def _resolve_data_path(arg: str) -> Path:
    return npc_kind.resolve_tree_path(arg, _REPO)


# ---------------------------------------------------------------------------
def cmd_new_tree(args: argparse.Namespace) -> int:
    kind_obj = npc_kind.by_name(args.npc_kind)
    target_dir = Path(args.dir) if args.dir else Path(kind_obj.default_dir)
    if not target_dir.is_absolute():
        target_dir = _REPO / target_dir
    target_dir.mkdir(parents=True, exist_ok=True)
    data_path = target_dir / f"{args.name}{kind_obj.data_ext}"
    meta_path = target_dir / f"{args.name}{kind_obj.meta_ext}"
    if (data_path.exists() or meta_path.exists()) and not args.force:
        print(f"error: {data_path.name} already exists (use --force)", file=sys.stderr)
        return 1

    guid = meta_mod.mint_guid()
    entry_guid = meta_mod.mint_guid()
    tree = model.Tree(entry=model.Entry(guid=entry_guid, pos=(120.0, 40.0), child=None),
                      params=[], kind=kind_obj.name)
    data_path.write_bytes(to_file_bytes(write_tree(tree)))

    content_path = meta_mod.content_path_for(args.name, target_dir, _REPO, kind=kind_obj.name)
    meta_mod.write_meta(meta_path, args.name, guid, content_path, kind=kind_obj.name)

    rel = data_path.relative_to(_REPO) if str(data_path).startswith(str(_REPO)) else data_path
    print(f"created {rel}")
    print(f"        {meta_path.name}")
    print(f"GUID:   {guid}")
    print()
    print(kind_obj.bind_hint)
    print(f'  == "{guid}"')
    return 0


def cmd_show(args: argparse.Namespace) -> int:
    path = _resolve_data_path(args.file)
    kind_obj = npc_kind.kind_for_path(path) or npc_kind.ENEMY
    cat = catalog_mod.load(kind=kind_obj.name)
    tree = read_tree(read_text(path), cat=cat, kind=kind_obj.name)
    _print_node(tree.entry, 0, cat, is_entry=True)
    if tree.params:
        print("\nblackboard:")
        for p in tree.params:
            print(f"  {p.name} : {p.kind} = {p.value}")
    else:
        print("\nblackboard: (empty)")
    return 0


def _print_node(node, depth: int, cat, is_entry: bool = False) -> None:
    # Full GUIDs, not truncated: every --node/--parent argument to the edit
    # verbs needs the complete 36-char UUID, so a shortened id here would be
    # unusable as copy-paste input for the very thing `show` is for.
    pad = "  " * depth
    if is_entry:
        print(f"{pad}Entry  {node.guid}  pos={_p(node.pos)}")
        if node.child is not None:
            _print_node(node.child, depth + 1, cat)
        return
    kind = type(node).__name__
    if isinstance(node, model.Action):
        extra = f'  "{node.name}"  -> {node.type_name}'
    elif isinstance(node, model.RandomSelector):
        extra = f"  weights={node.weights}"
    else:
        extra = ""
    print(f"{pad}{kind}  {node.guid}  pos={_p(node.pos)}{extra}")
    for c in model.children_of(node):
        _print_node(c, depth + 1, cat)


def _p(pos) -> str:
    return f"({pos[0]:g},{pos[1]:g})"


def _dispatch(fn):
    def run(args: argparse.Namespace) -> int:
        return fn(args)
    return run


def register(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("new-tree", help="create an empty tree + .meta (Enemy or FriendlyNpc)")
    p.add_argument("name")
    p.add_argument("--npc-kind", choices=sorted(npc_kind.BY_NAME), default="enemy",
                   help="which BehaviourTree flavor to create (default: enemy)")
    p.add_argument("--dir", default=None,
                   help="target directory (default: Assets/Data/EnemyBehaviour or "
                        "Assets/Data/FriendlyNpcBehviour, per --npc-kind)")
    p.add_argument("--force", action="store_true", help="overwrite existing files")
    p.set_defaults(func=cmd_new_tree)

    p = sub.add_parser("show", help="print a behaviour tree as an outline")
    p.add_argument("file")
    p.set_defaults(func=cmd_show)

    try:
        from . import cli_edit
        cli_edit.register(sub)
    except Exception:  # noqa: BLE001
        pass
