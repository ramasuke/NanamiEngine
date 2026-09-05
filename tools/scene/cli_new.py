"""``new-scene`` / ``new-prefab`` CLI subcommands."""

from __future__ import annotations

import argparse
from pathlib import Path

from tools.common.cereal_json import to_file_bytes

from . import edits, meta as scene_meta, model, writer

_REPO = Path(__file__).resolve().parents[2]


def cmd_new_scene(args: argparse.Namespace) -> int:
    out_dir = Path(args.dir) if args.dir else _REPO / "Assets/Scene"
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / f"{args.name}.scene"
    if path.exists() and not args.force:
        print(f"error: {path} already exists (use --force to overwrite)")
        return 1
    scene = model.Scene(name=args.name, roots=[])
    path.write_bytes(to_file_bytes(writer.write_scene(scene)))
    guid = scene_meta.mint_guid()
    content_path = scene_meta.content_path_for(scene_meta.SCENE_SPEC, args.name, out_dir, _REPO)
    scene_meta.write_meta(scene_meta.SCENE_SPEC, Path(str(path) + ".meta"), args.name, guid, content_path)
    print(f"created {path}")
    print(f"created {path}.meta  (guid {guid})")
    return 0


def cmd_new_prefab(args: argparse.Namespace) -> int:
    out_dir = Path(args.dir) if args.dir else _REPO / "Assets/Prefab"
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / f"{args.name}.prefab"
    if path.exists() and not args.force:
        print(f"error: {path} already exists (use --force to overwrite)")
        return 1
    root = edits.new_gameobject(args.name, kind=model.KIND_PREFAB_ROOT)
    prefab = model.Prefab(root=root, copied_object_guids=[])
    path.write_bytes(to_file_bytes(writer.write_prefab(prefab)))
    guid = scene_meta.mint_guid()
    content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, args.name, out_dir, _REPO)
    scene_meta.write_meta(scene_meta.PREFAB_SPEC, Path(str(path) + ".meta"), args.name, guid, content_path)
    print(f"created {path}")
    print(f"created {path}.meta  (guid {guid})")
    return 0


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("new-scene", help="create a new empty .scene + .meta")
    sp.add_argument("name")
    sp.add_argument("--dir", help="target directory (default Assets/Scene)")
    sp.add_argument("--force", action="store_true")
    sp.set_defaults(func=cmd_new_scene)

    sp = sub.add_parser("new-prefab", help="create a new empty .prefab + .meta")
    sp.add_argument("name")
    sp.add_argument("--dir", help="target directory (default Assets/Prefab)")
    sp.add_argument("--force", action="store_true")
    sp.set_defaults(func=cmd_new_prefab)
