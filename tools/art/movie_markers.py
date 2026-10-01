"""演出 (Movie) が使う位置のマーカーとカメラを各シーンに置き、GameManage.scene のコンテキストと BT から FIELD で指す。

    python tools/art/movie_markers.py [--dry-run]     # 何度流してもよい (同じ名前の物は GUID を保って置き直す)

- 各シーンの根に MovieMarkers を置き、その子に並べる。位置は下の MARKERS がワールド座標で持つ。
  名前が ...CameraStart / ...CameraEnd の物は VirtualCamera (同じシーンの ArrivalCamera から Follow を外した写し、
  優先度 -1)。LookAt の target は同じ頭の ...LookAt のマーカー。それ以外は部品なしの空の GameObject。
- 到着の空撮 (GrassLand / DrySand / DragonNest の SceneContext 版 17 / 5 / 3):
  arrivalOverviewStartCamera_ / EndCamera_ と、arrivalTourShots_ の各ショットの startCamera / endCamera
  (StageArrivalTourShot 版 1)。始めのカメラへ切ってから、終わりのカメラへ Brain の補間で動く (StageArrivalMovie)。
  ショットの名前・一言・尺はコンテキストに書いてあるものをそのまま使う。巣は空撮が無いので空の FIELD。
- 巣の終幕: DragonNestSceneContext::heartMoundCenterPos_ (心臓の山の中心)。
- 序章で崩れ落ちる橋・島・船: FirstEventDragon.enemyBehaviourData の各 FallIsland::pivotPos_ (版 2。傾きの中心)。
- desert_context.py / nest_context.py でコンテキストを作り直したら、続けてこれを流す。
"""
import argparse
import copy
import math
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common import cereal_json as cj  # noqa: E402
from tools.common.cereal_json import to_file_bytes  # noqa: E402
from tools.scene import edits, model, reader, validate, writer  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import bake_world_matrices, walk  # noqa: E402

SCENE_DIR = REPO / 'Assets' / 'Scene'
GAME_MANAGE = SCENE_DIR / 'GameManage.scene'
DRAGON_BT = REPO / 'Assets' / 'Data' / 'EnemyBehaviour' / 'FirstEventDragon.enemyBehaviourData'
ROOT_NAME = 'MovieMarkers'
CAMERA_SOURCE = 'ArrivalCamera'
CAMERA_LEAVES = ('CineMachineVirtualCamera', 'VirtualCameraLookAtBehaviour', 'NoiseCameraBehaviour')
CAMERA_SUFFIXES = ('CameraStart', 'CameraEnd')
NO_GUID = '00000000-0000-0000-0000-000000000000'
FIELD_POLY_ID = 1073741824

# シーンごとのマーカー (名前, ワールド座標)。数値は座標で持っていたころの値をそのまま移した
MARKERS = {
    'GrassLandScene.scene': [
        ('OverviewCameraStart', (1450.0, 520.0, 180.0)),
        ('OverviewCameraEnd', (1280.0, 450.0, 340.0)),
        ('OverviewLookAt', (760.0, 110.0, 860.0)),
        ('Tour1CameraStart', (1010.0, 170.0, 560.0)),
        ('Tour1CameraEnd', (965.0, 140.0, 585.0)),
        ('Tour1LookAt', (862.0, 70.0, 700.0)),
        ('Tour2CameraStart', (1120.0, 290.0, 1290.0)),
        ('Tour2CameraEnd', (1085.0, 265.0, 1255.0)),
        ('Tour2LookAt', (975.0, 188.0, 1130.0)),
        ('Tour3CameraStart', (600.0, 300.0, 690.0)),
        ('Tour3CameraEnd', (570.0, 280.0, 720.0)),
        ('Tour3LookAt', (400.0, 165.0, 850.0)),
    ],
    'DesertScene.scene': [
        ('OverviewCameraStart', (1450.0, 500.0, 150.0)),
        ('OverviewCameraEnd', (1300.0, 430.0, 300.0)),
        ('OverviewLookAt', (760.0, 60.0, 820.0)),
        ('Tour1CameraStart', (960.0, 270.0, 170.0)),
        ('Tour1CameraEnd', (935.0, 245.0, 215.0)),
        ('Tour1LookAt', (875.0, 80.0, 440.0)),
        ('Tour2CameraStart', (1240.0, 190.0, 760.0)),
        ('Tour2CameraEnd', (1215.0, 180.0, 790.0)),
        ('Tour2LookAt', (1100.0, 95.0, 900.0)),
        ('Tour3CameraStart', (770.0, 180.0, 890.0)),
        ('Tour3CameraEnd', (740.0, 165.0, 915.0)),
        ('Tour3LookAt', (600.0, 62.0, 1040.0)),
    ],
    'DragonNestScene.scene': [
        ('HeartMoundCenter', (750.0, 124.0, 720.0)),
    ],
}

# 序章で崩れ落ちる物 (FallIsland の target_ の GUID) -> 傾きの中心のマーカー
FALL_PIVOTS = {
    '7ADA0D80-0C26-41A5-A696-5CA2F86AD6BC': ('SecondIslandBridgeFallPivot', (-146.0, 65.0, 283.0)),
    '53C19489-C97D-4E5E-AB54-46686D20A634': ('ThirdIslandBridgeFallPivot', (16.0, 144.0, 579.0)),
    'BDFE58DD-1ADC-44D2-826A-7CC705C0FF4B': ('ForthIslandBridgeFallPivot', (88.0, 228.0, 467.0)),
    '0442C26B-9A8E-4597-B0CB-23D8961D5616': ('SecondIslandFallPivot', (-287.0, 60.0, 484.0)),
    '00E15713-43BA-4502-A4D1-D0A3E0A28D4B': ('ThirdIslandFallPivot', (278.0, 150.0, 660.0)),
    '13D5A911-3ADF-4B2B-B066-ADB4596A13E4': ('AirShipFallPivot', (-17.0, 40.0, -67.0)),
}
PROLOGUE_SCENE = 'FirstTouchDownMainIsLandScene.scene'
MARKERS[PROLOGUE_SCENE] = list(FALL_PIVOTS.values())

# コンテキスト -> (シーン, 版, 空撮を持つか, そのほかの GameObject の FIELD)
CONTEXTS = {
    'GameCore::Scene::GrassLandSceneContext': ('GrassLandScene.scene', 17, True, {}),
    'GameCore::Scene::DrySandSceneContext': ('DesertScene.scene', 5, True, {}),
    'GameCore::Scene::DragonNestSceneContext': ('DragonNestScene.scene', 3, False,
                                                {'heartMoundCenterPos_': 'HeartMoundCenter'}),
}
OVERVIEW_KEYS = (('arrivalOverviewStartCamera_', 'OverviewCameraStart'),
                 ('arrivalOverviewEndCamera_', 'OverviewCameraEnd'))
# 前の形 (座標、マーカーの FIELD) のキー。流し直すたびに外す
STALE_KEYS = ('arrivalOverviewCameraStart_', 'arrivalOverviewCameraEnd_', 'arrivalOverviewLookAt_', 'heartMoundCenter_',
              'arrivalOverviewCameraStartPos_', 'arrivalOverviewCameraEndPos_', 'arrivalOverviewLookAtPos_')
SHOT_KEYS = (('startCamera', 'CameraStart'), ('endCamera', 'CameraEnd'))
STALE_SHOT_KEYS = ('cameraStart', 'cameraEnd', 'lookAt', 'cameraStartPos', 'cameraEndPos', 'lookAtPos',
                   'startCamera', 'endCamera')
SHOT_VERSION = 1
FALL_ISLAND = 'GameCore::Npc::Enemy::Behaviour::Action::FallIsland'
FALL_ISLAND_VERSION = 2


def look_rotation(pos, target):
    """pos から target を向く quaternion (x, y, z, w)。DxLib は左手系で、前は +z"""
    d = [t - p for p, t in zip(pos, target)]
    yaw = math.atan2(d[0], d[2])
    pitch = -math.atan2(d[1], math.hypot(d[0], d[2]))
    qy = (0.0, math.sin(yaw / 2), 0.0, math.cos(yaw / 2))
    qx = (math.sin(pitch / 2), 0.0, 0.0, math.cos(pitch / 2))
    ax, ay, az, aw = qy
    bx, by, bz, bw = qx
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def leaf(comp):
    return comp.fqn.rsplit('::', 1)[-1]


def camera_components(source, old_node, look_guid):
    """source (ArrivalCamera) から Follow を外して写す。前に置いた物があれば部品の GUID を保つ"""
    old_guids = {leaf(c): model.find_component_guid(c) for c in (old_node.components if old_node else [])}
    comps = []
    for comp in source.components:
        if leaf(comp) not in CAMERA_LEAVES:
            continue
        comp = copy.deepcopy(comp)
        model.set_component_guid(comp, old_guids.get(leaf(comp)) or edits.mint_guid())
        let_writer_place_versions(comp.data)
        if leaf(comp) == 'CineMachineVirtualCamera':
            comp.data['priority_']['value'] = cj.Num.of_int(-1)
        elif leaf(comp) == 'VirtualCameraLookAtBehaviour':
            edits._set_field_guid(comp.data['target_'], look_guid)
            for key in ('value0', 'value1', 'value2'):
                comp.data['lookAtTargetOffset_'][key] = cj.Num.of_float(0.0)
        comps.append(comp)
    assert [leaf(c) for c in comps] == list(CAMERA_LEAVES), [leaf(c) for c in source.components]
    return comps


def place_markers(scene_name, markers, dry_run):
    """シーンの根の MovieMarkers に markers を置き直し、名前 -> (GameObject の GUID, VirtualCamera の GUID or None) を返す"""
    path = SCENE_DIR / scene_name
    scene = reader.read_scene_file(path)
    root = next((n for n in scene.roots if n.name == ROOT_NAME), None)
    if root is None:
        root = edits.new_gameobject(ROOT_NAME)
        scene.roots.append(root)
    old = {c.name: c for c in root.transform.children}
    root.transform.local_pos = edits._vec3_from_floats((0.0, 0.0, 0.0))
    root.transform.local_rot = edits._quat_from_floats((0.0, 0.0, 0.0, 1.0))
    root.transform.local_scale = edits._vec3_from_floats((1.0, 1.0, 1.0))

    positions = dict(markers)
    nodes = {name: edits.new_gameobject(name, pos=pos, guid=old[name].guid if name in old else None)
             for name, pos in markers}
    source = None
    for name, node in nodes.items():
        suffix = next((s for s in CAMERA_SUFFIXES if name.endswith(s)), None)
        if suffix is None:
            continue
        if source is None:
            source = next(n for r in scene.roots for n in walk(r) if n.name == CAMERA_SOURCE)
        look_name = name[:-len(suffix)] + 'LookAt'
        node.transform.local_rot = edits._quat_from_floats(look_rotation(positions[name], positions[look_name]))
        node.components = camera_components(source, old.get(name), nodes[look_name].guid)
    root.transform.children = list(nodes.values())
    bake_world_matrices(root)

    text = writer.write_scene(scene)
    check(text, validate.validate_scene(scene), scene_name)
    if not dry_run:
        path.write_bytes(to_file_bytes(text))
    cameras = sum(1 for n in nodes.values() if n.components)
    print(f'{scene_name}: {ROOT_NAME} [{root.guid}] {len(markers)} markers ({cameras} cameras)')

    def camera_guid(node):
        comp = next((c for c in node.components if leaf(c) == 'CineMachineVirtualCamera'), None)
        return model.find_component_guid(comp) if comp else None

    return {name: (node.guid, camera_guid(node)) for name, node in nodes.items()}


class Ids:
    """ファイルの中で使われていない ptr_wrapper の id を振る"""

    def __init__(self, text):
        ids = [int(m) for m in re.findall(r'"id": (\d+)', text)]
        self.next = max(i & 0x7FFFFFFF for i in ids if i & 0x80000000) + 1

    def take(self):
        value = 0x80000000 | self.next
        self.next += 1
        return cj.Num.of_int(value)


def field_blob(guid, ids):
    # NOTE: Field<IGameObject> / Field<CineMachineVirtualCamera> はどちらのファイルでも前に出てくるので、版キーは付けない
    data = cj.OrderedObj([('value0', cj.OrderedObj([('value_', guid)]))])
    inner = cj.OrderedObj([('polymorphic_id', cj.Num.of_int(FIELD_POLY_ID)),
                           ('ptr_wrapper', cj.OrderedObj([('id', ids.take()), ('data', data)]))])
    return cj.OrderedObj([('value0', inner)])


def find_polymorphic(doc, names):
    """polymorphic_name が names の物。名前は型の初出にしか書かれないので、2回目以降は polymorphic_id で拾う"""
    found = []
    ids = {}

    def visit(node):
        if isinstance(node, cj.OrderedObj):
            name = node.get('polymorphic_name')
            poly = node.get('polymorphic_id')
            if isinstance(name, str) and name in names:
                ids[poly.value & 0x7FFFFFFF] = name
            elif isinstance(poly, cj.Num) and 'ptr_wrapper' in node:
                name = ids.get(poly.value & 0x7FFFFFFF)
            if isinstance(name, str) and name in names:
                found.append((name, node))
            for value in node.values():
                visit(value)
        elif isinstance(node, list):
            for value in node:
                visit(value)

    visit(doc)
    return found


def rebuild_shot(shot, guids, index, ids):
    pairs = []
    for key, value in shot.items():
        if key == 'cereal_class_version':
            pairs.append((key, cj.Num.of_int(SHOT_VERSION)))
        elif key in STALE_SHOT_KEYS:
            continue
        elif key == 'duration_msecs':
            for field_key, suffix in SHOT_KEYS:
                pairs.append((field_key, field_blob(guids[f'Tour{index + 1}{suffix}'][1], ids)))
            pairs.append((key, value))
        else:
            pairs.append((key, value))
    return cj.OrderedObj(pairs)


def wire_contexts(scene_guids, dry_run):
    text = cj.read_text(GAME_MANAGE)
    doc = cj.loads(text)
    found = find_polymorphic(doc, CONTEXTS)
    missing = set(CONTEXTS) - {name for name, _ in found}
    if missing:
        sys.exit(f'contexts not found in {GAME_MANAGE.name}: {sorted(missing)}')

    ids = Ids(text)
    for name, component in found:
        scene_name, version, has_overview, extra = CONTEXTS[name]
        guids = scene_guids[scene_name]
        data = component['ptr_wrapper']['data']
        data['cereal_class_version'] = cj.Num.of_int(version)
        for key in STALE_KEYS + tuple(k for k, _ in OVERVIEW_KEYS) + tuple(extra):
            data.pop(key, None)

        shots = data.get('arrivalTourShots_', [])
        tour_count = sum(1 for n in guids if n.startswith('Tour') and n.endswith('LookAt'))
        if len(shots) != tour_count:
            sys.exit(f'{name}: {len(shots)} tour shots in the context, {tour_count} in MARKERS')
        data['arrivalTourShots_'] = [rebuild_shot(shot, guids, i, ids) for i, shot in enumerate(shots)]

        for key, marker in OVERVIEW_KEYS:
            data.append(key, field_blob(guids[marker][1] if has_overview else NO_GUID, ids))
        for key, marker in extra.items():
            data.append(key, field_blob(guids[marker][0], ids))
        print(f'{name.split("::")[-1]} v{version}: overview={"yes" if has_overview else "no"}, '
              f'shots={len(shots)}, extra={list(extra)}')

    if not dry_run:
        cj.write_file(GAME_MANAGE, doc)
        reader.read_scene_file(GAME_MANAGE)


def wire_fall_island(guids, dry_run):
    text = cj.read_text(DRAGON_BT)
    doc = cj.loads(text)
    found = find_polymorphic(doc, {FALL_ISLAND})
    if not found:
        sys.exit(f'{FALL_ISLAND} not found in {DRAGON_BT.name}')

    ids = Ids(text)
    for _, action in found:
        data = action['ptr_wrapper']['data']
        target = re.search(r"'value_', '([0-9A-F-]{36})'", repr(data['target_'])).group(1)
        if target not in FALL_PIVOTS:
            sys.exit(f'FallIsland target {target} has no entry in FALL_PIVOTS')
        pivot_guid = guids[FALL_PIVOTS[target][0]][0]
        pairs = []
        for key, value in data.items():
            if key == 'cereal_class_version':
                value = cj.Num.of_int(FALL_ISLAND_VERSION)
            if key in ('pivot_', 'pivotPos_'):
                pairs.append(('pivotPos_', field_blob(pivot_guid, ids)))
                continue
            pairs.append((key, value))
        action['ptr_wrapper']['data'] = cj.OrderedObj(pairs)
    if not dry_run:
        cj.write_file(DRAGON_BT, doc)
    print(f'{DRAGON_BT.name}: {len(found)} FallIsland -> pivotPos_')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    scene_guids = {name: place_markers(name, markers, args.dry_run) for name, markers in MARKERS.items()}
    wire_contexts(scene_guids, args.dry_run)
    wire_fall_island(scene_guids[PROLOGUE_SCENE], args.dry_run)


if __name__ == '__main__':
    main()
