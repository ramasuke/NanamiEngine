# `tools/effect` — Effekseer `.efkproj` particle-effect toolkit

Stdlib-only Python 3. Lets you create new Effekseer particle effects
(`.efkproj` source), compile them to `.efkefc` via the Effekseer CUI, and
install the result as a `Assets/Art/Effect/**/*.efkefc` `ParticleFile` asset
— **without** hand-editing raw XML or opening the Effekseer GUI editor.

Run from the repo root:

```
python -m tools.effect <command>        # or: python tools/effect.py <command>
```

## Commands

| command | purpose |
|---|---|
| `selftest` | correctness gate — run after touching `model.py` / `xmlio.py` / `presets.py` / `meta.py` |
| `new-project NAME` | create `NAME.efkproj` (empty project skeleton) |
| `show FILE` | print the node tree as an outline, with `[index.path]` addresses |
| `validate FILE` | structural sanity checks (well-formed XML, required top-level elements, known `DrawingValues` kinds) |
| `add-node` | add a `sprite` / `ring` / `ribbon` / `model` / `track` / `group` node under an existing node or the root |
| `set-params` | set fields on an existing node via dotted tag paths |
| `apply FILE OPS.json` | apply a batch of `add-node`/`set-params` ops atomically (primary agent interface) |
| `compile FILE` | compile `.efkproj` → `.efkefc` via the pinned Effekseer CUI |
| `install EFKEFC --dest ...` | copy a compiled `.efkefc` into `Assets/Art/Effect/`, mint a fresh-GUID `.meta`, and (with `--project`) commit the source under `Assets/Art/Effect/_Source/` |

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

Then bind the printed GUID to a `ParticleFile`-typed field (see
`Assets/Art/Effect/Laser01.efkefc.meta` for the shape) the same way any other
asset GUID is wired into a prefab/component.

### `add-node`'s dedicated flags

Beyond `--kind`/`--name`/`--parent`, `add-node` has dedicated flags for the
fields most real effects touch (still backed by `--set dotted.path=value`
for anything not listed here):

| flag | maps to | applies to |
|---|---|---|
| `--life`, `--max-generation`, `--infinite` | `CommonValues` | any kind |
| `--color-texture`, `--fade-in`, `--fade-out`, `--uv-scroll` | `RendererCommonValues` | any kind |
| `--generation-shape circle\|sphere\|point` + `--radius`/`--division`/`--angle-start`/`--angle-end` | `GenerationLocationValues` | any kind |
| `--billboard` | `Sprite.Billboard` | `sprite` |
| `--color R:G:B[:A]` | fixed color (`ColorAll_Fixed` for sprite/ribbon, all 3 ring colors, `Color_Fixed` for model) | `sprite`/`ribbon`/`ring`/`model` |
| `--color-random R,G,B[,A]` (each channel `CENTER` or `MIN:CENTER:MAX`) | `Sprite.ColorAll_Random` | `sprite` |
| `--model` (required), `--lighting` | `Model` block | `model` |
| `--track-color R:G:B[:A]` | all 6 `Track` rails, same fixed color | `track` |

PVA-shaped values accept `CENTER` (fixed) or `MIN:CENTER:MAX` (a range).
Per-corner Sprite offsets/colors, per-rail Track differentiation, Easing/
AxisPVA variants, `ColorAll_Easing`/Ring's per-position (`OuterColor`/
`CenterColor`/`InnerColor`) Random+Easing color modes, `LocationAbsValues`
(gravity/attractive force), and `SoundValues` have no dedicated flags yet -
build them via `tools.effect.presets` directly (see below) or `--set`.

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
  * A `Field`/turbulence/collision node concept — searched for across all
    310 samples, zero matches; likely absent from this Effekseer version.
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
* **Not a schema validator**: `validate` checks structure, not every field's
  legality — Effekseer's real schema is hundreds of fields across dozens of
  node kinds, most only present because they differ from the editor's
  default. `add-node`/`set-params` will happily write a field name that
  isn't real; the only hard check is the CUI compile step.
* **The CUI compile step is machine-specific.** This toolkit is pinned to
  the local Effekseer **1.7.3.0** CUI (`cli.DEFAULT_CUI_PATH`, overridable
  via `--cui-path` or `$EFFEKSEER_CUI`) — verified this session to produce
  `.efkefc` output whose `INFO`-chunk version matches assets already shipped
  in `Assets/Art/Effect/` byte-for-byte. The newer 1.80.2 CUI also present
  under `Downloads/` was **not** verified and should not be used for assets
  that ship in this repo.
* **Reverse direction (`.efkefc` → `.efkproj`) is not supported** — no local
  CUI build can do it (confirmed dead end, matches an upstream GitHub
  issue). This toolkit is for authoring new effects, not round-tripping
  ones already shipped as compiled binaries.

## After changing the codec or presets

* `.py` files here are plain UTF-8/ASCII — not subject to the Shift-JIS
  conversion hook that applies to `.h`/`.cpp`.
* Always run `python tools/effect/selftest.py` after editing `model.py`,
  `xmlio.py`, `presets.py`, or `meta.py`. Its CUI-compile stage is
  best-effort and skips cleanly on a machine without the pinned CUI.
