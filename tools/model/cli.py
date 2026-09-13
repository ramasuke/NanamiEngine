"""CLI subcommands for tools.model: convert, install.

Unlike ``tools/effect`` (which shells out to Effekseer's real, documented
CUI), DxLib's ``DxLibModelViewer_64bit.exe`` has no CLI/CUI mode at all - it's
a GUI-only tool. ``convert`` drives that GUI via ``pywinauto`` well enough to
behave like a CLI (see ``dxlib_modelviewer.py``); this is inherently more
fragile than a real subprocess call, see ``tools/model/README.md``.
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

from . import meta as meta_mod

_REPO = Path(__file__).resolve().parents[2]

# Pinned: verified 2026-09-12 against DxLibModelViewer ver3.24d (title bar
# "DxLibModelViewer [ DxLib ver3.24d ]") - round-tripped a real shipped .mv1
# (Assets/Art/Models/Basic/Cube.mv1) through Open -> Save As mesh only, and
# separately converted a real ~36MB textured .fbx end to end, both producing
# a valid MV11 header. See tools/model/dxlib_modelviewer.py's module
# docstring for details.
DEFAULT_MODELVIEWER_PATH: str | None = r"C:\DxLib_VC3_24d\DxLib_VC\Tool\DxLibModelViewer\DxLibModelViewer_64bit.exe"

# Empirically observed on 4 real shipped .mv1 files (both animation-clip and
# static/skinned-mesh) - not a documented DxLib format signature, so this is
# a sanity check, not proof of a well-formed file.
_MV1_MAGIC = b"MV11"
_MV1_MIN_SIZE = 256

# Matches the "textures/" sibling-folder convention already used by real
# shipped assets (e.g. Assets/Art/Models/Fantasy/DirtyHouse/textures/).
_TEXTURE_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds"}


class CliError(RuntimeError):
    pass


def _resolve(arg: str) -> Path:
    path = Path(arg)
    if path.is_absolute():
        return path
    if path.exists():
        return path.resolve()
    return _REPO / path


def looks_like_mv1(path: Path) -> list[str]:
    """Problems found (empty = looks OK). Not a structural validator - no
    ``.mv1`` format spec is available, only a magic-bytes + size sanity
    check (same epistemic status as ``tools/effect``'s ``EFKE``/``INFO``
    header check)."""
    if not path.exists():
        return [f"{path} does not exist"]
    problems: list[str] = []
    size = path.stat().st_size
    if size < _MV1_MIN_SIZE:
        problems.append(f"file is only {size} byte(s), suspiciously small")
    header = path.read_bytes()[:4]
    if header != _MV1_MAGIC:
        problems.append(f"header is {header!r}, expected {_MV1_MAGIC!r}")
    return problems


def _find_modelviewer_path(args: argparse.Namespace) -> Path:
    for candidate in (args.modelviewer_path, os.environ.get("DXLIB_MODELVIEWER"), DEFAULT_MODELVIEWER_PATH):
        if candidate and Path(candidate).exists():
            return Path(candidate)
    raise CliError(
        "DxLibModelViewer not found. Tried --modelviewer-path, $DXLIB_MODELVIEWER, and the "
        f"pinned default ({DEFAULT_MODELVIEWER_PATH!r} - not yet set, see tools/model/README.md). "
        "Pass --modelviewer-path pointing at your local "
        "Tool\\DxLibModelViewer\\DxLibModelViewer_64bit.exe."
    )


def cmd_convert(args: argparse.Namespace) -> int:
    in_path = _resolve(args.file)
    if not in_path.exists():
        raise CliError(f"{in_path} does not exist")
    if in_path.suffix.lower() != ".fbx":
        print(f"WARNING: {in_path.name} does not have a .fbx extension; DxLibModelViewer "
              "also loads .x/.mqo/.pmd/.pmx/.mv1, so this may still be intentional.",
              file=sys.stderr)

    out_path = _resolve(args.out)
    if out_path.exists():
        if not args.force:
            raise CliError(f"{out_path} already exists (use --force to overwrite)")
        # Delete up front so a failed run's leftover can't be mistaken for a
        # fresh success, and so DxLibModelViewer's own overwrite-confirm
        # dialog (if any) is never actually hit.
        out_path.unlink()

    exe_path = _find_modelviewer_path(args)

    try:
        from . import dxlib_modelviewer
    except ImportError as e:
        raise CliError(
            "tools.model convert requires the 'pywinauto' package to drive the "
            "DxLibModelViewer GUI. Install it with:\n\n"
            "    pip install pywinauto\n\n"
            "(pywinauto pulls in pywin32/comtypes automatically; this is the first "
            "third-party dependency any tools/* toolkit in this repo has needed - "
            "see tools/model/README.md.)"
        ) from e

    debug_dir = Path(args.debug_dir) if args.debug_dir else None
    try:
        dxlib_modelviewer.convert(in_path, out_path, exe_path, timeout=args.timeout, debug_dir=debug_dir)
    except dxlib_modelviewer.AutomationError as e:
        msg = f"conversion failed at step {e.step!r}: {e}"
        if e.debug_path:
            msg += f"\n  debug info saved to: {e.debug_path}"
        raise CliError(msg) from e

    problems = looks_like_mv1(out_path)
    if problems:
        print(f"WARNING: {out_path.name} does not look like a valid .mv1: "
              + "; ".join(problems), file=sys.stderr)
    print(f"converted {out_path}")
    return 0


def _copy_textures(src_dir: Path, dest_textures_dir: Path) -> int:
    """Copy every recognized image file directly under ``src_dir`` (no
    recursion) into ``dest_textures_dir``, overwriting existing files there.
    Returns the count copied. Does **not** try to determine which textures a
    ``.mv1`` actually references - DxLibModelViewer's own conversion appears
    to keep only one texture per material (confirmed by inspecting real
    shipped assets), so this is a deliberate "copy everything available"
    fallback rather than a filtered/verified set - see
    tools/model/README.md's "Known limitations"."""
    if not src_dir.is_dir():
        raise CliError(f"--textures {src_dir} is not a directory")
    files = sorted(p for p in src_dir.iterdir() if p.is_file() and p.suffix.lower() in _TEXTURE_EXTS)
    if not files:
        print(f"WARNING: no image files ({', '.join(sorted(_TEXTURE_EXTS))}) found directly under "
              f"{src_dir}", file=sys.stderr)
        return 0
    dest_textures_dir.mkdir(parents=True, exist_ok=True)
    for f in files:
        shutil.copyfile(f, dest_textures_dir / f.name)
    return len(files)


def cmd_install(args: argparse.Namespace) -> int:
    mv1_src = _resolve(args.mv1)
    dest = _resolve(args.dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(mv1_src, dest)

    problems = looks_like_mv1(dest)
    if problems:
        print(f"WARNING: {dest.name} does not look like a valid .mv1: "
              + "; ".join(problems), file=sys.stderr)

    if args.source:
        # .mv1 assets have no single root (unlike .efkefc's one Assets/Art/Effect
        # tree), so the .fbx source is kept next to its own .mv1 rather than in
        # one global _Source/ tree.
        source_src = _resolve(args.source)
        source_dest = dest.parent / "_Source" / f"{dest.stem}.fbx"
        source_dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source_src, source_dest)
        print(f"copied source {source_dest}")

    if args.textures:
        # Plain bulk copy, same "textures/" sibling-folder convention as real
        # shipped assets - no attempt to figure out which files a .mv1 (or its
        # source .fbx) actually references, see _copy_textures()'s docstring.
        textures_src = _resolve(args.textures)
        textures_dest = dest.parent / "textures"
        count = _copy_textures(textures_src, textures_dest)
        if count:
            print(f"copied {count} texture(s) to {textures_dest}")

    name = dest.stem
    meta_path = dest.with_suffix(dest.suffix + ".meta")
    if meta_path.exists():
        # Re-installing over an asset already wired into prefabs/components:
        # keep its .meta (and so its GUID) untouched so every existing
        # reference stays valid. Only a brand-new asset gets a fresh GUID.
        guid = meta_mod.read_meta(meta_path)["guid"]
        print(f"installed {dest}")
        print(f"          {meta_path.name} (existing, kept)")
        print(f"GUID:     {guid}  (reused)")
        return 0

    guid = meta_mod.mint_guid()
    content_path = meta_mod.content_path_for(name, dest.parent, _REPO)
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
    sp = sub.add_parser("convert", help="convert .fbx -> .mv1 by driving the DxLibModelViewer GUI")
    sp.add_argument("file", help="input model, e.g. a .fbx")
    sp.add_argument("out", help="output .mv1 path")
    sp.add_argument("--modelviewer-path", default=None,
                     help="override the DxLibModelViewer_64bit.exe path")
    sp.add_argument("--timeout", type=float, default=60.0, metavar="SECONDS")
    sp.add_argument("--force", action="store_true", help="overwrite an existing output file")
    sp.add_argument("--debug-dir", default=None,
                     help="where to save a screenshot + control-tree dump on failure")
    sp.set_defaults(func=_wrap(cmd_convert))

    sp = sub.add_parser("install", help="copy a .mv1 into Assets/ as a Mv1File asset")
    sp.add_argument("mv1", help=".mv1 file to install")
    sp.add_argument("--source", default=None,
                     help="source .fbx to also commit under <dest-dir>/_Source/ (off by default)")
    sp.add_argument("--textures", default=None,
                     help="directory of texture images to bulk-copy into <dest-dir>/textures/ "
                          "(no filtering - copies every recognized image file found; off by default)")
    sp.add_argument("--dest", required=True, help="e.g. Assets/Art/Models/MyProp/MyProp.mv1")
    sp.set_defaults(func=_wrap(cmd_install))
