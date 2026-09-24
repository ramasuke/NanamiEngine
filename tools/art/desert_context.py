"""GameManage.scene の SceneContexts に DrySandSceneContext (砂漠) を足す / 作り直す。

    python tools/art/desert_context.py        # desert_scene.py でシーンを作り直したら、続けて流す

- シーンのコンテキストは tools.scene のカタログに無い (docs: memory scene-context-fields) ので、JSON を直接いじる。
  草原の GrassLandSceneContext を写し、型名を DrySandSceneContext に替えて、SceneContexts の最後の component に足す。
  写したものは同じ型が2回目以降になるので、中の cereal_class_version はすべて消し、新しい型の版 (0) だけ付ける。
  ptr_wrapper の id と polymorphic_id は、ファイルの中で使われていない番号を振る。
- 参照先の付け替え:
  * シーンの中の物 (スポーン地点・敵の湧き地点のルート・到着カメラ・カメラの Brain・NetworkRunner・浮遊石とカメラ) は、
    草原のシーンで同じ名前の道筋にある物の GUID から、砂漠のシーンの GUID へ。浮遊石は GreenFloatingStone -> LightFloatingStone
  * アセット: 読むシーン = DesertScene、浮遊石のエフェクト = LightStoneLiftOff / LightStoneFlight
  * クリア条件: 骸竜 (EnemyKind 6) を倒したら DesertCleared (StoryFlag 5)
"""
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.common import meta_base  # noqa: E402
from tools.scene import model, reader  # noqa: E402

from game_over_prefab import asset_guid  # noqa: E402
from grassland_nature_scatter import walk  # noqa: E402

GAME_MANAGE = REPO / 'Assets' / 'Scene' / 'GameManage.scene'
GRASS = REPO / 'Assets' / 'Scene' / 'GrassLandScene.scene'
DESERT = REPO / 'Assets' / 'Scene' / 'DesertScene.scene'
SRC_TYPE = 'GameCore::Scene::GrassLandSceneContext'
NEW_TYPE = 'GameCore::Scene::DrySandSceneContext'
CLEAR_ENEMY_KIND, CLEAR_STORY_FLAG = 6, 5
RENAMES = {'GreenFloatingStone': 'LightFloatingStone', 'GreenStoneCamera': 'LightStoneCamera'}
MSB = 0x80000000


def index_scene(path, rename=None):
    """道筋 (名前の列, 部品の型) -> GUID。GameObject は (path, None)、部品は (path, fqn)"""
    scene = reader.read_scene_file(path)
    out = {}

    def rec(node, prefix):
        name = (rename or {}).get(node.name, node.name)
        p = prefix + (name,)
        out[(p, None)] = node.guid
        for c in node.components:
            g = model.find_component_guid(c)
            if g:
                out[(p, c.fqn)] = g
        for child in node.transform.children:
            rec(child, p)

    for r in scene.roots:
        rec(r, ())
    return out


def find_block(text, type_name):
    """"component_N": { ... polymorphic_name type_name ... } の (component の開始, { の位置, } の位置)"""
    i = text.find(f'"polymorphic_name": "{type_name}"')
    if i < 0:
        return None
    s = text.rfind('"component_', 0, i)
    j = text.find('{', s)
    depth, k = 0, j
    while True:
        if text[k] == '{':
            depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0:
                return s, j, k
        k += 1


def strip_versions(obj):
    if isinstance(obj, OrderedObj):
        if 'cereal_class_version' in obj.keys():
            obj.pop('cereal_class_version')
        for _, v in obj.items():
            strip_versions(v)


def renumber_ids(obj, next_id):
    if isinstance(obj, OrderedObj):
        for key, v in obj.items():
            if key == 'id' and isinstance(v, Num):
                obj[key] = Num.of_int(next_id[0])
                next_id[0] += 1
            else:
                renumber_ids(v, next_id)


def replace_guids(obj, table):
    if isinstance(obj, OrderedObj):
        for key, v in obj.items():
            if key == 'value_' and isinstance(v, str) and v in table:
                obj[key] = table[v]
            else:
                replace_guids(v, table)


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

    grass = index_scene(GRASS)
    desert = index_scene(DESERT)
    by_guid = {g: key for key, g in grass.items()}
    table = {}
    for guid, (path, fqn) in by_guid.items():
        new_path = tuple(RENAMES.get(p, p) for p in path)
        if path and path[0] == 'Settlement':
            new_path = tuple(RENAMES.get(p, p) for p in path[path.index('GreenFloatingStone'):]) \
                if 'GreenFloatingStone' in path else new_path
        if (new_path, fqn) in desert:
            table[guid] = desert[(new_path, fqn)]
    art = REPO / 'Assets'
    table[asset_guid(GRASS.with_suffix('.scene.meta'))] = asset_guid(DESERT.with_suffix('.scene.meta'))
    table[asset_guid(art / 'Prefab/Particle/GreenStoneLiftOff.prefab.meta')] = asset_guid(art / 'Prefab/Particle/LightStoneLiftOff.prefab.meta')
    table[asset_guid(art / 'Prefab/Particle/GreenStoneFlight.prefab.meta')] = asset_guid(art / 'Prefab/Particle/LightStoneFlight.prefab.meta')

    data = block['ptr_wrapper']['data']
    refs = re.findall(r'"value_": "([0-9A-F-]{36})"', text[j:k + 1])
    base_guid = data['value0']['value0']['guid_']['value_']
    unmapped = [g for g in refs if g not in table and g != base_guid]

    strip_versions(block)
    replace_guids(block, table)
    data['value0']['value0']['guid_']['value_'] = meta_base.mint_guid()
    data['clearEnemyKind_'] = Num.of_int(CLEAR_ENEMY_KIND)
    data['clearStoryFlag_'] = Num.of_int(CLEAR_STORY_FLAG)
    # 新しい型なので、版と polymorphic_name を付ける
    new_data = OrderedObj([('cereal_class_version', Num.of_int(0))] + list(data.items()))
    block['ptr_wrapper']['data'] = new_data
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
    body = dumps(block).strip()
    body = body.replace('\n', '\n' + indent)
    text = text[:lk + 1] + f',\n{indent}"component_{count}": ' + body + text[lk + 1:]
    text = text[:count_m.start(1)] + str(count + 1) + text[count_m.end(1):]

    GAME_MANAGE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(GAME_MANAGE)
    print(f'added {NEW_TYPE} as component_{count}; mapped {len([g for g in refs if g in table])} refs')
    if unmapped:
        print('kept as-is (assets shared with the grassland):', sorted(set(unmapped)))


if __name__ == '__main__':
    main()
