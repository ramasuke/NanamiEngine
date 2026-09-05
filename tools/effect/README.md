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
| `add-node` | add a `sprite` / `ring` / `ribbon` / `group` node under an existing node or the root |
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

### Building nodes programmatically

For anything beyond a couple of `--set` flags, it's usually easier to import
`tools.effect.presets` directly and build the tree in a small Python script
than to chain many CLI calls — see `presets.py`'s docstrings, or
`selftest.py`'s `stage_presets_roundtrip` for a worked example (builds a
ring + sprite node purely through the preset functions, no hand XML).

## Known limitations (v1)

* **Node kinds**: only `Sprite` / `Ring` / `Ribbon` `DrawingValues` are
  modeled — the 3 kinds actually evidenced across the 14 real AndrewFM01
  sample files this toolkit was built from (`Sprite` ×34, `Ring` ×49,
  `Ribbon` ×8). `Model` / `Track` and other kinds have no real example to
  crib field names/defaults from; add them the same way this v1 was built —
  from a real `.efkproj` sample, once one is available — in `presets.py`
  (`DRAWING_TYPE` + a new builder function) and `cli.py` (`_KIND_BUILDERS`).
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
