"""草原のイベント用ステージ (SceneType::GrassLandEvent = 6) のデータを用意する。

    python tools/art/grassland_event_stage.py

本編の草原と同じ GrassLandScene.scene / GrassLandScene を使い、次だけ変える (docs/LiveOps.md §2):
  GameManage.scene          草原の context を複製して sceneType_ 6、大顎を TyrannosaurusEnraged に差し替え、
                            物語のクリア・空撮・ナビの目標を外す (元の context は v18 にして sceneType_ 0)
  StageSelectUI.prefab      3行目 (Stage3) を足す。ステージは weekly_events.py が書く GrasslandEventStage.stageData
  LoadingRoute              2->6 / 6->6 / 6->2 の航路を足し、LoadingScreenUI.prefab と StageLoadingScene.scene に貼る
  TyrannosaurusEnraged.prefab  討伐の記録を EnragedTyrannosaurus(8) にして、本編の大顎と数えを分ける
シーンは tools.scene で書き戻すと中身が変わるので、どれも cereal JSON を直接いじる。何度実行しても同じになる。
"""
import copy
import re
import sys
import uuid
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from tools.common import meta_base  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, read_text, to_file_bytes  # noqa: E402

ASSETS = REPO / 'Assets'
GAME_MANAGE = ASSETS / 'Scene' / 'GameManage.scene'
STAGE_SELECT = ASSETS / 'Prefab' / 'UI' / 'StageSelectUI.prefab'
LOADING_SCREEN = ASSETS / 'Prefab' / 'UI' / 'LoadingScreenUI.prefab'
STAGE_LOADING_SCENE = ASSETS / 'Scene' / 'StageLoadingScene.scene'
ROUTE_DIR = ASSETS / 'Data' / 'LoadingRoute'
ENRAGED_PREFAB = ASSETS / 'Prefab' / 'Npc' / 'Enemy' / 'TyrannosaurusEnraged.prefab'
EVENT_STAGE = ASSETS / 'Data' / 'Stage' / 'GrasslandEventStage.stageData'

GRASS_LAND, MAIN_ISLAND, GRASS_LAND_EVENT = 0, 2, 6
TYRANNOSAURUS, ENRAGED_TYRANNOSAURUS = 3, 8
CONTEXT_FQN = 'GameCore::Scene::GrassLandSceneContext'
CONTEXT_VERSION = 18
HIGH_BIT = 0x80000000
EMPTY_GUID = '00000000-0000-0000-0000-000000000000'

# (新しい航路, 型紙, fromScene_, toScene_)
ROUTES = [
    ('DepartToGrassLandEvent', 'DepartToGrassLand', MAIN_ISLAND, GRASS_LAND_EVENT),
    ('RetryGrassLandEvent', 'RetryGrassLand', GRASS_LAND_EVENT, GRASS_LAND_EVENT),
    ('ReturnToMainIslandFromGrassLandEvent', 'ReturnToMainIsland', GRASS_LAND_EVENT, MAIN_ISLAND),
]


# ---------------------------------------------------------------- 共通
def asset_guid(meta_path):
    m = re.search(r'"guid_"\s*:\s*\{[^{}]*?"value_"\s*:\s*"([0-9A-Fa-f-]{36})"', read_text(meta_path))
    if not m:
        raise SystemExit(f'no guid_ in {meta_path}')
    return m.group(1).upper()


def walk(node):
    yield node
    if isinstance(node, OrderedObj):
        for _, value in node.items():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def max_ptr_id(tree):
    ids = [n['ptr_wrapper']['id'].value for n in walk(tree)
           if isinstance(n, OrderedObj) and 'ptr_wrapper' in n.keys() and 'id' in n['ptr_wrapper'].keys()]
    return max(ids)


def renumber_ptrs(subtree, next_id):
    """複製した木の shared_ptr に、ファイルで使われていない id を振り直す"""
    for n in walk(subtree):
        if isinstance(n, OrderedObj) and 'ptr_wrapper' in n.keys():
            wrapper = n['ptr_wrapper']
            if 'id' in wrapper.keys() and 'data' in wrapper.keys():
                wrapper['id'] = Num.of_int(next_id)
                next_id += 1
    return next_id


def as_repeat(subtree):
    """複製はファイルで2度目以降の出現なので、版キーと polymorphic_name を落とす"""
    for n in walk(subtree):
        if not isinstance(n, OrderedObj):
            continue
        if 'cereal_class_version' in n.keys():
            n.pop('cereal_class_version')
        if 'polymorphic_name' in n.keys():
            n.pop('polymorphic_name')
            n['polymorphic_id'] = Num.of_int(n['polymorphic_id'].value & ~HIGH_BIT)


def field(guid, ptr_id):
    """2度目以降の FIELD(T) (版キーなし)"""
    return OrderedObj([('value0', OrderedObj([
        ('polymorphic_id', Num.of_int(1073741824)),
        ('ptr_wrapper', OrderedObj([('id', Num.of_int(ptr_id)),
                                    ('data', OrderedObj([('value0', OrderedObj([('value_', guid)]))]))]))]))])


def field_guid(blob):
    return blob['value0']['ptr_wrapper']['data']['value0']['value_']


def write(path, tree):
    path.write_bytes(to_file_bytes(dumps(tree)))
    print(f'  {path.relative_to(REPO)}')


def new_guid():
    return str(uuid.uuid4()).upper()


# ---------------------------------------------------------------- GameManage.scene
def scene_contexts(tree):
    for n in walk(tree):
        if isinstance(n, OrderedObj) and n.get('name_') == 'SceneContexts':
            return n['components_']
    raise SystemExit('SceneContexts not found in GameManage.scene')


def set_context_fields(data, scene_type, override_kind, override_prefab, ptr_id):
    if 'sceneType_' in data.keys():
        data['sceneType_'] = Num.of_int(scene_type)
        data['enemyOverrideKind_'] = Num.of_int(override_kind)
        data['enemyOverridePrefab_']['value0']['ptr_wrapper']['data']['value0']['value_'] = override_prefab
        return ptr_id
    data.append('sceneType_', Num.of_int(scene_type))
    data.append('enemyOverrideKind_', Num.of_int(override_kind))
    data.append('enemyOverridePrefab_', field(override_prefab, ptr_id))
    return ptr_id + 1


def setup_contexts():
    tree = loads(read_text(GAME_MANAGE))
    components = scene_contexts(tree)
    keys = [k for k in components.keys() if k.startswith('component_')]
    contexts = []
    original = None
    for k in keys:
        c = components[k]
        if c.get('polymorphic_name') == CONTEXT_FQN:
            original = c
            type_id = c['polymorphic_id'].value & ~HIGH_BIT
    if original is None:
        raise SystemExit(f'{CONTEXT_FQN} not found')
    contexts = [components[k] for k in keys if components[k]['polymorphic_id'].value & ~HIGH_BIT == type_id]

    next_id = max_ptr_id(tree) + 1
    data = original['ptr_wrapper']['data']
    data['cereal_class_version'] = Num.of_int(CONTEXT_VERSION)
    next_id = set_context_fields(data, GRASS_LAND, -1, EMPTY_GUID, next_id)

    enraged = asset_guid(ENRAGED_PREFAB.with_name(ENRAGED_PREFAB.name + '.meta'))
    event = next((c for c in contexts if c is not original), None)
    if event is None:
        event = copy.deepcopy(original)
        as_repeat(event)
        next_id = renumber_ptrs(event, next_id)
        event_data = event['ptr_wrapper']['data']
        event_data['value0']['value0']['guid_']['value_'] = new_guid()
        index = len(keys)
        components.append(f'component_{index}', event)
        components['componentCount'] = Num.of_int(index + 1)
    event_data = event['ptr_wrapper']['data']
    # NOTE: 物語のクリア・初回の空撮・ナビの目標はイベント用のステージには持ち込まない
    event_data['clearEnemyKind_'] = Num.of_int(-1)
    event_data['clearStoryFlag_'] = Num.of_int(-1)
    event_data['arrivalOverview_msecs_'] = Num.of_int(0)
    event_data['value0']['navigationTargets_'] = []
    set_context_fields(event_data, GRASS_LAND_EVENT, TYRANNOSAURUS, enraged, next_id)
    write(GAME_MANAGE, tree)


# ---------------------------------------------------------------- ステージ選択の3行目
def find_named(tree, name):
    return next(n for n in walk(tree) if isinstance(n, OrderedObj) and n.get('name_') == name)


def remint_subtree(subtree):
    """GameObject / Component の guid を振り直し、木の中の参照も付け替える"""
    remap = {}
    for n in walk(subtree):
        if isinstance(n, OrderedObj) and 'guid_' in n.keys() and isinstance(n['guid_'], OrderedObj):
            old = n['guid_']['value_']
            remap[old] = remap.get(old) or new_guid()
            n['guid_']['value_'] = remap[old]
    for n in walk(subtree):
        if isinstance(n, OrderedObj) and 'value_' in n.keys() and n['value_'] in remap:
            n['value_'] = remap[n['value_']]
    return remap


def component_of(go, fqn_tail, type_ids):
    comps = go['components_']
    for k in comps.keys():
        c = comps[k]
        if isinstance(c, OrderedObj) and (c['polymorphic_id'].value & ~HIGH_BIT) in type_ids:
            return c
    raise SystemExit(f'{fqn_tail} not on {go["name_"]}')


def type_ids_of(tree, fqn):
    return {n['polymorphic_id'].value & ~HIGH_BIT for n in walk(tree)
            if isinstance(n, OrderedObj) and n.get('polymorphic_name') == fqn}


def setup_stage_row(stage_guid):
    tree = loads(read_text(STAGE_SELECT))
    stage_list = find_named(tree, 'StageList')
    transform = stage_list['transform_']
    rows = transform.values_for('child')
    names = [r['ptr_wrapper']['data']['name_'] for r in rows]
    row_ids = type_ids_of(tree, 'GamePlay::Ui::StageSelectStageUi')
    if 'Stage3' in names:
        row = rows[names.index('Stage3')]['ptr_wrapper']['data']
    else:
        source = rows[names.index('Stage2')]
        child = copy.deepcopy(source)
        renumber_ptrs(child, max_ptr_id(tree) + 1)
        remint_subtree(child)
        row = child['ptr_wrapper']['data']
        row['name_'] = 'Stage3'
        # NOTE: 行は 120 ずつ下へ並ぶ (Stage1 = 0, Stage2 = 120)
        row['transform_']['localPos_']['value1'] = Num.of_float(240.0)
        world = row['transform_']['worldMatrix_']['value3']
        world['value1'] = Num.of_float(world['value1'].value + 120.0)
        transform.append('child', child)
        transform['childCount'] = Num.of_int(len(rows) + 1)
    row_ui = component_of(row, 'StageSelectStageUi', row_ids)
    row_data = row_ui['ptr_wrapper']['data']
    row_data['stageData_']['value0']['ptr_wrapper']['data']['value0']['value_'] = stage_guid
    row_component_guid = row_data['value0']['guid_']['value_']

    # NOTE: StageSelectUi.stageSelectButtons_ の並びが行の並び (カーソルの順)
    ui = component_of(tree, 'StageSelectUi', type_ids_of(tree, 'GamePlay::Ui::StageSelectUi'))
    buttons = ui['ptr_wrapper']['data']['stageSelectButtons_']
    if row_component_guid not in [field_guid(b) for b in buttons]:
        buttons.append(field(row_component_guid, max_ptr_id(tree) + 1))
    write(STAGE_SELECT, tree)


# ---------------------------------------------------------------- ロード画面の航路
def write_routes():
    guids = []
    for name, template, from_scene, to_scene in ROUTES:
        meta = ROUTE_DIR / f'{name}.loadingRoute.meta'
        guid = asset_guid(meta) if meta.exists() else meta_base.mint_guid()
        tree = loads(read_text(ROUTE_DIR / f'{template}.loadingRoute.meta'))
        data = tree['value0']['ptr_wrapper']['data']
        data['value0']['contentPath_'] = f'Assets\\Data\\LoadingRoute/{name}.loadingRoute'
        data['value0']['guid_']['value_'] = guid
        data['isFromAnywhere_'] = False
        data['fromScene_'] = Num.of_int(from_scene)
        data['toScene_'] = Num.of_int(to_scene)
        write(meta, tree)
        (ROUTE_DIR / f'{name}.loadingRoute').write_bytes(b'')
        guids.append(guid)
    return guids


def pin_routes(path, guids):
    tree = loads(read_text(path))
    lists = [n['routes_'] for n in walk(tree) if isinstance(n, OrderedObj) and 'routes_' in n.keys()]
    if len(lists) != 1:
        raise SystemExit(f'{path.name}: expected one routes_, found {len(lists)}')
    routes = lists[0]
    present = {field_guid(r) for r in routes}
    next_id = max_ptr_id(tree) + 1
    for guid in guids:
        if guid not in present:
            routes.append(field(guid, next_id))
            next_id += 1
    write(path, tree)


# ---------------------------------------------------------------- 強い大顎の記録
def set_enraged_record_kind():
    tree = loads(read_text(ENRAGED_PREFAB))
    comp = next(n for n in walk(tree) if isinstance(n, OrderedObj)
                and n.get('polymorphic_name') == 'GamePlay::Npc::Enemy::Tyrannosaurus')
    data = comp['ptr_wrapper']['data']
    data['cereal_class_version'] = Num.of_int(7)
    if 'recordKind_' in data.keys():
        data['recordKind_'] = Num.of_int(ENRAGED_TYRANNOSAURUS)
    else:
        data.append('recordKind_', Num.of_int(ENRAGED_TYRANNOSAURUS))
    write(ENRAGED_PREFAB, tree)


def main():
    stage_meta = EVENT_STAGE.with_name(EVENT_STAGE.name + '.meta')
    if not stage_meta.exists():
        raise SystemExit('run tools/art/weekly_events.py first (it writes GrasslandEventStage.stageData)')
    setup_contexts()
    setup_stage_row(asset_guid(stage_meta))
    guids = write_routes()
    pin_routes(LOADING_SCREEN, guids)
    pin_routes(STAGE_LOADING_SCENE, guids)
    set_enraged_record_kind()


if __name__ == '__main__':
    main()
