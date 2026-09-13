# `tools/model` — DxLib ModelViewer `.fbx` → `.mv1` conversion toolkit

Converts `.fbx` (or any other format `DxLibModelViewer` can load) into
NanamiEngine's `Mv1File` asset (`.mv1`, DxLib's own model format), and
installs the result as an `Assets/Art/**/*.mv1` asset — **without** manually
opening DxLib's `DxLibModelViewer_64bit.exe` GUI tool.

**Unlike `tools/effect` (which wraps Effekseer's real, documented CUI),
`DxLibModelViewer` has no CLI/CUI mode at all.** `convert` works by driving
its GUI directly via [`pywinauto`](https://pywinauto.readthedocs.io/) —
clicking through File→Open / File→Save As the same way a person would. This
is a reverse-engineered automation, not an officially supported integration.
Read "Known limitations" below before relying on it for anything that
matters.

Run from the repo root:

```
python -m tools.model <command>        # or: python tools/model.py <command>
```

## Prerequisites

- **Windows only**, and a real (non-headless) interactive desktop session —
  `convert` genuinely launches `DxLibModelViewer_64bit.exe` and drives its
  window; there is no way to run it invisibly. Depending on whether the app
  exposes a proper UI Automation tree (unconfirmed — see below), the run may
  or may not need to take input focus. Don't do other GUI work while it runs.
- `pip install pywinauto` — **only needed for `convert`** (`install` and
  `selftest` are pure stdlib and work without it). This is the **first
  third-party Python dependency any `tools/*` toolkit in this repo has
  needed**. `pywinauto` pulls in `pywin32`/`comtypes` automatically.
- Your own local copy of `DxLibModelViewer_64bit.exe` (ships in the official
  DxLib SDK download's `Tool\DxLibModelViewer\` folder — **not** vendored in
  this repo, which only carries DxLib's headers + prebuilt `.lib` files).

## Commands

| command | purpose |
|---|---|
| `selftest` | correctness gate — run after touching `meta.py`; `dxlib_modelviewer.py` changes can only be verified against a real exe (stage 5, best-effort) |
| `convert FILE OUT` | convert `FILE` (e.g. `.fbx`) → `OUT` (`.mv1`) by driving the real `DxLibModelViewer_64bit.exe` GUI |
| `install MV1 --dest ...` | copy a converted `.mv1` into `Assets/`, mint a fresh-GUID `.meta` (an existing `.meta` at `--dest` is kept as-is, GUID included, so re-installing never breaks prefab references), and (with `--source`) copy the original `.fbx` under `<dest-dir>/_Source/`, and (with `--textures`) bulk-copy image files under `<dest-dir>/textures/` |

### `convert`'s path resolution

`--modelviewer-path` → `$DXLIB_MODELVIEWER` → a pinned default
(`cli.DEFAULT_MODELVIEWER_PATH`, verified 2026-09-12 against DxLibModelViewer
ver3.24d — see "Known limitations" below). Override it with `--modelviewer-path`
or `$DXLIB_MODELVIEWER` if you're on a different machine/install.

### Typical flow

```
python -m tools.model convert MyProp.fbx MyProp.mv1 --modelviewer-path "C:\...\DxLibModelViewer_64bit.exe"
python -m tools.model install MyProp.mv1 --source MyProp.fbx --textures MyProp.fbm --dest Assets/Art/Models/MyProp/MyProp.mv1
```

Then bind the printed GUID to a `Mv1File`-typed field the same way any other
asset GUID is wired into a prefab/component.

### `install --textures`

`--textures SRC_DIR` bulk-copies every recognized image file (`.png`/`.jpg`/
`.jpeg`/`.bmp`/`.tga`/`.dds`, case-insensitive) found **directly** under
`SRC_DIR` (no recursion) into `<dest-dir>/textures/` — matching the sibling
`textures/` folder convention already used by real shipped assets (e.g.
`Assets/Art/Models/Fantasy/DirtyHouse/textures/`). `SRC_DIR` is typically the
`.fbm` folder DxLibModelViewer/the FBX SDK auto-creates next to a source
`.fbx` with embedded textures (see `Known limitations` below), or wherever
the artist's loose texture files live.

This is a **plain, unfiltered copy** — it does not try to determine which
textures the `.mv1` actually references (see below for why that would be
unreliable anyway), and it does not rewrite any paths. `--textures` is
opt-in and off by default, same as `--source`.

## Known limitations

* **No official CLI/CUI exists for `DxLibModelViewer`.** `convert` is a
  reverse-engineered UI-automation wrapper (`dxlib_modelviewer.py`), not a
  vendor-supported integration. Expect it to need adjustment for a different
  `DxLibModelViewer` build/version, a different Windows display language, or
  a different screen/DPI configuration than whatever it was last verified
  against.
* **Verified 2026-09-12 against DxLibModelViewer ver3.24d** (title bar reads
  `DxLibModelViewer [ DxLib ver3.24d ]`, pinned at
  `cli.DEFAULT_MODELVIEWER_PATH`), two ways: a real shipped `.mv1`
  (`Assets/Art/Models/Basic/Cube.mv1`) round-tripped end to end through
  `python -m tools.model convert` (Open → Save As mesh only), and a real
  ~36MB textured `.fbx` converted the same way — both produced a
  plausible-sized `MV11`-header `.mv1`. If a future `DxLibModelViewer` build
  changes menu command ids or dialog control ids, re-run the inspection
  documented at the top of `dxlib_modelviewer.py` and update its
  `_MENU_ID_*`/`_FILENAME_EDIT_IDS` constants.
* **DxLibModelViewer's "Save As mesh only" keeps only one texture per
  material.** Inspecting the compiled output of two real conversions
  (`Assets/Art/Models/Fantasy/DirtyHouse/dirtyHouse.mv1`, and a fresh
  `Hyenas_A4_AllMotion_DxLib.fbx` conversion with Diffuse/Normal/Opacity
  source textures) shows exactly **one** embedded texture reference in each,
  even though multiple texture files exist alongside the source — Normal/
  Opacity/AO/Roughness maps don't survive as separate references. This is a
  property of DxLibModelViewer's own conversion, not a gap in this toolkit's
  automation. `install --textures` still copies every texture file it's
  given (see above) so they're available on disk, but wiring up anything
  beyond the one texture DxLib kept is outside this toolkit's scope — that's
  an engine/material-system question, not a conversion one.
* **`looks_like_mv1()` is a sanity check, not a structural validator.** It
  checks for the 4-byte `MV11` header (empirically confirmed across 4 real
  shipped `.mv1` files spanning both animation clips and static/skinned
  meshes — not a documented DxLib format signature) plus a minimum file
  size. It cannot detect a corrupt-but-plausible-looking `.mv1`.
* **No `_Source/` convention existed for `.mv1` assets before this
  toolkit.** Unlike `tools/effect`'s single `Assets/Art/Effect/_Source/`
  tree (because every `.efkefc` lives under one root), `.mv1` assets are
  scattered across `Assets/Art/Animation/**`, `Assets/Art/Models/**`, etc.
  `install --source` therefore copies the `.fbx` to a **sibling**
  `_Source/` folder next to each `.mv1` (`<dest-dir>/_Source/<Name>.fbx`)
  rather than one global tree. `--source` is opt-in and off by default —
  whether this repo wants `.fbx` sources committed at all is a judgment
  call, not a settled decision (zero `.fbx` files existed in the repo before
  this toolkit).
* **Failures have no exit code / stdout to inspect**, unlike a real
  subprocess. On failure, `convert` prints the step that failed and the path
  to a debug bundle (a screenshot + a control-identifier text dump of
  whatever window was on top) saved under `--debug-dir` (default: a fresh
  temp directory). Use `print_control_identifiers()` the same way to
  re-diagnose after a `DxLibModelViewer` update breaks something.

## After changing the codec or automation

* `.py` files here are plain UTF-8/ASCII — not subject to the Shift-JIS
  conversion hook that applies to `.h`/`.cpp`.
* Always run `python tools/model/selftest.py` after editing `meta.py`. Its
  end-to-end `convert()` stage is best-effort and skips cleanly on a machine
  without `pywinauto`, a `DxLibModelViewer` exe (`$DXLIB_MODELVIEWER` /
  `cli.DEFAULT_MODELVIEWER_PATH`), and a test `.fbx` (`$TOOLS_MODEL_TEST_FBX`
  — this toolkit has no committed `.fbx` fixture).
