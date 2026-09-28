"""ツリー単位の CLI サブコマンド: new-tree, show, validate と編集コマンド。"""

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
    if tree.detached:
        print("\ndetached (not connected, never runs):")
        for root in tree.detached:
            _print_node(root, 1, cat)
    if tree.params:
        print("\nblackboard:")
        for p in tree.params:
            print(f"  {p.name} : {p.kind} = {p.value}")
    else:
        print("\nblackboard: (empty)")
    return 0


def _print_node(node, depth: int, cat, is_entry: bool = False) -> None:
    # GUID は省略せず全桁で出す: 編集コマンドの --node/--parent 引数はすべて
    # 36 文字の完全な UUID を要するため、ここで短縮すると `show` の本来の用途である
    # コピペ入力に使えなくなる。
    pad = "  " * depth
    if is_entry:
        print(f"{pad}Entry  {node.guid}  pos={_p(node.pos)}")
        if node.child is not None:
            _print_node(node.child, depth + 1, cat)
        return
    kind = type(node).__name__
    if isinstance(node, model.Action):
        extra = f'  "{node.name}"  -> {node.type_name}'
        if node.params is not None and "cues_" in node.params.keys():
            extra += f"  duration={float(node.params['duration_secs_'].value):g}s"
            if node.params.get("once_"):
                extra += " once"
    elif isinstance(node, model.RandomSelector):
        extra = f"  weights={node.weights}"
    elif isinstance(node, model.BlackBoardGate):
        extra = "  " + _gate_summary(node)
    else:
        extra = ""
    print(f"{pad}{kind}  {node.guid}  pos={_p(node.pos)}{extra}")
    if isinstance(node, model.Action):
        _print_cues(node.params, depth + 1)
    for c in model.children_of(node):
        _print_node(c, depth + 1, cat)


def _gate_summary(node: model.BlackBoardGate) -> str:
    def fmt(pairs, op):
        return ", ".join(f"{k}{op}{v}" for k, v in pairs)
    parts = [f"if [{fmt(node.conditions, '==')}]"]
    if node.writes_on_start:
        parts.append(f"start [{fmt(node.writes_on_start, '=')}]")
    if node.writes_on_success:
        parts.append(f"ok [{fmt(node.writes_on_success, '=')}]")
    if node.once:
        parts.append("once")
    return " ".join(parts)


def _print_cues(params, depth: int) -> None:
    """ActionTimeline の cues_ を字下げして出す（入れ子の Timeline も辿る）。"""
    from .blob import Ptr, Ver
    from .cereal_json import Num
    if params is None or "cues_" not in params.keys():
        return
    pad = "  " * depth
    for cue in params["cues_"]:
        at = cue["at_secs_"]
        at = at.value if isinstance(at, Num) else at
        flags = [f for f in ("waitDone_", "keepTicking_") if cue.get(f)]
        ptr = cue["action_"]
        if not isinstance(ptr, Ptr) or ptr.null:
            print(f"{pad}@{float(at):g}s  (no action)")
            continue
        leaf = (ptr.fqn or "?").rsplit("::", 1)[-1]
        flag_text = ("  " + " ".join(f.rstrip("_") for f in flags)) if flags else ""
        print(f"{pad}@{float(at):g}s  {leaf}{flag_text}")
        body = ptr.data.body if isinstance(ptr.data, Ver) else None
        _print_cues(body, depth + 1)


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
