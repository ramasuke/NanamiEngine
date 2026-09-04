"""Tree-level CLI subcommands: new-tree, show, validate, and the edit verbs."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import catalog as catalog_mod
from . import meta as meta_mod
from . import model
from .cereal_json import read_text, to_file_bytes
from .reader import read_tree
from .writer import write_tree

_REPO = Path(__file__).resolve().parents[2]


def _resolve_data_path(arg: str) -> Path:
    p = Path(arg)
    if not p.suffix:
        p = p.with_suffix("") if p.name.endswith(meta_mod.DATA_EXT) else Path(str(p) + meta_mod.DATA_EXT)
    if not p.is_absolute():
        p = (_REPO / p) if not p.exists() else p
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
    entry_guid = meta_mod.mint_guid()
    tree = model.Tree(entry=model.Entry(guid=entry_guid, pos=(120.0, 40.0), child=None), params=[])
    data_path.write_bytes(to_file_bytes(write_tree(tree)))

    content_path = meta_mod.content_path_for(args.name, target_dir, _REPO)
    meta_mod.write_meta(meta_path, args.name, guid, content_path)

    rel = data_path.relative_to(_REPO) if str(data_path).startswith(str(_REPO)) else data_path
    print(f"created {rel}")
    print(f"        {meta_path.name}")
    print(f"GUID:   {guid}")
    print()
    print("bind it to an enemy: set the EnemyBase.behaviourData_ field to this asset")
    print("in the prefab inspector, or edit the prefab JSON so")
    print('  ...components_.component_N.data...behaviourData_.value0.ptr_wrapper.data.value0.value_')
    print(f'  == "{guid}"')
    return 0


def cmd_show(args: argparse.Namespace) -> int:
    path = _resolve_data_path(args.file)
    tree = read_tree(read_text(path))
    cat = catalog_mod.load()
    _print_node(tree.entry, 0, cat, is_entry=True)
    if tree.params:
        print("\nblackboard:")
        for p in tree.params:
            print(f"  {p.name} : {p.kind} = {p.value}")
    else:
        print("\nblackboard: (empty)")
    return 0


def _print_node(node, depth: int, cat, is_entry: bool = False) -> None:
    pad = "  " * depth
    if is_entry:
        print(f"{pad}Entry  {node.guid[:8]}  pos={_p(node.pos)}")
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
    print(f"{pad}{kind}  {node.guid[:8]}  pos={_p(node.pos)}{extra}")
    for c in model.children_of(node):
        _print_node(c, depth + 1, cat)


def _p(pos) -> str:
    return f"({pos[0]:g},{pos[1]:g})"


def _dispatch(fn):
    def run(args: argparse.Namespace) -> int:
        return fn(args)
    return run


def register(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("new-tree", help="create an empty .enemyBehaviourData + .meta")
    p.add_argument("name")
    p.add_argument("--dir", default=meta_mod.DEFAULT_DIR,
                   help=f"target directory (default: {meta_mod.DEFAULT_DIR})")
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
