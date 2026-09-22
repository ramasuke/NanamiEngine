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
| `copy-prefab SOURCE [--name] [--dir]` | duplicate a `.prefab` as a new standalone file + `.meta` (fresh asset GUID, every GameObject/Component guid re-minted and Guid references between them re-pointed at the copy, `copied_object_guids` cleared); default name is `<source stem>_copy` in the source's own directory |
| `show FILE` | print a `.scene`/`.prefab` GameObject tree as a readable outline |
| `validate FILE` | static checks: duplicate GUIDs, a UTF-8 BOM / malformed JSON, and `cereal_class_version` placement (see below) |
| `add-gameobject` / `remove-gameobject` / `move-gameobject` | structural edits (`move-gameobject` preserves world transform by default) |
| `set-transform` / `set-active` / `rename-gameobject` | per-GameObject field edits |
| `add-component` / `remove-component` / `set-component-params` | attach/detach/configure a Component instance |
| `instantiate-prefab PREFAB --into FILE [--parent]` | deep-copy a prefab's tree in, with every guid re-minted and Guid references into the tree (e.g. `FIELD(IGameObject)`) re-pointed at the copy, like the engine's Instantiate |
| `apply FILE OPS.json` | apply a batch of edit ops atomically (primary agent interface) |
| `regen-catalog [--check]` | rebuild `catalog.json` from the Component headers |

Every mutating command supports `--dry-run` (prints a unified diff, writes nothing).

## `cereal_class_version` placement

cereal writes `cereal_class_version` on a type's **first occurrence per file**
and never again, caching the value for the rest of the read. Getting that wrong
is not cosmetic, and both directions are fatal:

* missing on a first occurrence -> the named lookup throws
  `provided NVP (cereal_class_version) not found`;
* present on a repeat -> cereal never consumes it (it already knows the
  version), so the next **positional** read - a `base_class<>` slot, or
  `Field<T>`'s unnamed `archive(context_)` - descends into that number and
  rapidjson asserts `IsObject()`.

`validate` replays that bookkeeping over the raw document, resolving component
types through cereal's polymorphic id table: a repeat occurrence carries only a
numeric `polymorphic_id`, so a name-only scan skips exactly the nodes where
these bugs hide. Strays are reported as `FAIL` only when the type's first read
is positional; otherwise they are a `NOTE` (harmless, but not what the engine
writes). Every mutating command runs the same audit on the text it is about to
write, so an edit that moves a type's first occurrence — a new `FIELD(T)`
landing ahead of the file's existing `Field<T>`, which is how
`GameManage.scene` broke — is refused on the spot instead of surviving until
someone runs `validate`. A file containing a component type the catalog does
not model still downgrades `missing` findings to `NOTE` (see "Not covered"),
and those do not block the write.

When building blobs from a script rather than through `add-component`, use
`edits.field_blob()` / `edits.color32_blob()` instead of hand-rolling an
`OrderedObj` - they carry the `Ver` tags the writer needs to place the version
key correctly.

Not covered: FIELDs the catalog cannot see, i.e. ones declared on an
intermediate C++ base (`PlayerAvatarBase.featStep_`) or written by a hand-rolled
loop (`BoneSync`'s `sync` entries). Those still need care by hand.

## Known limitations (v1)

* Only "pure tree" data files (no shared/back-referenced GameObjects or Components) are supported.
  The reader raises `PureTreeError` otherwise — true of every real `.scene`/`.prefab` in this
  engine, since a scene's copy of a prefab is always a fully independent baked snapshot, never a
  live shared reference.
* `add-component`'s catalog covers components registered via the `ENGINE_REGISTER_COMPONENT` macro
  (~65 of them). The macro call `ENGINE_REGISTER_COMPONENT(T);` lives in the component's `.cpp`
  and its version in the header's `CEREAL_CLASS_VERSION(T, V)`; the scanner reads both. A brand-new instance is written with one unnamed `valueN` slot per base class the
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
* A GameObject's `mark_` (the editor-only `GameObjectMark` shown in GameWindow, index into
  `model.MARK_NAMES`) is round-tripped and printed by `show`, but no CLI verb sets it yet — pick it in
  the Inspector. It was appended after `transform_` in class version 1 (`SceneGameObject`,
  `CopiedPrefabGameObject`; `PrefabGameObject`'s in-memory copy is 2). cereal prints a type's version
  only once per file, so the writer treats `mark_` as all-or-nothing **per GameObject type**: if any
  node of that type carries a mark (`node.mark is not None`) the type is written at the new version and
  every node of it gets `mark_` (0 when unset); otherwise the pre-mark version and layout are kept, so
  older files round-trip byte-identically. A `.prefab` root has no version at all — the engine detects
  its `mark_` by key, and the writer emits it only when `prefab.root.mark is not None`.
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
