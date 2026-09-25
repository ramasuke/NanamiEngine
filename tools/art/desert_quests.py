"""砂漠の掲示板の依頼 (.boardQuest) を作り、拠点の掲示板 (MainIslandEventBoard) に貼る。

    python tools/art/desert_quests.py       # 作り直す (GUID は保つ)

- 形は草原の本筋の依頼 GrassLandTyrant.boardQuest.meta の写し (BoardQuest の版 1: 解放条件と、未開の文言つき)。
- DesertSkeletonDragon : 本筋 (DefeatMainStoryQuest)。骸竜 (EnemyKind 6) を倒すと DesertCleared (StoryFlag 5)。
                         緑の心臓が島に戻ってから (GreenStoneReturned) 貼られる。依頼主は教官
- DesertScorpionCull   : 稼ぎ口 (DefeatRequestQuest)。オアシスの水場の大サソリ (EnemyKind 4) を6匹。依頼主は隊商頭
- 砂に埋もれた荷 (QuestType::DesertLostCargo の収集依頼) は、拾う荷のアイテムと置き場所ができてから足す。
- 報酬は仮 (docs/Story.md §10: 報酬の相場は未決定)。
"""
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common import meta_base  # noqa: E402

from camp_people import asset_guid  # noqa: E402

DIR = REPO / 'Assets' / 'Data' / 'EventNotice'
TEMPLATE = DIR / 'GrassLandTyrant.boardQuest.meta'
BOARD = DIR / 'MainIslandEventBoard.eventBoard.meta'
DESERT_STAGE = asset_guid(REPO / 'Assets/Data/Stage/DesertStage.stageData.meta')

# QuestType (PlayerAvatar_QuestType.h) / EnemyKind / StoryFlag の値
QUEST_DESERT_DRAGON, QUEST_SCORPION_CULL = 8, 9
KIND_SCORPION, KIND_SKELETON_DRAGON = 4, 6
FLAG_GRASSLAND_CLEARED, FLAG_GREEN_STONE_RETURNED, FLAG_DESERT_CLEARED = 2, 3, 5

QUESTS = [
    dict(name='DesertSkeletonDragon', title='砂漠の骸竜', client='教官', rank=4,
         goal='城塞の広場に居着く骸竜を倒す',
         lines=['光の心臓は、砂に沈んだ城塞に落ちている。', '傍に居着く骸竜を倒し、心臓を島へ取り戻せ。'],
         quest=dict(kind='main', reward=5000, questType=QUEST_DESERT_DRAGON, enemyKind=KIND_SKELETON_DRAGON,
                    clearedFlag=FLAG_DESERT_CLEARED),
         unlock=FLAG_GREEN_STONE_RETURNED, locked='緑の心臓を島へ戻してから'),
    dict(name='DesertScorpionCull', title='水場のサソリ退治', client='隊商頭', rank=3,
         goal='オアシスに出る大サソリを6匹倒す',
         lines=['泉の水場まで、大サソリが出るようになった。', '隊商が水を汲めるよう、群れを減らしてくれ。'],
         quest=dict(kind='defeat', reward=600, questType=QUEST_SCORPION_CULL, enemyKind=KIND_SCORPION, count=6),
         unlock=FLAG_GRASSLAND_CLEARED, locked='草原の大顎を倒してから'),
]


def build(q):
    meta = json.loads(TEMPLATE.read_bytes().decode('utf-8-sig'))
    data = meta['value0']['ptr_wrapper']['data']
    path = DIR / f'{q["name"]}.boardQuest.meta'
    guid = asset_guid(path) if path.exists() else meta_base.mint_guid().upper()
    data['value0']['contentPath_'] = f'Assets\\Data\\EventNotice/{q["name"]}.boardQuest'
    data['value0']['guid_']['value_'] = guid
    data['title_'] = q['title']
    data['clientName_'] = q['client']
    data['rank_'] = q['rank']
    data['goalText_'] = q['goal']
    data['descriptionLines_'] = q['lines']
    data['stage_']['value0']['ptr_wrapper']['data']['value0']['value_'] = DESERT_STAGE
    quest = data['quest_']
    body = quest['ptr_wrapper']['data']
    reward = body['value0']['value0']['rewardMoney_']
    reward['value_'] = q['quest']['reward']
    if q['quest']['kind'] == 'main':
        body['questType_'] = q['quest']['questType']
        body['enemyKind_'] = q['quest']['enemyKind']
        body['clearedFlag_'] = q['quest']['clearedFlag']
    else:
        # DefeatRequestQuest: {value0: RequestQuestBase{value0: QuestBase, questType_, requiredCount_, startRecord_}, enemyKind_}
        quest['polymorphic_name'] = 'GameCore::PlayerAvatar::Quest::Request::DefeatRequestQuest'
        base = body['value0']
        quest['ptr_wrapper']['data'] = {
            'cereal_class_version': 0,
            'value0': {'cereal_class_version': 0, 'value0': base['value0'], 'questType_': q['quest']['questType'],
                       'requiredCount_': q['quest']['count'], 'startRecord_': {'nullopt': True}},
            'enemyKind_': q['quest']['enemyKind'],
        }
    data['unlockConditions_'][0]['ptr_wrapper']['data']['storyFlag_'] = q['unlock']
    data['lockedText_'] = q['locked']
    text = json.dumps(meta, indent=4, ensure_ascii=False).replace('\n', '\r\n')
    path.write_bytes(text.encode('utf-8'))
    body_file = DIR / f'{q["name"]}.boardQuest'
    if not body_file.exists():
        body_file.write_bytes(b'')
    print(f'  {q["name"]}.boardQuest  guid={guid}')
    return guid


def pin(guids):
    """掲示板の quests_ の末尾に、まだ無いものを足す"""
    raw = BOARD.read_bytes()
    meta = json.loads(raw.decode('utf-8-sig'))
    data = meta['value0']['ptr_wrapper']['data']
    quests = data['quests_']
    have = json.dumps(quests)
    ids = []

    def collect(o):
        if isinstance(o, dict):
            for k, v in o.items():
                if k == 'id' and isinstance(v, int):
                    ids.append(v)
                else:
                    collect(v)
        elif isinstance(o, list):
            for v in o:
                collect(v)
    collect(meta)
    next_id = max(ids) + 1
    added = 0
    for g in guids:
        if g in have:
            continue
        quests.append({'value0': {'polymorphic_id': 1073741824,
                                  'ptr_wrapper': {'id': next_id, 'data': {'value0': {'value_': g}}}}})
        next_id += 1
        added += 1
    text = json.dumps(meta, indent=4, ensure_ascii=False)
    if b'\r\n' in raw[:200]:
        text = text.replace('\n', '\r\n')
    BOARD.write_bytes(text.encode('utf-8'))
    print(f'  {BOARD.name}: +{added} quest(s)')


def main():
    pin([build(q) for q in QUESTS])


if __name__ == '__main__':
    main()
