# NanamiEngine

A custom C++ game engine + game (DxLib / ImGui / Jolt / cereal), toolset v143, C++20. `EnviroHunter.sln` has two
hand-maintained projects with explicit file lists — **there is no globbing**, so a new `.cpp`/`.h` must be added by hand
(and, optionally, to the `.vcxproj.filters`):

- `NanamiEngine.vcxproj` — static lib `lib/<Editor|Game>/<Debug|Release>/NanamiEngine.lib`: `Engine/`, `Packages/`,
  `Libs/`, `Main.cpp` (WinMain lives in the lib).
- `EnviroHunter.vcxproj` — the game exe (`EnviroHunter.exe`): `Assets/**` sources only; links the lib with `/WHOLEARCHIVE` (static
  self-registration would otherwise be dropped by the linker).

Shared compiler/linker settings live in `NanamiEngine.props` / `NanamiEngine.Game.props`, not in the vcxproj files.
Game code includes engine headers root-relative (`#include "Engine/..."`, `"Packages/..."`, `"Libs/..."`).

## Building from the CLI

**Do not build the project on your own initiative.** Only run a build when the
user explicitly asks for one (e.g. "ビルドして" / "build this"). Finishing an
edit, fixing code, or the user saying things like "動作確認して" is not by
itself a request to build — ask first if it's unclear.

```
MSBuild.exe EnviroHunter.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m:12
MSBuild.exe EnviroHunter.sln -p:Configuration=Release -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m:12
```

The solution only has `x64` configurations. `-p:NanamiApplicationMode=Game` builds the game (non-editor) variant into
`x64/Game/<Configuration>/` (the lib into `lib/Game/...`); the editor's *Build Settings* window runs exactly that through
`GameBuilder` on the project's own `.sln` (Release by default, Debug selectable; MSBuild is found with vswhere unless a
path is set). Build Settings
stores product name + start scene + client version in `ProjectConfig/Build/Runtime/` (shipped with the game, read at startup) and the
editor-only MSBuild path / output dir / configuration in `ProjectConfig/Build/`. Per-file `<ClCompile>` blocks must not hardcode configuration-specific settings
(`RuntimeLibrary`, `Optimization`, `PreprocessorDefinitions`, `ObjectFileName` under `x64\Debug\`, …) — Visual
Studio writes them when you edit a single file's properties, and they then leak into every configuration.

`-p:PreferredToolArchitecture=x64` is **required** — the 32-bit compiler runs out
of heap on the deep cereal template instantiations (`error C1060`). MSBuild lives at
`C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe`.

## Window display mode

`Display::WindowDisplayModeController` (`Engine/Core/Application/Display/`) switches between Windowed / Borderless /
Fullscreen. The draw resolution (`AppConfiguration::GetWindowWidth/Height`, `SetGraphMode`) stays the same in every
mode; DxLib scales the window. So code that reads those sizes (or `GetMousePoint`) needs no changes, and ImGui's
`DisplaySize` is overridden to the graph size in `ImGuiWrapper`. Switching is deferred to after `ScreenFlip`
(`Request` / Alt+Enter). The player's choice goes in `LocalPrefs/Display/WindowMode.json`. If there isn't one, the
game uses `ProjectConfig/Application/DefaultWindowMode` (shipped, default Borderless) and the editor uses Windowed.
Both are edited in Config > Application.

## Engine packages & NanamiHub

`python -m tools.engine_dist build --version <v> [--zip]` builds the lib in all 4 configurations and assembles
`dist/NanamiEngine-<v>/` (headers, libs, props, `stdafx.*`, and the new-project template from
`tools/engine_dist/template/`); running it **is a build**, so the same go-ahead rule applies (`--skip-build` reuses
`lib/`). `python tools/engine_dist/selftest.py` checks the assembly without building. NanamiHub (C# WPF, .NET 9;
separate repo `ramasuke/NanamiHub`, cloned next to this one) is the launcher: it installs those packages to `%LOCALAPPDATA%\NanamiHub\Engines\<v>\`, creates projects
from the template (`EnginePath.props` points a project at its engine, `ProjectConfig/ProjectInfo.json` records the
version), builds the editor exe and starts it with `-project <dir>`. This repo is itself a project ("source" engine).

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

## cereal registration goes in the .cpp

`NANAMI_REGISTER_TYPE(T, Base)` / `NANAMI_REGISTER_POLYMORPHIC_RELATION(Base, T)` / `ENGINE_REGISTER_COMPONENT(T)` /
`REGISTER_ATTACK_AREA_TYPE` / `REGISTER_PLAYER_AVATAR_BASE` belong at the end of the type's `.cpp`
(global scope), which must `#include` `Engine/Module/Serialization/Engine_Module_SerializationRegistration.h`
so the type is bound to both archives (JSON + PortableBinary). Don't write `CEREAL_REGISTER_TYPE` /
`CEREAL_REGISTER_POLYMORPHIC_RELATION` directly: the `NANAMI_` macros (defined in that header) expand to them and also
record the type, its base and the registering module in `Serialization::SerializationTypeRegistry`, which a
game-code hot reload needs to find what to unregister (`docs/HotReload.md`). Those are the only archives polymorphic types are
bound to - don't serialise polymorphic pointers through `cereal::BinaryArchive` (use PortableBinary).
`REGISTER_ASSET` / `REGISTER_SCRIPTABLE_OBJECT` / `REGISTER_CREATABLE_ASSET_EXTENSION` go in the `.cpp` too: in a
header they define a `static` registrar per including file (the factory's vectors got one entry per file) and
instantiate the JSON `.meta` loader everywhere. The legacy `ENGINE_REGISTER_COMPONENT(T, V)` still compiles, but
only use it in a `.cpp` when `V` is 0. `CEREAL_CLASS_VERSION` (and the wrappers'
`ATTACK_AREA_CLASS_VERSION` / `PLAYER_AVATAR_BASE_CLASS_VERSION`) stays in the **header**: it must be
visible wherever the type is serialised. A registration in a header re-instantiates the type's serialisers
in every file that includes it — that was ~90% of the object code. Never change the spelling of a
registered type name (cereal stores the macro argument as `polymorphic_name` in every saved file).

## Engine code must not depend on game code

`Engine/` and `Packages/` must never `#include` anything under `Assets/` (they are compiled into the engine lib that
other projects link). Game code plugs in through registration instead:
Add Component menu entries via `AddComponent::RegisterMenu` (game menu: `Assets/Scripts/Editor/AddComponentMenu/`),
editor toolbar buttons via `REGISTER_EDITOR_TOOLBAR_WIDGET(Type, order)` at the end of the widget's `.cpp`, inside its namespace
(`Engine/Core/Application/Window/Toolbar/Widget/`; built-ins are ordered 100..600),
lock-on framing by implementing `CineMachine::ILockOnCameraTarget` (game side: `ILockOnTarget`). Physics layers other than `Default` are
per-project data (`ProjectConfig/Physics/LayerNames.json` + `LayerCollisionMasks.json`, edited in Config > Physics);
look them up with `Physics::PhysicsLayers::NameToLayer("Enemy")`, never add enum values. The exe takes `-project <dir>` (sets the
working directory), and the game exe is built with `-p:NanamiApplicationMode=Game` (defines `NANAMI_GAME_BUILD`).

## Engine headers must not expose DxLib

Headers under `Engine/`, `Packages/` and `Libs/LibCore/` must not `#include "DxLib.h"` / `EffekseerForDXLib.h` or use
DxLib types (`VECTOR`, `MATRIX`, `DX_*`, …) in their declarations - public APIs take glm types, `Color32`,
`LibCore::Dxlib::BlendMode` (DxLib-free; values `static_assert`ed in `DxMath.cpp`) and plain `int` handles. The only
exception is the bridge folder `Libs/LibCore/DxLib/` (`DxMath.h`: `ToDxVector`/`ToDxMatrix`/`FromDxMatrix`, `ShiftJis.h`),
which is included **only from `.cpp` files**. Converting at the call site looks like
`MV1SetMatrix(handle, LibCore::Dxlib::ToDxMatrix(Transform().GetWorldMatrix()))`.

## Reactive code goes through R4 (not rxcpp)

rxcpp is wrapped by **`Packages/R4`** (`NanamiEngine::R4`, R3-style): `R4::Subject<T>`, `R4::Observable<T>`,
`R4::ReactiveProperty<T>` / `ReadOnlyReactiveProperty<T>` / `SerializableReactiveProperty<T>`, `R4::Unit`,
`R4::Disposable` / `CompositeDisposable` / `SerialDisposable`, `R4::CancellationToken`. Only `Packages/R4/Core/`
(and `stdafx.h`'s precompiled `rx.hpp`) may name `rxcpp::` - include `Packages/R4/R4.h` instead. `Subscribe` returns a
`[[nodiscard]]` `Disposable`: in a Component write `.Subscribe(...).AddTo(this)` (released by
`DestroyCancellationToken()`, which `ComponentGroup::OnDestroy` cancels after `OnDestroy()`); elsewhere keep it in a
`Disposable` / `SerialDisposable` member and dispose it yourself. See **`Packages/R4/README.md`**.

## Debug menu (DebugSheet)

In-game debug features (cheats, save reset, scene jumps, …) are pages in **`Packages/DebugSheet`** (F1, editor + Debug
game build only; `NANAMI_DEBUG_SHEET_ENABLED`). The engine never calls it - `GameCore::Game::OnUserInterfaceRender`
drives it. Add a page with `REGISTER_DEBUG_SHEET_PAGE` in a game `.cpp` wrapped in `#if NANAMI_DEBUG_SHEET_ENABLED`
(`Assets/Scripts/GamePlay/Debug/DebugSheet/`), built from `DebugSheet::Widgets`. See **`Packages/DebugSheet/README.md`**.
**Don't change game code for debug features.** Use existing public APIs from the debug files (e.g. write the LocalPrefs
file and call `Reload()`). When a hook is unavoidable, put every added line - include, base class, member, definition,
call - inside `#if NANAMI_DEBUG_SHEET_ENABLED` (`Game`'s `OnUserInterfaceRender`, `RecordBook::Reload`).

## Multiplayer: relay server & room codes

Online play goes through the relay server **NanamiRelay** (separate repo `ramasuke/EnviroHunter-Server`, cloned next
to this one; VPS `160.16.76.201:1234`). Host and clients all connect *out* to it, so no port forwarding; the game
logic still runs on the host. The wire format is one header kept **identical in both repos** -
`Assets/Scripts/GamePlay/Network/Relay/RelayProtocol.h` and `protocol/RelayProtocol.h` - bump `PROTOCOL_VERSION`
(server accepts `MIN_PROTOCOL_VERSION`..`PROTOCOL_VERSION`) whenever the bytes change, and re-run the server's
`relay_selftest` (`build_windows.bat`, then `relay_selftest.exe --port 34567` against a local `nanami-relay.exe`).

A stage is entered through `Game::Instance().Matchmaker().JoinOrHostAsync` (`StageMatchmaker`) with the room the player picked in stage select
(`Matchmaker().SetNextRoom`, used once, then back to a public room):

- **Public** (`RelayRoom::Mode::Public`) - share a room keyed by `(appId, stage)`; falls back to LAN discovery,
  then to hosting alone.
- **Create** - the relay mints a 6-digit code and answers `RoomCreated`; `CustomNetworkRunner::RelayRoomCode()`
  then has it, and `RoomCodeHud` shows it in the corner of the stage (`OtherPlayerStatusUiScene.scene`).
- **Join** - the code's room only. No LAN fallback; a refusal (`RoomNotFound` / `RoomFull` / `SessionMismatch`)
  comes back as the loading screen's failure message.

Relay connection settings (address / port / `appId`) are LocalPrefs, not shipped data: toolbar > LocalPrefs >
RelayServer (`LocalPrefs/Network/RelayServer.json`).

## In-game UI design

Before designing or building any in-game UI screen (sprites, prefab, View/Presenter), read
**`docs/UIDesign.md`**: the two visual families (tangible tavern props for screens vs. the teal HUD), the
palette / fonts / hint-tag / wording / motion conventions taken from the existing prefabs, the texture helpers
in `tools/art/character_select.py`, and the mock-on-a-real-screen -> `--emit` -> `*_prefab.py` workflow.
Show the user 2-3 composited mocks before implementing a new screen.

## Story & island restoration

Before writing dialogue, quests, facilities or story events, read **`docs/Story.md`** (the source of truth: canon
vs. 【未設計】 parts - don't invent the desert/rocky stories, propose them). Progress lives in
`GameCore::Story::StoryProgress` (`StoryFlag` / `Facility`, local-only, never synced in multiplayer); append new enum
values at the end. `GamePlay::Prop::RestorationGate` swaps a broken/restored child GameObject by `Facility`.
NPC dialogue and the story NPCs' BTs are generated by `python tools/art/story_npcs.py` (its `CHATS` is the source of
truth for the lines) - edit the script and re-run it instead of hand-editing `.npcChat.meta` / the trees.

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
python -m tools.model convert <in.fbx> <out.mv1> --mode mesh|anim|full [--with-textures] [--emissive [MATERIAL=]R,G,B ...] [--modelviewer-path <exe>]
python -m tools.model install <out.mv1> --dest Assets/Art/.../<Name>.mv1 [--with-textures] [--source <in.fbx>] [--textures <dir>]
python -m tools.model materials <mv1>                                   # material names + diffuse/emissive
python -m tools.model set-emissive <mv1> --emissive [MATERIAL=]R,G,B ... [--out <path>]
python -m tools.model set-culling <mv1> --mode none|left|right [--out <path>]      # none = double-sided meshes
python tools/model/selftest.py              # .meta/.mv1-codec gate; GUI-automation stage is best-effort/skips cleanly
```

`--emissive` is DxLibModelViewer's 自己発光 (emissive color): the toolkit patches the saved `.mv1`'s
material table and re-compresses it (`mv1.py`), rather than driving the viewer's material panel.

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

## Asset distribution (manifest + Cloudflare R2)

To cut a release of the runtime assets, use the toolkit instead of listing or uploading files by hand:

```
python -m tools.dist build --version <v> [--base-url <url>] [--out <file>]
python -m tools.dist upload [manifest.json] [--dry-run] [--no-release]   # blobs -> manifest-<v>.json -> manifest.json
python -m tools.dist show <manifest.json>              # summary + largest entries
python -m tools.dist diff <installed.json> <manifest.json>   # what clients would download
python tools/dist/selftest.py             # run after touching tools/dist/{manifest,upload,config,refs}.py
```

The editor toolbar's **Asset Dist** button (`Packages/AssetUpdater/Editor/`, shown only when `tools/dist/` exists) runs the
same `build` / `upload --dry-run` / `upload` (Release needs a successful build of that version first, then a confirm);
its Version / Python fields are stored in `ProjectConfig/Build/AssetDistribution/`.

`manifest.json` lists every deliverable file under `Assets/` (2026-09-18: **1,812 entries / 1.76 GB**, 3,146
unique blobs) and is what `Packages/AssetUpdater/` fetches at runtime. It is hosted on **Cloudflare R2**
(bucket `nanami-assets`, uploaded through the rclone remote `r2`; the target and public URL live in
`tools/dist/dist_config.json`, the keys in `%APPDATA%\rclone\rclone.conf` outside the repo). Server layout:
`files/<sha256>` (immutable blobs, bodies and `.meta` alike) + `manifest-<v>.json` (immutable) +
`manifest.json` (the only mutable object - swapping it *is* the release; rollback = copy an older
`manifest-<v>.json` over it). Clients fetch `baseUrl + <hash>`, so download URLs are always ASCII; an entry's
`path` is only where the file goes locally (the `Assets/` tree, because `contentPath_` and the `.mv1`/`.efkefc`
relative references depend on it). `upload` sends only hashes missing on the remote, re-hashes the actual
bytes before sending, and refuses to reuse a version number with different content. The public URL is still
the development-only `r2.dev` one: before shipping to players, replace it with a custom domain in
`dist_config.json` **and** in the client's `MANIFEST_URL` (compiled into the exe, so that needs a new build).

An entry covers a body file **and its `.meta`** (`guid`/`metaHash`/`metaSize`), because `.meta` is where the
guid and `contentPath_` live and the two must never drift apart.

**"No `.meta` means not shipped" is wrong** - 82 files are *companions* referenced by relative path
from inside another file (`.efkmodel` and textures under `.efkefc`, `<name>.fbm/*.png` under `.mv1`,
`Tree_VS.vso`/`Tree_PS.pso`, `.mat`/`.mtl`). They ship with an empty `guid`/`metaHash`, identified by
path. Exclusion is a denylist of dev-only things (`Assets/Scripts/`, **any `_Source/` directory at any
depth**, `*.fbx`, `*.blend`, `*.blend1`, `*.efkproj`, `*.h`, `*.cpp`, `*.bak`, `desktop.ini`) - see
`tools/dist/manifest.py`. `.blend` files embed the author's Windows user name and full paths (one used to
slip through); after loosening any rule, re-scan the shipped set for them (UTF-8/UTF-16LE/CP932, `.mv1`
decoded). `ProjectConfig/` and `LocalPrefs/` are intentionally **not** distributed.

`build` **refuses** (exit 1, no manifest written) when a shipped `.efkefc`/`.mv1` references a file that exists
on this PC but won't ship (outside `Assets/`, or excluded) - it would render here and be missing on players' PCs
(`tools/dist/refs.py`; 2026-09-18: 8 effects pointed at `Desktop\Effekseer素材`, recompiled to use the identical
copies next to them). Fix the reference; don't loosen the check. References that exist nowhere are not reported.

`upload` **refuses** a release that changes or removes an existing font (`.ttf`/`.otf`/`.ttc`) relative to the live
`manifest.json` unless `requiredClientVersion` is raised above the live one: the client applies updates on the title
screen while the game runs, and `TtfFontFile` keeps fonts registered via `AddFontResourceEx` until exit, so every
player's apply would fail. Ship font changes in a new zip. `requiredClientVersion` defaults to Build Settings' *Client Version* (the game's own version, compared as dot-separated numbers). The client side (`Packages/AssetUpdater`: check ->
confirm -> download to `.update/` -> transactional apply into `Assets/` -> relaunch) only ever runs when
`APPLICATION_MODE == Game` **and** `installed.json` exists (`GameBuilder` writes it next to the exported exe - a
hash list of the exported `Assets/` - unless Build Settings > *Asset Updates* is off); never let it run from the editor, where it would
overwrite the working `Assets/` with the published set and delete files that were never uploaded.

Hashing is cached by mtime+size, and the `.efkefc`/`.mv1` reference lists by content hash, in
`<repo>/.manifest_hash_cache.json` (gitignored, ~22s cold / ~4s warm).
`manifest.json` itself is a release artifact and is gitignored. Standing caveats: **74 asset paths are
non-ASCII** (harmless for download URLs, but the client must convert the UTF-8 `path` to UTF-16 before
touching the filesystem), **71 `.meta` are still CP932** (pre-`/execution-charset:utf-8`; `read_guid` falls
back to CP932), Python's default `urllib` User-Agent gets **403 from r2.dev**, and rclone's `--immutable` is
**ignored by `copyto`** (so `upload.py` checks versioned manifests itself). See **`tools/dist/README.md`**.

## AutoMCP (Claude Code <-> running editor)

**Do not use the `nanami` MCP tools on your own initiative.** Only drive the editor (screenshot,
play, component edits, `engine_launch`, …) when the user explicitly asks for it (e.g. "AutoMCPで確認して"
/ "スクショ撮って"). Finishing an implementation or fixing code is not by itself a request to test it in
the editor — report what changed and stop; ask first if it's unclear.

The project MCP server `nanami` (`.mcp.json` -> `python -m tools.automcp serve`, needs
`pip install "mcp>=2.2"`) lets you drive the **running editor**: `screenshot` (mode `full` with ImGui,
`game` = 3D only) to check real rendering, `windows_list`/`window_open`/`window_set`, `hierarchy`/
`gameobject_find`/`gameobject_get`, `component_get`/`component_set_params`, `gameobject_set_transform`,
`play`/`stop`/`end_play`, `camera_set`, `debug_draw_set` (collider/frustum drawing), `log_tail`, and the asset viewers (`assets_find` -> `model_view_open` / `animation_view_open` + `animation_view_set_clip`, `preview_camera` for the angle), etc. It only answers while the editor runs with
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
and `scene_reload`. The editor scans `Assets/` only at startup, so after adding or rewriting assets / `.meta`
files on disk call `assets_reload` (= *Config > Application > Reload Assets*) before `scene_reload`.
See **`tools/automcp/README.md`** for the tool/command table and limits; keep it,
`server.py` and `AutoMcpCommands.cpp` in sync when adding a command.
