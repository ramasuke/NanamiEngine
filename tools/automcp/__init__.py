"""AutoMCP: Claude Code と起動中の NanamiEngine エディタをつなぐ MCP (Model Context Protocol)
ブリッジ。

エンジンは ``127.0.0.1:<port>`` (Config > AutoMCP) で待ち受けて改行区切りの JSON を
話し、:mod:`tools.automcp.server` がそれを stdio 上の MCP ツールとして公開する。
``serve`` 以外はすべて標準ライブラリのみ。``serve`` には ``pip install mcp`` が必要。
"""

__version__ = "0.1.0"
