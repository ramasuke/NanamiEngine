"""AutoMCP: an MCP (Model Context Protocol) bridge between Claude Code and a running
NanamiEngine editor.

The engine listens on ``127.0.0.1:<port>`` (Config > AutoMCP) and speaks
newline-delimited JSON; :mod:`tools.automcp.server` exposes that as MCP tools over
stdio. Everything except ``serve`` is stdlib-only; ``serve`` needs ``pip install mcp``.
"""

__version__ = "0.1.0"
