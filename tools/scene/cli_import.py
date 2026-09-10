"""``import-sprite`` - copy a .png into Assets/ and mint its SpriteFile .png.meta.

There is no engine-side registration step for a new texture: ``.png`` -> the
``SpriteFile`` asset type is already wired via ``REGISTER_ASSET(SpriteFile,
".png")`` (``Engine/Module/Asset/Sprite/SpriteFile.h``), and the runtime asset
scan discovers any file under ``Assets/`` on its own (no ``.vcxproj`` entry
needed - that rule only applies to compiled ``.cpp``/``.h``). The only missing
piece is minting a correctly-shaped ``.png.meta`` sidecar, which this command
does via the generic thin-proxy codec in ``tools/common/meta_base.py`` (see
``tools/scene/sprite_meta.py`` for the ``SpriteFile`` binding), mirroring
``tools/effect/cli.py``'s ``install`` command for ``ParticleFile``.
"""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

from . import sprite_meta

_REPO = Path(__file__).resolve().parents[2]


def cmd_import_sprite(args: argparse.Namespace) -> int:
    src = Path(args.source).resolve()
    if not src.exists():
        print(f"error: {src} does not exist")
        return 1
    if src.suffix.lower() != ".png":
        print(f"error: {src} is not a .png file (the engine only registers SpriteFile for '.png', case-sensitive)")
        return 1

    dest_dir = (_REPO / args.dest).resolve() if args.dest else (_REPO / sprite_meta.SPRITE_SPEC.default_dir)
    dest_dir.mkdir(parents=True, exist_ok=True)
    dest = dest_dir / src.name
    if dest.exists() and not args.force:
        print(f"error: {dest} already exists (use --force to overwrite)")
        return 1
    shutil.copyfile(src, dest)

    name = dest.stem
    guid = sprite_meta.mint_guid()
    content_path = sprite_meta.content_path_for(name, dest.parent, _REPO)
    meta_path = Path(str(dest) + ".meta")
    sprite_meta.write_meta(meta_path, name, guid, content_path)

    print(f"imported {dest}")
    print(f"created  {meta_path.name}")
    print(f"GUID:    {guid}")
    return 0


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("import-sprite", help="copy a .png into Assets/ and mint its SpriteFile .png.meta")
    sp.add_argument("source", help="source .png file (any location)")
    sp.add_argument("--dest", help="destination directory under the repo (default Assets/Art/UI)")
    sp.add_argument("--force", action="store_true")
    sp.set_defaults(func=cmd_import_sprite)
