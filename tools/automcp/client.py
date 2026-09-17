"""TCP client for the engine's AutoMCP listener.

Wire format (one JSON object per line, UTF-8):
    request  ``{"id": 1, "cmd": "status", "args": {...}}``
    response ``{"id": 1, "ok": true, "result": {...}}`` or ``{"id": 1, "ok": false, "error": "..."}``

The engine polls the socket once per frame, so a call takes at least one frame;
``screenshot`` takes two (it returns the next frame's image).
"""

from __future__ import annotations

import itertools
import json
import os
import socket
import threading
from typing import Any

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 47321
PORT_ENV = "NANAMI_AUTOMCP_PORT"


class EngineUnavailable(RuntimeError):
    """The engine is not reachable (not running, AutoMCP disabled, frozen, ...)."""


class EngineCommandError(RuntimeError):
    """The engine answered ``ok: false``."""


def resolve_port(port: int | None = None) -> int:
    if port is not None:
        return port
    raw = os.environ.get(PORT_ENV, "").strip()
    if not raw:
        return DEFAULT_PORT
    try:
        return int(raw)
    except ValueError:
        raise EngineUnavailable(f"{PORT_ENV}={raw!r} is not a port number") from None


class EngineClient:
    """A reusable, thread-safe connection. Reconnects transparently when the engine restarts."""

    def __init__(self, host: str = DEFAULT_HOST, port: int | None = None,
                 connect_timeout: float = 2.0, request_timeout: float = 30.0) -> None:
        self.host = host
        self.port = resolve_port(port)
        self.connect_timeout = connect_timeout
        self.request_timeout = request_timeout
        self._sock: socket.socket | None = None
        self._buffer = b""
        self._ids = itertools.count(1)
        self._lock = threading.Lock()

    # -- connection -----------------------------------------------------------
    def close(self) -> None:
        if self._sock is not None:
            try:
                self._sock.close()
            except OSError:
                pass
        self._sock = None
        self._buffer = b""

    def _connect(self) -> None:
        try:
            sock = socket.create_connection((self.host, self.port), timeout=self.connect_timeout)
        except OSError as e:
            raise EngineUnavailable(
                f"cannot connect to NanamiEngine at {self.host}:{self.port} ({e}). "
                "Start the editor and enable Config > AutoMCP "
                f"(or set {PORT_ENV} if the port was changed)."
            ) from e
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self._sock = sock
        self._buffer = b""

    def _read_line(self) -> bytes:
        assert self._sock is not None
        while b"\n" not in self._buffer:
            chunk = self._sock.recv(1 << 16)
            if not chunk:
                raise ConnectionResetError("engine closed the connection")
            self._buffer += chunk
        line, _, self._buffer = self._buffer.partition(b"\n")
        return line

    # -- requests -------------------------------------------------------------
    def call(self, cmd: str, args: dict[str, Any] | None = None, timeout: float | None = None) -> dict[str, Any]:
        """Send one command and return its ``result`` object.

        Raises :class:`EngineUnavailable` when the engine can't be reached or doesn't
        answer in time, and :class:`EngineCommandError` when it rejects the command.
        """
        wait = timeout if timeout is not None else self.request_timeout
        with self._lock:
            for attempt in (0, 1):
                if self._sock is None:
                    self._connect()
                request_id = next(self._ids)
                payload = json.dumps({"id": request_id, "cmd": cmd, "args": args or {}},
                                     ensure_ascii=False, separators=(",", ":")) + "\n"
                try:
                    assert self._sock is not None
                    self._sock.settimeout(wait)
                    self._sock.sendall(payload.encode("utf-8"))
                    response = self._receive(request_id)
                except socket.timeout:
                    self.close()
                    raise EngineUnavailable(
                        f"NanamiEngine did not answer {cmd!r} within {wait:g}s "
                        "(frozen, loading, or paused in a debugger?)"
                    ) from None
                except OSError as e:
                    # A connection left over from a previous engine run fails on first use.
                    self.close()
                    if attempt == 0:
                        continue
                    raise EngineUnavailable(f"lost the connection to NanamiEngine during {cmd!r}: {e}") from e

                if not response.get("ok"):
                    raise EngineCommandError(response.get("error") or f"{cmd} failed")
                result = response.get("result")
                return result if isinstance(result, dict) else {}
        raise EngineUnavailable("unreachable")  # pragma: no cover

    def _receive(self, request_id: int) -> dict[str, Any]:
        while True:
            line = self._read_line()
            if not line.strip():
                continue
            try:
                response = json.loads(line.decode("utf-8", errors="replace"))
            except json.JSONDecodeError as e:
                raise EngineCommandError(f"engine sent invalid JSON: {e}") from e
            if response.get("id") in (request_id, None):
                return response
