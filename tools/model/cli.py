"""tools.model の CLI サブコマンド: convert, install, materials, set-emissive, set-culling。

``tools/effect`` (Effekseer の本物の文書化された CUI を呼び出す) と違い、DxLib の
``DxLibModelViewer_64bit.exe`` には CLI/CUI モードがまったく無い - GUI 専用の
ツール。``convert`` は ``pywinauto`` でその GUI を操作して CLI のように振る舞わせる
(``dxlib_modelviewer.py`` 参照)。本物のサブプロセス呼び出しより本質的に壊れやすい。
``tools/model/README.md`` を参照。
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

# 固定値: 2026-09-12 に DxLibModelViewer ver3.24d (タイトルバー
# "DxLibModelViewer [ DxLib ver3.24d ]") で確認済み - 配布済みの実 .mv1
# (Assets/Art/Models/Basic/Cube.mv1) を Open -> メッシュのみ名前を付けて保存 で往復させ、
# 別途 ~36MB のテクスチャ付き実 .fbx を最初から最後まで変換し、どちらも
# 正しい MV11 ヘッダになった。詳しくは tools/model/dxlib_modelviewer.py の
# モジュール docstring を参照。
DEFAULT_MODELVIEWER_PATH: str | None = r"C:\DxLib_VC3_24d\DxLib_VC\Tool\DxLibModelViewer\DxLibModelViewer_64bit.exe"

# 配布済みの実 .mv1 ファイル 4 つ (アニメーションクリップと静的/スキンメッシュの
# 両方) で経験的に観測したもの - 文書化された DxLib の形式シグネチャではないので、
# これは簡易チェックであって正しいファイルの証明ではない。
_MV1_MAGIC = mv1_mod.MAGIC
_MV1_MIN_SIZE = 256

_TEXTURE_EXTS = mv1_mod.TEXTURE_EXTS

# dxlib_modelviewer.SAVE_MODES のキー。argparse が pywinauto を import せずに
# 一覧できるようここに複製している (dxlib_modelviewer は遅延 import)。
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
    """見つかった問題 (空 = 問題なさそう)。構造の検証ではない - ``.mv1`` 形式の
    仕様は手に入らないので、マジックバイト + サイズの簡易チェックと「圧縮された本体が
    宣言どおりのサイズにデコードできる」こと (``mv1.decode`` 参照) だけ -
    ``tools/effect`` の ``EFKE``/``INFO`` ヘッダチェックと同程度の確からしさ。"""
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
    """``ref`` として参照されるテクスチャがディスク上にありそうな場所を、具体的な
    ものから順に: 各検索ディレクトリ以下の正確な相対パス、次にその直下の素の
    ファイル名、次にそこにある任意の ``*.fbm`` フォルダの中 (FBX SDK は埋め込み
    テクスチャを原本の隣の ``<fbx の stem>.fbm/`` に展開する)。"""
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
    """``mv1_path`` が参照するテクスチャをすべて、``.mv1`` が保存している相対パスで
    ``dest_dir`` にコピーし、``dest_dir`` からモデルを読み込んだときに DxLib が
    解決できるようにする。``(コピー済みまたは配置済み, 見つからないもの)`` を返す
    - ``missing`` の各要素は人が読める理由。

    絶対パスの参照 (元の FBX の ``C:\\...`` パス。一部のファイルでは相対パスと
    並んで保存されている) は ``dest_dir`` 以下に再現できない: 同じファイル名への
    相対参照があればスキップし、無ければファイルを ``dest_dir`` の直下に置く。
    ``dest_dir`` の外に出る相対参照 (``..\\``) は見つからないものとして報告する。"""
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
    """``--emissive`` の値を ``(マテリアル、全体なら None, rgb)`` として指定順に返す
    - 後のものが前のものを上書きする。"""
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
    """``src`` を、マテリアルの自己発光色を ``specs`` どおりに設定して ``dest``
    (同じファイルでもよい) に書き出す。"""
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
        # 先に削除しておき、失敗した実行の残骸を新しい成功と取り違えないように、
        # また DxLibModelViewer 自身の上書き確認ダイアログ (あれば) に
        # 実際には当たらないようにする。
        out_path.unlink()
    # 名前を付けて保存ダイアログは、まだ存在しないフォルダのパスを拒否する
    # (保存せずにメッセージボックスを出すので、出力待ちのタイムアウトとしてしか
    # 表面化しない)。
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
    """``src_dir`` 直下 (再帰しない) の認識できる画像ファイルをすべて
    ``dest_textures_dir`` にコピーし、既存のファイルは上書きする。コピーした数を
    返す。``.mv1`` が実際にどのテクスチャを参照しどこに置くことを期待しているかは
    **見ない** - それは ``--with-textures`` (``_collect_textures``) の役目で、こちらは
    素朴な「使えるものを全部 textures/ にコピーする」オプション。"""
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
        # .mv1 アセットには (.efkefc の Assets/Art/Effect ツリーのような) 単一のルートが
        # 無いので、.fbx の原本は 1 つの共通 _Source/ ツリーではなく、それぞれの .mv1 の
        # 隣に置く。
        source_src = _resolve(args.source)
        source_dest = dest.parent / "_Source" / f"{dest.stem}.fbx"
        source_dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source_src, source_dest)
        print(f"copied source {source_dest}")

    if args.textures:
        # 素朴な一括コピー。配布済みの実アセットと同じ「textures/」兄弟フォルダの
        # 規則 - .mv1 がどのファイルを参照するかは調べない。_copy_textures() の
        # docstring (と --with-textures) を参照。
        textures_src = _resolve(args.textures)
        textures_dest = dest.parent / "textures"
        count = _copy_textures(textures_src, textures_dest)
        if count:
            print(f"copied {count} texture(s) to {textures_dest}")

    name = dest.stem
    meta_path = dest.with_suffix(dest.suffix + ".meta")
    if meta_path.exists():
        # すでにプレハブ/コンポーネントから参照されているアセットへの再インストール:
        # .meta (つまり GUID) には手を付けず、既存の参照をすべて有効なままにする。
        # 新規アセットだけに新しい GUID を振る。
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
        # 最後に行うので、テクスチャが欠けていても完全な .mv1 + .meta は
        # 残る。0 以外の終了コードはテクスチャが揃っていないことだけを示す。
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


def cmd_set_culling(args: argparse.Namespace) -> int:
    src = _resolve(args.mv1)
    if not src.exists():
        raise CliError(f"{src} does not exist")
    dest = _resolve(args.out) if args.out else src
    try:
        body = mv1_mod.decode(src.read_bytes())
        before = mv1_mod.mesh_culling(body)
        patched = mv1_mod.with_mesh_culling(body, mv1_mod.CULLING_MODES[args.mode])
    except ValueError as e:
        raise CliError(f"cannot read the mesh table of {src.name}: {e}") from e
    encoded = mv1_mod.encode(patched)
    if mv1_mod.decode(encoded) != patched:
        raise CliError(f"internal error: re-encoded {src.name} does not decode back to the patched "
                       "model; nothing was written")
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_name(dest.name + ".tmp")
    tmp.write_bytes(encoded)
    os.replace(tmp, dest)
    names = {v: k for k, v in mv1_mod.CULLING_MODES.items()}
    print(f"culling   {len(before)} mesh(es): {', '.join(names[m] for m in before)} -> {args.mode}")
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

    sp = sub.add_parser("set-culling", help="set the back-face culling of every mesh in a .mv1 (none = draw both sides)")
    sp.add_argument("mv1", help=".mv1 file to modify")
    sp.add_argument("--mode", required=True, choices=sorted(mv1_mod.CULLING_MODES),
                     help="none = double-sided; left = DxLib's default for converted models")
    sp.add_argument("--out", default=None, help="write here instead of overwriting the input (its .meta is never touched)")
    sp.set_defaults(func=_wrap(cmd_set_culling))
