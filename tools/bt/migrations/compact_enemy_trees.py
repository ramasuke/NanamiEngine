"""敵 BehaviourTree を汎用ノード（BlackBoardGate / ActionTimeline / RandomWriteBlackBoard /
PlayerAngleDispatch、PlayAnimation.holdSeconds_）でまとめ直す。

    python -m tools.bt.migrations.compact_enemy_trees [--dry-run] [FILE ...]

FILE を省くと Assets/Data/EnemyBehaviour/*.enemyBehaviourData 全部。パス（順に適用）:

1. simplify     子 1 個の Sequence / Selector を子で置き換える
2. angle        Selector[Seq[ToPlayerAngle, Write K]..., Write K] -> PlayerAngleDispatch
3. random-write RandomSelector[Write K...] -> RandomWriteBlackBoard
4. timeline     Sequence 末尾の「すぐ終わる action + WaitSeconds」の並びを ActionTimeline に
                （[PlayAnimation, Wait] だけなら PlayAnimation.holdSeconds_）。OnceExecute の中身も展開する
5. once         OnceExecute[ActionTimeline] -> ActionTimeline.once_
6. gate         Sequence 先頭の ReadBlackBoard<Int> / WriteBlackBoard<Int> と末尾の WriteBlackBoard<Int>
                -> BlackBoardGate
7. simplify     もう一度

Timeline に入れた action は Sequence の再 Tick を再現するため keepTicking_ を立てる。ただし単発演出
（ONE_SHOT）は 1 回だけ実行する（元は Sequence が後ろの Wait を待つ間、毎フレーム鳴り直していた）。
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass, field
from pathlib import Path

from .. import catalog as catalog_mod
from .. import compose, model, reader, validate, writer
from ..blob import Ver
from ..cereal_json import Num, OrderedObj, read_text

REPO = Path(__file__).resolve().parents[3]
TREE_DIR = REPO / "Assets/Data/EnemyBehaviour"

WAIT = "WaitSeconds"
READ_INT = "ReadBlackBoard"
WRITE_INT = "WriteBlackBoard"
PLAY_ANIM = "PlayAnimation"
TO_PLAYER_ANGLE = "ToPlayerAngle"
TIMELINE = "ActionTimeline"

#: Running を返さない（すぐ Success / Failure が決まる）、条件判定ではない action
INSTANT = {
    "PlayAnimation", "PlaySE", "PlayBGM", "ShakeCamera", "Lightning", "SetStorm",
    "GenerateParticle", "AttachParticle", "RadiateProjectile", "SwapModel", "TremblePillars",
    "SetLinearVelocity", "ChangeIsGravity", "ChangeColliderEmotionType",
    "WriteBlackBoard", "WriteBlackBoardBool", "PurposeCamera", "ScenePurposeCamera",
    "LockPlayerCannon", "UnlockPlayerCannon", "LockPlayerControl", "UnlockPlayerControl",
    "ShowBossHealthGauge", "BossSandstorm", "ScatterFloatingStones", "FallIsland",
    "ShootDownAirShip", "ChangeToMainIslandScene", "StartChat", "OnDeath",
}
#: 何度 Tick しても 1 回分の効果しかない（または 1 回だけにしたい）単発演出
ONE_SHOT = {
    "PlaySE", "PlayBGM", "ShakeCamera", "Lightning", "GenerateParticle", "AttachParticle",
    "RadiateProjectile", "SwapModel", "TremblePillars", "StartChat", "ChangeToMainIslandScene",
    "ScatterFloatingStones", "FallIsland", "ShootDownAirShip",
}
#: 完了した後にもう一度 Tick されてもすぐ Success を返す、Running を返す action。
#: NOTE: ChargeRush は柱を崩した後なら即 Success だが、外したらもう一度突進するので入れない
RELATCHING: set[str] = set()
#: 副作用の無い条件判定（Sequence の中で順番を入れ替えても結果が同じ）
PURE_CONDITIONS = {
    "ReadBlackBoard", "ReadBlackBoardBool", "IsTargetInAttackArea", "IsOnDamage",
    "IsCurrentHealthRate", "ToPlayerDistance", "ToPlayerAngle", "ToPlayerRaycast", "IsBossSandstorm",
}


@dataclass
class Stats:
    before: int = 0
    after: int = 0
    applied: dict[str, int] = field(default_factory=dict)

    def hit(self, name: str, n: int = 1) -> None:
        self.applied[name] = self.applied.get(name, 0) + n


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------
def leaf(node) -> str:
    return node.type_fqn.rsplit("::", 1)[-1] if isinstance(node, model.Action) else ""


def num(v) -> float:
    return v.value if isinstance(v, Num) else v


def count_nodes(tree: model.Tree) -> int:
    n = sum(1 for _ in tree.walk())
    for root in tree.detached:
        stack = [root]
        while stack:
            x = stack.pop()
            n += 1
            stack.extend(model.children_of(x))
    return n


def roots(tree: model.Tree) -> list:
    """書き換えの起点（entry 直下と浮きノード）。"""
    return ([tree.entry.child] if tree.entry.child is not None else []) + list(tree.detached)


def rewrite(tree: model.Tree, fn) -> None:
    """全ノードを子から先に（post-order）fn に通し、返ったノードで置き換える。"""
    def visit(node):
        if isinstance(node, model.MULTI_CHILD):
            node.children = [visit(c) for c in node.children]
        elif isinstance(node, model.SINGLE_CHILD) and node.child is not None:
            node.child = visit(node.child)
        return fn(node)

    if tree.entry.child is not None:
        tree.entry.child = visit(tree.entry.child)
    tree.detached = [visit(r) for r in tree.detached]


def bb(node, kind: str):
    """ReadBlackBoard<Int> なら (key, equalValue)、WriteBlackBoard<Int> なら (key, value)。"""
    if leaf(node) != kind:
        return None
    p = node.params
    if kind == READ_INT:
        return p["keyName_"], int(num(p["equalValue_"]))
    return p["keyName_"], int(num(p["value_"]))


def upgrade_play_animation(params: OrderedObj) -> None:
    if "holdSeconds_" not in params.keys():
        params.append("holdSeconds_", Num.of_float(0.0))


def upgrade_all_play_animations(tree: model.Tree, cat) -> None:
    """PlayAnimation を v3（holdSeconds_ 付き）に揃える。バージョンは型ごとに 1 回しか書かれないので混在させない。"""
    version = int(cat.resolve_action("EnemyStatus::PlayAnimation")["version"])

    def fix_cues(params) -> None:
        if params is None or "cues_" not in params.keys():
            return
        for c in params["cues_"]:
            ptr = c["action_"]
            if ptr.null or not isinstance(ptr.data, Ver):
                continue
            if ptr.fqn.endswith("::" + PLAY_ANIM):
                upgrade_play_animation(ptr.data.body)
                ptr.data.version = version
            fix_cues(ptr.data.body)

    def fn(node):
        if leaf(node) == PLAY_ANIM:
            upgrade_play_animation(node.params)
            node.action_version = version
        if isinstance(node, model.Action):
            fix_cues(node.params)
        return node

    rewrite(tree, fn)


# ---------------------------------------------------------------------------
# passes
# ---------------------------------------------------------------------------
def pass_simplify(tree: model.Tree, st: Stats) -> None:
    def fn(node):
        if isinstance(node, (model.Sequence, model.Selector)) and len(node.children) == 1:
            st.hit("simplify")
            return node.children[0]
        return node
    rewrite(tree, fn)


def pass_angle(tree: model.Tree, cat, st: Stats) -> None:
    def fn(node):
        if not isinstance(node, model.Selector) or len(node.children) < 2:
            return node
        key = None
        ranges = []
        fallback = None
        for i, c in enumerate(node.children):
            if isinstance(c, model.Sequence) and len(c.children) == 2 and leaf(c.children[0]) == TO_PLAYER_ANGLE:
                w = bb(c.children[1], WRITE_INT)
                if w is None or (key is not None and w[0] != key):
                    return node
                key = w[0]
                p = c.children[0].params
                ranges.append((num(p["minDegree_"]), num(p["maxDegree_"]), bool(p["useAbsolute_"]), w[1]))
            elif i == len(node.children) - 1 and bb(c, WRITE_INT) and (key is None or bb(c, WRITE_INT)[0] == key):
                fallback = bb(c, WRITE_INT)[1]
            else:
                return node
        if not ranges or key is None:
            return node
        st.hit("angle-dispatch")
        return compose.angle_dispatch(cat, key, ranges, fallback=fallback, pos=node.pos)
    rewrite(tree, fn)


def pass_random_write(tree: model.Tree, cat, st: Stats) -> None:
    def fn(node):
        if not isinstance(node, model.RandomSelector) or len(node.children) < 2:
            return node
        writes = [bb(c, WRITE_INT) for c in node.children]
        if any(w is None for w in writes) or len({w[0] for w in writes}) != 1:
            return node
        st.hit("random-write")
        return compose.random_write(cat, writes[0][0], [(w[1], wt) for w, wt in zip(writes, node.weights)],
                                    pos=node.pos)
    rewrite(tree, fn)


def _flatten(items, in_once: bool):
    """Timeline にできる並びなら [(kind, payload, in_once)] を返す。kind は 'wait' / 'act'。できなければ None。"""
    out = []
    for it in items:
        if leaf(it) == WAIT:
            out.append(("wait", float(num(it.params["waitSeconds_"])), in_once))
        elif leaf(it) in INSTANT:
            out.append(("act", it, in_once))
        elif leaf(it) == TIMELINE and not it.params["once_"] \
                and not any(c["waitDone_"] for c in it.params["cues_"]):
            # 先に変換した内側の Timeline（OnceExecute の中の Sequence など）は Cue ごと取り込む
            out.append(("cues", it, in_once))
        elif isinstance(it, model.OnceExecute) and it.child is not None:
            inner = it.child.children if isinstance(it.child, model.Sequence) else [it.child]
            sub = _flatten(inner, True)
            if sub is None:
                return None
            out.extend(sub)
        else:
            return None
    return out


def _build_timeline(cat, flat, last=None, *, pos) -> model.Action:
    cues = []
    cursor = 0.0
    for kind, payload, in_once in flat:
        if kind == "wait":
            cursor += payload
            continue
        if kind == "cues":
            for c in payload.params["cues_"]:
                shifted = OrderedObj(c.items())
                shifted["at_secs_"] = Num.of_float(round(cursor + float(num(c["at_secs_"])), 4))
                shifted["keepTicking_"] = bool(c["keepTicking_"]) and not in_once
                cues.append(shifted)
            cursor += float(num(payload.params["duration_secs_"]))
            continue
        keep = not in_once and leaf(payload) not in ONE_SHOT
        cues.append(compose.cue(cursor, payload, keep_ticking=keep))
    duration = cursor
    if last is not None:
        cues.append(compose.cue(cursor, last, wait_done=True))
    return compose.timeline(cat, cues, duration_secs=round(duration, 4),
                            fail_on_child_failure=True, pos=pos)


def pass_timeline(tree: model.Tree, cat, st: Stats) -> None:
    def fn(node):
        if not isinstance(node, model.Sequence):
            return node
        kids = node.children
        # 末尾の WriteBlackBoard<Int> は gate パスで成功時の書き込みにするので残す
        end = len(kids)
        while end > 0 and bb(kids[end - 1], WRITE_INT):
            end -= 1

        out = []
        i = 0
        changed = False
        while i < end:
            # WriteBlackBoard<Int> から始めない（先頭なら gate の開始時の書き込みになる）
            if bb(kids[i], WRITE_INT) or _flatten([kids[i]], False) is None:
                out.append(kids[i])
                i += 1
                continue
            j = i
            while j < end and _flatten([kids[j]], False) is not None:
                j += 1
            # 並びの直後の 1 個は、Running を返す action でも waitDone_ の Cue として入れられる。
            # Sequence は前の子を毎フレーム Tick し直すので、途中なら再 Tick で即 Success に戻るものだけ、
            # 末尾なら条件判定以外なら何でもよい
            last = None
            if j < end and isinstance(kids[j], model.Action) and leaf(kids[j]) not in PURE_CONDITIONS \
                    and leaf(kids[j]) != WAIT and (j == end - 1 or leaf(kids[j]) in RELATCHING):
                last = kids[j]
            run = kids[i:j]
            flat = _flatten(run, False)
            if not any(k in ("wait", "cues") for k, *_ in flat) or len(run) + (last is not None) < 2:
                out.extend(run)
                i = j
                continue

            if last is None and len(run) == 2 and leaf(run[0]) == PLAY_ANIM and leaf(run[1]) == WAIT:
                # [PlayAnimation, Wait] だけなら PlayAnimation に待ち時間を持たせる
                anim = run[0]
                upgrade_play_animation(anim.params)
                anim.params["holdSeconds_"] = Num.of_float(float(num(run[1].params["waitSeconds_"])))
                st.hit("play-animation-hold")
                out.append(anim)
            else:
                out.append(_build_timeline(cat, flat, last, pos=run[0].pos))
                st.hit("timeline")
            changed = True
            i = j + (1 if last is not None else 0)
        if not changed:
            return node
        node.children = out + kids[end:]
        return node.children[0] if len(node.children) == 1 else node
    rewrite(tree, fn)


def pass_once(tree: model.Tree, st: Stats) -> None:
    def fn(node):
        if isinstance(node, model.OnceExecute) and leaf(node.child) == TIMELINE:
            node.child.params["once_"] = True
            st.hit("once")
            return node.child
        return node
    rewrite(tree, fn)


def pass_gate(tree: model.Tree, st: Stats) -> None:
    def fn(node):
        if not isinstance(node, model.Sequence):
            return node
        kids = list(node.children)
        # 副作用のない条件が先頭に並んでいる間は、その中の ReadBlackBoard<Int> を前に出しても結果は変わらない
        reads = []
        i = 0
        while i < len(kids) and leaf(kids[i]) in PURE_CONDITIONS:
            r = bb(kids[i], READ_INT)
            if r is not None:
                reads.append(r)
                kids.pop(i)
            else:
                i += 1
        starts = []
        while len(kids) > 1 and bb(kids[0], WRITE_INT):
            starts.append(bb(kids.pop(0), WRITE_INT))
        ends = []
        while kids and bb(kids[-1], WRITE_INT):
            ends.insert(0, bb(kids.pop(), WRITE_INT))
        # 書き込みだけで中身が無いなら、先頭側に回した分を戻す（成功時に書く）
        if not kids and starts:
            ends = starts + ends
            starts = []
        saved = len(reads) + len(starts) + len(ends) - (1 if len(kids) > 1 else 0)
        if not (reads or starts or ends) or saved < 1:
            return node
        if not kids:
            child = None
        elif len(kids) == 1:
            child = kids[0]
        else:
            node.children = kids
            child = node
        st.hit("gate")
        return compose.gate(child, conditions=reads, writes_on_start=starts, writes_on_success=ends,
                            pos=node.pos)
    rewrite(tree, fn)


# ---------------------------------------------------------------------------
def migrate(tree: model.Tree, cat) -> Stats:
    st = Stats(before=count_nodes(tree))
    upgrade_all_play_animations(tree, cat)
    pass_simplify(tree, st)
    pass_angle(tree, cat, st)
    pass_random_write(tree, cat, st)
    pass_timeline(tree, cat, st)
    pass_once(tree, st)
    pass_gate(tree, st)
    pass_simplify(tree, st)
    st.after = count_nodes(tree)
    return st


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="*")
    ap.add_argument("--dry-run", action="store_true", help="report only, write nothing")
    ap.add_argument("--out-dir", help="write the results here instead of over the originals")
    args = ap.parse_args(argv)

    cat = catalog_mod.load(kind="enemy")
    files = [Path(f) for f in args.files] or sorted(TREE_DIR.glob("*.enemyBehaviourData"))
    total_before = total_after = 0
    failed = False
    for path in files:
        tree = reader.read_tree(read_text(path), cat=cat, kind="enemy")
        st = migrate(tree, cat)
        problems = [p for p in validate.validate(tree, cat) if not p.startswith("note:")]
        total_before += st.before
        total_after += st.after
        applied = ", ".join(f"{k}={v}" for k, v in sorted(st.applied.items())) or "-"
        print(f"{path.name:40s} {st.before:4d} -> {st.after:4d}   {applied}")
        for p in problems:
            print(f"    ERROR {p}")
            failed = True
        if problems or args.dry_run:
            continue
        text = writer.write_tree(tree)
        # 書いたものがそのまま読み戻せることを確認してから保存する
        if writer.write_tree(reader.read_tree(text, cat=cat, kind="enemy")) != text:
            print("    ERROR output is not byte-stable on re-read; not written")
            failed = True
            continue
        out = Path(args.out_dir) / path.name if args.out_dir else path
        from ..cereal_json import to_file_bytes
        out.write_bytes(to_file_bytes(text))
    print(f"{'total':40s} {total_before:4d} -> {total_after:4d}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
