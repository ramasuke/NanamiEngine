# `tools/scene` — Scene / Prefab (GameObject + Component) toolkit

Stdlib-only Python 3. Lets you create & edit `Assets/Scene/*.scene` and `Assets/Prefab/**/*.prefab`
(the cereal-JSON GameObject/Component trees) **without** running the engine or the ImGui scene
editor. Shares its cereal-JSON codec, tagged-blob representation, `.meta` codec, and vcxproj editor
with `tools/bt` — see `tools/common/`.

Run from the repo root:

```
python -m tools.scene <command>        # or: python tools/scene.py <command>
```

## Commands

| command | purpose |
|---|---|
| `selftest` | correctness gate — run after touching `reader.py` / `writer.py` / `tools/common/*` |
| `new-scene NAME [--dir]` | create `NAME.scene` (empty) + `.meta` (fresh GUID) |
| `new-prefab NAME [--dir]` | create `NAME.prefab` (empty root GameObject) + `.meta` (fresh GUID) |
| `copy-prefab SOURCE [--name] [--dir]` | duplicate a `.prefab` as a new standalone file + `.meta` (fresh asset GUID, every GameObject/Component guid re-minted, `copied_object_guids` cleared); default name is `<source stem>_copy` in the source's own directory |
| `show FILE` | print a `.scene`/`.prefab` GameObject tree as a readable outline |
| `validate FILE` | static checks (duplicate GUIDs, ...) |
| `add-gameobject` / `remove-gameobject` / `move-gameobject` | structural edits (`move-gameobject` preserves world transform by default) |
| `set-transform` / `set-active` / `rename-gameobject` | per-GameObject field edits |
| `add-component` / `remove-component` / `set-component-params` | attach/detach/configure a Component instance |
| `instantiate-prefab PREFAB --into FILE [--parent]` | deep-copy a prefab's tree in, with every guid re-minted |
| `apply FILE OPS.json` | apply a batch of edit ops atomically (primary agent interface) |
| `regen-catalog [--check]` | rebuild `catalog.json` from the Component headers |

Every mutating command supports `--dry-run` (prints a unified diff, writes nothing).

## Known limitations (v1)

* Only "pure tree" data files (no shared/back-referenced GameObjects or Components) are supported.
  The reader raises `PureTreeError` otherwise — true of every real `.scene`/`.prefab` in this
  engine, since a scene's copy of a prefab is always a fully independent baked snapshot, never a
  live shared reference.
* `add-component`'s catalog covers components registered via the `ENGINE_REGISTER_COMPONENT` macro
  (~65 of them). A brand-new instance is written with one unnamed `valueN` slot per base class the
  component archives, in `save()` order — `ComponentBase` (`value0`: guid/enabled) plus an empty
  object for every field-less lifecycle mixin (`IInitRenderable`, `IUserInterfaceRenderable`,
  `IAwakable`, `IUpdatable`, `IRenderable`, ...; the catalog's `bases` table records which bases are
  empty). cereal reads those slots positionally, so none of them is optional. Components with a base
  that owns its **own fields** (every Collider, via `ColliderBase`; `NetworkComponent` subclasses; any
  `EnemyBase`/gameplay-script component) still round-trip losslessly, and `set-component-params` can
  still edit that component's *own* fields — but `add-component --type <Name>` refuses to construct a
  **brand-new** instance of such a type from scratch, since it doesn't know the base's required
  fields and constructing one without them could produce something the engine fails to load. Copy
  an existing GameObject/prefab that already has one instead (or use `instantiate-prefab`).
* A brand-new empty mixin slot is always written as `{"cereal_class_version": N}`, whereas the engine
  writes that key only at the type's first occurrence in a file (`{}` afterwards). The toolkit can't
  tell which occurrence it is, and the always-present form loads correctly either way, because
  nothing is ever read *inside* an empty base (unlike a `Field<T>`, where a stray version key shifts
  a positional read). The engine re-normalises the file on its next save.
* A component param is only settable via `set-component-params`/`add-component --param` when its
  shape is one of `int | float | bool | string | vec2 | vec3 | field` (`catalog.SETTABLE_SHAPES`).
  `vector`/`nested`/`unknown`-shaped params round-trip losslessly but must be finished in the
  editor or by hand.
* `add-component-type` (scaffolding a **brand-new C++ Component class**, mirroring `tools/bt`'s
  `add-action`) is not implemented yet — `AddComponent.cpp`'s nested, per-category ImGui menu
  structure is more involved than `tools/bt`'s flat Action-header aggregator. The catalog already
  records each component's `immediate_base`, which a future scaffold step needs.

## After changing generated C++ or the reader/writer

* `.h` / `.cpp` in this repo are **Shift-JIS (CP932)** — this toolkit doesn't generate any yet
  (see the limitation above), but keep this in mind once `add-component-type` exists.
* Always run `python tools/scene/selftest.py` after editing `reader.py`, `writer.py`, `model.py`,
  or anything under `tools/common/` (and re-run `python tools/bt/selftest.py` too, since `tools/bt`
  shares those modules).
* If you add/rename a Component (or change its serialised fields), run
  `python -m tools.scene regen-catalog` and commit `catalog.json`.
