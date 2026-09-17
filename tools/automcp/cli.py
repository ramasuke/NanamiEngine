"""Subcommands: ``serve`` (the MCP stdio server), ``call`` (send one raw command), ``check-env``."""

from __future__ import annotations

import argparse
import importlib.metadata
import json
import sys

from tools.automcp.client import PORT_ENV, EngineClient, EngineCommandError, EngineUnavailable


def cmd_serve(_args: argparse.Namespace) -> int:
    from tools.automcp.server import serve
    return serve()


def cmd_call(args: argparse.Namespace) -> int:
    try:
        payload = json.loads(args.args) if args.args else {}
    except json.JSONDecodeError as e:
        print(f"error: --args is not JSON: {e}", file=sys.stderr)
        return 2
    if not isinstance(payload, dict):
        print("error: --args must be a JSON object", file=sys.stderr)
        return 2

    client = EngineClient(port=args.port, request_timeout=args.timeout)
    try:
        result = client.call(args.command, payload)
    except (EngineUnavailable, EngineCommandError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    finally:
        client.close()

    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0


def cmd_check_env(args: argparse.Namespace) -> int:
    from tools.automcp.server import DEFAULT_EXE, REPO

    print(f"python        : {sys.version.split()[0]} ({sys.executable})")
    try:
        print(f"mcp           : {importlib.metadata.version('mcp')}")
    except importlib.metadata.PackageNotFoundError:
        print('mcp           : NOT INSTALLED - pip install "mcp>=2.2"')

    print(f"repo          : {REPO}")
    print(f"engine exe    : {DEFAULT_EXE} ({'found' if DEFAULT_EXE.is_file() else 'missing - not built'})")
    print(f".mcp.json     : {'found' if (REPO / '.mcp.json').is_file() else 'missing'}")

    client = EngineClient(port=args.port, request_timeout=5.0)
    print(f"engine port   : {client.port} (override with {PORT_ENV})")
    try:
        status = client.call("status")
        scene = status.get("mainScene") or {}
        print(f"engine        : reachable - playMode={status.get('playMode')} mainScene={scene.get('name')!r} "
              f"cwd={status.get('workingDirectory')}")
    except (EngineUnavailable, EngineCommandError) as e:
        print(f"engine        : not reachable ({e})")
    finally:
        client.close()
    return 0


def register(sub: argparse._SubParsersAction) -> None:
    sp = sub.add_parser("serve", help="run the MCP stdio server (started by .mcp.json)")
    sp.set_defaults(func=cmd_serve)

    sp = sub.add_parser("call", help="send one raw command to the engine and print the result")
    sp.add_argument("command", help="e.g. ping, status, hierarchy, screenshot")
    sp.add_argument("--args", default="", help='JSON object, e.g. \'{"maxDepth": 0}\'')
    sp.add_argument("--port", type=int, default=None)
    sp.add_argument("--timeout", type=float, default=30.0)
    sp.set_defaults(func=cmd_call)

    sp = sub.add_parser("check-env", help="show the Python/MCP SDK/exe/engine connection status")
    sp.add_argument("--port", type=int, default=None)
    sp.set_defaults(func=cmd_check_env)
