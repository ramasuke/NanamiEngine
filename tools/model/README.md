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
| `selftest` | correctness gate — run after touching `meta.py`, `mv1.py` or `cli.py`; `dxlib_modelviewer.py` changes can only be verified against a real exe (stage 9, best-effort) |
| `convert FILE OUT --mode mesh\|anim\|full [--with-textures] [--emissive ...]` | convert `FILE` (e.g. `.fbx`) → `OUT` (`.mv1`) by driving the real `DxLibModelViewer_64bit.exe` GUI; see "Save modes" / "`--with-textures`" / "Emissive" below |
| `materials MV1` | list the `.mv1`'s materials (index, name, diffuse, emissive) — the names `--emissive` takes |
| `set-emissive MV1 --emissive ... [--out PATH]` | set the emissive color of an already-converted `.mv1` (in place unless `--out`; the `.meta`/GUID is untouched) |
| `install MV1 --dest ...` | copy a converted `.mv1` into `Assets/`, mint a fresh-GUID `.meta` (an existing `.meta` at `--dest` is kept as-is, GUID included, so re-installing never breaks prefab references), and (with `--with-textures`) place every texture the `.mv1` references next to it, (with `--source`) copy the original `.fbx` under `<dest-dir>/_Source/`, and (with `--textures`) bulk-copy image files under `<dest-dir>/textures/` |

### Save modes (`convert --mode`, required)

| `--mode` | DxLibModelViewer File menu command (id) | output |
|---|---|---|
| `mesh` | 名前を付けてメッシュのみ保存 (6) | model only — geometry, materials, texture references; animations dropped |
| `anim` | 名前を付けてアニメーションのみ保存 (7) | animation clips only, no mesh/materials — for clip files shared by models with the same skeleton |
| `full` | 名前を付けて保存 (5) | model + animations in one file |

Verified 2026-09-13 with a textured, 27-clip `.fbx` (the Hyena source): `mesh` 833KB (2 texture
references, no clip names), `anim` 113KB (clip names, no texture references), `full` 942KB (both;
decoded size = mesh + anim).

### `convert`'s path resolution

`--modelviewer-path` → `$DXLIB_MODELVIEWER` → a pinned default
(`cli.DEFAULT_MODELVIEWER_PATH`, verified 2026-09-12 against DxLibModelViewer
ver3.24d — see "Known limitations" below). Override it with `--modelviewer-path`
or `$DXLIB_MODELVIEWER` if you're on a different machine/install.

### Typical flow

```
python -m tools.model convert work/MyProp.fbx work/out/MyProp.mv1 --mode full --with-textures
python -m tools.model install work/out/MyProp.mv1 --with-textures --source work/MyProp.fbx --dest Assets/Art/Models/MyProp/MyProp.mv1
```

Convert in a scratch folder outside `Assets/`: loading an `.fbx` with embedded
textures makes the FBX SDK extract them into `<fbx stem>.fbm/` **next to the
`.fbx`**. Then bind the printed GUID to a `Mv1File`-typed field the same way
any other asset GUID is wired into a prefab/component.

### `--with-textures` (`convert` and `install`)

A `.mv1` stores each texture as a path **relative to the `.mv1` itself**
(DxLibModelViewer keeps the path the FBX SDK saw, e.g.
`Hyena.fbm\Hyenas_A4_Diffuse.png` or `textures\Wall_base.png` — it does not
rewrite it for the save location), and DxLib resolves it from wherever the
model is loaded. `--with-textures` makes that resolve:

1. decompress the `.mv1` (`mv1.py`) and list its texture paths;
2. look each one up — for `convert` next to the input file, for `install`
   next to the source `.mv1` — as the exact relative path, then the bare file
   name, then inside any `*.fbm` folder there;
3. copy it to the same relative path beside the output `.mv1` (sub-folders
   created; files already in place are left alone).

If any reference can't be placed (not found, or a `..\` path pointing outside
the output folder) the found ones are still copied, the rest are listed, and
the command exits 1. An absolute reference (some FBX exporters store the
artist's `C:\...` path next to the relative one) is skipped when a relative
reference to the same file name exists, otherwise the file is placed directly
beside the `.mv1`. Textures the model doesn't reference (e.g. an unused
`*_Opacity.png` in the `.fbm`) are not copied. Rejected with `--mode anim`,
which has no materials.

### Emissive (`convert --emissive`, `set-emissive`)

DxLibModelViewer's 自己発光 (emissive color) setting, without clicking through its material
panel. `--emissive` is repeatable and applied in order, later values overriding earlier ones:

```
--emissive 1,0.8,0.3                          # every material
--emissive Lamp=1,0.8,0.3                     # materials named exactly "Lamp" (see `materials`)
--emissive 2=1,0.8,0.3                        # material index 2 (when no material is named "2")
--emissive 0,0,0 --emissive Lamp=1,0.8,0.3    # everything off except Lamp
```

R,G,B are floats ≥ 0 (1 = full); alpha is left as stored. `convert` saves through the viewer
first, then patches the output, so an unknown material name fails *after* the `.mv1` was
written (unpatched) — fix the name and run `set-emissive` on it instead of re-converting.
Rejected with `--mode anim`, which has no materials. FBX exporters often bring a grey emissive
along (e.g. 0.07 or 0.35 on many shipped models, which washes them out); `--emissive 0,0,0`
clears it.

How it works: `mv1.py` decodes the file, rewrites the 12 RGB bytes of each targeted material
record (layout in `mv1.py`'s docstring), and re-compresses with its own LZ encoder. The result
decodes to exactly DxLib's body with only those bytes changed, but the compressed bytes are not
DxLib's (about 1–2% larger), so the file's hash changes even for `--emissive` values equal to
the current ones. The engine's `ModelRenderer` draws with DxLib's standard shader, which uses
the material emissive; materials switched to a custom shader by an `IModelMaterialShaderPolicy`
only show it if that shader reads it, and `SkyDome3D` overwrites it at runtime (base × tint).

### `install --textures`

`--textures SRC_DIR` bulk-copies every recognized image file (`.png`/`.jpg`/
`.jpeg`/`.bmp`/`.tga`/`.dds`, case-insensitive) found **directly** under
`SRC_DIR` (no recursion) into `<dest-dir>/textures/` — matching the sibling
`textures/` folder convention already used by real shipped assets (e.g.
`Assets/Art/Models/Fantasy/DirtyHouse/textures/`). `SRC_DIR` is typically the
`.fbm` folder DxLibModelViewer/the FBX SDK auto-creates next to a source
`.fbx` with embedded textures, or wherever the artist's loose texture files
live.

This is a **plain, unfiltered copy** — it does not look at which textures the
`.mv1` references or where it expects them (that's `--with-textures`, which
is usually what you want), and it does not rewrite any paths. `--textures` is
opt-in and off by default, same as `--source`.

## Known limitations

* **No official CLI/CUI exists for `DxLibModelViewer`.** `convert` is a
  reverse-engineered UI-automation wrapper (`dxlib_modelviewer.py`), not a
  vendor-supported integration. Expect it to need adjustment for a different
  `DxLibModelViewer` build/version, a different Windows display language, or
  a different screen/DPI configuration than whatever it was last verified
  against.
* **Verified 2026-09-13 against DxLibModelViewer ver3.24d** (title bar reads
  `DxLibModelViewer [ DxLib ver3.24d ]`, pinned at
  `cli.DEFAULT_MODELVIEWER_PATH`): all three `--mode`s plus `--with-textures`
  end to end on a real ~36MB textured, animated `.fbx` (earlier, 2026-09-12:
  a `Cube.mv1` round trip through mesh-only save). If a future
  `DxLibModelViewer` build changes menu command ids or dialog control ids,
  re-run the inspection documented at the top of `dxlib_modelviewer.py` and
  update its `_MENU_ID_*`/`_FILENAME_EDIT_IDS` constants.
* **Save options come from the viewer's own `Setting.ini`**, next to the exe
  (`SaveNormal8bit`, `SavePosition16bit`, `SaveAnimKey16bit` — the lossy
  "保存オプション" menu toggles), as do load options like `NormalRemake`.
  `convert` does not change them; toggle them in the viewer once if you need
  full-precision output.
* **Animation output is not byte-for-byte reproducible.** Converting the same
  `.fbx` repeatedly (27 runs, 2026-09-13) yields one of two animation-data
  sizes for `anim` and `full` — e.g. Hyena `full` decodes to 1,471,812 or
  1,303,788 bytes; the header counts show ~8,000 more 20-byte animation keys
  in the larger one, with identical clip names. `mesh` output is identical
  every run. This is inside DxLibModelViewer's own save (a 3-second pause
  before saving didn't change the odds), so it applies to manual GUI
  conversion too; both variants are valid `.mv1` files. Don't rely on a
  re-convert producing an unchanged file (e.g. for "did anything change"
  diffs).
* **Earlier claim retracted: DxLibModelViewer does *not* keep only one texture
  per material.** That was concluded (2026-09-12) from grepping the raw
  `.mv1` bytes, which are LZ-compressed so strings appear fragmented.
  Decompressed (`mv1.py`), real outputs reference diffuse *and* normal /
  roughness / specular maps (e.g. the Hyena conversion keeps
  `Hyenas_A4_Diffuse.png` + `Hyenas_A4_Normal.png`; its `Opacity` map is
  genuinely unreferenced). Whether the engine's shaders use those extra maps
  is a separate, engine-side question.
* **`mv1.py` is reverse-engineered, not from a spec.** The container layout
  (`MV11` + DXArchive-style LZ header) decodes all 132 `.mv1` files under
  `Assets/` to exactly their declared size, which is strong evidence, but
  `texture_paths()` is still a heuristic string scan of the decoded body
  (NUL-terminated strings ending in an image extension), not a walk of the
  material table. `encode()` only emits the LZ forms found in DxLib's own
  output (matches ≤ 8195 bytes, overlapping matches, 1–3 byte distances).
* **The material table layout is reverse-engineered too.** Its offsets (`mv1.py` docstring)
  were read off real files and hold for every `.mv1` under `Assets/` (2026-09-19: 197 files,
  642 materials; the selftest re-checks the whole folder). `materials()` checks every record's
  index and name and refuses the file if anything is off, so a future DxLib layout change makes
  `set-emissive` fail instead of writing into the wrong bytes.
* **`looks_like_mv1()` is a sanity check, not a structural validator.** It
  checks the 4-byte `MV11` header, a minimum file size, and that the
  compressed body decodes to its declared size. It cannot detect a
  corrupt-but-decodable `.mv1`.
* **Automation runs alongside your own desktop use, within limits.** Dialog
  buttons are pressed with window messages (`BM_CLICK`), not synthesized
  mouse clicks, so other windows covering the viewer no longer swallow them
  (a mouse-based click did, 2026-09-13). A `BM_CLICK` that arrives while the
  shell dialog is still settling can be ignored, so OK is re-sent until the
  dialog closes. Don't interact with the viewer's own windows while it runs,
  and don't run two conversions at once. After these fixes, 24 back-to-back
  conversions (8 × each mode, ~7–13s each for a 36MB `.fbx`) all succeeded.
* **The output folder is created if missing.** The Save As dialog itself
  refuses a non-existent folder with a message box, which would otherwise
  surface only as a `wait-for-output` timeout.
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
* Always run `python tools/model/selftest.py` after editing `meta.py`,
  `mv1.py` or `cli.py`. Its end-to-end `convert()` stage (all three modes) is
  best-effort and skips cleanly on a machine without `pywinauto`, a
  `DxLibModelViewer` exe (`$DXLIB_MODELVIEWER` /
  `cli.DEFAULT_MODELVIEWER_PATH`), and a test `.fbx` (`$TOOLS_MODEL_TEST_FBX`
  — this toolkit has no committed `.fbx` fixture; point it at a copy outside
  `Assets/` because loading creates a `.fbm` folder beside it; per-mode
  timeout `$TOOLS_MODEL_TEST_TIMEOUT`, default 240s).
