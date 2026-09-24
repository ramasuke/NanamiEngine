"""``import-sprite`` - .png を Assets/ にコピーし、その SpriteFile の .png.meta を作る。

新しいテクスチャにエンジン側の登録手順はない: ``.png`` -> ``SpriteFile`` アセット型は
``REGISTER_ASSET(SpriteFile, ".png")`` (``Engine/Module/Asset/Sprite/SpriteFile.h``)
で既に結び付いており、実行時のアセットスキャンは ``Assets/`` 以下のファイルを
自分で見つける (``.vcxproj`` エントリは不要 - その規則はコンパイルされる
``.cpp``/``.h`` にだけ適用される)。足りないのは正しい形の ``.png.meta`` サイドカーを
作ることだけで、このコマンドは ``tools/common/meta_base.py`` の汎用 thin proxy
コーデックでそれを行う (``SpriteFile`` の結び付けは ``tools/scene/sprite_meta.py`` を
参照)。``ParticleFile`` 用の ``tools/effect/cli.py`` の ``install`` コマンドと同様。
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
