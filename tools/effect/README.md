# `tools/effect` — Effekseer `.efkproj` particle-effect toolkit

Stdlib-only Python 3. Lets you create new Effekseer particle effects
(`.efkproj` source), compile them to `.efkefc` via the Effekseer CUI, and
install the result as a `Assets/Art/Effect/**/*.efkefc` `ParticleFile` asset
— **without** hand-editing raw XML or opening the Effekseer GUI editor.

**Setting it up on another machine or in another project:** see
[`dist/docs/setup.md`](dist/docs/setup.md) (Japanese; the public user docs live in `dist/README.md` + `dist/docs/`). Everything machine- or project-specific
(target Effekseer version and its CUI, install destination, `.meta` output)
lives in `tools/effect/effect_config.json`.

**Effekseer versions:** every release from 1.50RC1 to 1.80.7 (families 1.5x,
1.6x, 1.7x, 1.80.x — all 33 Windows tools tested) is supported side by side —
see [Effekseer versions](#effekseer-versions). The toolkit is also published standalone at
https://github.com/ramasuke/EffekseerEfkprojTool (synced from here with `export`).

Run from the repo root:

```
python -m tools.effect <command>        # or: python tools/effect.py <command>
```

## Commands

| command | purpose |
|---|---|
| `selftest` | correctness gate — run after touching `model.py` / `xmlio.py` / `presets.py` / `enums.py` / `meta.py` / `versions.py` / `efkefc.py` |
| `new-project NAME` | create `NAME.efkproj` (empty project skeleton, `ToolVersion` `0.7CTP1` — opens in every supported editor, see [Effekseer versions](#effekseer-versions)) |
| `show FILE` | print the node tree as an outline, with `[index.path]` addresses (plus the file's `ToolVersion` / `CommonValues` layout) |
| `validate FILE` | structural sanity checks (well-formed XML, required top-level elements, known `DrawingValues` kinds, **Effekseer enum-domain check** for the target version — see below, the `ToolVersion` migration / too-new checks from [Effekseer versions](#effekseer-versions), and texture/model/sound paths that don't resolve relative to the file; `add-node`/`set-params`/`apply` warn about the latter) |
| `add-node` | add a `sprite` / `ring` / `ribbon` / `model` / `track` / `group` node under an existing node or the root |
| `set-params` | set fields on an existing node via dotted tag paths |
| `apply FILE OPS.json` | apply a batch of `add-node`/`set-params` ops atomically (primary agent interface) |
| `compile FILE` | compile `.efkproj` → `.efkefc` with the target version's Effekseer CUI (`effekseer.cui_paths` in `effect_config.json`). **Refuses** (before running the CUI) when a referenced texture/model/sound doesn't exist relative to the `.efkproj`, when `--out` is in another folder, when the CUI is a different Effekseer version family than the target, when the file's `ToolVersion` is newer than that CUI, or when a field is on the wrong side of the file's `ToolVersion` migrations — the CUI itself "compiles" all of these without complaint (exit 0). Afterwards checks the output's binary version (1500/1610/1710/1810 per family) |
| `upgrade FILE` | rewrite a `.efkproj` in the target editor's native format by letting its CUI migrate it (the project XML in the compiled `.efkefc`'s `EDIT` chunk); needed for 1.80-only `CommonValues` settings. Same refusals as `compile` |
| `install EFKEFC --dest ...` | copy a compiled `.efkefc` into `project.effect_dir` (`Assets/Art/Effect/` here) **together with every texture/model it references** (same relative paths next to `--dest`), mint a fresh-GUID `.meta` when `meta.enabled` (an existing `.meta` at `--dest` is kept as-is, GUID included, so re-installing never breaks prefab references), and (with `--project`) commit the source under `<effect_dir>/<source_subdir>/` (`Assets/Art/Effect/_Source/` here) with its asset paths **rewritten to point at the installed files**. Refuses, writing nothing, when a reference points outside the effect's folder (`../...` — exported into another folder than its `.efkproj`), can't be found, or would overwrite a different same-named file, **or** (when `project.runtime_version` is set) when the effect's binary version is newer than that runtime loads (e.g. `1.7` refuses 1.80-compiled effects) |
| `check-env` | print the resolved `effect_config.json` settings (target version and where it came from, the CUI and the version detected from it); exit 1 if the CUI can't be found or is a different version family than the target |
| `export --out DIR` | copy the distributable files (`export.MANIFEST`, with `dist/effect_config.json` swapped in) into DIR — how the public EffekseerEfkprojTool repository is updated |

Nodes have no stable id in the `.efkproj` format itself (unlike `tools/bt`'s
per-node GUIDs), so `--parent`/`--path` address a node by a dot-separated
0-based child-index path from the root, e.g. `"1.0"` = the root's 2nd child
node's 1st child node. `""` (or `"root"`) means the root itself. `show`
prints these paths next to each node.

### Typical flow

```
python -m tools.effect new-project Spark --dir <scratch dir>
python -m tools.effect add-node Spark.efkproj --kind ring --name Burst --set DrawingValues.Ring.CenterRatio_Fixed=0.85
python -m tools.effect add-node Spark.efkproj --kind sprite --name Glow
python -m tools.effect show Spark.efkproj
python -m tools.effect compile Spark.efkproj
python -m tools.effect install Spark.efkefc --project Spark.efkproj --dest Assets/Art/Effect/MyPack/Spark.efkefc
```

(The paths above are NanamiEngine's; elsewhere use your own
`project.effect_dir`.) In NanamiEngine, bind the printed GUID to a `ParticleFile`-typed field (see
`Assets/Art/Effect/Laser01.efkefc.meta` for the shape) the same way any other
asset GUID is wired into a prefab/component.

### `add-node`'s dedicated flags

Beyond `--kind`/`--name`/`--parent`, `add-node` has dedicated flags for the
fields most real effects touch (still backed by `--set dotted.path=value`
for anything not listed here):

| flag | maps to | applies to |
|---|---|---|
| `--life`, `--max-generation`, `--infinite` | `CommonValues` | any kind |
| `--color-texture`, `--fade-in`, `--fade-out`, `--uv-scroll` | `RendererCommonValues` (fade speeds: `-30,-20,-10,0,10,20,30` only, see below) | any kind |
| `--generation-shape circle\|sphere\|point` + `--radius`/`--division`/`--angle-start`/`--angle-end` | `GenerationLocationValues` | any kind |
| `--billboard` | `Sprite.Billboard` | `sprite` |
| `--color R:G:B[:A]` | fixed color (`ColorAll_Fixed` for sprite/ribbon, all 3 ring colors, `Color_Fixed` for model) | `sprite`/`ribbon`/`ring`/`model` |
| `--color-random R,G,B[,A]` (each channel `CENTER` or `MIN:CENTER:MAX`) | `Sprite.ColorAll_Random` | `sprite` |
| `--model` (required), `--lighting` | `Model` block | `model` |
| `--track-color R:G:B[:A]` | all 6 `Track` rails, same fixed color | `track` |
| `--generation-time CENTER\|MIN:CENTER:MAX`, `--trigger-to-start/--trigger-to-stop/--trigger-to-remove TRIGGER` | `CommonValues` (legacy `GenerationTime`/`TriggerParam`, or 1.80 `Generation`/`Removal` — whichever the file's `ToolVersion` needs) | any kind |
| `--generation-timing continuous\|trigger`, `--trigger TRIGGER`, `--trigger-count CENTER\|MIN:CENTER:MAX` | `CommonValues.Generation` | any kind, **files with ToolVersion ≥ 1.80β2 only** (`upgrade` first) |

PVA-shaped values accept `CENTER` (fixed) or `MIN:CENTER:MAX` (a range).
`TRIGGER` is `none`, `trigger0`..`trigger3`, `parent-removed`/`parent-collided`
(1.80 only), or the raw `TriggerType` int.

Every command that reads the target version also takes
`--effekseer-version VERSION` (see below).

**Easing speeds are enums, not floats.** `--fade-in`/`--fade-out`'s
`START_SPEED`/`END_SPEED`, and `start_speed`/`end_speed` on `presets.easing()`
/ `axis_easing()` / `renderer_common(fade_in=/fade_out=)`, map to Effekseer's
`EasingStart`/`EasingEnd` enums: only `-30,-20,-10,0,10,20,30` exist
(negative = *Slowly1-3*, positive = *Rapidly1-3*, `0` = linear). Any other
value compiles fine via the CUI but **crashes the Effekseer editor**
(`NullReferenceException` in `GUI.Component.Enum.Update`) the moment the
Basic Render Settings dock shows that node — which is how the first three
toolkit-built effects were shipped. The toolkit now rejects such values at
build time (`ValueError` / `CliError`), and `validate` / every `add-node` /
`set-params` / `apply` write runs the same enum-domain check (`enums.py`)
over the other enum-typed leaves it knows about (`Filter`, `AlphaBlend`,
`Billboard`, `UV`, the `Type` selectors, ...), refusing to write a violation.
Per-corner Sprite offsets/colors, per-rail Track differentiation, Easing/
AxisPVA variants, `ColorAll_Easing`/Ring's per-position (`OuterColor`/
`CenterColor`/`InnerColor`) Random+Easing color modes, `LocationAbsValues`
(gravity/attractive force), and `SoundValues` have no dedicated flags yet -
build them via `tools.effect.presets` directly (see below) or `--set`.

## Effekseer versions

Supported: every Effekseer release with a Windows tool from **1.50RC1 to
1.80.7**, in four families — **1.5x** (1.50RC1, 1.50RC2, 1.51), **1.6x**
(1.60–1.62e), **1.7x** (1.70–1.7.3.0) and **1.80.x** (1.80.0 RC1–RC3,
1.80.1–1.80.7). All 33 of those tools were downloaded and run through the
toolkit's per-CUI checks (`selftest` stage 5 + stage 11: compiles, binary
version, too-new refusal, migration guard ground truth, `upgrade` round trip).
Betas, 1.43 and older, and anything after 1.80 are refused as unsupported.

The toolkit targets one Effekseer version at a time, picked by
`--effekseer-version`, then `$EFFEKSEER_VERSION`, then `effekseer.version` in
`effect_config.json`; if none is set, the version of the CUI it finds, else
1.7.3.0. `effekseer.cui_paths` maps versions to their `Effekseer.exe`
(exact version first, then any of the same family; `$EFFEKSEER_CUI` /
`--cui-path` override it, and the older single `effekseer.cui_path` still
works). The CUI's real version is read from the `EffekseerCore.dll` next to
it (`Effekseer.exe`'s own file-version resource just says `1.0.0.0`; this
finds the exact `Core.Version` for every release except 1.50RC1/RC2, which
read as `1.50`), and a 1.80 `Tool/Effekseer.exe` launcher is run as the
`Tool/bin/Effekseer.exe` it wraps. `versions.py` holds the per-family
`Profile`s.

What the target family changes:

| | 1.5x | 1.6x | 1.7x | 1.80.x |
|---|---|---|---|---|
| enum domains (`enums.py`) | `ENUM_DOMAINS_15` | `ENUM_DOMAINS_16` | `ENUM_DOMAINS_17` | `ENUM_DOMAINS_180` |
| compiled binary version (INFO/BIN_) | 1500 | 1610 | 1710 | 1810 |

The enum tables were checked against each release's `EffekseerCore.dll` by
reflection (every `Value.Enum<T>` reachable from `Effekseer.Data.Node`, i.e.
exactly what the GUI's combo boxes offer): every release in a family has
identical domains and matches its table. Newer families only add members —
1.6: more `LocationValues`/`ScalingValues` types and `LocationEffectType`s,
`ModelReference`; 1.7: `FadeOutType`=2, Gradient colors,
`RotationValues.Type`=6, `TriggerParam`, `KillRulesValues`; 1.80:
`Wrap`=Mirror(2), `Billboard`=DirectionalBillboard(4),
`RotationValues.Type`=RotateToVelocity(7), `TriggerType`
ParentRemoved(2)/ParentCollided(3), `ModelReference` ExternalModel,
`Generation`/`Removal`/`CollisionsValues`/`GpuParticles` — except that
**`TextureUVType` was renumbered in 1.80** (Tile 1 → 2, TilePerParticle = 1).
A value only a newer family has is a violation when targeting an older one
(that editor crashes on it). Blocks an older family doesn't have at all
(`GpuParticles`, `TriggerParam`, ...) only produce a warning — it ignores them.

A runtime refuses effects above its own binary version, so set
`project.runtime_version` to the game's runtime family and `install` refuses
anything newer (NanamiEngine: `1.7`).

### `ToolVersion`: why new projects are `0.7CTP1`, and `upgrade`

Each editor migrates a file on load according to **the file's own
`<ToolVersion>`**, not the editor's version (`Core.LoadFromXml` +
`Utils/ProjectVersionUpdater.cs`): anything older than a migration's
threshold has old-style fields moved into their current place, and anything
at or above it is read as written. `presets.py` builds the old-style shapes
of the samples it was derived from, so `new-project` always writes
`ToolVersion` **`0.7CTP1`** (unparsable → oldest → every migration runs),
which every 1.50RC1–1.80.7 editor opens and reads correctly. The migrations
that touch what the toolkit writes (`versions.MIGRATIONS`):

| a file with ToolVersion ≥ | ignores (old shape) | old files get it moved into (overwritten) |
|---|---|---|
| `1.50β3` | `RendererCommonValues/Distortion`, `DrawingValues/Model/Lighting`, `Model/NormalTexture` (when the node has `RendererCommonValues`) | `RendererCommonValues/Material`, `NormalTexture` |
| `1.60α1` | flat `RendererCommonValues/UVAnimation/*` | `UVAnimation/AnimationParams` |
| `1.60α3` | `LocationAbsValues/Type`, `Gravity`, `AttractiveForce` | `LocationAbsValues/LocalForceField4` |
| `1.70α1` | `DrawingValues/Sprite/ColorAll*`, `DrawingValues/Model/Color*` | `DrawingValues/ColorAll` (sprite/model nodes) |
| `1.70α2` | `DrawingValues/Track/Color*` rails | `DrawingValues/TrailColor*` (track nodes) |
| `1.80β2` | `CommonValues/GenerationTime*`, `RemoveWhen*`, `TriggerParam` | `CommonValues/Generation`, `Removal` |

`validate`, `compile`, `upgrade` and every write refuse a field on the wrong
side of its file's thresholds (Effekseer would silently drop or overwrite
it). `selftest` stage 11 proves the table complete per CUI: it compiles
presets trees under each threshold `ToolVersion` and compares the decoded
`EDIT` chunk with the `0.7CTP1` compile — every difference must be flagged
and nothing else. (Ribbon/track `TrailSmoothing`/`TrailTimeSource` are also
pinned by the `1.70α1` migration, but that is behaviour, not a lost field.
A model node with `RendererCommonValues` and no `Lighting` also gets the
lit material only in an old file.)

**1.80-only `CommonValues` settings** (`Generation/Timing`=Trigger,
`Trigger`, `TriggerCount`, i.e. `add-node --generation-timing/--trigger/
--trigger-count`) only exist in a file at or above `1.80β2`. Convert a
project with **`upgrade FILE --effekseer-version 1.80.x`**: it compiles the
file to a scratch `.efkefc` next to it and writes back the project XML the CUI
stores in the `EDIT` chunk (`efkefc.edit_project`) — the editor's own
migration, not a port — so the result is in that editor's native format with
its `ToolVersion`. After that, old-shape flags (e.g. `add-node --color` on a
sprite) are refused in that file; use `--set` with the native paths
(`DrawingValues.ColorAll.Fixed.R=...`). `presets.common_values(layout=...)`
takes `versions.layout_of(project)` (`"legacy"` below `1.80β2`, `"v180"`
from it), and `add-node`/`apply` pick it from the file.

An editor also refuses a `ToolVersion` newer than itself (the CUI prints
"Version Error" and exits 0 with no output), which `validate`/`compile`/
`upgrade` check up front.

`GpuParticles` and `CollisionsValues` (1.80) are enum-checked but have no
builders or flags yet — use `--set` in a 1.80 file.

### Building nodes programmatically

For anything beyond a couple of `--set` flags, it's usually easier to import
`tools.effect.presets` directly and build the tree in a small Python script
than to chain many CLI calls — see `presets.py`'s docstrings, or
`selftest.py`'s `stage_presets_roundtrip` for a worked example (builds a
ring + sprite node purely through the preset functions, no hand XML).

## Known limitations

* **Node kinds**: `Sprite` / `Ring` / `Ribbon` / `Model` / `Track`
  `DrawingValues` are modeled (`DRAWING_TYPE` in `presets.py`), plus the two
  `Node`-level sibling blocks `SoundValues` and `LocationAbsValues`
  (gravity/attractive force) — evidenced across a 310-file corpus spanning
  11 real asset packs (AndrewFM01, MAGICALxSPIRAL, NextSoft01, NitoriBox,
  Pierre01_130, Pierre02_130, ProjectDanmakuGirls, Suzuki01, TouhouStrategy,
  tktk01, tktk02). Still unmodeled, same reasoning as before (rare and/or no
  confirmed-*active* real example to crib from):
  * FCurve (keyframed) variants — Scaling/Rotation `Type=5`, Sprite
    `ColorAll`/GenerationLocationValues `Type=3`/`4` FCurve modes.
  * `RotationValues` `Type=4`/`AxisEasing` (`presets.axis_easing()` exists,
    built by structural analogy with `AxisPVA`/`Easing`, but never appears
    *actively selected* in any of the 310 samples — treat as unverified).
  * Project-root `Behavior`/`TargetLocation`/`Culling` (camera/viewer
    metadata, siblings of `<Root>` under `<EffekseerProject>`, unrelated to
    per-node particle motion).
  * A `Field`/turbulence/collision node concept — absent before 1.80 (zero
    matches across all 310 samples). 1.80 has `CollisionsValues` and
    `GpuParticles` (with vortex/turbulence forces) blocks; their enums are
    checked, but no builder exists (no real sample uses them yet).
  * `GenerationLocationValues`'s `Model`-shaped emission (spawn from another
    model's surface) — real but rare (concentrated in one effect family),
    unlike the `Point`/`Circle`/`Sphere` shapes which are modeled.

  Ring's `OuterColor`/`CenterColor`/`InnerColor` and Sprite/Ribbon's
  `ColorAll` all support the same Fixed/Random/Easing triad (each an
  independent 0/1/2 selector, confirmed real) — see `presets._append_color_mode`.

  Add any of these the same way the rest of this toolkit was built — from a
  real `.efkproj` sample that actively uses the feature — in `presets.py`
  (a new builder + `DRAWING_TYPE` entry for a new `DrawingValues` kind) and
  `cli.py` (`_KIND_BUILDERS` / `_build_drawing()`).
* **Not a schema validator**: `validate` checks structure plus the enum
  domains in `enums.py` (int values of the enum-typed leaves the toolkit
  writes, verified against a 310-file real corpus), not every field's
  legality — Effekseer's real schema is hundreds of fields across dozens of
  node kinds, most only present because they differ from the editor's
  default. `add-node`/`set-params` will happily write a field name that
  isn't real (Effekseer's loader is also **case-sensitive**: `<center>` is
  silently ignored where `<Center>` is meant — `presets.pva()` used to do
  exactly that for per-axis dicts); the only hard check for anything not in
  `enums.py` is the CUI compile step — and the CUI does *not* catch enum
  values the editor will crash on.
* **The CUI compile step is machine-specific.** See
  [Effekseer versions](#effekseer-versions) for how the CUI is found and
  which releases were tested. NanamiEngine's own runtime (EffekseerForDXLib)
  is 1.7, so its `effect_config.json` targets 1.7.3.0 with
  `project.runtime_version` `1.7`. For a release that doesn't work, contact
  NiceBody via [Issues](https://github.com/ramasuke/EffekseerEfkprojTool/issues).
* **Don't run several Effekseer CUIs at once.** With 4 in parallel, every
  family's CUI occasionally exited 1 with no output; the same compiles pass
  when run one at a time. `compile`/`upgrade` say so in that error.
* **A node without `DrawingValues` (or without its `Type`) is a Sprite** in
  every editor — `group` nodes therefore write `DrawingValues/Type` 0
  explicitly (older toolkit builds didn't, so their "groups" drew a default
  sprite).
* **Reverse direction (`.efkefc` → `.efkproj`) is not supported** as a
  command — no CUI build can do it (matches an upstream GitHub issue).
  `efkefc.edit_project()` does decode the editor XML stored in a `.efkefc`'s
  `EDIT` chunk (the selftest uses it as ground truth), but that is the
  editor's post-migration view, not a supported round-trip path. This toolkit is for authoring new effects, not round-tripping
  ones already shipped as compiled binaries.

## After changing the codec or presets

* `.py` files here are plain UTF-8/ASCII — not subject to the Shift-JIS
  conversion hook that applies to `.h`/`.cpp`.
* Always run `python tools/effect/selftest.py` after editing `model.py`,
  `xmlio.py`, `presets.py`, `enums.py`, `meta.py`, `config.py`,
  `versions.py`, `efkefc.py`, or `export.py`. Its CUI-compile stage and the
  `EDIT`-chunk ground-truth stage (both run once per CUI in
  `effekseer.cui_paths`; stage 11 compiles ~100 trees per CUI) and the
  real-corpus enum sweep (`$EFFEKSEER_CORPUS`, else `selftest.corpus_dir` in
  `effect_config.json`) are best-effort and skip
  cleanly on a machine without them; the stages reading shipped
  `Assets/Art/Effect/` assets skip outside NanamiEngine.
* A new file the toolkit needs at runtime must also be added to
  `export.MANIFEST`, or the public repository won't get it.
