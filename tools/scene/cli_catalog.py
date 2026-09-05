"""``regen-catalog`` CLI subcommand."""

from __future__ import annotations

import argparse

from . import catalog_scan


def cmd_regen_catalog(args: argparse.Namespace) -> int:
    if args.check:
        ok, msg = catalog_scan.check_fresh()
        print(msg)
        return 0 if ok else 1
    data = catalog_scan.scan()
    path = catalog_scan.write_catalog(data)
    print(f"wrote {path}  ({len(data['components'])} components)")
    return 0


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("regen-catalog", help="rescan Component headers into catalog.json")
    sp.add_argument("--check", action="store_true", help="exit 1 if catalog.json is stale, write nothing")
    sp.set_defaults(func=cmd_regen_catalog)
