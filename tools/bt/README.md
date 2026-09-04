# `tools/bt` — enemy BehaviourTree / Action toolkit

Stdlib-only Python 3. Lets you create & edit `Assets/Data/EnemyBehaviour/*.enemyBehaviourData`
(the cereal-JSON behaviour trees) and scaffold + wire a new enemy behaviour Action, **without**
running the engine or the ImGui graph editor.

Run from the repo root:

```
python -m tools.bt <command>        # or: python tools/bt.py <command>
```

## Commands

| command | purpose |
|---|---|
| `selftest` | correctness gate — run after touching `cereal_json.py` / `reader.py` / `writer.py` |
| `new-tree NAME` | create `NAME.enemyBehaviourData` (empty tree) + `.meta` (fresh GUID) |
| `show FILE` | print a tree as a readable outline |
| `validate FILE` | check every node/action against the catalog |
| `add-node` / `remove-node` / `move-node` | structural edits (auto-arrange the tree unless `--no-layout`) |
| `layout` | re-arrange every node into a tidy top-down tree (`--dx` / `--dy` gaps) |
| `set-params` / `set-weight` | edit an action's params / a RandomSelector weight (positions untouched) |
| `add-bb-param` / `remove-bb-param` | edit the blackboard parameter list |
| `apply FILE OPS.json` | apply a batch of edit ops atomically (primary agent interface) |
| `add-action` / `remove-action` | scaffold / unscaffold a C++ Action and its wiring |
| `regen-catalog [--check]` | rebuild `catalog.json` from the Action headers |

See `docs/BehaviourTree.md` for the file formats, the Action C++ anatomy, the three build-wiring
points, and end-to-end workflows.

## Known limitations (v1)

* Only "pure tree" data files (no shared / back-referenced nodes) are supported. The reader raises
  `PureTreeError` otherwise.
* Blackboard parameters: `AnimationParameter<int>` only.
* Action params of shape `nested:*` (e.g. an embedded `WaitSeconds` / `PlaySE` / `Position`) and
  `unknown` are **not settable** via `set-params`; they round-trip losslessly but must be edited in
  the ImGui editor or by hand. Everything else in the same action still edits normally.
* `OnceExecute` / `OnceSuccess` writer support is implemented from source but no committed fixture
  exercises it.

## After changing generated C++ or the emitter

* `.h` / `.cpp` in this repo are **Shift-JIS (CP932)**. `add-action` writes them as CP932 directly
  (ASCII-only templates). After any manual non-ASCII edit run `Scripts/convDx.ps1 <file>`.
* Always run `python tools/bt/selftest.py` after editing `cereal_json.py`, `reader.py`, `writer.py`
  or `model.py`.
* If you add/rename an Action, run `python -m tools.bt regen-catalog` and commit `catalog.json`.
