"""序章でドラゴンが飛んで来るまでの音の演出を FirstEventDragon の BT に組む。

    python tools/art/dragon_omen_bt.py        # 何度流してもよい (足したノードがあれば何もしない)

音は tools/art/dragon_omen_sfx.py で作る。流れ (State0 → State10):
  訓練が終わってドラゴンが呼ばれる → のどかな BGM を 4 秒で下げて無音 (嵐の音だけ) → 雲の向こうの遠吠え → 警鐘
  → 不穏な BGM が上がってくる → 飛行船に火球が当たった瞬間に BGM を切る → 船が墜ちたあと、急降下で BGM が戻る
  → 着地の前に BGM を下げる → 着地・静けさ → 咆哮と同時に戦闘 BGM (Play FightBGM を咆哮の枝へ移す)

- ToDestroyAirShip (MoveEventRoute) は ActionTimeline "Omen Approach" の Cue に入れ、飛んでいる間に音を時刻で鳴らす。
- 足すノードの GUID は固定。tools/art/ancient_dragon.py は序章の BT を写して State0 を作り替えるので、これらも消す。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.bt import cli_edit, compose, edits, model  # noqa: E402
from tools.bt.layout import auto_layout  # noqa: E402
from tools.bt.validate import validate  # noqa: E402

from camp_people import asset_guid  # noqa: E402

TREE = REPO / 'Assets' / 'Data' / 'EnemyBehaviour' / 'FirstEventDragon.enemyBehaviourData'
SE_DIR = REPO / 'Assets' / 'Audio' / 'Enemy' / 'FirstEventDragon'
BGM_DIR = REPO / 'Assets' / 'Audio' / 'BGM'

# 足すノード (ancient_dragon.py の STATE0_REMOVE も同じ値を持つ)
OMEN_APPROACH = '2C2E254E-76A4-435F-85DD-3FD847DF67AC'
OMEN_BGM_CUT = '53E3B11A-1894-4164-BC6F-D2A0FDB726C0'
OMEN_BGM_RESUME = '50C4A7DF-E279-4784-9A0B-6B665CB26191'
OMEN_BGM_LANDING = '055914D0-3CF1-4E93-ACBF-D7444D87193E'

# 序章の BT のノード
STATE0_SEQ = '04E3F0D6-E481-4609-BA33-F4FC957497E5'
TO_DESTROY_AIRSHIP = '424A00CA-3E2E-4CAE-B507-3924871C02A2'
SHIP_IMPACT_ONCE = '65F01DCA-ECBB-4F72-830F-FA2ED1AC9ECF'
TO_TOUCH_DOWN_ISLAND = '80D57EC7-B83A-465B-AFC9-1EC16ACF4AFF'
TO_TOUCH_DOWN_ANIMATION = 'C4E33394-7FE1-45E9-BC77-289C7D8C8CB6'
ROAR_SEQ = 'F5FD05A6-5186-4541-8FC0-7D7D4CEF4004'
ROAR_SOUND = 'EB0E56AD-72F9-4754-A527-8B5C46E23E08'
PLAY_FIGHT_BGM = '682C55B3-50C7-44A8-AC43-ABA01699129C'

# 飛来 (ToDestroyAirShip は約 25 秒) の中の時刻
CALM_FADE_OUT_SECS = 4.0
DISTANT_ROAR_AT = 3.5
ALARM_BELL_AT = 6.0
OMEN_BGM_AT, OMEN_FADE_IN_SECS = 9.0, 5.0
RESUME_FADE_IN_SECS = 2.5
LANDING_FADE_OUT_SECS = 1.5


def sound(path):
    return asset_guid(Path(str(path) + '.meta'))


def new_action(cat, spec, name, **params):
    """params を入れた新しい action。Cue に入れる物はツリーに無いので set_params を通さずに書く"""
    node = compose.new_action(cat, spec, name=name)
    entry = cat.resolve_action(spec)
    for key, value in params.items():
        pinfo = edits._leaf_param(cat, entry, key)
        raw = str(value).lower() if isinstance(value, bool) else str(value)
        if pinfo['shape'] == 'field':
            edits._set_field_guid(node.params[pinfo['key']], raw)
        else:
            node.params[pinfo['key']] = edits._coerce(pinfo['shape'], raw)
    return node


def fade(cat, name, bgm=None, fade_out=0.0, fade_in=0.0):
    params = {'fadeOut_secs_': fade_out, 'fadeIn_secs_': fade_in}
    if bgm:
        params['bgm_'] = bgm
    return new_action(cat, 'Sound::FadeBGM', name, **params)


def once(guid, child):
    return model.OnceExecute(guid=guid, child=child)


def insert_before(seq, anchor_guid, node):
    index = next(i for i, c in enumerate(seq.children) if c.guid == anchor_guid)
    seq.children.insert(index, node)


def insert_after(seq, anchor_guid, node):
    index = next(i for i, c in enumerate(seq.children) if c.guid == anchor_guid)
    seq.children.insert(index + 1, node)


def main():
    text, tree, cat = cli_edit._load(TREE)
    if tree.find(OMEN_APPROACH) is not None:
        print(f'{TREE.name}: omen nodes already there, nothing to do')
        return

    omen_bgm = sound(BGM_DIR / 'Omen_DragonApproach.mp3')
    seq = tree.find(STATE0_SEQ)

    # 飛来: MoveEventRoute を Cue にして、飛んでいる間に音を鳴らす
    flight = tree.find(TO_DESTROY_AIRSHIP)
    approach = compose.timeline(cat, [
        compose.cue(0.0, fade(cat, 'Omen Calm BGM Fade Out', fade_out=CALM_FADE_OUT_SECS)),
        compose.cue(0.0, flight, wait_done=True),
        compose.cue(DISTANT_ROAR_AT, new_action(cat, 'Sound::PlaySE', 'Omen Distant Roar',
                                                sound_=sound(SE_DIR / 'DragonOmen_DistantRoar.mp3'))),
        compose.cue(ALARM_BELL_AT, new_action(cat, 'Sound::PlaySE', 'Omen Alarm Bell',
                                              sound_=sound(SE_DIR / 'DragonOmen_AlarmBell.mp3'))),
        compose.cue(OMEN_BGM_AT, fade(cat, 'Omen BGM In', bgm=omen_bgm, fade_in=OMEN_FADE_IN_SECS)),
    ], once=True, name='Omen Approach', pos=flight.pos, guid=OMEN_APPROACH)
    seq.children[seq.children.index(flight)] = approach

    # 火球が船に当たった瞬間に切り、急降下で戻し、着地の前に下げる
    insert_after(seq, SHIP_IMPACT_ONCE, once(OMEN_BGM_CUT, fade(cat, 'Omen BGM Cut', fade_out=0.15)))
    insert_before(seq, TO_TOUCH_DOWN_ISLAND,
                  once(OMEN_BGM_RESUME, fade(cat, 'Omen BGM Resume', bgm=omen_bgm, fade_in=RESUME_FADE_IN_SECS)))
    insert_before(seq, TO_TOUCH_DOWN_ANIMATION,
                  once(OMEN_BGM_LANDING, fade(cat, 'Omen BGM Fade Before Landing', fade_out=LANDING_FADE_OUT_SECS)))

    # 戦闘 BGM は咆哮と同時に
    edits.move_node(tree, guid=PLAY_FIGHT_BGM, parent_guid=ROAR_SEQ,
                    index=[c.guid for c in tree.find(ROAR_SEQ).children].index(ROAR_SOUND) + 1)

    auto_layout(tree)
    added = (OMEN_APPROACH, OMEN_BGM_CUT, OMEN_BGM_RESUME, OMEN_BGM_LANDING, PLAY_FIGHT_BGM)
    # NOTE: 序章の BT には古い版のノードがあって全体の検証に落ちるので、触ったノードだけ確かめる
    problems = [p for p in validate(tree, cat=cat) if not p.startswith('note:') and any(g in p for g in added)]
    if problems:
        raise SystemExit('BT validation failed:\n  ' + '\n  '.join(problems))
    out = cli_edit.write_tree(tree)
    if cli_edit.write_tree(cli_edit.read_tree(out, cat=cat, kind=tree.kind)) != out:
        raise SystemExit('output is not byte-stable on re-read; not written')
    TREE.write_bytes(cli_edit.to_file_bytes(out))
    print(f'wrote {TREE.name}')


if __name__ == '__main__':
    main()
