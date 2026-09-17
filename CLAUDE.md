# NanamiEngine

A custom C++ game engine + game (DxLib / ImGui / Jolt / cereal). Single Visual
Studio project `NanamiEngine.vcxproj` (toolset v143, C++20), hand-maintained with
explicit file lists — **there is no globbing**, so a new `.cpp`/`.h` must be added
to the `.vcxproj` by hand (and, optionally, `.vcxproj.filters`).

## Building from the CLI

**Do not build the project on your own initiative.** Only run a build when the
user explicitly asks for one (e.g. "ビルドして" / "build this"). Finishing an
edit, fixing code, or the user saying things like "動作確認して" is not by
itself a request to build — ask first if it's unclear.

```
MSBuild.exe NanamiEngine.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m:12
```

`-p:PreferredToolArchitecture=x64` is **required** — the 32-bit compiler runs out
of heap on the deep cereal template instantiations (`error C1060`). MSBuild lives at
`C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe`.

## Source encoding

`.h` / `.cpp` files are **UTF-8 with BOM** — write/edit them normally, no special handling
needed. MSVC (v143 toolset) reads UTF-8-with-BOM source natively. If you ever encounter a
`.h`/`.cpp` that decodes cleanly as Shift-JIS (CP932) but not as UTF-8, that's a leftover from
files a since-removed PostToolUse hook force-converted; re-save it as UTF-8 with BOM to match
the rest of the codebase rather than leaving it as the odd one out.

Narrow string literals are compiled as **UTF-8** (`/execution-charset:utf-8`), which is what ImGui,
`Module::Log` and `TextRenderer::text_` expect. DxLib's string parameters are Shift-JIS, so pass
through `LibCore::Dxlib::Utf8ToShiftJis`. The flag lives in every `<AdditionalOptions>` of the
`.vcxproj`, including per-file ones that don't inherit `%(AdditionalOptions)` — keep it when adding one.

## Behaviour trees & actions (Enemy + FriendlyNpc)

To create or edit a behaviour tree — enemy (`Assets/Data/EnemyBehaviour/*.enemyBehaviourData`)
or friendly NPC (`Assets/Data/FriendlyNpcBehviour/*.friendBehaviourData`) — or add a new
behaviour action, use the toolkit instead of hand-editing the cereal JSON:

```
python -m tools.bt new-tree <Name> [--npc-kind enemy|friendly]     # new tree + .meta (default: enemy)
python -m tools.bt show|validate <file>                        # flavor auto-detected from the file
python -m tools.bt add-node|set-params|apply <file> ...
python -m tools.bt add-action --name <X> --category "<Cat>" [--npc-kind friendly] [--param n:type=default ...]
python tools/bt/selftest.py                 # run after touching tools/bt/{cereal_json,reader,writer,model}.py
```

Full reference and the file-format notes: **`docs/BehaviourTree.md`**.
If you add or rename an action, run `python -m tools.bt regen-catalog [--npc-kind friendly]`
and commit `tools/bt/catalog.json` / `catalog_friendly.json`.

## Scenes, GameObjects, Prefabs & Components

To create or edit a `.scene`/`.prefab` (GameObjects + Components), use the
toolkit instead of hand-editing the cereal JSON:

```
python -m tools.scene new-scene|new-prefab <Name> [--dir]     # new file + .meta
python -m tools.scene copy-prefab <SourcePrefab> [--name] [--dir]  # duplicate + fresh guids
python -m tools.scene show|validate <file>
python -m tools.scene add-gameobject|set-transform|add-component|apply <file> ...
python -m tools.scene instantiate-prefab <prefab> --into <file> [--parent]
python tools/scene/selftest.py            # run after touching tools/scene/{reader,writer,model}.py
                                           # or anything under tools/common/ (shared with tools/bt)
```

`tools/scene` shares its cereal-JSON codec, `.meta` codec, tagged-blob representation, and vcxproj
editor with `tools/bt` (see `tools/common/`) — see **`tools/scene/README.md`** for the full command
reference and known v1 limitations (e.g. `add-component` won't construct a brand-new instance of a
component with an intermediate C++ base it doesn't model, like any Collider).
If you add/rename a Component, run `python -m tools.scene regen-catalog` and commit
`tools/scene/catalog.json`.

## AnimationTrees

To create or edit an AnimationTree (`Assets/Animations/*.animTree`), use the
toolkit instead of hand-editing the cereal JSON:

```
python -m tools.animtree new-tree <Name>              # new tree + .meta
python -m tools.animtree show|validate <file>
python -m tools.animtree add-clip-node|add-transition|add-condition|apply <file> ...
python tools/animtree/selftest.py           # run after touching tools/animtree/{reader,writer,model}.py
                                             # or anything under tools/common/ (shared with tools/bt, tools/scene)
```

`tools/animtree` shares its cereal-JSON codec, `.meta` codec, tagged-blob representation, and vcxproj
editor with `tools/bt`/`tools/scene` (see `tools/common/`) — see **`docs/AnimationTree.md`** for the
full command reference and known v1 limitations (no cross-node auto-layout, no `add-node-type`
scaffold — adding a new `IAnimationNode` subclass is by-hand, see that doc's §4). Bind a tree to a
GameObject via `tools/scene add-component --type Animator --param animationTreeFile_=<guid>`.
If you add a new `IAnimationNode` subclass, run `python -m tools.animtree regen-catalog` and commit
`tools/animtree/catalog.json`.

## Particle effects (Effekseer)

To author a new particle effect, use the toolkit instead of hand-writing Effekseer's
`.efkproj` XML or opening the Effekseer GUI editor:

```
python -m tools.effect new-project <Name>                  # new .efkproj skeleton
python -m tools.effect show|validate <file>
python -m tools.effect add-node|set-params|apply <file> ...
python -m tools.effect compile <file>                       # .efkproj -> .efkefc via the Effekseer CUI
python -m tools.effect upgrade <file> --effekseer-version <v>  # rewrite in that editor's native format (via its CUI)
python -m tools.effect install <efkefc> --dest Assets/Art/Effect/<Sub>/<Name>.efkefc
python -m tools.effect check-env                            # show resolved effect_config.json, find the CUI
python -m tools.effect export --out <EffekseerEfkprojTool clone>  # sync the public standalone repo
python tools/effect/selftest.py             # run after touching tools/effect/{model,xmlio,presets,enums,meta,config,export,versions,efkefc}.py
```

Every Effekseer release from **1.50RC1 to 1.80.7** (families 1.5x/1.6x/1.7x/1.80.x, all 33 Windows tools
tested) is supported: `effekseer.version` (or `--effekseer-version` / `$EFFEKSEER_VERSION`) picks the target —
its CUI from `effekseer.cui_paths`, its family's enum domains and binary version. NanamiEngine's runtime
(EffekseerForDXLib) is **1.7**, so the config targets 1.7.3.0 and sets `project.runtime_version` `1.7`: a
1.80-compiled `.efkefc` (binary 1810) won't load in-game and `install` refuses it. `new-project` always writes
`ToolVersion` `0.7CTP1` because presets build pre-migration shapes that editors only migrate for old files;
fields on the wrong side of a file's `ToolVersion` (`versions.MIGRATIONS`) are silently dropped by Effekseer, so
every write/validate/compile refuses them. 1.80-only `CommonValues` settings need `upgrade --effekseer-version
1.80.x` first (the CUI migrates the file). See README "Effekseer versions".

`install` mints a fresh-GUID `.efkefc.meta` (`ParticleFile`, via `tools/common/meta_base.py` —
shared with `tools/bt`/`tools/scene`) and, with `--project`, commits the `.efkproj` source under
`Assets/Art/Effect/_Source/`. See **`tools/effect/README.md`** for the full command reference and
known scope limits (`Sprite`/`Ring`/`Ribbon`/`Model`/`Track` node kinds plus `SoundValues`/
`LocationAbsValues` are modeled; FCurve/keyframed variants and project-root camera/viewer
metadata are not; the CUI compile step is machine-specific).
Effekseer stores texture/model paths relative to the file, and the CUI compiles broken ones silently, so
`compile` refuses missing references and `--out` in another folder, and `install` copies referenced
files next to `--dest` (rebasing the `--project` copy) and refuses `../` references — don't bypass
these by copying `.efkefc`/`.efkproj` files by hand. Saving from the Effekseer GUI turns an `.efkproj`
into `.efkefc` format (`_Source/tktk01/DragonFireBall.efkproj` is like that), which the toolkit can't read.
Machine-/project-specific values (target version, CUI paths, runtime version, install dir, `.meta` on/off, selftest corpus) live in
`tools/effect/effect_config.json` — don't hardcode paths in the code. The toolkit is also published
as the public repo `ramasuke/EffekseerEfkprojTool`: NanamiEngine is the source of truth, `export`
copies `export.MANIFEST` (with the neutral `tools/effect/dist/effect_config.json`) into a clone of
it; add any new runtime file to that manifest. User docs for the public repo (README + setup/usage/versions/troubleshooting/development) live in `tools/effect/dist/` — update them with any user-visible change.

## Model conversion (DxLib ModelViewer)

To convert a `.fbx` source model into DxLib's `.mv1` format, use the toolkit instead of
manually opening `DxLibModelViewer_64bit.exe` — DxLib has no documented CLI/CUI for this
conversion, so the toolkit drives the real GUI tool via `pywinauto`:

```
python -m tools.model convert <in.fbx> <out.mv1> --mode mesh|anim|full [--with-textures] [--modelviewer-path <exe>]
python -m tools.model install <out.mv1> --dest Assets/Art/.../<Name>.mv1 [--with-textures] [--source <in.fbx>] [--textures <dir>]
python tools/model/selftest.py              # .meta/.mv1-codec gate; GUI-automation stage is best-effort/skips cleanly
```

`--mode` picks the DxLibModelViewer save command: `mesh` = model only (animations dropped),
`anim` = animation clips only (no mesh), `full` = model + animations in one file.
`--with-textures` (not with `anim`) decompresses the output `.mv1` (`tools/model/mv1.py` — DxLib's
DXArchive LZ, verified against all 132 `.mv1` under `Assets/`), reads the texture paths it stores
**relative to the `.mv1`** (e.g. `Hyena.fbm\Hyenas_A4_Diffuse.png`; normal/roughness maps survive
too), and copies each one from next to the input / its `*.fbm` folder to that same relative path
beside the output, failing with a list if any can't be found. `install --with-textures` does the
same for an installed asset. `install` mints a fresh-GUID `.mv1.meta` (`Mv1File`, via
`tools/common/meta_base.py`) the same way `tools/effect install` does for `ParticleFile`;
`--textures <dir>` is the older unfiltered bulk copy into `<dest-dir>/textures/`. `convert`
requires `pip install pywinauto` (the first third-party dependency any `tools/*` toolkit in this
repo has needed) and a local copy of `DxLibModelViewer_64bit.exe` (not vendored in this repo;
pinned path in `tools/model/cli.py`'s `DEFAULT_MODELVIEWER_PATH`; all three modes verified
2026-09-13 against ver3.24d with a real textured+animated `.fbx`). See **`tools/model/README.md`**
for prerequisites and known fragility — this is a reverse-engineered UI-automation wrapper, not an
officially supported CLI.

## AutoMCP (Claude Code <-> running editor)

**Do not use the `nanami` MCP tools on your own initiative.** Only drive the editor (screenshot,
play, component edits, `engine_launch`, …) when the user explicitly asks for it (e.g. "AutoMCPで確認して"
/ "スクショ撮って"). Finishing an implementation or fixing code is not by itself a request to test it in
the editor — report what changed and stop; ask first if it's unclear.

The project MCP server `nanami` (`.mcp.json` -> `python -m tools.automcp serve`, needs
`pip install "mcp>=2.2"`) lets you drive the **running editor**: `screenshot` (mode `full` with ImGui,
`game` = 3D only) to check real rendering, `windows_list`/`window_open`/`window_set`, `hierarchy`/
`gameobject_find`/`gameobject_get`, `component_get`/`component_set_params`, `gameobject_set_transform`,
`play`/`stop`/`end_play`, `camera_set`, `log_tail`, and the asset viewers (`assets_find` -> `model_view_open` / `animation_view_open` + `animation_view_set_clip`, `preview_camera` for the angle), etc. It only answers while the editor runs with
*Config > AutoMCP > Enable AutoMCP* checked (engine side: `Engine/Core/Application/AutoMcp/`, polled
from `EditorApplication::OnFrame`, 127.0.0.1:47321 by default, `NANAMI_AUTOMCP_PORT` to change).

```
python -m tools.automcp check-env                         # SDK / exe / engine reachability
python -m tools.automcp call status [--args '{...}']       # raw engine command, for debugging
python tools/automcp/selftest.py                          # run after touching tools/automcp/* (fake engine, no editor)
```

`engine_launch` starts the already-built exe but never builds it (building still needs the user's
go-ahead). `component_set_params`/`gameobject_set_json` rebuild the whole GameObject from cereal JSON
(components re-initialise), so prefer edit mode; nothing is saved to disk - persist with `tools.scene`
and `scene_reload`. See **`tools/automcp/README.md`** for the tool/command table and limits; keep it,
`server.py` and `AutoMcpCommands.cpp` in sync when adding a command.
