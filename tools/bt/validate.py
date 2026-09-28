"""デコード済みの :class:`tools.bt.model.Tree` に対する静的チェック。"""

from __future__ import annotations

from . import catalog as catalog_mod
from . import model
from .blob import Ptr, Ver
from .cereal_json import Num, OrderedObj


def validate(tree: model.Tree, cat: catalog_mod.Catalog | None = None) -> list[str]:
    cat = cat or catalog_mod.load()
    problems: list[str] = []

    def err(msg: str) -> None:
        problems.append(msg)

    seen: set[str] = set()

    def visit(node) -> None:
        if node.guid in seen:
            err(f"duplicate node guid {node.guid}")
        seen.add(node.guid)

        if isinstance(node, model.RandomSelector):
            if len(node.weights) != len(node.children):
                err(f"RandomSelector {node.guid}: {len(node.weights)} weights "
                    f"for {len(node.children)} children")
            if node.children and not any(w > 0 for w in node.weights):
                err(f"RandomSelector {node.guid}: all weights are 0")
        if isinstance(node, (model.OnceExecute, model.OnceSuccess)):
            pass  # 子が1つであることはモデルが保証
        if isinstance(node, model.BlackBoardGate):
            if node.child is None and not node.conditions:
                err(f"note: BlackBoardGate {node.guid}: no child and no conditions (always Success)")
            for k, _v in node.conditions + node.writes_on_start + node.writes_on_success:
                if not k:
                    err(f"BlackBoardGate {node.guid}: empty keyName_")
        if isinstance(node, model.Action):
            _check_action(node, cat, err)
        for c in model.children_of(node):
            visit(c)

    if tree.entry.child is None:
        # 空のツリーは有効（T-Rex と同じ）。ただし注記は出す
        problems.append("note: tree has no root node (entryNode_.nextNode_ is null)")
    else:
        visit(tree.entry.child)

    for root in tree.detached:
        problems.append(f"note: detached subtree {root.guid} ({type(root).__name__}) is not "
                        f"connected to the entry and never runs")
        visit(root)

    for p in tree.params:
        if p.kind != "int":
            err(f"blackboard param {p.name!r}: kind {p.kind!r} unsupported (int only)")

    return problems


def _is_older_version(version: int, entry: dict, want: list[str], got: list[str]) -> bool:
    """古いバージョンで保存されたファイル: 後から足したメンバー（load の version 分岐）が末尾に無いだけ。"""
    return int(version) < int(entry.get("version", 0)) and want[:len(got)] == got


def _check_cues(owner_guid: str, cues, cat: catalog_mod.Catalog, err) -> None:
    """ActionTimeline の cues_ に入った action を ActionNode 直下と同じく照合する。"""
    for i, cue in enumerate(cues):
        ptr = cue.get("action_") if isinstance(cue, OrderedObj) else None
        if not isinstance(ptr, Ptr) or ptr.null:
            err(f"note: action {owner_guid}: cue #{i} has no action")
            continue
        entry = cat.action_by_fqn(ptr.fqn or "")
        if entry is None:
            err(f"action {owner_guid}: cue #{i} unknown type {ptr.fqn!r}")
            continue
        body = ptr.data.body if isinstance(ptr.data, Ver) else OrderedObj()
        want = [p["key"] for p in cat.params_of(entry)]
        got = [k for k in body.keys() if k != "value0"]
        version = ptr.data.version if isinstance(ptr.data, Ver) else 0
        if want != got and not _is_older_version(version, entry, want, got):
            err(f"action {owner_guid}: cue #{i} ({entry['class']}): param keys {got} != catalog {want}")
        if "cues_" in body.keys():
            _check_cues(owner_guid, body["cues_"], cat, err)


def _check_action(node: model.Action, cat: catalog_mod.Catalog, err) -> None:
    entry = cat.action_by_fqn(node.type_fqn)
    if entry is None:
        err(f"action {node.guid}: unknown type {node.type_fqn!r} "
            f"(not in catalog - run regen-catalog?)")
        return
    if node.params is None:
        err(f"action {node.guid} ({node.type_name}): no params blob")
        return
    want = [p["key"] for p in cat.params_of(entry)]
    got = [k for k in node.params.keys() if k != "value0"]
    if want != got:
        if _is_older_version(node.action_version, entry, want, got):
            err(f"note: action {node.guid} ({node.type_name}): v{node.action_version} file lacks "
                f"{want[len(got):]} (catalog v{entry.get('version', 0)}); loads with defaults")
        else:
            err(f"action {node.guid} ({node.type_name}): param keys {got} != catalog {want}")
    if "cues_" in node.params.keys():
        _check_cues(node.guid, node.params["cues_"], cat, err)
    for p in cat.params_of(entry):
        if p["shape"] == "unknown":
            # 既知の v1 の制限（docs/BehaviourTree.md 参照）で、対処すべき欠陥ではない:
            # パラメータは無損失でラウンドトリップし、set-params で設定できないだけ。
            # "note:" のままにすること - ハードエラーにすると、ツリー内のどこかでそのような
            # パラメータを使うファイルでは、すべての CLI 編集コマンド（add-node/set-params/apply
            # はいずれも `validate` のハードな問題で中断する）が恒久的に使えなくなり、
            # このツールでは解決できなくなる。
            err(f"note: action {node.guid} ({node.type_name}): param {p['member']} has "
                f"unknown shape (type {p.get('type')!r}); round-trips but not settable")
