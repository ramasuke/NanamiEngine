"""``regen-catalog`` - rebuild ``catalog.json`` from the ``IAnimationNode``
subclass headers."""

from __future__ import annotations

import argparse
import sys

from . import catalog_scan


def cmd_regen_catalog(a: argparse.Namespace) -> int:
    if a.check:
        ok, msg = catalog_scan.check_fresh()
        print(msg)
        return 0 if ok else 1
    data = catalog_scan.scan()
    path = catalog_scan.write_catalog(data)
    print(f"wrote {path} ({len(data['node_types'])} node type(s))")
    return 0


def register(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("regen-catalog", help="rebuild catalog.json from the IAnimationNode headers")
    p.add_argument("--check", action="store_true", help="only check freshness; exit 1 if stale")
    p.set_defaults(func=cmd_regen_catalog)
