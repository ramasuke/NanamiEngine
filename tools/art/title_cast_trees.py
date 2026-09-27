"""タイトル画面 (TitleScene) の船の上の人たちの AnimationTree を組み直す。

    python tools/art/title_cast_trees.py            # すべて
    python tools/art/title_cast_trees.py TitleSwordMan

ライズのタイトルのように、じっと立たせず仕草を順に流す。どれもクリップの終わりで次へ移るだけの一本道のループ
(条件のない遷移は1つのノードから1本しか出せないので、同じクリップでもノードを分けて置く)。
- TitleSwordMan: 主人公 (剣士)。手すりの前で 遠くを眺める -> 手をかざす -> 体重移動 -> 振り返る
- TitleKunoichi / TitleYoungMan: 甲板で向かい合って話す二人。話す番と聞く番 (うなずく・笑う) を互い違いに並べる
仕草は Mixamo からそのキャラ (Brute / Kachujin) に書き出した骨だけのクリップ。モデルとフレームの並びが違うので
nameCheck_ を入れて名前で当てる。飛行船の青年 (NaughtyYoung) は Mixamo のキャラが分からないので、序章と同じく
クノイチ (Kachujin) のクリップを使う。
"""
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
BLEND_SECS = 0.7

WOMAN = 'Assets/Art/Animation/WomanNpc'
BRUTE = 'Assets/BruteAnimation'
MAN = 'Assets/Art/Animation/Man/Title'

# ツリー名: [(ノード名, クリップ, speed_, nameCheck_)]。speed_ 30 が 30fps の書き出しの実時間
# (剣士の NeutralIdle は SwordManAnimation と同じ速さ。伸び (Arm Stretching) は斧が顔の前を横切るので使わない)
TREES = {
    'TitleSwordMan': [
        ('Idle', f'{BRUTE}/NeutralIdle', 55.0, False),
        ('LookDistance', f'{MAN}/TitleSM_LookDistanceL', 30.0, True),  # 右手に斧を持つので左右反転
        ('Idle2', f'{BRUTE}/NeutralIdle', 55.0, False),
        ('ShadeEyes', f'{MAN}/TitleSM_ShadeEyesL', 30.0, True),  # 右手に斧を持つので左右反転 (左手でかざす)
        ('WeightShift', f'{MAN}/TitleSM_WeightShift', 30.0, True),
        ('LookBehind', f'{MAN}/TitleSM_LookBehind', 30.0, True),
    ],
    'TitleKunoichi': [
        ('Conversation', f'{WOMAN}/TitleKN_Conversation', 30.0, True),
        ('Listen', f'{WOMAN}/Idle', 30.0, False),
        ('Laugh', f'{WOMAN}/TitleKN_Laugh', 30.0, True),
        ('Ask', f'{WOMAN}/TitleKN_Ask', 30.0, True),
        ('Agree', f'{WOMAN}/TitleKN_Agree', 30.0, True),
        ('Listen2', f'{WOMAN}/Idle', 30.0, False),
    ],
    'TitleYoungMan': [
        ('Listen', f'{WOMAN}/Idle', 30.0, False),
        ('Agree', f'{WOMAN}/TitleKN_Agree', 30.0, True),
        ('Funny', f'{WOMAN}/TitleKN_Funny', 30.0, True),
        ('Talk', f'{WOMAN}/Talking', 30.0, False),
        ('Laugh', f'{WOMAN}/TitleKN_Laugh', 30.0, True),
        ('Listen2', f'{WOMAN}/Idle', 30.0, False),
    ],
}


def run(*args):
    out = subprocess.run([sys.executable, '-m', 'tools.animtree', *args], cwd=REPO, capture_output=True,
                         text=True, encoding='utf-8', errors='replace')
    if out.returncode != 0:
        raise SystemExit(out.stdout + out.stderr)
    return out.stdout


def build(name, chain):
    tree = f'Assets/Animations/{name}.animTree'
    meta = REPO / (tree + '.meta')
    keep = REPO / (tree + '.meta.keep')
    if (REPO / tree).exists():
        (REPO / tree).unlink()
    if meta.exists():
        meta.rename(keep)
    run('new-tree', name)
    # NOTE: 作り直しても参照が切れないよう、前の .meta (guid) を戻す
    if keep.exists():
        meta.unlink()
        keep.rename(meta)

    entry = None
    for line in run('show', tree).splitlines():
        if line.startswith('Entry'):
            entry = line.split()[1]

    guids = []
    for i, (node, clip, speed, name_check) in enumerate(chain):
        out = run('add-clip-node', tree, '--name', node, '--clip', f'{clip}.mv1', '--speed', str(speed),
                  '--blend-offset', str(BLEND_SECS))
        guid = next(l.split(':', 1)[1].strip() for l in out.splitlines() if l.startswith('new AnimationClipNode'))
        if name_check:
            run('set-node-params', tree, '--node', guid, 'nameCheck_=true')
        run('move-node', tree, '--node', guid, '--pos', f'{400 + 180 * i},{40 + 90 * (i % 2)}')
        guids.append(guid)

    run('add-transition', tree, '--from', entry, '--next', guids[0], '--duration', '0')
    if len(guids) > 1:
        for a, b in zip(guids, guids[1:] + guids[:1]):
            run('add-transition', tree, '--from', a, '--next', b, '--duration', str(BLEND_SECS))
    print(f'{name}: {run("validate", tree).strip()}  ({len(guids)} nodes)')


def main():
    names = sys.argv[1:] or list(TREES)
    for name in names:
        build(name, TREES[name])


if __name__ == '__main__':
    main()
