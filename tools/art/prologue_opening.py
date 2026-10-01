"""序章 (FirstTouchDownMainIsLandScene) の冒頭のカットを置く。

    python tools/art/prologue_opening.py       # 何度流してもよい (前に置いたカットは置き直す)

- AbordAirShipMovie/OpeningShots: 子の VirtualCamera を上から順に映す (AboardAirShipMovie::AirShipMovieOpeningShotsAsync)。
  各カメラの子 End (VirtualCamera) が動いていく先で、Brain の補間で動かす。最後のカットの End は合流の経由点。位置と向きは AutoMCP で見て決めた (船は (40, 26, -500) で +z の島へ向かう)。
    1. WideShot: 雲海の近くから、島々の手前を進む船を斜め後ろから見上げ、上がりながら寄る。タイトルロゴはここ
    2. HullShot: 船腹と翼に沿って船首の方へ抜ける
    3. BowShot: 船首の先に拠点の島。島へ寄る
- SecondMove/VirtualCamera (secondVirtualCamera_): 甲板のカメラ。メインマストの右から船首楼へ向けて、奥に島が入る
- AirShip/PlayerSpawnPosition → PlayerFirstMoveTargetPos: メインマストの右から船首楼の階段の下へ歩く (島の方を向く)
- FirstMove/VirtualCamera: もう使わないので優先度を -1 にする
- GameManage.scene の FirstTouchDownMainIsLandSceneContext (版 23) に openingShots_ / openingShotDurations_secs_ を入れ、
  外した firstVirtualCamera_ / virtualCameraFirstMoveTarget_ / virtualCameraFirstMoveTargetDuring_msecs_ を消す
"""
import copy
import math
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, to_file_bytes  # noqa: E402
from tools.scene import edits, reader, validate, writer  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import bake_world_matrices, walk  # noqa: E402
from desert_scene import component, find  # noqa: E402
from desert_context import find_block  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
GAME_MANAGE = REPO / 'Assets' / 'Scene' / 'GameManage.scene'
CONTEXT = 'GameCore::Scene::FirstTouchDownMainIsLandSceneContext'
CONTEXT_VERSION = 23

# (名前, 秒, 始めの位置, 始めの向き, 終わりの位置, 終わりの向き)。向きは quaternion (x, y, z, w)、ワールド
SHOTS = [
    ('WideShot', 6.0,
     (-60.0, -12.0, -610.0), (-0.0985641, 0.3291962, 0.0345744, 0.9384666),
     (-40.0, 12.0, -572.0), (-0.0604464, 0.3445804, 0.0222399, 0.9365447)),
    ('HullShot', 4.5,
     (-10.0, 4.0, -548.0), (-0.0915222, 0.3901441, 0.0390084, 0.9153631),
     (-4.0, 8.0, -520.0), (-0.0863383, 0.3707814, 0.0346439, 0.9240490)),
    ('BowShot', 4.0,
     (62.0, 28.0, -470.0), (-0.0021048, -0.0655317, -0.0001382, 0.9978483),
     (56.0, 32.0, -448.0), (0.0044211, -0.0621368, 0.0002752, 0.9980578)),
]
DECK_CAMERA_POS = (50.0, -3.0, -508.0)
DECK_CAMERA_ROT = (-0.0247780, -0.1522961, -0.0038194, 0.9880168)
PLAYER_SPAWN_POS = (46.0, -8.6, -500.0)
PLAYER_WALK_TARGET_POS = (44.5, -8.6, -481.0)
# NOTE: 前の歩き (右舷の手すりまで約 20 を 1.5 秒) と同じ速さ
PLAYER_WALK_MSECS = 1500

IDENTITY = (0.0, 0.0, 0.0, 1.0)


def floats(v):
    names = ('x', 'y', 'z', 'w') if hasattr(v, 'w') else ('x', 'y', 'z')
    return tuple(float(getattr(v, n).value) for n in names)


def q_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def q_normalize(q):
    n = math.sqrt(sum(c * c for c in q))
    return IDENTITY if n < 1e-6 else tuple(c / n for c in q)


def q_inverse(q):
    x, y, z, w = q_normalize(q)
    return (-x, -y, -z, w)


def q_rotate(q, v):
    x, y, z, _ = q_mul(q_mul(q, (v[0], v[1], v[2], 0.0)), q_inverse(q))
    return (x, y, z)


def to_local(parent_pos, parent_rot, parent_scale, pos, rot=None):
    """parent の子としての local (位置, 向き)。スケールは一様とみなす"""
    d = tuple(p - o for p, o in zip(pos, parent_pos))
    local_pos = tuple(c / parent_scale for c in q_rotate(q_inverse(parent_rot), d))
    local_rot = q_normalize(q_mul(q_inverse(parent_rot), rot)) if rot is not None else None
    return local_pos, local_rot


def set_local(node, pos, rot=IDENTITY):
    node.transform.local_pos = edits._vec3_from_floats(tuple(float(v) for v in pos))
    node.transform.local_rot = edits._quat_from_floats(tuple(float(v) for v in rot))


def set_priority(camera_node, priority):
    component(camera_node, 'CineMachineVirtualCamera').data['priority_']['value'] = Num.of_int(priority)


def empty_node(source, name):
    node = copy.deepcopy(source)
    node.name = name
    node.components = []
    node.transform.children = []
    edits._remint_guids(node, {})
    set_local(node, (0.0, 0.0, 0.0))
    return node


def build_scene():
    scene = reader.read_scene_file(SCENE)
    movie = find(scene, 'AbordAirShipMovie')
    movie.transform.children = [c for c in movie.transform.children if c.name != 'OpeningShots']
    # NOTE: この3つは向きが (0,0,0,0) で保存されている。行列としては単位と同じだが、焼くと子の回転が消えるので単位に直す
    for node in walk(movie):
        if floats(node.transform.local_rot) == (0.0, 0.0, 0.0, 0.0):
            node.transform.local_rot = edits._quat_from_floats(IDENTITY)
    for node in (movie, find(scene, 'AbordAirShipMovie', 'FirstMove'), find(scene, 'AbordAirShipMovie', 'SecondMove')):
        assert q_normalize(floats(node.transform.local_rot)) == IDENTITY, node.name
    movie_pos = floats(movie.transform.local_pos)

    # カット: 甲板のカメラから Follow を外した写し。End は動いていく先で、最後以外は Brain の補間の行き先のカメラ
    deck_camera = find(scene, 'AbordAirShipMovie', 'SecondMove', 'VirtualCamera')
    target_template = find(scene, 'AbordAirShipMovie', 'FirstMove', 'VirtualCameraTargetPos')
    shots_root = empty_node(target_template, 'OpeningShots')
    for i, (name, _, start_pos, start_rot, end_pos, end_rot) in enumerate(SHOTS):
        shot = copy.deepcopy(deck_camera)
        shot.components = [c for c in shot.components if c.fqn.endswith('::CineMachineVirtualCamera')]
        shot.transform.children = []
        edits._remint_guids(shot, {})
        shot.name = name
        set_priority(shot, -1)
        local_start, _ = to_local(movie_pos, IDENTITY, 1.0, start_pos)
        set_local(shot, local_start, start_rot)
        end = empty_node(target_template, 'End')
        end_local_pos, end_local_rot = to_local(start_pos, start_rot, 1.0, end_pos, end_rot)
        set_local(end, end_local_pos, end_local_rot)
        # NOTE: 最後のカットは End を経由点にして追従カメラへ合流するので、カメラを載せない
        if i + 1 < len(SHOTS):
            end.components = copy.deepcopy(shot.components)
            edits._remint_guids(end, {})
        shot.transform.children.append(end)
        shots_root.transform.children.append(shot)
    movie.transform.children.append(shots_root)

    # 甲板のカメラ: Follow の的は船なので、オフセットも船から
    airship = find(scene, 'AirShip')
    ship_pos = floats(airship.transform.local_pos)
    ship_rot = q_normalize(floats(airship.transform.local_rot))
    ship_scale = float(airship.transform.local_scale.x.value)
    second_move = find(scene, 'AbordAirShipMovie', 'SecondMove')
    second_pos = tuple(a + b for a, b in zip(movie_pos, floats(second_move.transform.local_pos)))
    deck_local, _ = to_local(second_pos, IDENTITY, 1.0, DECK_CAMERA_POS)
    set_local(deck_camera, deck_local, DECK_CAMERA_ROT)
    offset = component(deck_camera, 'VirtualCameraFollowBehaviour').data['followOffset_']
    for i, v in enumerate(p - s for p, s in zip(DECK_CAMERA_POS, ship_pos)):
        offset[f'value{i}'] = Num.of_float(v)
    set_priority(deck_camera, 1)

    set_priority(find(scene, 'AbordAirShipMovie', 'FirstMove', 'VirtualCamera'), -1)
    for name, pos in (('PlayerSpawnPosition', PLAYER_SPAWN_POS), ('PlayerFirstMoveTargetPos', PLAYER_WALK_TARGET_POS)):
        local, _ = to_local(ship_pos, ship_rot, ship_scale, pos)
        set_local(find(scene, 'AirShip', name), local)

    # NOTE: 触った所だけ焼く。ほかの根には (0,0,0,0) の向きのまま置かれている物がある。
    #       StrayVersionStripper はかけない: 元のファイルの重複した版キーまで消し、初出のものも落として読めなくなった
    for node in walk(shots_root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(movie)
    airship_trs = edits._node_local_trs(airship)
    for name in ('PlayerSpawnPosition', 'PlayerFirstMoveTargetPos'):
        bake_world_matrices(find(scene, 'AirShip', name), airship_trs)
    text = writer.write_scene(scene)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    scene = reader.read_scene_file(SCENE)
    guid = find(scene, 'AbordAirShipMovie', 'OpeningShots').guid
    print(f'placed OpeningShots {guid} ({", ".join(s[0] for s in SHOTS)}), deck camera {DECK_CAMERA_POS}')
    return guid


def field_text(guid, next_id, indent):
    return (f'{{\n{indent}    "value0": {{\n{indent}        "polymorphic_id": 1073741824,\n'
            f'{indent}        "ptr_wrapper": {{\n{indent}            "id": {next_id},\n'
            f'{indent}            "data": {{\n{indent}                "value0": {{\n'
            f'{indent}                    "value_": "{guid}"\n{indent}                }}\n'
            f'{indent}            }}\n{indent}        }}\n{indent}    }}\n{indent}}}')


def member_span(block, key):
    """block 中の "key": <値> の、キーの行頭から値の終わりまで"""
    k = block.index(f'"{key}":')
    line_start = block.rindex('\n', 0, k)
    i = k + len(key) + 3
    while block[i] == ' ':
        i += 1
    if block[i] not in '{[':
        j = i
        while block[j] not in ',\n':
            j += 1
        return line_start, j
    opener, closer = block[i], '}' if block[i] == '{' else ']'
    depth = 0
    j = i
    while True:
        if block[j] == opener:
            depth += 1
        elif block[j] == closer:
            depth -= 1
            if depth == 0:
                return line_start, j + 1
        j += 1


def remove_member(block, key):
    s, e = member_span(block, key)
    if block[e] == ',':
        return block[:s] + block[e + 1:]
    # 最後のメンバーは前のカンマごと外す
    return block[:s - 1] + block[e:] if block[s - 1] == ',' else block[:s] + block[e:]


def wire_context(shots_guid):
    raw = GAME_MANAGE.read_bytes()
    text = raw.decode('utf-8-sig')
    crlf = '\r\n' in text
    text = text.replace('\r\n', '\n')
    _, j, k = find_block(text, CONTEXT)
    block = text[j:k + 1]
    indent = re.search(r'\n(\s*)"swordManCameraGroupPrefab_"', block).group(1)

    # 前に足したものは外して足し直す
    for key in ('openingShotDurations_secs_', 'openingShots_'):
        if f'"{key}"' in block:
            block = remove_member(block, key)

    # 外した FIELD。Field<CineMachineVirtualCamera> の版キーは初出の firstVirtualCamera_ にあるので secondVirtualCamera_ へ移す
    if '"firstVirtualCamera_"' in block:
        s, e = member_span(block, 'firstVirtualCamera_')
        had_version = '"cereal_class_version"' in block[s:e]
        block = remove_member(block, 'firstVirtualCamera_')
        if had_version:
            s, e = member_span(block, 'secondVirtualCamera_')
            second = block[s:e]
            if '"cereal_class_version"' not in second:
                second = second.replace('{\n', '{\n' + indent + '    "cereal_class_version": 0,\n', 1)
                second = re.sub(r'("data": \{\n)(\s*)', r'\1\2"cereal_class_version": 0,\n\2', second, count=1)
                block = block[:s] + second + block[e:]
    for key in ('virtualCameraFirstMoveTarget_', 'virtualCameraFirstMoveTargetDuring_msecs_'):
        if f'"{key}"' in block:
            block = remove_member(block, key)

    block = re.sub(r'"playerFirstMoveDuring_msecs_": \d+', f'"playerFirstMoveDuring_msecs_": {PLAYER_WALK_MSECS}', block)

    _, end_of_last = member_span(block, 'swordManCameraGroupPrefab_')
    ids = [int(x) for x in re.findall(r'"id": (\d+)', text)]
    durations = ', '.join(f'{s[1]}' for s in SHOTS)
    extra = (f',\n{indent}"openingShots_": ' + field_text(shots_guid, max(ids) + 1, indent)
             + f',\n{indent}"openingShotDurations_secs_": [\n'
             + ',\n'.join(f'{indent}    {s[1]}' for s in SHOTS) + f'\n{indent}]')
    block = block[:end_of_last] + extra + block[end_of_last:]
    block = re.sub(r'("data": \{\s*"cereal_class_version": )\d+', rf'\g<1>{CONTEXT_VERSION}', block, count=1)
    text = text[:j] + block + text[k + 1:]
    if crlf:
        text = text.replace('\n', '\r\n')
    GAME_MANAGE.write_bytes((b'\xef\xbb\xbf' if raw[:3] == b'\xef\xbb\xbf' else b'') + text.encode('utf-8'))
    reader.read_scene_file(GAME_MANAGE)
    print(f'wired {CONTEXT} v{CONTEXT_VERSION} (openingShots_ {shots_guid}, durations [{durations}])')


def main():
    wire_context(build_scene())


if __name__ == '__main__':
    main()
