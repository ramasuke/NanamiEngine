"""CLI subcommands for tools.model: convert, install, materials, set-emissive.

Unlike ``tools/effect`` (which shells out to Effekseer's real, documented
CUI), DxLib's ``DxLibModelViewer_64bit.exe`` has no CLI/CUI mode at all - it's
a GUI-only tool. ``convert`` drives that GUI via ``pywinauto`` well enough to
behave like a CLI (see ``dxlib_modelviewer.py``); this is inherently more
fragile than a real subprocess call, see ``tools/model/README.md``.
"""

from __future__ import annotations

import argparse
import math
import os
import shutil
import sys
from pathlib import Path, PureWindowsPath

from . import meta as meta_mod
from . import mv1 as mv1_mod

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
_MV1_MAGIC = mv1_mod.MAGIC
_MV1_MIN_SIZE = 256

_TEXTURE_EXTS = mv1_mod.TEXTURE_EXTS

# Keys of dxlib_modelviewer.SAVE_MODES, duplicated here so argparse can list
# them without importing pywinauto (dxlib_modelviewer is imported lazily).
_SAVE_MODES = ("mesh", "anim", "full")


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
    check plus "the compressed body decodes to its declared size" (see
    ``mv1.decode``) - same epistemic status as ``tools/effect``'s
    ``EFKE``/``INFO`` header check."""
    if not path.exists():
        return [f"{path} does not exist"]
    problems: list[str] = []
    data = path.read_bytes()
    if len(data) < _MV1_MIN_SIZE:
        problems.append(f"file is only {len(data)} byte(s), suspiciously small")
    header = data[:4]
    if header != _MV1_MAGIC:
        problems.append(f"header is {header!r}, expected {_MV1_MAGIC!r}")
        return problems
    try:
        mv1_mod.decode(data)
    except ValueError as e:
        problems.append(f"body does not decompress: {e}")
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


def _texture_candidates(ref: str, search_dirs: list[Path]) -> list[Path]:
    """Where a texture referenced as ``ref`` might live on disk, most specific
    first: the exact relative path under each search dir, then the bare file
    name directly in it, then inside any ``*.fbm`` folder there (the FBX SDK
    extracts embedded textures to ``<fbx stem>.fbm/`` next to the source)."""
    rel = PureWindowsPath(ref)
    name = rel.name
    candidates: list[Path] = []
    if rel.is_absolute():
        candidates.append(Path(ref))
    for d in search_dirs:
        if not rel.is_absolute():
            candidates.append(d / Path(*rel.parts))
        candidates.append(d / name)
        candidates.extend(sorted(p / name for p in d.glob("*.fbm") if p.is_dir()))
    return candidates


def _collect_textures(mv1_path: Path, search_dirs: list[Path], dest_dir: Path) -> tuple[list[Path], list[str]]:
    """Copy every texture ``mv1_path`` references into ``dest_dir`` at the
    relative path the ``.mv1`` stores, so DxLib resolves it when loading the
    model from ``dest_dir``. Returns ``(copied_or_already_in_place, missing)``
    - ``missing`` entries are human-readable reasons.

    An absolute reference (the source FBX's original ``C:\\...`` path, stored
    alongside a relative one in some files) can't be reproduced under
    ``dest_dir``: it's skipped when a relative reference to the same file name
    exists, otherwise the file is placed directly in ``dest_dir``. A relative
    reference that escapes ``dest_dir`` (``..\\``) is reported as missing."""
    try:
        refs = mv1_mod.texture_paths(mv1_path)
    except ValueError as e:
        raise CliError(f"cannot read texture references from {mv1_path.name}: {e}") from e
    relative_names = {PureWindowsPath(r).name.lower() for r in refs if not PureWindowsPath(r).is_absolute()}
    dest_root = dest_dir.resolve()
    placed: list[Path] = []
    missing: list[str] = []
    for ref in refs:
        rel = PureWindowsPath(ref)
        if rel.is_absolute():
            if rel.name.lower() in relative_names:
                continue
            target = dest_dir / rel.name
        else:
            target = dest_dir / Path(*rel.parts)
            try:
                target.resolve().relative_to(dest_root)
            except ValueError:
                missing.append(f"{ref} (points outside {dest_dir})")
                continue
        if target in placed:
            continue
        if target.exists():
            placed.append(target)
            continue
        source = next((c for c in _texture_candidates(ref, search_dirs) if c.is_file()), None)
        if source is None:
            missing.append(f"{ref} (not found under {', '.join(str(d) for d in search_dirs)})")
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        placed.append(target)
    return placed, missing


def _report_textures(mv1_path: Path, placed: list[Path], missing: list[str], dest_dir: Path) -> None:
    if not placed and not missing:
        print(f"texture   (none referenced by {mv1_path.name})")
    for p in placed:
        print(f"texture   {p.relative_to(dest_dir) if p.is_relative_to(dest_dir) else p}")
    if missing:
        raise CliError(f"{len(missing)} texture(s) referenced by {mv1_path.name} could not be placed "
                       f"next to it ({len(placed)} placed):\n  " + "\n  ".join(missing))


Rgb = tuple[float, float, float]


def _parse_emissive_specs(specs: list[str]) -> list[tuple[str | None, Rgb]]:
    """``--emissive`` values as ``(material or None for all, rgb)``, in the
    order given - later entries override earlier ones."""
    parsed: list[tuple[str | None, Rgb]] = []
    for spec in specs:
        target, sep, color = spec.rpartition("=")
        parts = color.split(",")
        try:
            rgb = tuple(float(p) for p in parts)
        except ValueError:
            rgb = ()
        if len(rgb) != 3:
            raise CliError(f"--emissive {spec!r}: expected [MATERIAL=]R,G,B, e.g. 1,0.8,0.3 or Lamp=1,0.8,0.3")
        if not all(math.isfinite(v) and v >= 0 for v in rgb):
            raise CliError(f"--emissive {spec!r}: R,G,B must be finite and >= 0")
        parsed.append((target if sep else None, rgb))
    return parsed


def _fmt_color(values) -> str:
    return "(" + ", ".join(f"{v:.4g}" for v in values) + ")"


def _resolve_emissive(materials: list[mv1_mod.Material], specs: list[tuple[str | None, Rgb]],
                      model_name: str) -> dict[int, Rgb]:
    colors: dict[int, Rgb] = {}
    for target, rgb in specs:
        if target is None:
            indices = [m.index for m in materials]
        else:
            indices = [m.index for m in materials if m.name == target]
            if not indices and target.isascii() and target.isdigit() and int(target) < len(materials):
                indices = [int(target)]
            if not indices:
                listing = ", ".join(f"[{m.index}] {m.name}" for m in materials)
                raise CliError(f"--emissive {target}=...: {model_name} has no material named {target!r} "
                               f"(materials: {listing})")
        for i in indices:
            colors[i] = rgb
    return colors


def _read_materials(path: Path) -> tuple[bytes, list[mv1_mod.Material]]:
    try:
        body = mv1_mod.decode(path.read_bytes())
        return body, mv1_mod.materials(body)
    except ValueError as e:
        raise CliError(f"cannot read the material table of {path.name}: {e}") from e


def _apply_emissive(src: Path, dest: Path, specs: list[tuple[str | None, Rgb]]) -> None:
    """Write ``src`` to ``dest`` (may be the same file) with its materials'
    emissive color set per ``specs``."""
    body, materials = _read_materials(src)
    if not materials:
        raise CliError(f"{src.name} has no materials (an animation-only .mv1?), so there is no "
                       "emissive color to set")
    colors = _resolve_emissive(materials, specs, src.name)
    patched = mv1_mod.with_emissive(body, colors)
    encoded = mv1_mod.encode(patched)
    if mv1_mod.decode(encoded) != patched:
        raise CliError(f"internal error: re-encoded {src.name} does not decode back to the patched "
                       "model; nothing was written")
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_name(dest.name + ".tmp")
    tmp.write_bytes(encoded)
    os.replace(tmp, dest)
    for i in sorted(colors):
        m = materials[i]
        print(f"emissive  [{i}] {m.name}  {_fmt_color(m.emissive[:3])} -> {_fmt_color(colors[i])}")


def cmd_convert(args: argparse.Namespace) -> int:
    in_path = _resolve(args.file)
    if not in_path.exists():
        raise CliError(f"{in_path} does not exist")
    if args.with_textures and args.mode == "anim":
        raise CliError("--with-textures has no effect with --mode anim (an animation-only .mv1 "
                       "carries no materials); use --mode mesh or --mode full")
    if args.emissive and args.mode == "anim":
        raise CliError("--emissive has no effect with --mode anim (an animation-only .mv1 "
                       "carries no materials); use --mode mesh or --mode full")
    emissive = _parse_emissive_specs(args.emissive)
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
    # The Save As dialog refuses a path in a folder that doesn't exist yet (it
    # pops a message box instead of saving, which would surface only as a
    # wait-for-output timeout).
    out_path.parent.mkdir(parents=True, exist_ok=True)

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
        dxlib_modelviewer.convert(in_path, out_path, exe_path, mode=args.mode,
                                   timeout=args.timeout, debug_dir=debug_dir)
    except dxlib_modelviewer.AutomationError as e:
        msg = f"conversion failed at step {e.step!r}: {e}"
        if e.debug_path:
            msg += f"\n  debug info saved to: {e.debug_path}"
        raise CliError(msg) from e

    problems = looks_like_mv1(out_path)
    if problems:
        print(f"WARNING: {out_path.name} does not look like a valid .mv1: "
              + "; ".join(problems), file=sys.stderr)
    print(f"converted {out_path}  (mode: {args.mode})")

    if emissive:
        _apply_emissive(out_path, out_path, emissive)

    if args.with_textures:
        placed, missing = _collect_textures(out_path, [in_path.parent], out_path.parent)
        _report_textures(out_path, placed, missing, out_path.parent)
    return 0


def _copy_textures(src_dir: Path, dest_textures_dir: Path) -> int:
    """Copy every recognized image file directly under ``src_dir`` (no
    recursion) into ``dest_textures_dir``, overwriting existing files there.
    Returns the count copied. Does **not** look at which textures the
    ``.mv1`` actually references or where it expects them - that's
    ``--with-textures`` (``_collect_textures``); this is the plain "copy
    everything available into textures/" option."""
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
        # shipped assets - no attempt to figure out which files the .mv1
        # references, see _copy_textures()'s docstring (and --with-textures).
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
    else:
        guid = meta_mod.mint_guid()
        content_path = meta_mod.content_path_for(name, dest.parent, _REPO)
        meta_mod.write_meta(meta_path, name, guid, content_path)
        print(f"installed {dest}")
        print(f"          {meta_path.name}")
        print(f"GUID:     {guid}")

    if args.with_textures:
        # Done last so a missing texture still leaves a complete .mv1 + .meta
        # behind; the non-zero exit only flags the incomplete texture set.
        placed, missing = _collect_textures(dest, [mv1_src.parent], dest.parent)
        _report_textures(dest, placed, missing, dest.parent)
    return 0


def cmd_materials(args: argparse.Namespace) -> int:
    path = _resolve(args.mv1)
    if not path.exists():
        raise CliError(f"{path} does not exist")
    _, materials = _read_materials(path)
    if not materials:
        print(f"{path.name}: no materials (an animation-only .mv1?)")
    for m in materials:
        print(f"[{m.index}] {m.name}  diffuse={_fmt_color(m.diffuse)}  emissive={_fmt_color(m.emissive[:3])}")
    return 0


def cmd_set_emissive(args: argparse.Namespace) -> int:
    src = _resolve(args.mv1)
    if not src.exists():
        raise CliError(f"{src} does not exist")
    dest = _resolve(args.out) if args.out else src
    _apply_emissive(src, dest, _parse_emissive_specs(args.emissive))
    print(f"wrote {dest}")
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


_EMISSIVE_HELP = ("emissive color (floats >= 0, 1 = full; repeatable): R,G,B for every material, MATERIAL=R,G,B for the "
                  "material(s) with that name (or index); later values override earlier ones")


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("convert", help="convert .fbx -> .mv1 by driving the DxLibModelViewer GUI")
    sp.add_argument("file", help="input model, e.g. a .fbx")
    sp.add_argument("out", help="output .mv1 path")
    sp.add_argument("--mode", required=True, choices=_SAVE_MODES,
                     help="what to save: mesh = model only (animations dropped), "
                          "anim = animations only (no mesh), full = model + animations")
    sp.add_argument("--with-textures", action="store_true",
                     help="also copy every texture the output .mv1 references next to it, at the "
                          "relative path it expects (looked up next to the input file and in its "
                          "*.fbm folder); not valid with --mode anim")
    sp.add_argument("--emissive", action="append", default=[], metavar="[MATERIAL=]R,G,B",
                     help=_EMISSIVE_HELP + "; applied to the output after saving; not valid with --mode anim")
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
    sp.add_argument("--with-textures", action="store_true",
                     help="copy every texture the .mv1 references (resolved next to the source .mv1 "
                          "and in its *.fbm folders) into <dest-dir> at the relative path it expects")
    sp.add_argument("--dest", required=True, help="e.g. Assets/Art/Models/MyProp/MyProp.mv1")
    sp.set_defaults(func=_wrap(cmd_install))

    sp = sub.add_parser("materials", help="list a .mv1's materials with their diffuse/emissive colors")
    sp.add_argument("mv1", help=".mv1 file to inspect")
    sp.set_defaults(func=_wrap(cmd_materials))

    sp = sub.add_parser("set-emissive", help="set the emissive (self-illumination) color of a .mv1's materials")
    sp.add_argument("mv1", help=".mv1 file to modify")
    sp.add_argument("--emissive", action="append", required=True, metavar="[MATERIAL=]R,G,B",
                     help=_EMISSIVE_HELP)
    sp.add_argument("--out", default=None, help="write here instead of overwriting the input (its .meta is never touched)")
    sp.set_defaults(func=_wrap(cmd_set_emissive))
