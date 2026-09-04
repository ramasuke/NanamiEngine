"""CLI: add-action / remove-action / regen-catalog."""

from __future__ import annotations

import argparse
import sys

from . import catalog_scan
from . import scaffold


def cmd_add_action(a: argparse.Namespace) -> int:
    try:
        params = [scaffold.parse_param(s) for s in (a.param or [])]
        log = scaffold.add_action(
            a.name, a.category, params=params, version=a.version,
            subdir=a.subdir, filters=not a.no_filters,
            encoding=a.encoding, dry_run=a.dry_run,
        )
    except scaffold.ScaffoldError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    for line in log:
        print("  " + line)
    if not a.dry_run:
        print("\nnext: build with")
        print("  MSBuild.exe NanamiEngine.sln -p:Configuration=Debug -p:Platform=x64 "
              "-p:PreferredToolArchitecture=x64 -m")
        print(f'then the editor shows the action at  {a.category} > {a.name}')
    return 0


def cmd_remove_action(a: argparse.Namespace) -> int:
    try:
        log = scaffold.remove_action(a.name, a.category, subdir=a.subdir,
                                     filters=not a.no_filters, dry_run=a.dry_run)
    except scaffold.ScaffoldError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    for line in log:
        print("  " + line)
    return 0


def cmd_regen_catalog(a: argparse.Namespace) -> int:
    if a.check:
        ok, msg = catalog_scan.check_fresh()
        print(msg)
        return 0 if ok else 1
    data = catalog_scan.scan()
    path = catalog_scan.write_catalog(data)
    print(f"wrote {path} ({len(data['actions'])} actions, {len(data['structs'])} structs)")
    return 0


def register(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("add-action", help="scaffold a C++ action and wire the build")
    p.add_argument("--name", required=True, help="C++ class name, e.g. FleeFromPlayer")
    p.add_argument("--category", required=True,
                   help='editor menu path, e.g. "Basic" or "Custom::AI"')
    p.add_argument("--param", action="append", metavar="name:type[=default]",
                   help="scalar param (type: int|float|bool|string); repeatable")
    p.add_argument("--version", type=int, default=0,
                   help="CEREAL_CLASS_VERSION (default 0 = macro omitted)")
    p.add_argument("--subdir", help="folder under Content/ (default: category with :: -> /)")
    p.add_argument("--no-filters", action="store_true", help="skip the .vcxproj.filters edit")
    p.add_argument("--encoding", default="cp932", choices=["cp932", "utf-8"])
    p.add_argument("--dry-run", action="store_true")
    p.set_defaults(func=cmd_add_action)

    p = sub.add_parser("remove-action", help="undo add-action (files + 3 wiring points)")
    p.add_argument("--name", required=True)
    p.add_argument("--category", required=True)
    p.add_argument("--subdir")
    p.add_argument("--no-filters", action="store_true")
    p.add_argument("--dry-run", action="store_true")
    p.set_defaults(func=cmd_remove_action)

    p = sub.add_parser("regen-catalog", help="rebuild tools/bt/catalog.json from the headers")
    p.add_argument("--check", action="store_true", help="exit non-zero if stale (no write)")
    p.set_defaults(func=cmd_regen_catalog)
