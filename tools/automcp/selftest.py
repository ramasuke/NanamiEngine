"""Correctness gate for tools/automcp. Needs no running engine.

Stages:
  1. client   - EngineClient against an in-process fake engine: results, ok:false errors,
                connection refused, reconnect after the engine drops the connection, timeouts.
  2. patch    - set_component_params on real GameObjects taken from Assets/Scene/*.scene:
                float/bool/base-class fields, vector list shorthand, and the error cases.
  3. server   - the MCPServer tool list and a few tool calls routed to the fake engine
                (skipped when the ``mcp`` package is not installed).

Run: ``python tools/automcp/selftest.py`` (or ``python -m tools.automcp selftest``).
"""

from __future__ import annotations

import asyncio
import inspect
import json
import socket
import struct
import sys
import tempfile
import threading
import time
import traceback
import zlib
from pathlib import Path

_HERE = Path(__file__).resolve()
_REPO = _HERE.parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.automcp import patch  # noqa: E402
from tools.automcp.client import EngineClient, EngineCommandError, EngineUnavailable  # noqa: E402
from tools.common import cereal_json as cj  # noqa: E402

SCENE_DIR = _REPO / "Assets" / "Scene"

EXPECTED_TOOLS = {
    "engine_status", "engine_launch", "screenshot", "windows_list", "window_open", "window_close",
    "window_set", "main_window_switch", "scene_list", "scene_load", "scene_reload", "hierarchy",
    "gameobject_find", "gameobject_get", "gameobject_get_json", "gameobject_set_json", "component_get",
    "component_set_params", "gameobject_set_transform", "gameobject_set_enable", "component_set_enable",
    "gameobject_select", "gameobject_destroy", "play", "stop", "end_play", "time_set_scale",
    "camera_get", "camera_set", "debug_draw_get", "debug_draw_set", "log_tail",
    "assets_find", "assets_reload", "model_view_open", "model_view_state", "model_view_select", "model_view_close",
    "animation_view_open", "animation_view_state", "animation_view_set", "animation_view_set_clip", "preview_camera",
}


class Reporter:
    def __init__(self) -> None:
        self.failures = 0
        self.passes = 0

    def section(self, title: str) -> None:
        print(f"\n== {title}")

    def ok(self, name: str) -> None:
        self.passes += 1
        print(f"  PASS {name}")

    def fail(self, name: str, err: str) -> None:
        self.failures += 1
        print(f"  FAIL {name}\n{err}")

    def check(self, name: str, fn) -> None:
        try:
            fn()
            self.ok(name)
        except Exception:  # noqa: BLE001
            self.fail(name, traceback.format_exc())

    def finish(self) -> int:
        print(f"\n{self.passes} passed, {self.failures} failed")
        return 1 if self.failures else 0


# ---------------------------------------------------------------------------
# fake engine
# ---------------------------------------------------------------------------
def _tiny_png() -> bytes:
    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    header = struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(b"\x00\xff\x00\x00")) + chunk(b"IEND", b"")


class FakeEngine:
    """Speaks the engine's line protocol for a handful of commands."""

    def __init__(self, game_object_json: str, screenshot_dir: Path) -> None:
        self.game_object_json = game_object_json
        self.screenshot_dir = screenshot_dir
        self.received: list[dict] = []
        self.drop_next = False
        self.stall_next = False
        # ModelView/AnimationView: the model loads, the animation source loads and the clip attaches after N polls
        self.model_ready_after = 2
        self.source_ready_after = 2
        self.attach_after = 2
        self.view_polls = 0
        self.source_polls = 0
        self.attach_polls = 0
        # assets.reload: status reports this many loading resources, one fewer per poll
        self.loading_after_reload = 2
        self.loading_left = 0
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(4)
        self.port = self._listener.getsockname()[1]
        self._stop = threading.Event()
        threading.Thread(target=self._accept_loop, daemon=True).start()

    def close(self) -> None:
        self._stop.set()
        self._listener.close()

    def _accept_loop(self) -> None:
        while not self._stop.is_set():
            try:
                conn, _ = self._listener.accept()
            except OSError:
                return
            threading.Thread(target=self._serve, args=(conn,), daemon=True).start()

    def _serve(self, conn: socket.socket) -> None:
        buffer = b""
        with conn:
            while True:
                try:
                    chunk = conn.recv(65536)
                except OSError:
                    return
                if not chunk:
                    return
                buffer += chunk
                while b"\n" in buffer:
                    line, _, buffer = buffer.partition(b"\n")
                    request = json.loads(line)
                    self.received.append(request)
                    if self.drop_next:
                        self.drop_next = False
                        return
                    if self.stall_next:
                        self.stall_next = False
                        continue
                    conn.sendall((json.dumps(self._handle(request)) + "\n").encode("utf-8"))

    def _handle(self, request: dict) -> dict:
        cmd, args = request["cmd"], request.get("args", {})
        response: dict = {"id": request["id"], "ok": True}
        if cmd == "ping":
            response["result"] = {"engine": "NanamiEngine", "protocol": 1}
        elif cmd == "status":
            response["result"] = {"playMode": False, "mainScene": {"name": "Fake"}, "workingDirectory": "C:/fake",
                                  "loadingResourceCount": self.loading_left}
            self.loading_left = max(0, self.loading_left - 1)
        elif cmd == "assets.reload":
            self.loading_left = self.loading_after_reload
            response["result"] = {"previousAssetCount": 3, "assetCount": 4}
        elif cmd in ("debugdraw.get", "debugdraw.set"):
            response["result"] = {"colliders": args.get("colliders", False), "saved": args.get("save", False)}
        elif cmd == "boom":
            return {"id": request["id"], "ok": False, "error": "boom failed"}
        elif cmd == "screenshot":
            path = self.screenshot_dir / "screenshot_1.png"
            path.write_bytes(_tiny_png())
            response["result"] = {"path": str(path), "format": "png", "mode": args.get("mode"), "width": 1,
                                  "height": 1, "sourceWidth": 1, "sourceHeight": 1}
        elif cmd == "gameobject.get_json":
            response["result"] = {"guid": args["guid"], "json": self.game_object_json}
        elif cmd == "gameobject.set_json":
            self.game_object_json = args["json"]
            response["result"] = {"guid": args["guid"], "components": []}
        elif cmd in ("modelview.open", "animationview.open"):
            self.view_polls = 0
            response["result"] = self._view_state()
        elif cmd in ("modelview.state", "animationview.state"):
            self.view_polls += 1
            self.source_polls += 1
            self.attach_polls += 1
            response["result"] = self._view_state()
        elif cmd == "animationview.set_clip":
            if "animationPath" in args or "animationGuid" in args or "useModelClips" in args:
                self.source_polls = 0
            if "clipName" in args and self.source_polls < self.source_ready_after:
                return {"id": request["id"], "ok": False, "error": "clips are not loaded yet"}
            if "clipName" in args or "clipIndex" in args:
                self.attach_polls = 0
            response["result"] = self._view_state()
        else:
            return {"id": request["id"], "ok": False, "error": f"unknown command: {cmd}"}
        return response

    def _view_state(self) -> dict:
        source_ready = self.source_polls >= self.source_ready_after
        return {
            "modelReady": self.view_polls >= self.model_ready_after,
            "model": {"path": "Assets/Art/Fake.mv1", "guid": "MODEL"},
            "slots": {"A": {"sourceReady": source_ready,
                            "attached": source_ready and self.attach_polls >= self.attach_after},
                      "B": {"sourceReady": True, "attached": False}},
        }


# ---------------------------------------------------------------------------
# fixtures
# ---------------------------------------------------------------------------
def _game_object_documents():
    """Yield ``(scene name, json text of {"gameObject": root})`` for every root in the committed scenes."""
    for scene in sorted(SCENE_DIR.glob("*.scene"), key=lambda p: p.stat().st_size):
        doc = cj.loads(cj.read_text(scene))
        count = doc.get("gameObjectCount")
        for i in range(int(count.value) if isinstance(count, cj.Num) else 0):
            root = doc.get(f"gameObject_{i}")
            if isinstance(root, cj.OrderedObj) and "polymorphic_name" in root:
                yield scene.name, cj.dumps(cj.OrderedObj([("gameObject", root)]))


def _find_fixture():
    """A component with a top-level float field, plus that GameObject's json."""
    for scene_name, text in _game_object_documents():
        doc = cj.loads(text)
        for _i, name, data in patch.iter_components(patch.game_object_data(doc)):
            guid = patch.component_base_guid(data)
            if guid is None:
                continue
            for key, value in data.items():
                if isinstance(value, cj.Num) and not value.is_int and not patch._BASE_KEY.match(key):
                    return scene_name, text, guid, key, name
    raise AssertionError("no component with a float field found in Assets/Scene/*.scene")


def _find_vector_fixture():
    for _scene_name, text in _game_object_documents():
        doc = cj.loads(text)
        for _i, _name, data in patch.iter_components(patch.game_object_data(doc)):
            guid = patch.component_base_guid(data)
            if guid is None:
                continue
            for key, value in data.items():
                if isinstance(value, cj.OrderedObj) and patch._positional_keys(value) == ["value0", "value1", "value2"] \
                        and all(isinstance(value[k], cj.Num) for k in ("value0", "value1", "value2")):
                    return text, guid, key
    return None


# ---------------------------------------------------------------------------
# stages
# ---------------------------------------------------------------------------
def stage_client(r: Reporter, engine: FakeEngine) -> None:
    r.section("client")
    client = EngineClient(port=engine.port, request_timeout=2.0)

    def result_roundtrip():
        assert client.call("ping")["engine"] == "NanamiEngine"
        assert client.call("status")["mainScene"]["name"] == "Fake"
    r.check("call returns result", result_roundtrip)

    def command_error():
        try:
            client.call("boom")
        except EngineCommandError as e:
            assert "boom failed" in str(e)
            return
        raise AssertionError("expected EngineCommandError")
    r.check("ok:false raises EngineCommandError", command_error)

    def reconnect():
        engine.drop_next = True
        try:
            client.call("ping")
        except EngineUnavailable:
            pass
        assert client.call("ping")["protocol"] == 1
    r.check("reconnects after the engine drops the connection", reconnect)

    def timeout():
        engine.stall_next = True
        started = time.monotonic()
        try:
            client.call("ping", timeout=0.3)
        except EngineUnavailable as e:
            assert "did not answer" in str(e)
            assert time.monotonic() - started < 2.0
            assert client.call("ping")["protocol"] == 1
            return
        raise AssertionError("expected EngineUnavailable on timeout")
    r.check("timeout raises EngineUnavailable and recovers", timeout)

    def refused():
        probe = socket.socket()
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
        probe.close()
        try:
            EngineClient(port=port, connect_timeout=0.5).call("ping")
        except EngineUnavailable as e:
            assert "AutoMCP" in str(e)
            return
        raise AssertionError("expected EngineUnavailable")
    r.check("connection refused raises EngineUnavailable", refused)
    client.close()


def stage_patch(r: Reporter) -> None:
    r.section("patch")
    try:
        scene_name, text, guid, key, type_name = _find_fixture()
    except Exception:  # noqa: BLE001
        r.fail("fixture", traceback.format_exc())
        return
    print(f"  fixture: {scene_name} component {type_name or '(repeated type)'} {guid} field {key}")

    def float_field():
        original = cj.loads(text)
        before = patch.find_component(original, guid)[key]
        patched, changed = patch.set_component_params(text, guid, {key: 12.5})
        assert changed == [key]
        doc = cj.loads(patched)
        assert patch.find_component(doc, guid)[key] == cj.Num.of_float(12.5)
        patch.find_component(doc, guid)[key] = before
        assert cj.dumps(doc) == cj.dumps(original), "only the patched field may change"
    r.check("float field", float_field)

    def base_bool_field():
        patched, _ = patch.set_component_params(text, guid, {"isEnable_": False})
        doc = cj.loads(patched)
        assert patch.component_base_guid(patch.find_component(doc, guid)) == guid
        owner = patch._field_owner(patch.find_component(doc, guid), "isEnable_")
        assert owner is not None and owner["isEnable_"] is False
    r.check("base-class bool field (isEnable_)", base_bool_field)

    def guid_case_insensitive():
        patch.set_component_params(text, guid.lower(), {key: 1})
    r.check("component guid is case-insensitive; int accepted for float", guid_case_insensitive)

    def expect_error(params, needle, component=None):
        try:
            patch.set_component_params(text, component or guid, params)
        except patch.PatchError as e:
            assert needle in str(e), f"{needle!r} not in {e}"
            return
        raise AssertionError(f"expected PatchError for {params}")

    r.check("unknown field is rejected", lambda: expect_error({"noSuchField_": 1}, "no such field"))
    r.check("type mismatch is rejected", lambda: expect_error({key: "text"}, "expected a number"))
    r.check("cereal metadata is rejected", lambda: expect_error({"cereal_class_version": 3}, "not a field name"))
    r.check("component guid_ is read-only", lambda: expect_error({"guid_": {"value_": "X"}}, "can't be changed"))
    r.check("unknown component is rejected", lambda: expect_error({key: 1}, "is not on this GameObject",
                                                                 "00000000-0000-0000-0000-000000000000"))

    def vector_shorthand():
        fixture = _find_vector_fixture()
        if fixture is None:
            print("    (no vec3 field in the scenes; skipped)")
            return
        vec_text, vec_guid, vec_key = fixture
        patched, _ = patch.set_component_params(vec_text, vec_guid, {vec_key: [1, 2.5, -3]})
        value = patch.find_component(cj.loads(patched), vec_guid)[vec_key]
        assert [value[f"value{i}"].value for i in range(3)] == [1, 2.5, -3]
        try:
            patch.set_component_params(vec_text, vec_guid, {vec_key: [1, 2]})
        except patch.PatchError:
            return
        raise AssertionError("wrong vector length must be rejected")
    r.check("vector list shorthand", vector_shorthand)

    def fields_listing():
        fields = json.loads(patch.component_fields_json(text, guid))
        assert key in fields
    r.check("component_fields_json", fields_listing)


def stage_server(r: Reporter, engine: FakeEngine) -> None:
    r.section("server")
    try:
        import mcp  # noqa: F401
    except ImportError:
        r.ok('skipped: mcp not installed (pip install "mcp>=2.2")')
        return

    from tools.automcp.server import build_server

    async def run(value):
        return await value if inspect.isawaitable(value) else value

    client = EngineClient(port=engine.port, request_timeout=5.0)
    server = build_server(client, poll_interval=0.01)

    def tool_list():
        tools = asyncio.run(run(server.list_tools()))
        names = {t.name for t in tools}
        missing = EXPECTED_TOOLS - names
        extra = names - EXPECTED_TOOLS
        assert not missing and not extra, f"missing={missing} extra={extra}"
    r.check("tool list", tool_list)

    def call(name, arguments):
        result = asyncio.run(run(server.call_tool(name, arguments)))
        return result

    def status_tool():
        result = call("engine_status", {})
        assert not getattr(result, "is_error", False), result
        assert json.loads(result.content[0].text)["mainScene"]["name"] == "Fake"
    r.check("engine_status", status_tool)

    def screenshot_tool():
        result = call("screenshot", {"mode": "game", "format": "png"})
        assert not getattr(result, "is_error", False), result
        kinds = [c.type for c in result.content]
        assert kinds == ["image", "text"], kinds
        assert result.content[0].mime_type == "image/png"
        assert engine.received[-1]["args"]["mode"] == "game"
    r.check("screenshot returns an image", screenshot_tool)

    def params_tool():
        _scene, text, guid, key, _type = _find_fixture()
        engine.game_object_json = text
        result = call("component_set_params", {"guid": "GO", "component_guid": guid, "params": {key: 7.25}})
        assert not getattr(result, "is_error", False), result
        sent = next(q for q in reversed(engine.received) if q["cmd"] == "gameobject.set_json")
        assert patch.find_component(cj.loads(sent["args"]["json"]), guid)[key] == cj.Num.of_float(7.25)
    r.check("component_set_params patches and sends set_json", params_tool)

    # In-process call_tool raises ToolError; the stdio handler turns it into an is_error result.
    from mcp.server.mcpserver.exceptions import ToolError

    def expect_tool_error(target, name, arguments, needle):
        try:
            asyncio.run(run(target.call_tool(name, arguments)))
        except ToolError as e:
            assert needle in str(e), f"{needle!r} not in {e}"
            return
        raise AssertionError("expected ToolError")

    r.check("patch errors raise ToolError", lambda: expect_tool_error(
        server, "component_set_params", {"guid": "GO", "component_guid": "nope", "params": {"x": 1}},
        "is not on this GameObject"))

    def unreachable_tool():
        probe = socket.socket()
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
        probe.close()
        offline = build_server(EngineClient(port=port, connect_timeout=0.5))
        expect_tool_error(offline, "engine_status", {}, "AutoMCP")
    r.check("engine not running -> ToolError with a hint", unreachable_tool)

    def commands_since(start: int) -> list[dict]:
        return engine.received[start:]

    def model_view_waits():
        start = len(engine.received)
        result = call("model_view_open", {"path": "Fake.mv1"})
        state = json.loads(result.content[0].text)
        assert state["modelReady"] is True, state
        cmds = [q["cmd"] for q in commands_since(start)]
        assert cmds[0] == "modelview.open" and cmds.count("modelview.state") == engine.model_ready_after, cmds
    r.check("model_view_open waits for the model to load", model_view_waits)

    def model_view_timeout():
        engine.model_ready_after = 10**9
        try:
            expect_tool_error(server, "model_view_open", {"path": "Fake.mv1", "timeout_seconds": 0.05}, "timed out")
        finally:
            engine.model_ready_after = 2
    r.check("model_view_open times out with the last state", model_view_timeout)

    def set_clip_sequence():
        start = len(engine.received)
        result = call("animation_view_set_clip", {"slot": "A", "animation_path": "Anim.mv1",
                                                   "clip_name": "Idle", "time": 0.5})
        state = json.loads(result.content[0].text)
        assert state["slots"]["A"]["attached"] is True, state
        sets = [q["args"] for q in commands_since(start) if q["cmd"] == "animationview.set_clip"]
        assert len(sets) == 2, sets
        assert "animationPath" in sets[0] and "clipName" not in sets[0], sets
        assert sets[1].get("clipName") == "Idle" and sets[1].get("time") == 0.5, sets
    r.check("animation_view_set_clip loads the source, then picks the clip and waits to attach", set_clip_sequence)

    def set_clip_no_wait():
        start = len(engine.received)
        call("animation_view_set_clip", {"slot": "A", "clip_index": 3, "wait": False})
        cmds = [q["cmd"] for q in commands_since(start)]
        assert cmds == ["animationview.set_clip"], cmds
    r.check("animation_view_set_clip wait=False sends one command", set_clip_no_wait)

    def debug_draw_set_args():
        call("debug_draw_set", {"colliders": True, "shapes": {"StaticMesh": True}, "layers": False})
        sent = engine.received[-1]
        assert sent["cmd"] == "debugdraw.set", sent
        assert sent["args"] == {"colliders": True, "shapes": {"StaticMesh": True}, "layers": False}, sent["args"]
        call("debug_draw_set", {"main_camera_frustum": False, "save": True})
        assert engine.received[-1]["args"] == {"mainCameraFrustum": False, "save": True}, engine.received[-1]["args"]
    r.check("debug_draw_set sends only the given settings", debug_draw_set_args)

    def assets_reload_waits():
        start = len(engine.received)
        result = call("assets_reload", {})
        state = json.loads(result.content[0].text)
        assert state == {"previousAssetCount": 3, "assetCount": 4, "loadingResourceCount": 0}, state
        cmds = [q["cmd"] for q in commands_since(start)]
        assert cmds == ["assets.reload"] + ["status"] * (engine.loading_after_reload + 1), cmds
    r.check("assets_reload waits until nothing is loading", assets_reload_waits)

    def assets_reload_no_wait():
        start = len(engine.received)
        state = json.loads(call("assets_reload", {"wait": False}).content[0].text)
        assert "loadingResourceCount" not in state, state
        cmds = [q["cmd"] for q in commands_since(start)]
        assert cmds == ["assets.reload"], cmds
        engine.loading_left = 0
    r.check("assets_reload wait=False sends one command", assets_reload_no_wait)
    client.close()


def main() -> int:
    r = Reporter()
    with tempfile.TemporaryDirectory() as tmp:
        try:
            _scene, text, _guid, _key, _type = _find_fixture()
        except Exception:  # noqa: BLE001
            text = "{}"
        engine = FakeEngine(text, Path(tmp))
        try:
            stage_client(r, engine)
            stage_patch(r)
            stage_server(r, engine)
        finally:
            engine.close()
    return r.finish()


if __name__ == "__main__":
    raise SystemExit(main())
