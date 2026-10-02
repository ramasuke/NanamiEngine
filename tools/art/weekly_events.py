"""毎週のイベント (docs/LiveOps.md) のデータを WEEKS から書き出す。

    python tools/art/weekly_events.py

週ごとに次を書く (本体は 0 バイト、中身は .meta。既存の .meta があれば guid を保つ):
  Assets/Data/Decoration/<Asset>.decoration                 島の飾り
  Assets/Data/EventNotice/Weekly/<Asset>.eventNotice         掲示板の告知 (期間)
  Assets/Data/EventNotice/Weekly/<Asset>.boardQuest          依頼 (event_ = 告知、1度だけ、報酬 = お金 + 飾り)
  Assets/Data/EventNotice/Weekly/Announcement_<Asset>.announcement  開始2日前のお知らせ
stage='event' の週は、草原のイベント用ステージ (Assets/Data/Stage/GrasslandEventStage.stageData) をその期間だけ
ステージ選択に出す。ステージの中身 (強い大顎・context) は tools/art/grassland_event_stage.py が用意する。
MainIslandEventBoard.eventBoard.meta の notices_ / quests_ / announcements_ に足し (RETIRED_NOTICES は外す)、
MainIslandScene の IslandDecorations に飾りの置き場所 (ConditionalObject) を置き直す。
文面の決まりは docs/Story.md §3 (口調) / §8 (1行22字、「〜」は使えない)。
"""
import copy
import math
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common import meta_base  # noqa: E402
from tools.common.blob import Ptr, Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, read_text, to_file_bytes  # noqa: E402

DECORATION_DIR = REPO / 'Assets' / 'Data' / 'Decoration'
EVENT_DIR = REPO / 'Assets' / 'Data' / 'EventNotice'
WEEKLY_DIR = EVENT_DIR / 'Weekly'
BOARD_META = EVENT_DIR / 'MainIslandEventBoard.eventBoard.meta'
NOTICE_TEMPLATE = EVENT_DIR / 'HyenaHuntWeek.eventNotice.meta'
ANNOUNCEMENT_TEMPLATE = EVENT_DIR / 'Announcement_HyenaHuntWeek.announcement.meta'
SCENE = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
PREFAB_DIR = REPO / 'Assets' / 'Prefab'
FIRST_ID = 2147483649
EXACT_TYPE = 1073741824
EMPTY_GUID = '00000000-0000-0000-0000-000000000000'

GRASSLAND_STAGE = REPO / 'Assets' / 'Data' / 'Stage' / 'GrasslandStage.stageData.meta'
EVENT_STAGE = REPO / 'Assets' / 'Data' / 'Stage' / 'GrasslandEventStage.stageData'
# NOTE: バナーは週ごとに Banner/<asset>.png (964x400、草原の実画面を AutoMCP で撮って色を付けたもの)
BANNER_DIR = REPO / 'Assets' / 'Art' / 'UI' / 'EventBoard' / 'Banner'
HERB = REPO / 'Assets' / 'Data' / 'Item' / 'Herb.itemData.meta'

# NOTE: 掲示板から外す告知。データは残す (嵐を呼ぶ竜: 古竜のステージができたら日付を入れ直して使う)
RETIRED_NOTICES = ['StormDragonRaid.eventNotice.meta']

# GameCore::Npc::Enemy::EnemyKind
HYENA, ENRAGED_TYRANNOSAURUS = 2, 8
# GameCore::Scene::Main::SceneType
GRASS_LAND_EVENT = 6
# NOTE: QuestType はナビ (Main.navGuide の QuestTakingCondition: 3/4/9) が見ていない Kill10Slimes を使い回す。
#       掲示板から受けた依頼は BoardQuest の guid で見分けるので、週ごとに同じ値でよい
QUEST_TYPE = 1
# Data_AnnouncementKind.h
ANNOUNCEMENT_EVENT = 3

# 飾りの置き場所は掲示板 (54.8, 23.0, 109.3) の周り。階段から訓練の島への道 (半幅 8) と切り株 (70, 120) を避ける。y は地面 + 0.8
BOARD_POS = (54.8, 109.3)

WEEKS = [
    dict(asset='HyenaHunt_1009', start='2026-10-09 12:00', end='2026-10-16 04:59', posted='2026-10-07 12:00',
         notice=dict(title='草原の群狼 討伐週間', tag='討伐イベント',
                     lines=['草原地帯のハイエナが群れで現れる。', '15匹倒すと、酒場から報酬と旗が出る。']),
         quest=dict(title='群狼の討伐', client='酒場の仲介人', goal='草原のハイエナを15匹倒す',
                    lines=['草原にハイエナの群れが増えてる。', '15匹片づけてきな。旗もつけてやる。'],
                    defeat=HYENA, count=15),
         money=1500,
         decoration=dict(asset='WolfPackBanner', name='群狼の旗',
                         lines=['草原の群狼を退けた狩人に贈られる旗。', '狩人の一族の印が染め抜かれている。'],
                         prefab='Prop/Settlement/TrailBanner.prefab', pos=(64.0, 21.9, 124.0)),
         announcement=dict(title='「草原の群狼 討伐週間」開催', lines=[
             '10/9(金) 12:00 から「草原の群狼 討伐週間」を開催します。',
             '期間中に草原のハイエナを15匹倒すと、',
             '1,500 G と島の飾り「群狼の旗」を受け取れます。',
             '',
             '期間: 10/9(金) 12:00 から 10/16(金) 4:59 まで',
             '掲示板の「依頼」から受けられます。報酬は1回だけです。'])),
    dict(asset='HerbGathering_1016', start='2026-10-16 12:00', end='2026-10-23 04:59', posted='2026-10-14 12:00',
         notice=dict(title='草原の薬草あつめ', tag='採集イベント',
                     lines=['よろず屋が薬草を買い集めている。', '草原で10個摘むと、礼の品が出る。']),
         quest=dict(title='薬草あつめ', client='商人', goal='草原で薬草を10個摘む',
                    lines=['薬草が品切れで困ってるんだ。', '草原で10個摘んできてくれ。礼ははずむよ！'],
                    collect=HERB, count=10),
         money=1200,
         decoration=dict(asset='HerbBasket', name='薬草のかご',
                         lines=['草原の薬草を山と集めた狩人のかご。', 'よろず屋の主人が礼にくれたもの。'],
                         prefab='Prop/Settlement/SupplyPile.prefab', pos=(68.0, 21.9, 110.0)),
         announcement=dict(title='「草原の薬草あつめ」開催', lines=[
             '10/16(金) 12:00 から「草原の薬草あつめ」を開催します。',
             '依頼を受けてから草原で薬草を10個摘むと、',
             '1,200 G と島の飾り「薬草のかご」を受け取れます。',
             '',
             '期間: 10/16(金) 12:00 から 10/23(金) 4:59 まで',
             '掲示板の「依頼」から受けられます。報酬は1回だけです。'])),
    dict(asset='AngryTyrant_1023', start='2026-10-23 12:00', end='2026-10-30 04:59', posted='2026-10-21 12:00',
         notice=dict(title='怒れる大顎', tag='大物出現',
                     lines=['気の立った大顎が草原に現れた。', '期間中だけ、ステージ選択から行ける。']),
         quest=dict(title='怒れる大顎', client='見張り', goal='草原の怒れる大顎を倒す',
                    lines=['しっ……大顎の様子がおかしい。', 'いつもよりずっと気が立ってる。気をつけて。'],
                    defeat=ENRAGED_TYRANNOSAURUS, count=1),
         stage='event',
         money=3000,
         decoration=dict(asset='TyrantTotem', name='大顎のトーテム',
                         lines=['怒れる大顎を倒した証の柱。', '牙と爪が飾りつけてある。'],
                         prefab='Prop/Settlement/Totem.prefab', pos=(40.0, 21.9, 128.0)),
         announcement=dict(title='「怒れる大顎」開催', lines=[
             '10/23(金) 12:00 から「怒れる大顎」を開催します。',
             '期間中はステージ選択に「怒れる大顎」が出ます。',
             'いつもの大顎よりずっと手強い、気の立った大顎です。',
             '倒すと 3,000 G と島の飾り「大顎のトーテム」を受け取れます。',
             '',
             '期間: 10/23(金) 12:00 から 10/30(金) 4:59 まで',
             '本編の草原地帯の大顎はいつもどおりです。',
             '仲間と一緒に挑むのがおすすめです。'])),
    dict(asset='HyenaHunt_1030', start='2026-10-30 12:00', end='2026-11-06 04:59', posted='2026-10-28 12:00',
         notice=dict(title='群狼討伐 第2回', tag='討伐イベント',
                     lines=['草原の群狼がまた数を増やしている。', '20匹倒すと、酒場から報酬が出る。']),
         quest=dict(title='群狼の討伐 第2回', client='酒場の仲介人', goal='草原のハイエナを20匹倒す',
                    lines=['また群れが増えてきやがった。', '今度は20匹だ。腕が鈍ってないといいがな。'],
                    defeat=HYENA, count=20),
         money=1500,
         decoration=dict(asset='HideDryingRack', name='毛皮の干し台',
                         lines=['群狼の毛皮を干す狩人の台。', '二度目の討伐週間の記念の品。'],
                         prefab='Prop/Settlement/DryingRack.prefab', pos=(44.0, 21.9, 140.0)),
         announcement=dict(title='「群狼討伐 第2回」開催', lines=[
             '10/30(金) 12:00 から「群狼討伐 第2回」を開催します。',
             '期間中に草原のハイエナを20匹倒すと、',
             '1,500 G と島の飾り「毛皮の干し台」を受け取れます。',
             '',
             '期間: 10/30(金) 12:00 から 11/6(金) 4:59 まで',
             '掲示板の「依頼」から受けられます。報酬は1回だけです。'])),
]


def asset_guid(meta_path):
    m = re.search(r'"guid_"\s*:\s*\{[^{}]*?"value_"\s*:\s*"([0-9A-Fa-f-]{36})"', read_text(meta_path))
    if not m:
        raise SystemExit(f'no guid_ in {meta_path}')
    return m.group(1).upper()


def keep_or_mint(meta_path):
    return asset_guid(meta_path) if meta_path.exists() else meta_base.mint_guid()


def content_path(path):
    rel = path.relative_to(REPO)
    return '\\'.join(rel.parts[:-1]) + '/' + rel.parts[-1]


def ver(version, pairs=()):
    return OrderedObj([('cereal_class_version', Num.of_int(version)), *pairs])


def field(guid, blob_id, first):
    """FIELD(T) 1つ。cereal は型ごとにファイルで初出のときだけ版を書く"""
    inner = OrderedObj([('value0', OrderedObj([('value_', guid)]))])
    if first:
        inner.insert(0, 'cereal_class_version', Num.of_int(0))
    wrapper = OrderedObj([('id', Num.of_int(blob_id)), ('data', inner)])
    holder = OrderedObj([('value0', OrderedObj([('polymorphic_id', Num.of_int(EXACT_TYPE)), ('ptr_wrapper', wrapper)]))])
    if first:
        holder.insert(0, 'cereal_class_version', Num.of_int(0))
    return holder


def poly(type_id, fqn, ptr_id, data):
    pairs = [('polymorphic_id', Num.of_int(type_id))]
    if fqn:
        pairs.append(('polymorphic_name', fqn))
    pairs.append(('ptr_wrapper', OrderedObj([('id', Num.of_int(ptr_id)), ('data', data)])))
    return OrderedObj(pairs)


def asset_root(fqn, data):
    return OrderedObj([('value0', poly(FIRST_ID, fqn, FIRST_ID, data))])


def asset_base(path, guid):
    return ver(0, [('value0', ver(0)), ('contentPath_', content_path(path)),
                   ('guid_', ver(0, [('value_', guid)]))])


def write_asset(path, root):
    path.parent.mkdir(parents=True, exist_ok=True)
    meta = path.with_name(path.name + '.meta')
    meta.write_bytes(to_file_bytes(dumps(root)))
    path.write_bytes(b'')
    print(f'  {path.relative_to(REPO)}')


def from_template(template, path, guid, fields):
    tree = loads(read_text(template))
    data = tree['value0']['ptr_wrapper']['data']
    data['value0']['contentPath_'] = content_path(path)
    data['value0']['guid_']['value_'] = guid
    for key, value in fields.items():
        data[key] = value
    return tree


# ---------------------------------------------------------------- アセット
def write_decoration(week):
    d = week['decoration']
    path = DECORATION_DIR / f"{d['asset']}.decoration"
    guid = keep_or_mint(path.with_name(path.name + '.meta'))
    data = ver(0, [('value0', asset_base(path, guid)), ('name_', d['name']), ('descriptionLines_', list(d['lines'])),
                   ('iconSprite_', field(EMPTY_GUID, FIRST_ID + 1, first=True))])
    write_asset(path, asset_root('NanamiEngine::Module::Asset::DecorationData', data))
    return guid


def write_notice(week):
    path = WEEKLY_DIR / f"{week['asset']}.eventNotice"
    guid = keep_or_mint(path.with_name(path.name + '.meta'))
    n = week['notice']
    tree = from_template(NOTICE_TEMPLATE, path, guid, {
        'title_': n['title'], 'tagText_': n['tag'], 'startAt_': week['start'], 'endAt_': week['end'],
        'descriptionLines_': list(n['lines']), 'unlockConditions_': []})
    tree['value0']['ptr_wrapper']['data']['bannerSprite_']['value0']['ptr_wrapper']['data']['value0']['value_'] = \
        asset_guid(BANNER_DIR / f"{week['asset']}.png.meta")
    write_asset(path, tree)
    return guid


def write_announcement(week):
    path = WEEKLY_DIR / f"Announcement_{week['asset']}.announcement"
    guid = keep_or_mint(path.with_name(path.name + '.meta'))
    a = week['announcement']
    tree = from_template(ANNOUNCEMENT_TEMPLATE, path, guid, {
        'kind_': Num.of_int(ANNOUNCEMENT_EVENT), 'title_': a['title'], 'postedAt_': week['posted'],
        'bodyLines_': list(a['lines'])})
    write_asset(path, tree)
    return guid


def write_board_quest(week, notice_guid, decoration_guid, stage_guid):
    """依頼の中身は Defeat/CollectRequestQuest。ITakeableQuest v1 (rewards_)、RequestQuestBase v1 (repeatable_)"""
    path = WEEKLY_DIR / f"{week['asset']}.boardQuest"
    guid = keep_or_mint(path.with_name(path.name + '.meta'))
    q = week['quest']
    is_collect = 'collect' in q
    leaf = 'CollectRequestQuest' if is_collect else 'DefeatRequestQuest'

    # NOTE: ptr の id は書き出し順。root=1, stage_=2, event_=3, quest_=4, お金=5, 飾り=6, 飾りの FIELD=7, item_=8
    money = poly(FIRST_ID + 2, 'GameCore::Reward::MoneyReward', FIRST_ID + 4, ver(0, [
        ('value0', ver(0, [('conditions_', [])])),
        ('amount_', ver(0, [('value_', Num.of_int(week['money']))]))]))
    decoration = poly(FIRST_ID + 3, 'GameCore::Reward::DecorationReward', FIRST_ID + 5, ver(0, [
        ('value0', OrderedObj([('conditions_', [])])),
        ('decoration_', field(decoration_guid, FIRST_ID + 6, first=True))]))
    takeable = ver(1, [('rewards_', [money, decoration])])
    request = ver(1, [('value0', takeable), ('questType_', Num.of_int(QUEST_TYPE)),
                      ('requiredCount_', Num.of_int(q['count'])),
                      ('startRecord_', OrderedObj([('nullopt', True)])), ('repeatable_', False)])
    if is_collect:
        content = ver(0, [('value0', request), ('item_', field(asset_guid(q['collect']), FIRST_ID + 7, first=True))])
    else:
        content = ver(0, [('value0', request), ('enemyKind_', Num.of_int(q['defeat']))])

    data = ver(2, [
        ('value0', asset_base(path, guid)),
        ('title_', q['title']), ('clientName_', q['client']), ('goalText_', q['goal']),
        ('descriptionLines_', list(q['lines'])),
        ('stage_', field(stage_guid, FIRST_ID + 1, first=True)),
        ('event_', field(notice_guid, FIRST_ID + 2, first=True)),
        ('quest_', poly(FIRST_ID + 1, f'GameCore::PlayerAvatar::Quest::Request::{leaf}', FIRST_ID + 3, content)),
        ('unlockConditions_', []),
        ('lockedText_', ''),
    ])
    write_asset(path, asset_root('NanamiEngine::Module::Asset::BoardQuest', data))
    return guid


# ---------------------------------------------------------------- イベント用ステージ
EVENT_STAGE_TEXT = dict(
    name='怒れる大顎', tag='大物出現 ・ 緑属性', difficulty=3,
    lines=['気の立った大顎が草原に現れた。', 'いつもの大顎より、ずっと手強い。'],
    locked=['イベントの期間中だけ行ける。'])


def period_condition(week, type_id, ptr_id, first, base_first=None):
    base = ver(0) if (first if base_first is None else base_first) else OrderedObj()
    data = OrderedObj([('value0', base), ('startAt_', week['start']), ('endAt_', week['end'])])
    if first:
        data.insert(0, 'cereal_class_version', Num.of_int(0))
    fqn = 'GameCore::Condition::PeriodCondition' if first else None
    return poly(type_id | (0x80000000 if first else 0), fqn, ptr_id, data)


def write_event_stage(weeks):
    """草原の大物の週だけステージ選択に出す (期間外は StageData::hideWhenLocked_ で行ごと隠す)"""
    meta = EVENT_STAGE.with_name(EVENT_STAGE.name + '.meta')
    guid = keep_or_mint(meta)
    t = EVENT_STAGE_TEXT
    # NOTE: ptr の id は root=1, thumbnail=2, element=3 の続き。週が複数なら AnyOf で期間をまとめる
    if len(weeks) == 1:
        conditions = [period_condition(weeks[0], 2, FIRST_ID + 3, first=True)]
    else:
        # NOTE: ICondition の版は AnyOf が先に書くので、期間の側は持たない
        periods = [period_condition(w, 3, FIRST_ID + 4 + i, first=i == 0, base_first=False) for i, w in enumerate(weeks)]
        any_of = ver(0, [('value0', ver(0)), ('conditions_', periods)])
        conditions = [poly(FIRST_ID + 1, 'GameCore::Condition::AnyOfCondition', FIRST_ID + 3, any_of)]
    tree = from_template(GRASSLAND_STAGE, EVENT_STAGE, guid, {
        'displayName_': t['name'], 'sceneType_': Num.of_int(GRASS_LAND_EVENT), 'difficulty_': Num.of_int(t['difficulty']),
        'tagText_': t['tag'], 'descriptionLines_': list(t['lines']), 'unlockConditions_': conditions,
        'lockedDescriptionLines_': list(t['locked'])})
    data = tree['value0']['ptr_wrapper']['data']
    data['cereal_class_version'] = Num.of_int(4)
    data['hideWhenLocked_'] = True
    write_asset(EVENT_STAGE, tree)
    return guid


# ---------------------------------------------------------------- 掲示板
def entry_guid(item):
    return item['value0']['ptr_wrapper']['data']['value0']['value_']


def pin_on_board(notices, quests, announcements):
    tree = loads(read_text(BOARD_META))
    data = tree['value0']['ptr_wrapper']['data']
    lists = ('notices_', 'quests_', 'announcements_', 'facilities_')
    ids = [item['value0']['ptr_wrapper']['id'].value for key in lists if key in data.keys() for item in data[key]]
    next_id = max(ids, default=FIRST_ID) + 1

    retired = {asset_guid(EVENT_DIR / name) for name in RETIRED_NOTICES}
    for key, guids in (('notices_', notices), ('quests_', quests), ('announcements_', announcements)):
        items = [item for item in data[key] if entry_guid(item) not in retired]
        present = {entry_guid(item) for item in items}
        for guid in guids:
            if guid not in present:
                items.append(field(guid, next_id, first=False))
                next_id += 1
        # NOTE: 版キーは各一覧の先頭の1件だけが持つ (一覧ごとに FIELD の型が違う)
        for i, item in enumerate(items):
            has_version = 'cereal_class_version' in item.keys()
            wrapper_data = item['value0']['ptr_wrapper']['data']
            if i == 0 and not has_version:
                item.insert(0, 'cereal_class_version', Num.of_int(0))
                wrapper_data.insert(0, 'cereal_class_version', Num.of_int(0))
            elif i > 0 and has_version:
                item.pop('cereal_class_version')
                wrapper_data.pop('cereal_class_version')
        data[key] = items
    BOARD_META.write_bytes(to_file_bytes(dumps(tree)))
    print(f'  {BOARD_META.relative_to(REPO)}: notices_ {len(data["notices_"])}, quests_ {len(data["quests_"])}, '
          f'announcements_ {len(data["announcements_"])}')


# ---------------------------------------------------------------- 島の飾り
def yaw_quat(yaw):
    return (0.0, math.sin(yaw / 2.0), 0.0, math.cos(yaw / 2.0))


def conditional_object(cat, guid, decoration_guid, prefab_guid):
    from tools.scene import edits
    comp = edits.new_component(cat.component_by_fqn('GamePlay::Prop::ConditionalObject'), guid=guid, cat=cat)
    holder = Ver(('type', 'FieldHolder<DecorationData>'), 0, OrderedObj([('value0', OrderedObj([('value_', decoration_guid)]))]))
    decoration = Ver(('type', 'Field<DecorationData>'), 0, OrderedObj([
        ('value0', Ptr(exact=True, null=False, fqn=None, wrapper='shared', data=holder))]))
    condition = Ver(('type', 'DecorationOwnedCondition'), 0, OrderedObj([
        ('value0', Ver(('type', 'ICondition'), 0, OrderedObj())), ('decoration_', decoration)]))
    comp.data['conditions_'] = [Ptr(exact=False, null=False, fqn='GameCore::Condition::DecorationOwnedCondition',
                                    wrapper='shared', data=condition)]
    comp.data['target_'] = edits.field_blob('IGameObject')
    comp.data['prefab_'] = edits.field_blob('PrefabGameObjectFile', prefab_guid)
    return comp


def place_decorations(decorations):
    from tools.scene import catalog as catalog_mod, edits, model, reader, validate, writer
    from game_over_prefab import check, let_writer_place_versions
    from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, walk

    cat = catalog_mod.load()
    scene = reader.read_scene_file(SCENE)
    old = next((r for r in scene.roots if r.name == 'IslandDecorations'), None)
    # NOTE: 置き直しても guid が変わらないよう、前の置き場所の guid を名前で引き継ぐ
    old_guids = {}
    if old:
        for node in walk(old):
            old_guids[node.name] = (node.guid, [model.find_component_guid(c) for c in node.components])
    scene.roots = [r for r in scene.roots if r.name != 'IslandDecorations']

    root_guid = old_guids.get('IslandDecorations', (None, []))[0]
    root = edits.new_gameobject('IslandDecorations', guid=root_guid)
    for d, decoration_guid in decorations:
        name = f"Decoration_{d['asset']}"
        guid, component_guids = old_guids.get(name, (None, []))
        x, y, z = d['pos']
        yaw = math.atan2(BOARD_POS[0] - x, BOARD_POS[1] - z)
        node = edits.new_gameobject(name, pos=(x, y, z), rot=yaw_quat(yaw), guid=guid)
        prefab_guid = asset_guid(PREFAB_DIR / f"{d['prefab']}.meta")
        node.components.append(conditional_object(cat, component_guids[0] if component_guids else None,
                                                  decoration_guid, prefab_guid))
        root.transform.children.append(node)
    scene.roots.append(root)

    for n in walk(root):
        for comp in n.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root)
    text = writer.write_scene(scene)
    tree = loads(text)
    StrayVersionStripper(cat, '/').run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'  {SCENE.relative_to(REPO)}: IslandDecorations {len(decorations)}')


def main():
    notices, quests, announcements, decorations = [], [], [], []
    event_weeks = [w for w in WEEKS if w.get('stage') == 'event']
    stage_guids = {'grassland': asset_guid(GRASSLAND_STAGE), 'event': write_event_stage(event_weeks)}
    for week in WEEKS:
        decoration_guid = write_decoration(week)
        notice_guid = write_notice(week)
        stage_guid = stage_guids[week.get('stage', 'grassland')]
        quests.append(write_board_quest(week, notice_guid, decoration_guid, stage_guid))
        notices.append(notice_guid)
        announcements.append(write_announcement(week))
        decorations.append((week['decoration'], decoration_guid))
    pin_on_board(notices, quests, announcements)
    place_decorations(decorations)


if __name__ == '__main__':
    main()
