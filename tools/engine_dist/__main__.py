"""``python -m tools.engine_dist <command>`` のエントリポイント。"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

_REPO = Path(__file__).resolve().parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.engine_dist import package as pkg  # noqa: E402


def _cmd_build(args: argparse.Namespace) -> int:
    repo = _REPO
    out_dir = Path(args.out).resolve() / f"NanamiEngine-{args.version}"
    try:
        if not args.skip_build:
            pkg.build_libs(repo, pkg.find_msbuild(args.msbuild))
        result = pkg.assemble(repo, out_dir, args.version, allow_missing_libs=args.allow_missing_libs)
    except pkg.PackageError as e:
        print(f"ERROR: {e}")
        return 1
    print(f"packaged {result.file_count} files -> {result.out_dir}")
    if result.missing_libs:
        print("WARNING: lib が無い構成: " + ", ".join(result.missing_libs))
    if args.zip:
        print(f"zip -> {pkg.make_zip(result.out_dir)}")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m tools.engine_dist")
    sub = parser.add_subparsers(dest="command", required=True)

    build = sub.add_parser("build", help="lib を 4 構成ビルドし、NanamiHub がインストールできるフォルダにまとめる")
    build.add_argument("--version", required=True)
    build.add_argument("--out", default=str(_REPO / "dist"))
    build.add_argument("--msbuild", help="MSBuild.exe (省略時は vswhere で探す)")
    build.add_argument("--skip-build", action="store_true", help="ビルド済みの lib/ をそのまま使う")
    build.add_argument("--allow-missing-libs", action="store_true", help="lib が無い構成があっても続ける")
    build.add_argument("--zip", action="store_true", help="フォルダを zip にもする")
    build.set_defaults(func=_cmd_build)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
