# `tools/animtree` — AnimationTree toolkit

Stdlib-only Python 3. Lets you create & edit `Assets/Animations/*.animTree` (the
cereal-JSON animation state graphs) **without** running the engine or the ImGui
graph editor.

Run from the repo root:

```
python -m tools.animtree <command>        # or: python tools/animtree.py <command>
```

## Commands

| command | purpose |
|---|---|
| `selftest` | correctness gate — run after touching `reader.py` / `writer.py` / `model.py` or anything under `tools/common/` |
| `new-tree NAME` | create `NAME.animTree` (Entry + AnyState only) + `.meta` (fresh GUID) |
| `show FILE` | print a tree as a readable outline |
| `validate FILE` | check every node/transition/condition/parameter against the catalog |
| `add-clip-node` / `remove-node` / `set-node-params` / `move-node` | node edits |
| `add-transition` / `remove-transition` / `set-transition-params` | transition edits (positional or `--from`/`--next` addressing — transitions have no identity guid) |
| `add-condition` / `remove-condition` | edit a transition's AND-group of equality guards |
| `add-param` / `remove-param` / `set-param` | edit the `additionParameters_` list (bool/int/float) |
| `apply FILE OPS.json` | apply a batch of edit ops atomically (primary agent interface) |
| `regen-catalog [--check]` | rebuild `catalog.json` from the `IAnimationNode` headers |

See `docs/AnimationTree.md` for the file format, the bookkeeping conventions, and
end-to-end workflows.

## Known limitations (v1)

* Only "pure tree" archives (no shared/back-referenced nodes) are supported — the
  reader raises `PureTreeError` otherwise. Every real `.animTree` file the engine
  writes is one.
* No cross-node auto-layout (unlike `tools/bt`'s recursive tree layout — an
  AnimationTree is a general graph with no parent/child structure to recurse
  over). `add-clip-node` places a new node in a deterministic grid slot unless
  `--pos` is given; `move-node --pos` is the only way to reposition an existing
  one.
* `nodes_N`'s array order carries no semantic meaning (the engine stores nodes in
  a `std::unordered_map`) — don't read anything into it in a diff.
* No `add-node-type` scaffold: the in-engine "Add AnimationClipNode" affordance is
  a hardcoded ImGui menu branch inside `AnimationTree.cpp` itself, not a
  peripheral generated-content file the way BehaviourTree Actions are — see
  `docs/AnimationTree.md` §4 for the by-hand anatomy of a new `IAnimationNode`
  subtype.
* No byte-exact `new-tree` reference fixture exists (both committed `.animTree`
  fixtures are non-empty real trees) — `selftest` instead checks the empty
  tree's top-level key order, internal formatting stability, and that it
  validates clean.

## After changing generated C++ or the emitter

* Always run `python tools/animtree/selftest.py` after editing `reader.py`,
  `writer.py`, `model.py`, or anything under `tools/common/` (shared with
  `tools/bt`/`tools/scene`).
* If you add a new `IAnimationNode` subclass, run `python -m tools.animtree
  regen-catalog` and commit `catalog.json`.
