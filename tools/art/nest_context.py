"""GameManage.scene の SceneContexts に DragonNestSceneContext (古竜の巣) を足す / 作り直す。

    python tools/art/nest_context.py        # nest_scene.py でシーンを作り直したら、続けて流す (GameObject の GUID が変わるので)

- 砂漠の DrySandSceneContext を写し、型名を DragonNestSceneContext に替えて、SceneContexts の最後の component に足す
  (やり方は desert_context.py と同じ。浮遊石 floatingStone_ は巣に無いので外す)。
- 参照先の付け替え: シーンの中の物 (スポーン地点・敵の湧き地点のルート・到着カメラ・カメラの Brain・NetworkRunner) は、
  砂漠のシーンで同じ名前の道筋にある物の GUID から、巣のシーンの GUID へ。読むシーン = DragonNestScene。
  BGM と到着のポータルは砂漠のまま。
- クリア条件は古竜 (EnemyKind::AncientDragon) -> AncientDragonDefeated。空撮は無し (巣の地形に合わせた座標がまだ無い)。
- 古竜を倒した後の「心臓が空へ散る」演出の参照 (版 1): 心臓の山 (Nest/Hearts)・漂う物 (Nest/Floating)・
  山を映すカメラ (NestEndingCamera。tools/art/ancient_dragon.py が置く)・色ごとの光の尾・山の閃光。
"""
import copy
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.common import meta_base  # noqa: E402
from tools.scene import reader  # noqa: E402

from game_over_prefab import asset_guid  # noqa: E402
from desert_context import MSB, find_block, index_scene, renumber_ids, replace_guids, strip_versions  # noqa: E402

GAME_MANAGE = REPO / 'Assets' / 'Scene' / 'GameManage.scene'
DESERT = REPO / 'Assets' / 'Scene' / 'DesertScene.scene'
NEST = REPO / 'Assets' / 'Scene' / 'DragonNestScene.scene'
SRC_TYPE = 'GameCore::Scene::DrySandSceneContext'
NEW_TYPE = 'GameCore::Scene::DragonNestSceneContext'
DROP_KEYS = ('floatingStone_',)
# シーンの中を指す FIELD。どれも巣に同じ名前の物がある
SCENE_REFS = ('playerSpawnPoint_', 'networkRunner_', 'enemySpawnPointsRoot_', 'arrivalCamera_', 'cameraBrain_')
KIND_ANCIENT_DRAGON = 7          # EnemyKind::AncientDragon
FLAG_ANCIENT_DRAGON_DEFEATED = 9  # StoryFlag::AncientDragonDefeated
PARTICLE_DIR = REPO / 'Assets' / 'Prefab' / 'Particle'
# 心臓の色ごとの光の尾と、山の閃光 (DragonNestSceneContext の版 1)
ENDING_PREFABS = {'greenHeartTrail_': 'GreenStoneFlight', 'lightHeartTrail_': 'LightStoneFlight',
                  'fireHeartTrail_': 'FireStoneFlight', 'heartBurst_': 'HeartShardScatter'}
ENDING_VALUES = {'heartMoundCenter_': (750.0, 124.0, 720.0), 'endingDelay_secs_': 1.0, 'heartRise_secs_': 1.8,
                 'heartFly_secs_': 3.2, 'heartStagger_secs_': 2.4, 'endingHold_secs_': 3.0, 'heartRiseHeight_': 60.0,
                 'heartFlyDistance_': 3000.0}


def field_guid(blob):
    return re.search(r"'value_', '([0-9A-F-]{36})'", repr(blob)).group(1)


def main():
    text = GAME_MANAGE.read_text(encoding='utf-8')
    old = find_block(text, NEW_TYPE)
    if old:
        # 作り直し: 前に足したものを外し、その components_ の数を1つ減らしてから足し直す
        s, j, k = old
        comps_start = text.rfind('"components_": {', 0, s)
        before = text.rfind(',', 0, s)
        text = text[:before] + text[k + 1:]
        m = re.compile(r'"componentCount": (\d+)').search(text, comps_start)
        text = text[:m.start(1)] + str(int(m.group(1)) - 1) + text[m.end(1):]

    s, j, k = find_block(text, SRC_TYPE)
    block = loads(text[j:k + 1])

    desert = index_scene(DESERT)
    nest = index_scene(NEST)
    table = {g: nest[key] for key, g in desert.items() if key in nest}
    table[asset_guid(DESERT.with_suffix('.scene.meta'))] = asset_guid(NEST.with_suffix('.scene.meta'))

    data = block['ptr_wrapper']['data']
    for key in DROP_KEYS:
        data.pop(key)

    strip_versions(block)
    replace_guids(block, table)
    base = data['value0']
    # SceneContextBase の中 (読むシーン・スポーン地点) と、このクラスの FIELD の両方を確かめる
    missing = [key for key in SCENE_REFS
               if field_guid(data.get(key) or base[key]) not in table.values()]
    if missing:
        sys.exit(f'not found in {NEST.name}: {missing}')

    data['value0']['value0']['guid_']['value_'] = meta_base.mint_guid()
    data['clearEnemyKind_'] = Num.of_int(KIND_ANCIENT_DRAGON)
    data['clearStoryFlag_'] = Num.of_int(FLAG_ANCIENT_DRAGON_DEFEATED)
    data['arrivalOverview_msecs_'] = Num.of_int(0)

    # 版 1: 古竜を倒した後の演出。FIELD の形は同じ型の既存の FIELD を写す (版キーは strip_versions で外してある)
    def field(template_key, guid):
        blob = copy.deepcopy(data[template_key])
        blob['value0']['ptr_wrapper']['data']['value0']['value_'] = guid
        return blob

    for key, path in (('heartsRoot_', ('Nest', 'Hearts')), ('floatingRoot_', ('Nest', 'Floating'))):
        data[key] = field('enemySpawnPointsRoot_', nest[(path, None)])
    camera = next(g for (p, fqn), g in nest.items()
                  if p == ('NestEndingCamera',) and fqn and fqn.endswith('::CineMachineVirtualCamera'))
    data['endingCamera_'] = field('arrivalCamera_', camera)
    for key, name in ENDING_PREFABS.items():
        data[key] = field('arrivalPortalPrefab_', asset_guid(PARTICLE_DIR / f'{name}.prefab.meta'))
    for key, value in ENDING_VALUES.items():
        if isinstance(value, tuple):
            data[key] = OrderedObj([(f'value{i}', Num.of_float(v)) for i, v in enumerate(value)])
        else:
            data[key] = Num.of_float(value)
    # 新しい型なので、版と polymorphic_name を付ける
    block['ptr_wrapper']['data'] = OrderedObj([('cereal_class_version', Num.of_int(1))] + list(data.items()))
    poly_ids = [int(x) for x in re.findall(r'"polymorphic_id": (\d+)', text) if int(x) & MSB and int(x) != 1073741824]
    block['polymorphic_id'] = Num.of_int(max(poly_ids) + 1)
    block['polymorphic_name'] = NEW_TYPE
    ids = [int(x) for x in re.findall(r'"id": (\d+)', text)]
    renumber_ids(block['ptr_wrapper'], [max(ids) + 1])

    # SceneContexts の最後の component の後ろへ
    comps_start = text.rfind('"components_": {', 0, s)
    count_m = re.compile(r'"componentCount": (\d+)').search(text, comps_start)
    count = int(count_m.group(1))
    last = text.rfind(f'"component_{count - 1}": ', comps_start)
    lj = text.find('{', last)
    depth, lk = 0, lj
    while True:
        if text[lk] == '{':
            depth += 1
        elif text[lk] == '}':
            depth -= 1
            if depth == 0:
                break
        lk += 1
    line_start = text.rfind('\n', 0, last) + 1
    indent = text[line_start:last]
    body = dumps(block).strip().replace('\n', '\n' + indent)
    text = text[:lk + 1] + f',\n{indent}"component_{count}": ' + body + text[lk + 1:]
    text = text[:count_m.start(1)] + str(count + 1) + text[count_m.end(1):]

    GAME_MANAGE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(GAME_MANAGE)
    print(f'added {NEW_TYPE} as component_{count}')


if __name__ == '__main__':
    main()
