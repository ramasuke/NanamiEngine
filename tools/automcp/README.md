# `tools/automcp` — Claude Code ⇄ running NanamiEngine editor (MCP)

Lets Claude Code look at and drive the running editor: screenshots of the real rendering,
ImGui window operations, scene/GameObject/component inspection and edits, play mode, camera,
log. Two halves:

```
Claude Code ──stdio (MCP)──> python -m tools.automcp serve ──TCP 127.0.0.1:47321 (JSON lines)──> EnviroHunter.exe
                              (.mcp.json, this package)                                          (Config > AutoMCP)
```

- **Engine** (`Engine/Core/Application/AutoMcp/`): `AutoMcpServer` listens on `127.0.0.1:<port>`
  while *Config > AutoMCP > Enable AutoMCP* is checked (saved in `ProjectConfig/AutoMcp/*.json`),
  polled once per frame from `EditorApplication::OnFrame` — no threads. Editor builds only.
- **Bridge** (this package): a stdio MCP server that Claude Code starts from `.mcp.json` for the
  whole session. It connects to the engine lazily per tool call, so the editor can be started,
  restarted or toggled at any time.

## Prerequisites

```
pip install "mcp>=2.2"          # only `serve` needs it; everything else is stdlib-only
```

Then open the repo in Claude Code, approve the project MCP server `nanami` (`/mcp`), start the
editor and check *Config > AutoMCP > Enable AutoMCP*. If you change the port there, set
`NANAMI_AUTOMCP_PORT` for Claude Code too.

## Commands

| Command | Purpose |
|---|---|
| `python -m tools.automcp serve` | MCP stdio server (what `.mcp.json` runs) |
| `python -m tools.automcp call <cmd> [--args JSON]` | send one raw engine command and print the result (debugging) |
| `python -m tools.automcp check-env` | Python / `mcp` version, exe, `.mcp.json`, engine reachability |
| `python tools/automcp/selftest.py` | correctness gate against a fake engine (no editor needed) |

## MCP tools → engine commands

| Tool | Engine `cmd` | Notes |
|---|---|---|
| `engine_status` | `status` | play state, fps, screen, main window/scene, working directory |
| `engine_launch` | – | starts `x64/Debug/EnviroHunter.exe` with cwd = repo root (`NANAMI_ENGINE_EXE` / `NANAMI_ENGINE_CWD` override) and waits for AutoMCP |
| `screenshot` | `screenshot` | `mode` `full` (with ImGui) / `game` (3D only); returns an image. Saved under `<cwd>/AutoMcp/Screenshots/` (last 20 kept) |
| `windows_list`, `window_open`, `window_close`, `window_set`, `main_window_switch` | `windows.*`, `mainwindow.switch` | `window_set` takes the exact ImGui name incl. `##id` |
| `scene_list`, `scene_load`, `scene_reload` | `scene.*` | |
| `hierarchy`, `gameobject_find`, `gameobject_get` | `hierarchy`, `gameobject.find`, `gameobject.get` | |
| `gameobject_get_json`, `gameobject_set_json` | `gameobject.get_json` / `set_json` | cereal JSON, same format as `.scene` |
| `component_get`, `component_set_params` | `get_json` → `patch.py` → `set_json` | |
| `gameobject_set_transform`, `gameobject_set_enable`, `component_set_enable` | `gameobject.*`, `component.set_enable` | direct setters, fine while playing |
| `gameobject_select`, `gameobject_destroy` | `gameobject.select` / `destroy` | |
| `play`, `stop`, `end_play`, `time_set_scale` | `play`, `stop`, `end`, `time.set_scale` | |
| `camera_get`, `camera_set` | `camera.get` / `set` | `camera_set` is edit-mode only |
| `debug_draw_get`, `debug_draw_set` | `debugdraw.get` / `set` | *Config > DebugDraw*: `colliders` master switch, `shapes`/`layers` (true/false for all or `{"Name": bool}`), `triggers`, main/virtual camera frustums. Memory only unless `save` (writes the git-tracked `ProjectConfig/DebugDraw/`) |
| `log_tail` | `log.tail` | Console history (2000 records kept) |
| `assets_find` | `assets.find` | asset paths (`/`-separated), guids and types; filter by substring and extension |
| `assets_reload` | `assets.reload` | same as *Config > Application > Reload Assets*: re-scans `Assets/` so assets / `.meta` added or rewritten on disk are registered; waits until `loadingResourceCount` is 0 (polls `status`). Loaded scenes keep the old asset instances, so `scene_reload` afterwards |
| `model_view_open`, `model_view_state`, `model_view_select`, `model_view_close` | `modelview.*` | show a `.mv1` in ModelView by path (full or unique tail like `SwordMan.mv1`) or guid; `open` waits until loaded |
| `animation_view_open`, `animation_view_state`, `animation_view_set`, `animation_view_set_clip` | `animationview.*` | model + clip slots A/B (source `.mv1`, clip by name/index, time, speed, loop, range), play/pause, blend, root-motion lock; `set_clip` waits for the source to load and the clip to attach |
| `animation_view_bones` | `animationview.bones` | world matrix (position + axis rows) of every frame/bone in the current pose, optional name filter, plus the model matrix |
| `preview_camera` | `preview.camera` | ModelView/AnimationView camera: re-fit from yaw/pitch, or exact position + look-at |

Wire format: request `{"id":1,"cmd":"status","args":{}}\n`, response
`{"id":1,"ok":true,"result":{...}}\n` or `{"id":1,"ok":false,"error":"..."}\n`.
`windows.set` runs at the start of the next frame; `screenshot` answers with the next frame's image;
everything else runs at the end of the frame it arrives in.

## Known limitations (v1)

- **Parameter edits rebuild the GameObject.** There is no runtime reflection, so
  `component_set_params` / `gameobject_set_json` round-trip the whole GameObject through cereal JSON
  and replace it (same guids; components are destroyed and re-initialised, Awake/Start run again in
  play mode). Prefer edit mode for them; transform/enable/time/camera edits are direct.
- **Nothing is saved.** Persist with `tools.scene` on the `.scene` file, then `scene_reload`
  (or the editor's Save button).
- `gameobject_set_enable` doesn't update the GameObject's serialized `isActive_` (engine behaviour);
  read component `enabled` flags instead.
- `camera_set` only moves the editor free camera; while playing Cinemachine drives the camera.
- Only objects in the GameWindow's loaded scenes are reachable (not PrefabView contents).
- Game scenes remove themselves from the GameWindow when they end (e.g. `FirstTouchDownMainIsLandScene`), and End /
  `end_play` reloads `GameManage.scene`, whose scene name is `Scene`; a hierarchy showing only `Scene` means that happened.
- Engine internals are reached only through `AutoMcpEngineAccess` (Attorney, friend of the engine classes); don't add public
  API to engine classes for new commands.
- The listener accepts any local process on `127.0.0.1`; keep AutoMCP off when you don't need it.

## After changing the protocol

Keep `AutoMcpCommands.cpp` / `AutoMcpServer.cpp`, `server.py` and this table in sync, then run
`python tools/automcp/selftest.py` (update `EXPECTED_TOOLS` there when adding or renaming a tool).
