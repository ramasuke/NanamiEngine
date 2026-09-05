# AnimationTree: file formats, and `tools/animtree`

This document lets a contributor (human or agent) **create / edit an
AnimationTree** without opening the in-engine ImGui graph editor. The
`tools/animtree` Python toolkit automates this; the format notes below also
let you hand-edit safely.

---

## 1. Where things live

| thing | path |
|---|---|
| tree data files | `Assets/Animations/*.animTree` (+ `*.meta`) |
| runtime tree object | `Engine/Module/AnimationTree/AnimationTree.{h,cpp}` |
| asset wrapper (thin proxy) | `Engine/Module/Asset/AnimationTree/AnimationTreeFile.{h,cpp}` |
| node types (`IAnimationNode` subclasses) | `Engine/Module/AnimationTree/Node/**` |
| transitions / conditions | `Engine/Module/AnimationTree/NodePath/**` |
| controller parameters (`additionParameters_`) | `Libs/LibCore/BlackBoard/{AnimationParameter,IAnimationParameter}.h`, `Libs/LibCore/BlackBoard/Group/ParameterGroup.h` |
| in-engine graph editor | `Engine/Core/Application/Window/Main/Animator/AnimatorWindow.{h,cpp}`, `AnimationTree::OnDrawGraphEditorGui` |
| the toolkit | `tools/animtree/` — `python -m tools.animtree <cmd>` |

A GameObject/Prefab binds a tree through the `Animator` component's
`FIELD(Asset::AnimationTreeFile) animationTreeFile_` — already reachable via
`tools/scene`:

```
python -m tools.scene add-component --guid <gameobject-guid> --type Animator --param animationTreeFile_=<tree-guid>
```

(`tools/scene/catalog.json`'s `Animator` entry only models `animationTreeFile_`
today, not the version-2+ `animationSyncs_` field — an existing gap in
`tools/scene`, not in scope here.)

---

## 2. `.animTree` format

Output of `cereal::JSONOutputArchive`, hand-rolled in `AnimationTree::OnSave`
(this class does **not** use cereal's own `serialize`/`save`/`load` dispatch —
every top-level key is written by an explicit `archive(cereal::make_nvp(...))`
call). Six top-level keys, in this exact order:

1. `additionParameters_` — `shared_ptr<BlackBoard::ParameterGroup>`, the
   controller's bool/int/float parameters.
2. `entryNode` — `shared_ptr<AnimatorEntryNode>`, the graph's single fixed
   entry point.
3. `visualAnyStateNode` — `shared_ptr<AnimationVisualAnyStateNode>`, the fixed
   "any state" node (a transition sourced from here fires regardless of the
   currently active node).
4. `nodesCount` + `nodes_0..N-1` — `shared_ptr<IAnimationNode>` (polymorphic).
   Only `AnimationClipNode` exists today. **This array's order carries no
   semantic meaning** — the engine holds `nodes_` in a
   `std::unordered_map<Guid, ...>`; hash-bucket iteration order at save time,
   not insertion order, decides it.
5. `fromNodeNodePathCount` + `fromNodeNodePath_0..N-1` — direct transitions.
6. `fromAnyStateNodeNodePathCount` + `fromAnyStateNodeNodePath_0..N-1` —
   any-state transitions.

On disk: UTF-8, **no BOM**, **CRLF**, no trailing newline, 4-space indent.

### cereal bookkeeping you will see everywhere

| construct | meaning |
|---|---|
| `"polymorphic_id": 0` | null polymorphic pointer |
| `"polymorphic_id": 1073741824` (`0x40000000`) | pointer whose dynamic type == static type (no type name needed) — every `entryNode`/`visualAnyStateNode`/transition slot uses this, since each is always the concrete type its field declares |
| `"polymorphic_id": 0x80000000\|N` + `"polymorphic_name"` | first time type *N* is written; later refs use just `"polymorphic_id": N` |
| `"ptr_wrapper": {"id": 0x80000000\|K, "data": …}` | a `shared_ptr` (K counts every shared_ptr **and** every `Field<T>` inner pointer, in write order, across the *whole file*) |
| `"ptr_wrapper": {"valid": 1, "data": …}` | a `unique_ptr` (`additionConditionGroup_`) |
| `"cereal_class_version": V` | written **once per type per file**, on that type's first instance; omitted afterwards. `BlackBoard::ParameterGroup` itself is the one exception — its `save`/`load` take no version argument, so it never emits this key at all. |
| nested `"value0"` | a serialised base class: `IAnimationNode` (empty stub) for every node, `IObject` for a transition, `IAnimationNodePathAdditionCondition`/`IAnimationParameter` (both empty stubs) for a condition/parameter |

### Node types

| type (`polymorphic_name`) | `CEREAL_CLASS_VERSION` | fixed / addable | fields (save() order) |
|---|---|---|---|
| `NanamiEngine::Module::AnimationTree::AnimatorEntryNode` | 0 | fixed singleton (`entryNode`) | `position_`, `guid_`, `speed_` (vestigial, unused for logic) |
| `NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode` | 0 | fixed singleton (`visualAnyStateNode`) | `position_`, `guid_` |
| `NanamiEngine::Module::AnimationTree::AnimationClipNode` | 2 | addable (`nodes_N`) | `animationFile_` (`FIELD(Asset::Mv1File)`), `name_`, `position_`, `guid_`, `speed_`, `blendAnimationOffset_secs_` (v≥1), `modelAnimationIndex_` (v≥2) |

`tools/animtree/catalog.json` (regenerable, `regen-catalog`) records this same
field order for every type, plus two sentinel shapes — `self_guid`/`self_pos`
— marking which key holds the node's identity guid / canvas position, since
every type interleaves them among its own named fields rather than sharing one
common base struct.

`blendAnimationOffset_secs_` is how many seconds before a clip's nominal end
an outgoing transition may start blending in
(`GetAnimDuration_secs() = duration_secs_ - blendAnimationOffset_secs_`) — not
an "exit time" in the Unity sense; there is no separate exit-time field or
flag anywhere in this format.

### Transitions (`AnimationNodePath`)

`CEREAL_CLASS_VERSION` 1. Fields, save() order: `additionConditionGroup_`
(`unique_ptr<AnimationNodePathAdditionConditionGroup>`), `transitionDuration_secs_`
(float — crossfade/blend length), `fromNodeGuid_`, `nextNodeGuid_`,
`visualFromNodeGuid_` (v≥1 — the editor-only "drawn from" node; equal to
`fromNodeGuid_` for a directly-authored transition). **A transition has no
identity guid of its own** — `AnimationNodePath::GetGuid()` is a stub bug that
always returns an empty guid — so `tools/animtree` addresses one positionally
(its index in `fromNodeNodePath_N`/`fromAnyStateNodeNodePath_N`) or by its
`(fromNodeGuid_, nextNodeGuid_)` pair.

A direct transition's `fromNodeGuid_` must never equal the AnyState node's
guid, and vice versa — that's exactly what routes it into
`fromNodeNodePath_N` vs. `fromAnyStateNodeNodePath_N`; `tools/animtree
validate` enforces this.

### Condition groups (`additionConditionGroup_`)

Hand-rolled `{count, cond1, cond2, ...}` (unnamed `archive(count); for(...) archive(condition)`
calls → `value0` = count, `value1..N` = each condition). An **empty** group
(`value0: 0`, no further keys) means an unconditional/"always" transition.
Each condition (`AnimationNodePathAdditionCondition<bool|int|float>`, fields
`name_` + `equalValue_`) does **equality only** — there is no `<`/`>`/`!=`
operator anywhere in this format. `name_` looks up a parameter by name in
`additionParameters_`.

### `additionParameters_`

`{ptr_wrapper: {id: K, data: {value0: <count>, value1: <param>, value2: <param>, ...}}}`
— note **no `polymorphic_id`** at this level (`ParameterGroup` has no virtual
base) and, per the bookkeeping table above, no `cereal_class_version` either.
Each `value{i}` is a normal polymorphic `shared_ptr<IAnimationParameter>`,
concrete type `AnimationParameter<bool|int|float>` (fields `name_`, `value_`).
Unlike `tools/bt`'s blackboard (self-restricted to `AnimationParameter<int>`
as a stated v1 limitation, not an engine constraint), `tools/animtree` supports
all three kinds from day one.

### `.meta`

`cereal::JSONOutputArchive` of a `shared_ptr<AssetBase>` = `AnimationTreeFile`
with `contentPath_` (backslash dirs, forward slash before the filename) and
the stable asset `guid_`. Created by `File::OnSave`. **Regenerate with the
tool, don't hand-write.**

---

## 3. `tools/animtree` — usage

Run from the repo root. `python tools/animtree.py <cmd>` is identical to
`python -m tools.animtree <cmd>`.

```
python -m tools.animtree selftest                    # correctness gate - run after touching the emitter

# --- create / inspect ---
python -m tools.animtree new-tree Wolf                # -> Assets/Animations/Wolf.animTree + .meta, prints the GUID
python -m tools.animtree show     Wolf                # readable outline
python -m tools.animtree validate Wolf

# --- node edits (all take --dry-run to preview a diff) ---
python -m tools.animtree add-clip-node    Wolf --name Idle --clip Assets/Art/Animation/Man/Idle.mv1 --speed 1.0
python -m tools.animtree add-clip-node    Wolf --name Walk --clip <mv1-asset-guid> --blend-offset 0.2
python -m tools.animtree set-node-params  Wolf --node <clip-guid> speed_=1.5 name_=WalkFast
python -m tools.animtree move-node        Wolf --node <clip-guid> --pos 480,120
python -m tools.animtree remove-node      Wolf --node <clip-guid> [--cascade]

# --- transition / condition edits ---
python -m tools.animtree add-transition       Wolf --from <entry-guid> --next <idle-guid>
python -m tools.animtree add-transition       Wolf --from <anystate-guid> --next <walk-guid> --any-state --duration 0.3
python -m tools.animtree add-condition        Wolf --any-state --from <anystate-guid> --next <walk-guid> --name State --kind int --value 1
python -m tools.animtree remove-condition     Wolf --any-state --index 0 --condition-index 0
python -m tools.animtree set-transition-params Wolf --index 0 --duration 0.5
python -m tools.animtree remove-transition    Wolf --index 0

# --- parameters ---
python -m tools.animtree add-param    Wolf --name State --kind int --value 0
python -m tools.animtree set-param    Wolf --name State --value 1
python -m tools.animtree remove-param Wolf --name State

# --- batch (recommended for agents: atomic, one write) ---
python -m tools.animtree apply Wolf ops.json     # ops.json = [{"op":"add-clip-node",...}, {"op":"add-transition",...}, ...]
```

**`--clip`** (on `add-clip-node`) accepts either a raw `Mv1File` asset GUID or
a path to a `.mv1`/`.mv1.meta` file (resolved to that asset's guid via its
`.meta` sidecar) — a convenience specific to this toolkit; neither
`tools/bt`/`tools/scene`'s own `field`-shaped CLI arguments resolve a path
today, they only accept a raw GUID.

**Transition addressing.** A transition has no identity guid (see §2), so
every transition-editing verb takes either `--index N` (from `show`'s
`[N]` prefix) or the endpoint pair `--from`/`--next`, plus `--any-state` to
pick which of the two lists (`fromNodeNodePath_N` vs.
`fromAnyStateNodeNodePath_N`) it lives in.

**`move-node`** repositions a node on the editor canvas only — unlike
`tools/bt`'s `move-node`, which *reparents* (this format has no parent/child
relation to reparent within; use `add-transition`/`remove-transition` to
change the graph's edges).

**Node positions.** `add-clip-node` places a new node in a deterministic grid
slot (`tools/animtree/layout.py`) unless `--pos X,Y` is given. There is **no**
`layout` verb that re-arranges an *existing* tree the way `tools/bt`'s does —
an AnimationTree is a general graph, not a tree, so there's no analogous
parent/child structure to lay out from; use `move-node --pos` for manual
control.

### Limitations (v1)

* "Pure tree" archives only (no shared/back-referenced nodes) — the reader
  raises `PureTreeError` otherwise. Every real `.animTree` the engine writes
  is one.
* No `add-node-type` scaffold — see §4.
* `nodes_N`'s array order carries no semantic meaning (§2) — don't read
  anything into it when reviewing a diff.
* No byte-exact `new-tree` reference fixture exists (both committed
  `.animTree` fixtures are non-empty real trees, unlike `tools/bt`'s empty
  `T-Rex.enemyBehaviourData`) — `selftest` instead checks the empty tree's
  top-level key order, internal formatting stability, and that it validates
  clean.

---

## 4. Adding a new `IAnimationNode` type

**No scaffold command exists for this in v1.** The in-engine "Add node"
affordance is a single hardcoded block inside
`AnimationTree::OnDrawGraphEditorGui()` in `AnimationTree.cpp` itself:

```cpp
if (ImGui::MenuItem("Add AnimationClipNode"))
{
    const auto newNode = std::make_shared<AnimationClipNode>(...);
    nodes_[newNode->GetGuid()] = newNode;
}
```

— engine core, not a peripheral generated-content file the way BehaviourTree's
`Enemy_Behaviour_ActionHeaders.h` aggregator is. `IAnimationNode`'s virtual
interface (`InitForGamePlay`, `OnUpdateBlendRate`, `OnUpdateAnimation`,
`OnExitNode`, `OnUpdated`, `Position`, `GetAnimDuration_secs`,
`OnDrawGraphEditorGui`) is also far more involved than BehaviourTree's
`ActionBase` (effectively just `DoTick`), so a generated stub would compile
but be non-functional in the graph editor — worse than "doesn't compile yet."
This mirrors `tools/scene`'s existing precedent of not having an
`add-component-type` scaffold either.

### By hand — the anatomy

1. A new `.h`/`.cpp` under `Engine/Module/AnimationTree/Node/<Name>/`:
   `class <Name> final : public IAnimationNode` implementing every pure
   virtual. Parameter members are `[[serialize(0)]]`-tagged, with hand-rolled
   `save`/`load` templates: first `archive(cereal::base_class<IAnimationNode>(this))`,
   then each `CEREAL_NVP(member_)` (gate newer members in `load` behind
   `if (version >= N)`), always including a `guid_` (Guid) and `position_`
   (glm::vec2) member somewhere in the list — `tools/animtree`'s catalog
   scanner keys off those two member *names* specifically (`self_guid`/
   `self_pos`), not their type, to find the node's identity/canvas-position
   fields. After the class, at file scope: `CEREAL_CLASS_VERSION`,
   `CEREAL_REGISTER_TYPE(<fqn>)`,
   `CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::AnimationTree::IAnimationNode, <fqn>)`.
   Copy `Node/ClipNode/AnimationClipNode.{h,cpp}` as a starting point.
2. `NanamiEngine.vcxproj` (+ `.vcxproj.filters`) — add `<ClCompile>`/
   `<ClInclude>` entries by hand (no toolkit command does this yet for
   AnimationTree — `tools/animtree/vcxproj.py` is a re-export shim kept for
   structural symmetry, unused in v1).
3. `AnimationTree.cpp`'s `OnDrawGraphEditorGui()` — add a matching
   `ImGui::MenuItem("Add <Name>")` block to the node-context-menu code shown
   above, constructing and inserting the new node the same way
   `AnimationClipNode` does.
4. `python -m tools.animtree regen-catalog` and commit `tools/animtree/catalog.json`
   so the toolkit can read/write the new type (`add-clip-node`-equivalent
   authoring for it is not generated automatically — extend `edits.py`/
   `cli_edit.py` the way `add_clip_node`/`add-clip-node` already do, if you
   want a dedicated CLI verb for it, or address it generically once a real
   node exists via `set-node-params`).

`.h`/`.cpp` in this repo are **Shift-JIS (CP932)** — after any manual
non-ASCII edit, run `Scripts/convDx.ps1 <file>`.

---

## 5. Verifying changes end-to-end

1. `python tools/animtree/selftest.py` — cereal-JSON formatting fidelity,
   byte-identical round-trip of both committed `*.animTree` fixtures (incl.
   bookkeeping order), `.meta` round-trip, new-tree shape sanity, catalog
   freshness, edit-then-inverse == original (nodes, transitions, conditions,
   params — all 3 parameter kinds), and `validate()` sanity checks.
2. Make a scratch tree: `new-tree`, `add-clip-node` a couple of times
   (referencing a real `.mv1.meta` under `Assets/Art/Animation/`), wire up an
   `add-transition`/`add-condition`, `add-param`, `show`, `validate`.
3. `apply` a small JSON batch and confirm it's atomic (a bad op in the middle
   aborts with nothing written).
4. Optional: point a scratch prefab's `Animator.animationTreeFile_` GUID at a
   generated tree (`tools/scene add-component --type Animator --param
   animationTreeFile_=<guid>`), launch the editor, confirm it loads in the
   `AnimatorWindow` graph editor without errors.
