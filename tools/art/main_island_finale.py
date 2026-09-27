"""拠点の島 (MainIslandScene) に、砂漠の後に戻る光の心臓と、古竜の巣へ向かう演出のカメラを置く (docs/Story.md 終章)。

    python tools/art/main_island_finale.py       # 何度流してもよい (前に置いた物は置き直す)

- 光の心臓 (LightFloatingStone): シーンの緑の心臓 (IslandHeart/GreenFloatingStone) を写し、石のモデル・オーラ・光の尾・
  はまる瞬間の光を光の心臓のものに替える (ステージ用の LightFloatingStone.prefab は火口と当たり判定を持つので使わない)。
  緑の石と並べて島の底の先端にぶら下げる。飛んでくる向きだけ緑と変える。カメラは緑と同じ GreenStoneCamera。
- NestDepartureCamera: 島が北の嵐 (古竜の巣) へ引かれていく演出のカメラ。草原の GreenStoneCamera と同じ部品で、
  向きは置いた Transform のまま (MainIslandScene::BeginNestDeparture は LookAt の的を替えない)。
- GameManage.scene の MainIslandSceneContext (版 6) に lightStone_ / nestDepartureCamera_ / nestDepartureSound_ /
  nestDeparture_secs_ を入れる。
"""
import copy
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model as scene_model, reader, validate, writer  # noqa: E402

from camp_people import asset_guid  # noqa: E402
from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, walk  # noqa: E402
from desert_scene import component, find, set_trs  # noqa: E402
from desert_context import find_block  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
GAME_MANAGE = REPO / 'Assets' / 'Scene' / 'GameManage.scene'
STORY_PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Story'
PARTICLE = REPO / 'Assets' / 'Prefab' / 'Particle'
CONTEXT = 'GameCore::Scene::MainIslandSceneContext'

# 緑の心臓 (IslandHeart の子、local (0, -105, 95)) の隣。緑の石のカメラ (GreenStoneCamera) から2つ並んで見える所 (AutoMCP で確認)
LIGHT_STONE_POS = (-48.0, -98.0, 72.0)
# はまる位置から見た飛び始めの位置 (緑は (900, -350, 900))。砂の島の方から来る
LIGHT_START_OFFSET = (-900.0, -300.0, 900.0)
# 島の南の外から、島と北の空 (+z) を一緒に映す
DEPARTURE_CAMERA_POS = (40.0, 70.0, -420.0)
# 北 (+z) を向いて、少し見上げる (quaternion x, y, z, w。オイラー (-5, 0, 0))
DEPARTURE_CAMERA_ROT = (-0.0436194, 0.0, 0.0, 0.9990482)
DEPARTURE_SECS = 6.0
# 序章で心臓が抜けるときの地鳴り (FirstEventDragon の BT の "Heart Break Quake Sound")
DEPARTURE_SOUND = 'Heart Break Quake Sound'

PROLOGUE_SCENE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
SHAKE_SOURCE = 'Heart Dive Camera'   # 序章の、揺れる VirtualCamera


def shake_component():
    """序章の Heart Dive Camera の ShakeCameraBehaviour の写し (揺れは部品を持つ VirtualCamera にしか効かない)。
    付けた GameObject ごと _remint_guids すること"""
    scene = reader.read_scene_file(PROLOGUE_SCENE)
    node = next(n for r in scene.roots for n in walk(r) if n.name == SHAKE_SOURCE)
    return copy.deepcopy(component(node, 'ShakeCameraBehaviour'))


GUID = re.compile(r'[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}')


def prefab_guids(prefab, node_name, leaf):
    model = reader.read_prefab_file(prefab)
    node = next(n for n in walk(model.root) if n.name == node_name)
    comp = component(node, leaf)
    # 先頭は部品自身の GUID。続くのが参照しているアセット
    return GUID.findall(repr(comp.data))[1:]


def replace_guids(obj, table):
    """Ver / OrderedObj / list / Ptr を辿って、'value_' の GUID を table で置き換える"""
    if isinstance(obj, Ver):
        replace_guids(obj.body, table)
    elif isinstance(obj, OrderedObj):
        for key, v in obj.items():
            if key == 'value_' and isinstance(v, str) and v.upper() in table:
                obj[key] = table[v.upper()]
            else:
                replace_guids(v, table)
    elif isinstance(obj, list):
        for v in obj:
            replace_guids(v, table)
    elif hasattr(obj, 'data'):
        replace_guids(obj.data, table)


def departure_sound():
    from tools.bt import reader as bt_reader, model as bt_model
    tree = bt_reader.read_tree_file(REPO / 'Assets/Data/EnemyBehaviour/FirstEventDragon.enemyBehaviourData')
    for node, *_ in tree.walk():
        if isinstance(node, bt_model.Action) and node.name == DEPARTURE_SOUND:
            return GUID.findall(repr(node.params))[0]
    raise SystemExit(f'no action {DEPARTURE_SOUND!r} in FirstEventDragon BT')


def build_scene():
    scene = reader.read_scene_file(SCENE)
    heart = find(scene, 'IslandHeart')
    heart.transform.children = [c for c in heart.transform.children if c.name != 'LightFloatingStone']
    scene.roots = [r for r in scene.roots if r.name != 'NestDepartureCamera']

    green = find(scene, 'IslandHeart', 'GreenFloatingStone')
    light = copy.deepcopy(green)
    edits._remint_guids(light, {})
    light.name = 'LightFloatingStone'
    green_model, = prefab_guids(STORY_PREFAB / 'GreenFloatingStone.prefab', 'Stone', 'ModelRenderer')
    light_model, = prefab_guids(STORY_PREFAB / 'LightFloatingStone.prefab', 'Stone', 'ModelRenderer')
    green_aura, = prefab_guids(STORY_PREFAB / 'GreenFloatingStone.prefab', 'GreenCoreAura', 'ParticleSystem')
    light_aura, = prefab_guids(STORY_PREFAB / 'LightFloatingStone.prefab', 'LightCoreAura', 'ParticleSystem')
    table = {green_model: light_model, green_aura: light_aura,
             asset_guid(PARTICLE / 'GreenStoneFlight.prefab.meta'): asset_guid(PARTICLE / 'LightStoneFlight.prefab.meta'),
             asset_guid(PARTICLE / 'GreenStoneDock.prefab.meta'): asset_guid(PARTICLE / 'LightStoneLiftOff.prefab.meta')}
    for node in walk(light):
        if node.name == 'GreenCoreAura':
            node.name = 'LightCoreAura'
        for comp in node.components:
            replace_guids(comp.data, table)
    stone = component(light, 'FloatingStone')
    shot = stone.data['returnShot_']
    body = shot.body if isinstance(shot, Ver) else shot
    for i, v in enumerate(LIGHT_START_OFFSET):
        body['startOffset'][f'value{i}'] = Num.of_float(v)
    set_trs(light, pos=LIGHT_STONE_POS)
    heart.transform.children.append(light)

    camera = copy.deepcopy(find(scene, 'GreenStoneCamera'))
    # NOTE: 向きは Transform で決めるので LookAt は外す (的が無いまま残すと向きを奪われかねない)
    camera.components = [c for c in camera.components if not c.fqn.endswith('::VirtualCameraLookAtBehaviour')]
    camera.components.append(shake_component())
    edits._remint_guids(camera, {})
    camera.name = 'NestDepartureCamera'
    set_trs(camera, pos=DEPARTURE_CAMERA_POS)
    camera.transform.local_rot = edits._quat_from_floats(DEPARTURE_CAMERA_ROT)
    index = next(i for i, r in enumerate(scene.roots) if r.name == 'GreenStoneCamera')
    scene.roots.insert(index + 1, camera)

    for r in scene.roots:
        for node in walk(r):
            for comp in node.components:
                let_writer_place_versions(comp.data)
        bake_world_matrices(r)
    text = writer.write_scene(scene)
    tree = loads(text)
    StrayVersionStripper(catalog_mod.load(), '/').run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    scene = reader.read_scene_file(SCENE)

    light = find(scene, 'IslandHeart', 'LightFloatingStone')
    camera = find(scene, 'NestDepartureCamera')
    stone_guid = scene_model.find_component_guid(component(light, 'FloatingStone'))
    camera_guid = scene_model.find_component_guid(component(camera, 'CineMachineVirtualCamera'))
    print(f'placed LightFloatingStone {LIGHT_STONE_POS} and NestDepartureCamera {DEPARTURE_CAMERA_POS}')
    return stone_guid, camera_guid


def field_text(guid, next_id, indent):
    return (f'{{\n{indent}    "value0": {{\n{indent}        "polymorphic_id": 1073741824,\n'
            f'{indent}        "ptr_wrapper": {{\n{indent}            "id": {next_id},\n'
            f'{indent}            "data": {{\n{indent}                "value0": {{\n'
            f'{indent}                    "value_": "{guid}"\n{indent}                }}\n'
            f'{indent}            }}\n{indent}        }}\n{indent}    }}\n{indent}}}')


def wire_context(stone_guid, camera_guid, sound_guid):
    raw = GAME_MANAGE.read_bytes()
    text = raw.decode('utf-8-sig')
    crlf = '\r\n' in text
    text = text.replace('\r\n', '\n')
    s, j, k = find_block(text, CONTEXT)
    block = text[j:k + 1]

    # fountainIsland_ の閉じ括弧の後ろから data の閉じ括弧までを捨てる (前に足したものは外して足し直す)
    depth, i = 0, block.index('{', block.index('"fountainIsland_"'))
    while True:
        if block[i] == '{':
            depth += 1
        elif block[i] == '}':
            depth -= 1
            if depth == 0:
                break
        i += 1
    end_of_fountain = i + 1
    indent = re.search(r'\n(\s*)"fountainIsland_"', block).group(1)
    rest = block[end_of_fountain:]
    rest = rest[rest.index('\n' + indent[:-4] + '}'):]
    ids = [int(x) for x in re.findall(r'"id": (\d+)', text)]
    next_id = max(ids) + 1
    extra = (f',\n{indent}"lightStone_": ' + field_text(stone_guid, next_id, indent)
             + f',\n{indent}"nestDepartureCamera_": ' + field_text(camera_guid, next_id + 1, indent)
             + f',\n{indent}"nestDepartureSound_": ' + field_text(sound_guid, next_id + 2, indent)
             + f',\n{indent}"nestDeparture_secs_": {DEPARTURE_SECS}')
    block = block[:end_of_fountain] + extra + rest
    block = re.sub(r'("data": \{\s*"cereal_class_version": )\d+', r'\g<1>6', block, count=1)
    text = text[:j] + block + text[k + 1:]
    if crlf:
        text = text.replace('\n', '\r\n')
    GAME_MANAGE.write_bytes((b'\xef\xbb\xbf' if raw[:3] == b'\xef\xbb\xbf' else b'') + text.encode('utf-8'))
    reader.read_scene_file(GAME_MANAGE)
    print(f'wired {CONTEXT} v6 (lightStone_ {stone_guid}, nestDepartureCamera_ {camera_guid}, sound {sound_guid})')


def main():
    stone_guid, camera_guid = build_scene()
    wire_context(stone_guid, camera_guid, departure_sound())


if __name__ == '__main__':
    main()
