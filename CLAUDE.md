# NanamiEngine

A custom C++ game engine + game (DxLib / ImGui / Jolt / cereal). Single Visual
Studio project `NanamiEngine.vcxproj` (toolset v143, C++20), hand-maintained with
explicit file lists — **there is no globbing**, so a new `.cpp`/`.h` must be added
to the `.vcxproj` by hand (and, optionally, `.vcxproj.filters`).

## Building from the CLI

```
MSBuild.exe NanamiEngine.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m
```

`-p:PreferredToolArchitecture=x64` is **required** — the 32-bit compiler runs out
of heap on the deep cereal template instantiations (`error C1060`). MSBuild lives at
`C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe`.

## Source encoding

`.h` / `.cpp` files are **Shift-JIS (CP932)**, not UTF-8. A Claude Code `PostToolUse`
hook (`.claude/scripts/convert_to_shiftjis.ps1`) converts files written via Write/Edit
back to CP932 automatically. Tools that write source files another way must emit
CP932 themselves; `Scripts/convDx.ps1 <path>` converts on demand.

## Enemy behaviour trees & actions

To create or edit an enemy behaviour tree (`Assets/Data/EnemyBehaviour/*.enemyBehaviourData`)
or add a new behaviour action, use the toolkit instead of hand-editing the cereal JSON:

```
python -m tools.bt new-tree <Name>          # new tree + .meta
python -m tools.bt show|validate <file>
python -m tools.bt add-node|set-params|apply <file> ...
python -m tools.bt add-action --name <X> --category "<Cat>" [--param n:type=default ...]
python tools/bt/selftest.py                 # run after touching tools/bt/{cereal_json,reader,writer,model}.py
```

Full reference and the file-format notes: **`docs/BehaviourTree.md`**.
If you add or rename an action, run `python -m tools.bt regen-catalog` and commit
`tools/bt/catalog.json`.
