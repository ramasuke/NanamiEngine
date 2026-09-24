"""起動中の NanamiEngine エディタを Claude Code に公開する MCP サーバー (stdio)。

``.mcp.json`` から ``python -m tools.automcp serve`` として起動される。プロセスは
Claude Code のセッション中ずっと動き続け、ツール呼び出しのたびに遅延でエンジンに
接続するので、エディタの起動・再起動や AutoMCP の切り替えはいつでもできる。
ここでは決して stdout に出力しないこと: stdout は MCP の通信路。
"""

from __future__ import annotations

import json
import logging
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Literal

from tools.automcp import patch
from tools.automcp.client import EngineClient, EngineCommandError, EngineUnavailable

REPO = Path(__file__).resolve().parents[2]
EXE_ENV = "NANAMI_ENGINE_EXE"
CWD_ENV = "NANAMI_ENGINE_CWD"
DEFAULT_EXE = REPO / "x64" / "Debug" / "EnviroHunter.exe"

INSTRUCTIONS = """\
Tools for the running NanamiEngine editor (C++ DxLib/ImGui game engine).
The editor must be running with Config > AutoMCP enabled; engine_status tells you whether it is reachable.
- Only use these tools when the user explicitly asks you to; never to test your own changes unprompted.
- When asked to change things, look before and after:screenshot(mode="full") shows the editor with ImGui windows, mode="game" shows only the rendered scene.
- GameObject/component guids come from hierarchy or gameobject_find; guids are stable across edits.
- gameobject_set_transform / *_set_enable / camera_set / time_set_scale apply immediately, also while playing.
- component_set_params and gameobject_set_json rebuild the whole GameObject (its components re-initialise), so prefer edit mode for them.
- None of these save to disk. Persist scene edits with tools.scene (python -m tools.scene ...) and then scene_reload.
- The editor reads Assets/ only at startup: after adding or rewriting assets or .meta files on disk, call assets_reload, then scene_reload.
- Paths (scene_load) are relative to workingDirectory reported by engine_status."""

log = logging.getLogger("tools.automcp")


def _dump(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def build_server(client: EngineClient | None = None, poll_interval: float = 0.1):
    """MCPServer を作る。``mcp`` は遅延 import するので、ツールキットの他の部分は標準ライブラリのみのまま。"""
    from mcp.server.mcpserver import Image, MCPServer
    from mcp.server.mcpserver.exceptions import ToolError

    engine = client or EngineClient()
    mcp = MCPServer("nanami", instructions=INSTRUCTIONS)

    def call(cmd: str, args: dict[str, Any] | None = None, timeout: float | None = None) -> dict[str, Any]:
        try:
            return engine.call(cmd, {k: v for k, v in (args or {}).items() if v is not None}, timeout)
        except (EngineUnavailable, EngineCommandError) as e:
            raise ToolError(str(e)) from e

    def tool(fn):
        return mcp.tool(structured_output=False)(fn)

    def wait_for(cmd: str, args: dict[str, Any], ready, timeout_seconds: float, what: str) -> dict[str, Any]:
        """``ready(state)`` になるまで ``cmd`` をポーリングする。エンジンはモデルの読み込みやクリップの付与を後のフレームで行う。"""
        deadline = time.monotonic() + timeout_seconds
        state = call(cmd, args)
        while not ready(state):
            if time.monotonic() >= deadline:
                raise ToolError(f"timed out after {timeout_seconds:g}s waiting for {what}; last state: {_dump(state)}")
            time.sleep(poll_interval)
            state = call(cmd, args)
        return state

    # -- エンジン -------------------------------------------------------------
    @tool
    def engine_status() -> str:
        """Play state, time scale, FPS, screen size, current main window, main scene and working directory of the running editor."""
        return _dump(call("status"))

    @tool
    def engine_launch(wait_seconds: float = 90.0) -> str:
        """Start x64/Debug/EnviroHunter.exe (working directory = repo root) and wait until AutoMCP answers.
        Does nothing if the editor already answers. The exe must already be built; the build is the user's call."""
        try:
            return _dump({"alreadyRunning": True, **engine.call("status", timeout=3.0)})
        except (EngineUnavailable, EngineCommandError):
            pass

        exe = Path(os.environ.get(EXE_ENV) or DEFAULT_EXE)
        cwd = Path(os.environ.get(CWD_ENV) or REPO)
        if not exe.is_file():
            raise ToolError(f"{exe} does not exist; build the Debug|x64 configuration first (ask the user before building)")

        flags = 0
        if sys.platform == "win32":
            flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
        process = subprocess.Popen([str(exe)], cwd=str(cwd), stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=flags)

        deadline = time.monotonic() + wait_seconds
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise ToolError(f"NanamiEngine exited with code {process.returncode} during startup")
            try:
                return _dump({"launched": True, "pid": process.pid, **engine.call("status", timeout=3.0)})
            except (EngineUnavailable, EngineCommandError):
                time.sleep(1.0)
        raise ToolError(f"NanamiEngine started (pid {process.pid}) but AutoMCP did not answer within {wait_seconds:g}s. "
                        "Enable Config > AutoMCP in the editor (saved to ProjectConfig/AutoMcp/Enabled.json).")

    @tool
    def screenshot(mode: Literal["full", "game"] = "full", format: Literal["jpeg", "png"] = "jpeg",
                   max_width: int = 0, quality: int = 85):
        """Capture the next rendered frame. mode="full" includes the ImGui editor windows and gizmos,
        mode="game" is the 3D/game rendering only. max_width 0 uses the engine's Config value (default 1280)."""
        result = call("screenshot", {"mode": mode, "format": format, "maxWidth": max_width, "quality": quality})
        path = Path(result.get("path", ""))
        try:
            data = path.read_bytes()
        except OSError as e:
            raise ToolError(f"engine saved the screenshot to {path} but it can't be read here: {e}") from e
        caption = (f"{result.get('mode')} {result.get('width')}x{result.get('height')} "
                   f"(source {result.get('sourceWidth')}x{result.get('sourceHeight')}) {path}")
        return [Image(data=data, format=result.get("format", format)), caption]

    # -- ウィンドウ -------------------------------------------------------------
    @tool
    def windows_list(include_hidden: bool = False) -> str:
        """Open popup windows (type + guid), openable popup types, main windows, and every ImGui window
        with its exact name (including ##id), position, size, collapsed and focus state."""
        return _dump(call("windows.list", {"includeHidden": include_hidden}))

    @tool
    def window_open(type: str) -> str:
        """Open a popup window by type, e.g. InspectorWindow, ConsoleWindow, ProjectWindow (see windows_list popupTypes)."""
        return _dump(call("windows.open", {"type": type}))

    @tool
    def window_close(guid: str) -> str:
        """Close a popup window by the guid from windows_list."""
        return _dump(call("windows.close", {"guid": guid}))

    @tool
    def window_set(name: str, position: list[float] | None = None, size: list[float] | None = None,
                   collapsed: bool | None = None, focus: bool = False) -> str:
        """Move, resize, collapse or focus an ImGui window. name must be exact (e.g. "Inspector##0"); position/size are [x, y] in pixels."""
        return _dump(call("windows.set", {"name": name, "position": position, "size": size,
                                          "collapsed": collapsed, "focus": focus}))

    @tool
    def main_window_switch(name: str) -> str:
        """Switch the main window (GameWindow, ModelViewWindow, AnimationViewWindow, PrefabViewWindow, AnimatorWindow; see windows_list mainWindows)."""
        return _dump(call("mainwindow.switch", {"name": name}))

    # -- シーン -----------------------------------------------------------------
    @tool
    def scene_list() -> str:
        """Loaded scenes with name, guid, file path, whether it is the main scene, and root object count."""
        return _dump(call("scene.list"))

    @tool
    def scene_load(path: str, make_main: bool = True) -> str:
        """Load a .scene file (path relative to the engine working directory, e.g. Assets/Scene/TitleScene.scene) in addition to the loaded ones."""
        return _dump(call("scene.load", {"path": path, "makeMain": make_main}))

    @tool
    def scene_reload(guid: str | None = None) -> str:
        """Re-read a loaded scene from its file (default: the main scene), e.g. after editing it with tools.scene. Unsaved in-editor changes to it are lost."""
        return _dump(call("scene.reload", {"guid": guid}))

    @tool
    def hierarchy(scene_guid: str | None = None, max_depth: int = -1, include_components: bool = True) -> str:
        """GameObject tree of the loaded scenes (or one scene): name, guid, type, isActive, mark, components (type, guid, enabled) and children.
        max_depth 0 lists roots only; -1 is unlimited."""
        return _dump(call("hierarchy", {"sceneGuid": scene_guid, "maxDepth": max_depth,
                                        "includeComponents": include_components}))

    # -- GameObjects ------------------------------------------------------------
    @tool
    def gameobject_find(name: str, exact: bool = False, limit: int = 50) -> str:
        """Find GameObjects by name (case-insensitive substring unless exact) across loaded scenes; returns guid and hierarchy path."""
        return _dump(call("gameobject.find", {"name": name, "exact": exact, "limit": limit}))

    @tool
    def gameobject_get(guid: str) -> str:
        """One GameObject: header, parent, local/world transform (positions, rotations as [x,y,z,w] and Euler degrees, scale), components, children."""
        return _dump(call("gameobject.get", {"guid": guid}))

    @tool
    def gameobject_get_json(guid: str) -> str:
        """The GameObject (with its components and children) as the engine's cereal JSON - the same format as .scene files."""
        return call("gameobject.get_json", {"guid": guid}).get("json", "")

    @tool
    def gameobject_set_json(guid: str, json_text: str) -> str:
        """Rebuild a GameObject from cereal JSON obtained with gameobject_get_json (keep all guids). Components re-initialise; not saved to disk."""
        return _dump(call("gameobject.set_json", {"guid": guid, "json": json_text}))

    @tool
    def component_get(guid: str, component_guid: str) -> str:
        """Field names and current values of one component (cereal JSON; base-class fields sit under value0/value1...)."""
        text = call("gameobject.get_json", {"guid": guid}).get("json", "")
        try:
            return patch.component_fields_json(text, component_guid)
        except patch.PatchError as e:
            raise ToolError(str(e)) from e

    @tool
    def component_set_params(guid: str, component_guid: str, params: dict[str, Any]) -> str:
        """Set component fields by name, e.g. {"speed_": 3.5, "isLoop_": true, "offset_": [0, 1, 0]}.
        Values must match the existing JSON type; objects merge key by key; a list sets value0..valueN of a vector.
        Rebuilds the GameObject (components re-initialise); not saved to disk."""
        text = call("gameobject.get_json", {"guid": guid}).get("json", "")
        try:
            patched, changed = patch.set_component_params(text, component_guid, params)
        except patch.PatchError as e:
            raise ToolError(str(e)) from e
        result = call("gameobject.set_json", {"guid": guid, "json": patched})
        return _dump({"changed": changed, **result})

    @tool
    def gameobject_set_transform(guid: str, local_position: list[float] | None = None,
                                 local_euler_degrees: list[float] | None = None,
                                 local_rotation: list[float] | None = None,
                                 local_scale: list[float] | None = None,
                                 world_position: list[float] | None = None,
                                 world_euler_degrees: list[float] | None = None) -> str:
        """Set any of the transform values ([x,y,z]; local_rotation is a quaternion [x,y,z,w]). Returns the resulting transform."""
        return _dump(call("gameobject.set_transform", {
            "guid": guid, "localPosition": local_position, "localEulerDegrees": local_euler_degrees,
            "localRotation": local_rotation, "localScale": local_scale,
            "worldPosition": world_position, "worldEulerDegrees": world_euler_degrees}))

    @tool
    def gameobject_set_enable(guid: str, enable: bool) -> str:
        """Enable or disable a GameObject (all its components and children). isActive in hierarchy may stay stale; check component enabled flags."""
        return _dump(call("gameobject.set_enable", {"guid": guid, "enable": enable}))

    @tool
    def component_set_enable(guid: str, component_guid: str, enable: bool) -> str:
        """Enable or disable one component."""
        return _dump(call("component.set_enable", {"guid": guid, "componentGuid": component_guid, "enable": enable}))

    @tool
    def gameobject_select(guid: str) -> str:
        """Show the GameObject in the Inspector (opens one if none is open) and target the transform gizmo at it."""
        return _dump(call("gameobject.select", {"guid": guid}))

    @tool
    def gameobject_destroy(guid: str) -> str:
        """Destroy a GameObject and its children (applied next frame; not saved to disk)."""
        return _dump(call("gameobject.destroy", {"guid": guid}))

    # -- プレイモード / 時間 / カメラ / ログ ----------------------------------------
    @tool
    def play() -> str:
        """Enter play mode (or resume after stop)."""
        return _dump(call("play"))

    @tool
    def stop() -> str:
        """Leave play mode but keep the play session state (the editor's Stop / Escape)."""
        return _dump(call("stop"))

    @tool
    def end_play() -> str:
        """End the play session: clears all scenes and reloads Assets/Scene/GameManage.scene (the editor's End button)."""
        return _dump(call("end"))

    @tool
    def time_set_scale(scale: float) -> str:
        """Set the global time scale (0 pauses simulation, 1 is normal)."""
        return _dump(call("time.set_scale", {"scale": scale}))

    @tool
    def camera_get() -> str:
        """Editor free-camera pose and the camera DxLib actually rendered with (Cinemachine while playing)."""
        return _dump(call("camera.get"))

    @tool
    def camera_set(position: list[float] | None = None, look_at: list[float] | None = None,
                   euler_degrees: list[float] | None = None, rotation: list[float] | None = None) -> str:
        """Move the editor camera (edit mode only). position/look_at are world [x,y,z]; rotation is [x,y,z,w]. Takes effect next frame."""
        return _dump(call("camera.set", {"position": position, "lookAt": look_at,
                                         "eulerDegrees": euler_degrees, "rotation": rotation}))

    @tool
    def debug_draw_get() -> str:
        """Current debug-draw settings (Config > DebugDraw): colliders (master switch for drawing every collider),
        shapes and layers filters, triggers (sensor colliders, drawn light blue), main/virtual camera frustums."""
        return _dump(call("debugdraw.get"))

    @tool
    def debug_draw_set(colliders: bool | None = None, shapes: bool | dict[str, bool] | None = None,
                       layers: bool | dict[str, bool] | None = None, triggers: bool | None = None,
                       main_camera_frustum: bool | None = None, virtual_camera_frustums: bool | None = None,
                       save: bool = False) -> str:
        """Change the editor's debug drawing, e.g. colliders=True to see every collider in screenshots.
        shapes/layers take true/false for all, or {"StaticMesh": true, "Enemy": false} per name (case-insensitive;
        shapes Box/Sphere/Capsule/Cylinder/StaticMesh - StaticMesh is heavy; layers as listed by debug_draw_get).
        shapes/layers/triggers only filter while colliders is on; the Inspector's selected object always draws its collider.
        Editor only, applies immediately (also while playing). save=True also writes ProjectConfig/DebugDraw (git-tracked)."""
        return _dump(call("debugdraw.set", {
            "colliders": colliders, "shapes": shapes, "layers": layers, "triggers": triggers,
            "mainCameraFrustum": main_camera_frustum, "virtualCameraFrustums": virtual_camera_frustums,
            "save": save or None}))

    @tool
    def log_tail(count: int = 50, min_level: Literal["info", "warning", "error"] = "info",
                 contains: str | None = None) -> str:
        """The newest engine log records (the Console window's history, at most 2000 kept), oldest first."""
        return _dump(call("log.tail", {"count": count, "minLevel": min_level, "contains": contains}))

    # -- アセット / ModelView / AnimationView -----------------------------------------
    @tool
    def assets_find(query: str = "", extension: str = ".mv1", limit: int = 50) -> str:
        """Find assets by a case-insensitive path substring. extension filters by suffix (".mv1", ".prefab", ".scene", ...; "" for all).
        Returns path ("/"-separated), guid and asset type."""
        return _dump(call("assets.find", {"query": query, "extension": extension, "limit": limit}))

    @tool
    def assets_reload(wait: bool = True, timeout_seconds: float = 180.0) -> str:
        """Re-scan Assets/ like Config > Application > Reload Assets, so assets and .meta files added or rewritten on disk
        are registered (scene_load/scene_reload alone only see what was read at startup). Loaded scenes keep the old
        asset instances, so call scene_reload afterwards. With wait, returns once the async loads have finished."""
        result = call("assets.reload", timeout=timeout_seconds)
        if wait:
            # エンジンは応答後のフレームで、次のコマンドを処理する前に新しいアセットの読み込みを始める
            state = wait_for("status", {}, lambda s: s.get("loadingResourceCount") == 0, timeout_seconds,
                             "the reloaded assets to finish loading")
            result["loadingResourceCount"] = state.get("loadingResourceCount")
        return _dump(result)

    @tool
    def model_view_open(path: str | None = None, guid: str | None = None, wait: bool = True,
                        timeout_seconds: float = 20.0) -> str:
        """Show a .mv1 in ModelView (switches the main window to ModelViewWindow). path may be the full asset path
        or a unique tail such as "SwordMan.mv1" (see assets_find). With wait, returns once the model has loaded,
        so screenshot(mode="game") right after shows it; adjust the view with preview_camera."""
        if not path and not guid:
            raise ToolError("pass path or guid")
        state = call("modelview.open", {"path": path, "guid": guid})
        if wait and not state.get("modelReady"):
            state = wait_for("modelview.state", {}, lambda s: s.get("modelReady"), timeout_seconds, "the model to load")
        return _dump(state)

    @tool
    def model_view_state() -> str:
        """Models open in ModelView (path, guid, selected), whether the selected one has loaded, and the preview camera."""
        return _dump(call("modelview.state"))

    @tool
    def model_view_select(guid: str | None = None, path: str | None = None) -> str:
        """Switch ModelView to another model that is already open."""
        return _dump(call("modelview.select", {"guid": guid, "path": path}))

    @tool
    def model_view_close(guid: str) -> str:
        """Close a model in ModelView."""
        return _dump(call("modelview.close", {"guid": guid}))

    @tool
    def animation_view_open(model_path: str | None = None, model_guid: str | None = None, wait: bool = True,
                            timeout_seconds: float = 20.0) -> str:
        """Put a model into AnimationView (switches the main window to AnimationViewWindow). Then list clips with
        animation_view_state(include_clips=True) and choose one with animation_view_set_clip."""
        if not model_path and not model_guid:
            raise ToolError("pass model_path or model_guid")
        state = call("animationview.open", {"modelPath": model_path, "modelGuid": model_guid})
        if wait and not state.get("modelReady"):
            state = wait_for("animationview.state", {}, lambda s: s.get("modelReady"), timeout_seconds, "the model to load")
        return _dump(state)

    @tool
    def animation_view_state(include_clips: bool = False) -> str:
        """AnimationView model, playback settings, root-motion lock and both clip slots (animation source, clip, time,
        range, speed, attached). include_clips lists every clip (index, name, length) of each slot's source."""
        return _dump(call("animationview.state", {"includeClips": include_clips}))

    @tool
    def animation_view_set(playing: bool | None = None, use_blend: bool | None = None, blend_weight: float | None = None,
                           name_check: bool | None = None, lock_root_motion: bool | None = None,
                           root_frame_index: int | None = None) -> str:
        """Playback settings. playing=False freezes the pose (pair with animation_view_set_clip(time=...)).
        blend_weight is slot B's share when use_blend. lock_root_motion keeps the model in place on XZ."""
        return _dump(call("animationview.set", {
            "playing": playing, "useBlend": use_blend, "blendWeight": blend_weight, "nameCheck": name_check,
            "lockRootMotion": lock_root_motion, "rootFrameIndex": root_frame_index}))

    @tool
    def animation_view_bones(name_contains: str | None = None) -> str:
        """World matrices of the AnimationView model's frames (bones) in the current pose: position and the
        axisX/axisY/axisZ rows, plus the model matrix. name_contains filters frames (case-insensitive substring).
        Freeze the pose first (animation_view_set(playing=False) + animation_view_set_clip(time=...))."""
        return _dump(call("animationview.bones", {"nameContains": name_contains}))

    @tool
    def animation_view_set_clip(slot: Literal["A", "B"] = "A", animation_path: str | None = None,
                                animation_guid: str | None = None, use_model_clips: bool = False,
                                clip_name: str | None = None, clip_index: int | None = None,
                                time: float | None = None, speed: float | None = None, loop: bool | None = None,
                                start: float | None = None, end: float | None = None,
                                wait: bool = True, timeout_seconds: float = 20.0) -> str:
        """Choose what a slot plays. animation_path/animation_guid take clips from another .mv1 (animation-only files);
        use_model_clips goes back to the model's own clips. clip_name matches case-insensitively (exact, else a unique
        substring). time is seconds into the clip; end 0 means the clip end. With wait, returns once the clip is attached."""
        state_args = {"includeClips": False}

        def slot_state(state: dict[str, Any]) -> dict[str, Any]:
            return (state.get("slots") or {}).get(slot, {})

        changes_source = bool(animation_path or animation_guid or use_model_clips)
        if changes_source:
            call("animationview.set_clip", {"slot": slot, "animationPath": animation_path,
                                            "animationGuid": animation_guid, "useModelClips": use_model_clips or None})

        if changes_source or clip_name is not None:
            if wait:
                wait_for("animationview.state", state_args,
                         lambda s: s.get("modelReady") and slot_state(s).get("sourceReady"),
                         timeout_seconds, f"slot {slot}'s clips to load")

        rest = {"clipName": clip_name, "clipIndex": clip_index, "speed": speed, "loop": loop,
                "start": start, "end": end, "time": time}
        state: dict[str, Any] | None = None
        if any(value is not None for value in rest.values()):
            state = call("animationview.set_clip", {"slot": slot, **rest})

        if wait:
            return _dump(wait_for("animationview.state", state_args, lambda s: slot_state(s).get("attached"),
                                  timeout_seconds, f"slot {slot}'s clip to attach"))
        return _dump(state if state is not None else call("animationview.state", state_args))

    @tool
    def preview_camera(frame: bool = False, yaw_degrees: float | None = None, pitch_degrees: float | None = None,
                       position: list[float] | None = None, look_at: list[float] | None = None) -> str:
        """Move the ModelView/AnimationView preview camera (the current main window must be one of them).
        yaw_degrees/pitch_degrees re-fit the whole model from that angle on the next frame: yaw 0 looks from +Z toward -Z,
        90 from -X, 180 from -Z, 270 from +X; pitch > 0 looks down (default 20 when only yaw is given).
        position + look_at place the camera exactly. frame=True re-fits keeping the current angle."""
        return _dump(call("preview.camera", {"frame": frame or None, "yawDegrees": yaw_degrees,
                                             "pitchDegrees": pitch_degrees, "position": position, "lookAt": look_at}))

    return mcp


def serve() -> int:
    logging.basicConfig(level=logging.INFO, stream=sys.stderr, format="%(asctime)s %(name)s %(levelname)s %(message)s")
    try:
        server = build_server()
    except ImportError as e:
        print(f'error: the MCP SDK is not installed ({e}). Run:  pip install "mcp>=2.2"', file=sys.stderr)
        return 1
    log.info("serving NanamiEngine AutoMCP tools over stdio (engine port %s)", EngineClient().port)
    server.run()
    return 0
