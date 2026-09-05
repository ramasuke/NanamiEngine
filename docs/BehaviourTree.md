# BehaviourTree: file formats, actions, and `tools/bt`

This document lets a contributor (human or agent) **create / edit a behaviour
tree** and **add a new behaviour action** — for either of the two flavors the
engine supports, **Enemy** and **FriendlyNpc** (`Editor::BehaviourTreeType::EnemyNpc` /
`FriendlyNpc`) — without opening the in-engine ImGui graph editor. The `tools/bt`
Python toolkit automates both; the format notes below also let you hand-edit safely.

The two flavors share the same composite/control node types, file-format shell
(cereal JSON, `.meta` convention) and CLI verbs; only the leaf **ActionNode** —
each flavor's own `ActionBase` hierarchy, factory, and concrete action classes —
differs. Every command below takes `--npc-kind enemy` (the default) or `--npc-kind
friendly` to pick which one it targets; `show`/`validate`/the edit verbs instead
auto-detect the flavor from the file's own extension, so `--npc-kind` is rarely
needed there.

---

## 1. Where things live

| thing | Enemy | FriendlyNpc |
|---|---|---|
| tree data files | `Assets/Data/EnemyBehaviour/*.enemyBehaviourData` (+ `*.meta`) | `Assets/Data/FriendlyNpcBehviour/*.friendBehaviourData` (+ `*.meta`) |
| runtime tree object | `Assets/Scripts/Core/Game/Npc/Enemy/Behaviour/Enemy_BehaviourTree.{h,cpp}` | `Assets/Scripts/Core/Game/Npc/Friendly/Behaviour/Friendly_BehaviourTree.{h,cpp}` |
| asset wrapper (ScriptableObject) | `Assets/Data/EnemyBehaviour/Data_EnemyBehaviourFile.{h,cpp}` | `Assets/Data/FriendlyNpcBehviour/Data_FriendNpcBehaviourFile.{h,cpp}` |
| ActionNode (wraps one ActionBase) | `Assets/Scripts/Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionNode.{h,cpp}` | `Assets/Scripts/Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionNode.{h,cpp}` |
| action base class | `.../Enemy/Behaviour/Action/Enemy_Behaviour_ActionBase.h` | `.../Friendly/Behaviour/Action/Friendly_Behaviour_ActionBase.h` |
| concrete actions | `.../Enemy/Behaviour/Action/Content/<Category>/<Name>/` | `.../Friendly/Behaviour/Action/Content/<Category>/<Name>/` |
| editor include aggregator | `.../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionHeaders.h` | `.../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionHeaders.h` |
| registration macro | `REGISTER_ENEMY_ACTION[_WITH_NAME]` | `REGISTER_FRIENDLY_ACTION[_WITH_NAME]` |
| the toolkit's catalog | `tools/bt/catalog.json` | `tools/bt/catalog_friendly.json` |

Node types shared by both (Selector, Sequence, RandomSelector, OnceExecute,
OnceSuccess, Entry) live once, flavor-agnostic, under
`Assets/Scripts/Editor/BehaviourTree/Window/Node/**`. The toolkit itself lives
at `tools/bt/` — `python -m tools.bt <cmd>`; which flavor's paths/classes/macros
a given command uses is resolved through `tools/bt/npc_kind.py`.

An enemy binds a tree through `EnemyBase::behaviourData_` (a `FIELD(Asset::EnemyBehaviourFile)`);
a friendly NPC through `FriendlyNpc::friendlyNpcBehaviourFile_` (a
`FIELD(Asset::FriendNpcBehaviourFile)`). Either way the prefab stores the asset
**GUID**, which must equal the `guid_` in the tree's `.meta` (e.g.
`Assets/Prefab/Npc/Enemy/Hyena.prefab` ↔ `HyenaBehaviour.enemyBehaviourData.meta`).
`OnAwake` loads the tree; `OnUpdate` ticks it - on both components.

---

## 2. `.enemyBehaviourData` / `.friendBehaviourData` format

Output of `cereal::JSONOutputArchive` (`BehaviourTree::OnSave`). Two top-level keys,
in this order:

* `entryNode_` — `shared_ptr<Editor::Npc::Behaviour::EntryNode>`, the graph root.
  Its `nextNode_` holds the actual tree (or `{"polymorphic_id": 0}` when empty).
* `parameters_` — `unique_ptr<ParameterGroup>`, the blackboard.

On disk: UTF-8, **no BOM**, **CRLF**, no trailing newline, 4-space indent.

### cereal bookkeeping you will see everywhere

| construct | meaning |
|---|---|
| `"polymorphic_id": 0` | null polymorphic pointer |
| `"polymorphic_id": 1073741824` | pointer whose dynamic type == static type (no type name needed) |
| `"polymorphic_id": 0x80000000\|N` + `"polymorphic_name"` | first time type *N* is written; later refs use just `"polymorphic_id": N` |
| `"ptr_wrapper": {"id": 0x80000000\|K, "data": …}` | a `shared_ptr` (K counts every shared_ptr **and** every `Field<T>` inner pointer, in write order) |
| `"ptr_wrapper": {"valid": 1, "data": …}` | a `unique_ptr` (`action_`, `parameters_`) |
| `"cereal_class_version": V` | written **once per type per file**, on that type's first instance; omitted afterwards |
| nested `"value0"` | a serialised base class (`NodeBase` → `IObject`; each action → `ActionBase`) |

Every node object is `{ [cereal_class_version,] value0: <NodeBase header>, <derived fields> }`
where the NodeBase header is `{ [ccv,] value0: <IObject {}>, guid_: {value_: "<UPPER-GUID>"},
position_: {value0: x, value1: y} }` (`position_` is editor-canvas coords).

### Node types

| type (`polymorphic_name`) | `CEREAL_CLASS_VERSION` | derived fields |
|---|---|---|
| `Editor::Npc::Behaviour::EntryNode` | 0 | `nextNode_` |
| `Editor::Npc::Behaviour::SelectorNode` | 0 | `children_[]` — first child to return Success/Running wins |
| `Editor::Npc::Behaviour::SequenceNode` | 0 | `children_[]` — stops at first Failure/Running |
| `Editor::Npc::Behaviour::RandomSelectorNode` | 1 | `children_[]`, `weights_[]` (one int per child) — picks **one** weighted child per tick and returns exactly what it returns, with **no fallback**: unlike `SelectorNode`, a child that fails makes the whole node fail that tick, it does not try another child. A branch can't be made conditional in isolation inside a `RandomSelector` — if that branch's guard fails, the pick is wasted, not retried — so a variant that's sometimes unavailable needs its own separate weighted pool (see `Editor::Npc::Behaviour::SelectorNode` above, gated by e.g. a blackboard condition, with each pool as one branch), not a guard clause on one child. Since the file is a "pure tree" (below), that second pool can't share nodes with the first — `copy-node` (below) clones one pool's whole subtree so only the diff (a swapped-in branch, a changed weight) needs hand-editing afterwards. |
| `Editor::Npc::Behaviour::OnceExecute` | 0 | `child_`, `state_` |
| `Editor::Npc::Behaviour::OnceSuccessNode` | 0 | `child_` |
| `Editor::Npc::Enemy::Behaviour::ActionNode` | 1 | `name_` (label), `action_` (`unique_ptr<GameCore::Npc::Enemy::Behaviour::ActionBase>`) |
| `Editor::Npc::Friendly::Behaviour::ActionNode` | 1 | `name_` (label), `action_` (`unique_ptr<GameCore::Npc::Friendly::Behaviour::ActionBase>`) |

A file mixes only one ActionNode flavor - which one it is fixes the whole
tree's `--npc-kind` (`tools/bt` detects this on read and records it as `Tree.kind`;
`new-tree` and `add-action` take it as an explicit flag instead, since there's
no file yet to detect it from).

The action `data` block is
`{ [ccv = <action CEREAL_CLASS_VERSION>,] value0: <ActionBase {}>, <members in save() order> }`.
`CEREAL_NVP(x_)` keeps the key `x_`; a bare `archive(x_)` serialises as `value1`, `value2`, …
`FIELD(T)` members serialise as a versioned wrapper holding an inner
`ptr_wrapper` whose data is `{value0: {value_: "<asset GUID>"}}`.

### `parameters_` (blackboard)

`{ptr_wrapper: {valid: 1, data: {value0: <count>, value1: <param>, value2: <param>, …}}}`.
Only `NanamiEngine::Module::AnimationTree::AnimationParameter<int>` is used today:
`{name_: "<key>", value_: <int>}`.

### `.meta`

`cereal::JSONOutputArchive` of a `shared_ptr<AssetBase>` = `EnemyBehaviourFile` /
`FriendNpcBehaviourFile` with `contentPath_` (backslash dirs, forward slash before
the filename) and the stable asset `guid_`. Created by `File::OnSave`.
**Regenerate with the tool, don't hand-write.**

---

## 3. `tools/bt` — usage

Run from the repo root. `python tools/bt.py <cmd>` is identical to `python -m tools.bt <cmd>`.
Examples below use an Enemy tree (`Wolf`, the default `--npc-kind`); pass `--npc-kind
friendly` to `new-tree`/`add-action`/`remove-action`/`regen-catalog` for a
FriendlyNpc one instead - `show`/`validate`/the edit verbs need no flag, they
detect the flavor from the file's own extension.

```
python -m tools.bt selftest                         # correctness gate - run after touching the emitter

# --- create / inspect ---
python -m tools.bt new-tree Wolf                     # -> Assets/Data/EnemyBehaviour/Wolf.enemyBehaviourData + .meta, prints the GUID
python -m tools.bt new-tree Guide --npc-kind friendly # -> Assets/Data/FriendlyNpcBehviour/Guide.friendBehaviourData + .meta
python -m tools.bt show   Wolf                       # readable outline
python -m tools.bt validate Wolf

# --- structural edits (all take --dry-run to preview a diff) ---
python -m tools.bt add-node    Wolf --parent entry --kind selector
python -m tools.bt add-node    Wolf --parent <selector-guid> --kind action --name Chase --type "Basic::ChasePlayerForPathFinding"
python -m tools.bt add-node    Wolf --parent <selector-guid> --kind action --name Wait  --type "Wait::Seconds::WaitSeconds"
python -m tools.bt move-node   Wolf --node <guid> --parent <guid> --index 0
python -m tools.bt copy-node   Wolf --node <guid> --parent <guid> --index 0 --weight 70  # deep copy, fresh guids throughout
python -m tools.bt remove-node Wolf --node <guid>
python -m tools.bt layout      Wolf                  # re-arrange all nodes tidily (--dx / --dy)

# --- parameter edits ---
python -m tools.bt set-params Wolf --node <action-guid> moveSpeed_=4.0 animationNumber_=2
python -m tools.bt set-params Wolf --node <action-guid> attackPower_.value_=10 spawnPosition_.offset_=0,0,-3
python -m tools.bt set-weight Wolf --node <child-guid> --weight 40
python -m tools.bt add-bb-param Wolf --name State --int 0

# --- batch (recommended for agents: atomic, one write) ---
python -m tools.bt apply Wolf ops.json     # ops.json = [{"op":"add-node",...}, {"op":"set-params",...}, ...]
```

`--type` for `add-node --kind action` accepts the editor display name
(`"Basic::ToPlayerDistance"`), the fully-qualified class, or the bare class leaf -
resolved against whichever catalog the target file's own flavor uses. Run
`python -m tools.bt regen-catalog` (Enemy) or `--npc-kind friendly` (FriendlyNpc) and
inspect `tools/bt/catalog.json` / `catalog_friendly.json` for the full list and
each action's parameters.

**Nested params.** `set-params` also accepts a **dotted key** to reach inside a
shape-`nested` struct member (`attackPower_`/`finishedAttackWriteBlackBoard_` on
`PhysicsAttack`, `spawnPosition_`/`targetPosition_`/`physicsDamage_` on
`RadiateProjectile`, `waitAnimationSound_secs_`/`animationSound_` on
`PlayAnimation`, …) — e.g. `attackPower_.value_=10`,
`spawnPosition_.targetObject_=<guid>`, `spawnPosition_.offset_=0,0,-3`
(a whole `vec2`/`vec3`/`field` leaf sets in one assignment; only the innermost
scalar/vec/field leaf is ever the settable thing — you can't assign a whole
nested struct at once, and a leaf whose own shape is `unknown` still isn't
settable). This works on both a node you just `add-node`'d and a pre-existing
one; `show`/`validate` still print the member as before, dotted or not — it's
purely an addressing scheme for `set-params`, not a new node shape.

`show` prints every GUID in full (36 chars) — copy it straight into
`--node`/`--parent` on the next command.

**Node positions.** `add-node` / `move-node` / `remove-node` / `apply` re-run a
top-down auto-layout so nodes never overlap in the graph editor; pass `--no-layout`
(or an explicit `--pos X,Y` on `add-node`) to keep the positions you already have.
`set-params` / `set-weight` / the blackboard verbs never move nodes. `layout`
re-arranges an existing tree on demand.

### Binding a new tree to an enemy or a friendly NPC

`new-tree` prints the asset GUID and the field to bind it to (`EnemyBase.behaviourData_`
for `--npc-kind enemy`, `FriendlyNpc.friendlyNpcBehaviourFile_` for `--npc-kind friendly`).
Either drag the asset onto that field in the prefab inspector, or edit the prefab
JSON so the field's `...value0.ptr_wrapper.data.value0.value_` equals that GUID.

### Limitations (v1)

* "Pure tree" archives only (no shared / back-referenced nodes) — the reader raises
  `PureTreeError` otherwise.
* Blackboard: `AnimationParameter<int>` only.
* Action params of shape `nested:*` (an embedded `WaitSeconds` / `PlaySE` / `Position`
  / `PhysicsPower` / …) are reachable through `set-params` via a **dotted key**
  (`attackPower_.value_=10`, `spawnPosition_.targetObject_=<guid>`) — see
  "Nested params" above. A struct with no settable leaf at all, and a leaf whose
  own shape is `unknown` (a couple of enum members, e.g.
  `ChangeColliderEmotionType.emotionType_`), still round-trip losslessly but stay
  **not settable**; finish those in the editor or by hand. Everything else edits
  normally, dotted or not.
* `RandomSelectorNode` has no fallback on a failed child (see the node-types table
  above) — there is no way to make one branch of a random pool conditional without
  affecting how often the pool "whiffs"; split into separate weighted pools instead.
* `OnceExecute` / `OnceSuccess` writer support is implemented from source but no
  committed fixture exercises it.
* Both flavors share one catalog scanner, but each keeps its own `catalog.json`
  (Enemy) / `catalog_friendly.json` (FriendlyNpc) — an action display name / leaf
  is only unique **within** one flavor, not across both.
* A `FIELD(Asset::X)` reference embedded inside a polymorphic object outside
  either catalog's scan root (e.g. a Quest reached through a FriendlyNpc action's
  raw, un-modeled `shared_ptr<ITakeable...>`, as `TrySwordManQuest.quest_` does)
  can, if some *other*, catalog-modeled `FIELD(Asset::X)` for the same `X` also
  appears anywhere else in the same file, come back with one extra, harmless
  `"cereal_class_version": 0` on read-then-write (two different `Field<T>`
  instantiations serialise identically when opaque, so the un-modeled occurrence
  can't be bucketed into the properly-modeled one's version slot). This never
  changes a value or drops a key cereal actually needs — its JSON archives look
  members up by name, so the extra key just sits there unread — only a strict
  byte-for-byte re-save differs; `Assets/Data/FriendlyNpcBehviour/ActionInstructure.friendBehaviourData`
  is the one committed fixture that hits this (see `tools/bt/selftest.py`'s
  `KNOWN_LIMITATION_EXTRA_LINES`).

---

## 4. Adding a new behaviour action

### With the tool

```
python -m tools.bt add-action --name FleeFromPlayer --category "Basic" \
    --param fleeSpeed:float=4.0 --param panicSeconds:float=2.0 --version 1 --dry-run
python -m tools.bt add-action --name FleeFromPlayer --category "Basic" \
    --param fleeSpeed:float=4.0 --param panicSeconds:float=2.0 --version 1

# FriendlyNpc: same shape, --npc-kind friendly picks the Friendly paths/macros/base instead
python -m tools.bt add-action --npc-kind friendly --name Wave --category "NpcStatus::Emote" \
    --param waveSeconds:float=1.5 --dry-run
```

This creates `Content/Basic/FleeFromPlayer/Enemy_Behaviour_Action_FleeFromPlayer.{h,cpp}`
(ASCII, written as UTF-8 with a BOM by default - `--encoding` also accepts `utf-8`
or the legacy `cp932`) and patches the **three wiring points** below (the Friendly
equivalents, when `--npc-kind friendly`). `--param` supports `int|float|bool|string`;
add `glm::vec3` / `FIELD(T)` members by hand afterwards. Then build:

```
MSBuild.exe NanamiEngine.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m
```

`remove-action --name FleeFromPlayer --category "Basic"` reverses everything (files
+ all three wiring points); pass the same `--npc-kind` it was created with.

### By hand — the anatomy (Enemy; FriendlyNpc is the same shape with `Friendly` swapped in)

1. **`Enemy_Behaviour_Action_<Name>.h`** under `Content/<Category>/<Name>/`:
   `class <Name> final : public ActionBase` in `namespace GameCore::Npc::Enemy::Behaviour::Action`;
   override `TickStatus DoTick(const TickContext&)` (return `Success` / `Running` /
   `Failure` / `Abort` from `TickStatus.h`); optional `void DoDrawGui()`.
   Parameter members are `[[serialize(0)]]`-tagged; hand-write
   `template<class Archive> void save/load(Archive&, const std::uint32_t version)`
   that first archives `cereal::base_class<ActionBase>(this)` then `CEREAL_NVP(each_)`
   (gate newer members in `load` with `if (version >= N)`).
   After the class: `REGISTER_ENEMY_ACTION_WITH_NAME(<Name>, "<Category>::<Name>")`
   (inside the namespace), then at global scope `CEREAL_CLASS_VERSION` (only if
   versioned), `CEREAL_REGISTER_TYPE(<fqn>)`,
   `CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, <fqn>)`.
   Copy `Content/Other/Sample/MoveFront/…SampleMoveFront.{h,cpp}` (no params) or
   `Content/Wait/Seconds/…WaitSeconds.{h,cpp}` (one param) as a starting point.

2. **`Enemy_Behaviour_Action_<Name>.cpp`**: `#include` its own header, implement
   `DoTick` / `DoDrawGui` in `namespace GameCore::Npc::Enemy::Behaviour` with an
   `Action::` qualifier. `stdafx.h` is force-included by the project — no PCH line needed.

3. **Three wiring points** (no code-gen / globbing exists):
   * `Assets/Scripts/Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionHeaders.h`
     — add one `#include "../../../../../Core/Game/Npc/Enemy/Behaviour/Action/Content/<Category>/<Name>/Enemy_Behaviour_Action_<Name>.h"`.
   * `NanamiEngine.vcxproj` — add `<ClCompile Include="Assets\Scripts\…\<Name>.cpp" />`
     and `<ClInclude Include="Assets\Scripts\…\<Name>.h" />` (bare entries inherit the
     cereal include dirs + `/bigobj` + forced `stdafx.h` from the `Debug|x64`
     `ItemDefinitionGroup`). **Mandatory.**
   * `NanamiEngine.vcxproj.filters` — mirror the two entries with `<Filter>Source Files</Filter>`
     / `<Filter>Header Files</Filter>`. Optional (Solution Explorer only).

`.h`/`.cpp` in this repo are **UTF-8 with a BOM** (see CLAUDE.md) - `add-action`
writes that by default (`--encoding utf-8-sig`); no post-processing needed.

---

## 5. Verifying changes end-to-end

1. `python tools/bt/selftest.py` — cereal-JSON formatting fidelity, byte-identical
   round-trip of the 4 committed `*.enemyBehaviourData` **and** the 4 committed
   `*.friendBehaviourData` (incl. bookkeeping order - one, `ActionInstructure`,
   is checked against the one documented known-limitation divergence instead of
   strict byte-identity), `.meta` round-trip (both flavors), `new-tree` == `T-Rex`
   shape, catalog freshness (both `catalog.json` and `catalog_friendly.json`),
   edit-then-inverse == original (both flavors), `add-action --dry-run` planning
   (both flavors).
2. Make a scratch tree (`new-tree`, `--npc-kind friendly` too): add a couple of
   action nodes referencing existing actions, `set-params`, `validate`, `show`.
3. For a new action: `add-action … --dry-run`, then for real, inspect `git diff`,
   build with the MSBuild line above, confirm the new TU compiles, then
   `remove-action` and rebuild. Repeat with `--npc-kind friendly` when adding a
   FriendlyNpc action.
4. Optional: point a scratch enemy prefab's `behaviourData_` (or friendly NPC
   prefab's `friendlyNpcBehaviourFile_`) GUID at a generated tree, launch the
   editor, confirm it loads and ticks.
