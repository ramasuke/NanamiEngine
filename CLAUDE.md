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
MSBuild.exe NanamiEngine.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m
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
python -m tools.effect install <efkefc> --dest Assets/Art/Effect/<Sub>/<Name>.efkefc
python tools/effect/selftest.py             # run after touching tools/effect/{model,xmlio,presets,meta}.py
```

`install` mints a fresh-GUID `.efkefc.meta` (`ParticleFile`, via `tools/common/meta_base.py` —
shared with `tools/bt`/`tools/scene`) and, with `--project`, commits the `.efkproj` source under
`Assets/Art/Effect/_Source/`. See **`tools/effect/README.md`** for the full command reference and
known scope limits (`Sprite`/`Ring`/`Ribbon`/`Model`/`Track` node kinds plus `SoundValues`/
`LocationAbsValues` are modeled; FCurve/keyframed variants and project-root camera/viewer
metadata are not; the CUI compile step is pinned to a specific local Effekseer 1.7.3.0 install
and is machine-specific).
