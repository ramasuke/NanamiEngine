"""CLI entry point for tools.bt.

    python -m tools.bt <command> [args]
    python tools/bt.py <command> [args]     (equivalent shim)

Commands are added incrementally; see docs/BehaviourTree.md for the full list.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

_REPO = Path(__file__).resolve().parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))


def _cmd_selftest(args: argparse.Namespace) -> int:
    from tools.bt.selftest import main as selftest_main
    return selftest_main()


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="tools.bt", description=__doc__)
    sub = p.add_subparsers(dest="command", required=True)

    sp = sub.add_parser("selftest", help="run the correctness gate")
    sp.set_defaults(func=_cmd_selftest)

    # further subcommands registered here as modules land:
    try:
        from tools.bt import cli_tree
        cli_tree.register(sub)
    except Exception:  # noqa: BLE001
        pass
    try:
        from tools.bt import cli_action
        cli_action.register(sub)
    except Exception:  # noqa: BLE001
        pass

    return p


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
