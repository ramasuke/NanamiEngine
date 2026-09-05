"""``python -m tools.scene <command>`` entry point."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

_REPO = Path(__file__).resolve().parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))


def _cmd_selftest(_args: argparse.Namespace) -> int:
    from tools.scene.selftest import main as selftest_main
    return selftest_main()


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="tools.scene", description=__doc__)
    sub = p.add_subparsers(dest="command", required=True)

    sp = sub.add_parser("selftest", help="run the correctness gate")
    sp.set_defaults(func=_cmd_selftest)

    try:
        from tools.scene import cli_show
        cli_show.register(sub)
    except Exception:  # noqa: BLE001 - stay usable while the toolkit is built incrementally
        pass

    try:
        from tools.scene import cli_new
        cli_new.register(sub)
    except Exception:  # noqa: BLE001
        pass

    try:
        from tools.scene import cli_edit
        cli_edit.register(sub)
    except Exception:  # noqa: BLE001
        pass

    try:
        from tools.scene import cli_catalog
        cli_catalog.register(sub)
    except Exception:  # noqa: BLE001
        pass

    return p


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
