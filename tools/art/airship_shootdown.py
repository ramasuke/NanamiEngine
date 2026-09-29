"""序章で古竜が飛行船を撃ち落とす演出を組む (docs/Story.md 序章 3.)。

    python tools/art/airship_shootdown.py       # 何度流してもよい (前に足した物は外して足し直す)

流れ (FirstEventDragon の BT の State0。竜が島を回り込んで船の正面に来てから):
  操作ロック → 船の甲板を空ける (ShootDownAirShip: 乗客の NPC を消し、甲板のプレイヤーを桟橋へ) → 襲撃カメラ (島の上から
  船越しに竜を LookAt) → 咆哮 → 火球 → マストに着弾 (爆発・船に付く炎と黒煙・揺れ・雷) → 墜落カメラ (南西から船を横に見る)
  → 船が船首から傾いて雲の下へ落ちる (FallIsland) → 竜が落ちる船の横を急降下し、桟橋の下から上がって着地 (既存の State10 へ)

- シーン (FirstTouchDownMainIsLandScene): DestroyIslandMovie の下に AirShip Attack Camera / AirShip Fall Camera
  (Heart Dive Camera の写し。墜落カメラは LookAt を外して向きを固定)、桟橋の上に AirShipEvacuatePoint を置き、
  AirShip (v2) の evacuatePoint_ に繋ぐ。
- ルート: ToDestroyAirShip は船の正面で止まるところまで、急降下は TouchDownIsLand の頭に付ける。
- BT: State0 の Sequence に "Ship ..." の名前でノードを足す (流し直すと "Ship ..." を外して足し直す)。
カメラの位置と向きは AutoMCP で船を停泊位置に置いて決めた。
"""
import copy
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model as scene_model, reader, validate, writer  # noqa: E402
from tools.bt import catalog as bt_catalog, edits as bt_edits, model as bt_model, reader as bt_reader, writer as bt_writer  # noqa: E402
from tools.bt import meta as bt_meta  # noqa: E402
from tools.bt.layout import auto_layout  # noqa: E402
from tools.bt.validate import validate as bt_validate  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, walk  # noqa: E402
from desert_scene import component, find, set_trs  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
TREE = REPO / 'Assets' / 'Data' / 'EnemyBehaviour' / 'FirstEventDragon.enemyBehaviourData'
ROUTES = REPO / 'Assets' / 'Data' / 'EventNpcWalkingRoute' / 'FirstEventDragon'

# 停泊中の船 (AbordAirShipMovie/SecondMove/AirShipTargetPos)。船首は -X、甲板は y≈41、マストの上は y≈120
SHIP_POS = (-17.5, 76.3, -67.2)
MAST_HIT = (-17.0, 80.0, -67.0)
DECK_FIRE = (-17.0, 60.0, -67.0)
# 桟橋 (AirShipPedestal の箱) の上面の真ん中。上面は y≈35、x -48..24、z -40..-4
EVACUATE_POS = (-12.0, 38.0, -15.0)
DECK_HALF_EXTENTS = (60.0, 60.0, 22.0)

# 竜が止まって火球を撃つ所 (船の正面 = -Z 側の空)。火球は口の少し前から出す
HOVER_POS = (10.0, 120.0, -260.0)
# NOTE: ToTouchDownIsland は向きを変えないので、ここへ入る向きが着地の向き (= 竜の子の着地カメラの向き) になる。
#       撃墜を足す前のルートの終わりと同じ yaw -24° (+Z から少し -X) で入るよう、手前に1点置く
HOVER_APPROACH = (71.0, 118.0, -397.0)
FIREBALL_FROM = (2.7, 114.0, -246.7)
FIREBALL_SPEED = 120.0          # 着弾まで約 1.5 秒

# 島の上 (船の後ろ) から船越しに南の空を見る。LookAt で竜を追う
ATTACK_CAMERA_POS = (30.0, 165.0, 70.0)
ATTACK_CAMERA_ROT = (-0.0023164, 0.9967552, -0.0741976, -0.0311182)
# 南西から船を横に見る。下に落ちていく空間を空けておく
FALL_CAMERA_POS = (-175.0, 55.0, -205.0)
FALL_CAMERA_ROT = (0.1046198, 0.3997647, -0.0459867, 0.9094658)
CAMERA_SOURCE = 'Heart Dive Camera'
ATTACK_CAMERA = 'AirShip Attack Camera'
FALL_CAMERA = 'AirShip Fall Camera'
EVACUATE_POINT = 'AirShipEvacuatePoint'
EMPTY_SOURCE = 'SampleAppearDragonPos'     # コンポーネントを持たない空の GameObject

# 船首から (z 軸まわり、正で -X が下がる) 少し横にも傾いて落ちる
FALL = dict(pivot=(-17.0, 40.0, -67.0), axis=(0.25, 0.0, 1.0), tilt=12.0, tilt_secs=1.4,
            fall_angle=30.0, distance=1400.0, fall_secs=5.5, sink=8.0)

# 元のルート (流し直しても同じになるよう、ここから組み直す)
# NOTE: 教官の Look DragonPurposeCamera は 2 点目を映すので、そこまでは会話に間に合う速さのまま。
#       島を回り込む所は秒速 100 前後で詰めた (合計 37 秒 -> 25 秒)。曲がり角は BT の rotateSpeedDeg_ 60 で間に合う
TO_AIRSHIP = [
    ((-412.9505310058594, 293.67498779296877, 616.6802368164063), 2.0),
    ((-515.930908203125, 280.8128967285156, 390.9531555175781), 7.0),
    ((-622.0043334960938, 300.54913330078127, -127.98577117919922), 4.0),
    ((-413.53778076171877, 260.98028564453127, -327.32525634765627), 2.5),
    ((-293.0191650390625, 246.77154541015626, -420.09796142578127), 1.5),
    ((-148.380126953125, 199.03660583496095, -555.305908203125), 2.0),
    ((26.165861129760743, 95.50621795654297, -584.581298828125), 2.0),
    (HOVER_APPROACH, 2.0),
    (HOVER_POS, 1.8),
]
TOUCH_DOWN = [
    # 落ちていく船の横を急降下して、桟橋の下へ潜る
    ((5.0, 70.0, -215.0), 1.6),
    ((-15.678999900817871, -10.621999740600586, -174.68600463867188), 1.4),
    ((-10.36607837677002, -29.575674057006837, -83.93424987792969), 2.0),
    ((-10.366000175476075, 81.0770034790039, -83.93399810791016), 2.0),
    ((-10.366000175476075, 81.1520004272461, 69.0999984741211), 3.0),
    ((-10.366000175476075, 60.0, 69.0999984741211), 1.0),
]

STATE0 = '04E3F0D6-E481-4609-BA33-F4FC957497E5'
PREFIX = 'Ship '
# 写し元 (FirstEventDragon の BT の既存ノード)
SRC_LOCK = 'BC7ED4A3-0E3F-4569-915C-3ACCAC5C9B47'           # Lock PlayerControl
SRC_CAMERA_ON = '2DEFB607-D7B0-4ED7-A4C3-EA42E77A36A4'      # Enable Heart Dive Camera
SRC_CAMERA_OFF = 'B91C2100-95EC-4BCF-ABC2-11AB48020CA2'     # Disable Island Fall Camera
SRC_ROAR = '208BE965-AF35-46E8-A39E-180C6DCB0195'           # Roar Sound
SRC_WAIT = 'C12A07F5-4AA8-4D7E-81F6-7A2E87736FDD'           # Heart Claw Wait
SRC_FIREBALL = 'A697D2A5-CABD-4F0C-9DAA-5FB81E8F8B50'       # FireBall1
SRC_IMPACT = '225A65B0-643B-467A-B764-62199CFC4E80'         # SecondIsland Impact Particle
SRC_BURNING = '7235BA51-2648-409B-ACF8-BB56B5801ED0'        # SecondIsland Burning Particle
SRC_SHAKE = '43734BE2-8DB0-4CFE-9137-E75596A5BC5F'          # SecondIsland Impact Shake
SRC_IMPACT_SOUND = '3AF7CEEA-CE1E-46B7-9749-870CB4BA1D90'   # SecondIsland Impact Sound
SRC_FALL = '5FBD98FB-8483-4674-97D8-99227154388C'           # Fall SecondIsland
SRC_FALL_SOUND = '34BAFAE7-8350-4D65-9C38-01038567B4E0'     # SecondIsland Fall Sound


def vec(v):
    return ','.join(str(float(x)) for x in v)


# ---------------------------------------------------------------------------
# シーン

def build_scene():
    scene = reader.read_scene_file(SCENE)
    movie = find(scene, 'DestroyIslandMovie')
    movie.transform.children = [c for c in movie.transform.children if c.name not in (ATTACK_CAMERA, FALL_CAMERA)]
    scene.roots = [r for r in scene.roots if r.name != EVACUATE_POINT]

    source = find(scene, 'DestroyIslandMovie', CAMERA_SOURCE)
    attack = copy.deepcopy(source)
    edits._remint_guids(attack, {})
    attack.name = ATTACK_CAMERA
    set_trs(attack, pos=ATTACK_CAMERA_POS)
    attack.transform.local_rot = edits._quat_from_floats(ATTACK_CAMERA_ROT)

    fall = copy.deepcopy(source)
    # NOTE: 向きは Transform で決めるので LookAt は外す (的が空だと ScenePurposeCamera が竜を追わせる)
    fall.components = [c for c in fall.components if not c.fqn.endswith('::VirtualCameraLookAtBehaviour')]
    edits._remint_guids(fall, {})
    fall.name = FALL_CAMERA
    set_trs(fall, pos=FALL_CAMERA_POS)
    fall.transform.local_rot = edits._quat_from_floats(FALL_CAMERA_ROT)

    index = movie.transform.children.index(source)
    movie.transform.children[index + 1:index + 1] = [attack, fall]

    evacuate = copy.deepcopy(find(scene, EMPTY_SOURCE))
    edits._remint_guids(evacuate, {})
    evacuate.name = EVACUATE_POINT
    evacuate.transform.children = []
    set_trs(evacuate, pos=EVACUATE_POS)
    scene.roots.append(evacuate)

    ship = find(scene, 'AirShip')
    comp = component(ship, 'AirShip')
    comp.class_version = 2
    data = comp.data
    for key in ('evacuatePoint_', 'deckHalfExtents_'):
        data.pop(key, None)
    point = copy.deepcopy(data['shootDownParticle_'])
    edits._set_field_guid(point, evacuate.guid)
    data['evacuatePoint_'] = point
    data['deckHalfExtents_'] = OrderedObj([(f'value{i}', Num.of_float(v)) for i, v in enumerate(DECK_HALF_EXTENTS)])

    for r in scene.roots:
        for node in walk(r):
            for c in node.components:
                let_writer_place_versions(c.data)
        bake_world_matrices(r)
    text = writer.write_scene(scene)
    tree = loads(text)
    StrayVersionStripper(catalog_mod.load(), '/').run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))

    scene = reader.read_scene_file(SCENE)
    cvc = lambda name: scene_model.find_component_guid(component(find(scene, 'DestroyIslandMovie', name), 'CineMachineVirtualCamera'))
    guids = dict(attack=cvc(ATTACK_CAMERA), fall=cvc(FALL_CAMERA), ship=find(scene, 'AirShip').guid)
    print(f'placed {ATTACK_CAMERA} {guids["attack"]}, {FALL_CAMERA} {guids["fall"]}, {EVACUATE_POINT} {EVACUATE_POS}')
    return guids


# ---------------------------------------------------------------------------
# ルート

def write_route(name, points):
    path = ROUTES / f'{name}.eventNpcWalkingRoute.meta'
    raw = path.read_bytes()
    doc = loads(raw.decode('utf-8-sig'))
    data = doc['value0']['ptr_wrapper']['data']
    first = next(v for k, v in data.items() if k == 'route_0')
    for key in [k for k in data.keys() if k.startswith('route_')]:
        data.pop(key)
    data['count'] = Num.of_int(len(points))
    for i, (pos, secs) in enumerate(points):
        entry = OrderedObj()
        if i == 0 and 'cereal_class_version' in first:
            entry['cereal_class_version'] = first['cereal_class_version']
        entry['position_'] = OrderedObj([(f'value{j}', Num.of_float(v)) for j, v in enumerate(pos)])
        entry['duration_secs_'] = Num.of_float(secs)
        data[f'route_{i}'] = entry
    text = dumps(doc)
    if '\r\n' in raw.decode('utf-8-sig'):
        text = text.replace('\n', '\r\n')
    path.write_bytes((b'\xef\xbb\xbf' if raw[:3] == b'\xef\xbb\xbf' else b'') + text.encode('utf-8'))
    print(f'wrote {path.name} ({len(points)} points)')


# ---------------------------------------------------------------------------
# BT

def label(node):
    if isinstance(node, bt_model.Action):
        return node.name
    child = getattr(node, 'child', None)
    if child is not None:
        return label(child)
    kids = getattr(node, 'children', None)
    return label(kids[0]) if kids else ''


def build_tree(guids):
    cat = bt_catalog.load()
    tree = bt_reader.read_tree_file(TREE)
    seq = tree.find(STATE0)
    seq.children = [c for c in seq.children if not label(c).startswith(PREFIX)]
    by_name = {label(c): c for c in seq.children}

    # 前に Once で包んだ ShootDownAirShip は元の Action に戻す
    shoot = by_name['ShootDownAirShip']
    if isinstance(shoot, bt_model.OnceExecute):
        shoot = shoot.child

    def clone(src, name):
        node = bt_edits._clone_node(tree.find(src), bt_meta.mint_guid)
        node.name = PREFIX + name
        return node

    def once(node):
        return bt_model.OnceExecute(guid=bt_meta.mint_guid(), child=node)

    def once_seq(*nodes):
        return once(bt_model.Sequence(guid=bt_meta.mint_guid(), children=list(nodes)))

    params = []   # (node, {key: value}) を付けてから set_params する

    def with_params(node, **kv):
        params.append((node, kv))
        return node

    def camera(src, name, cvc, priority):
        return with_params(clone(src, name), purposeCamera_=cvc, priority_=str(priority))

    def wait(name, secs):
        return with_params(clone(SRC_WAIT, name), waitSeconds_=str(secs))

    fireball = with_params(clone(SRC_FIREBALL, 'FireBall'), **{
        'spawnPosition_.offset_': vec(FIREBALL_FROM), 'targetPosition_.offset_': vec(MAST_HIT),
        'moveSpeed_': str(FIREBALL_SPEED)})
    fall = with_params(clone(SRC_FALL, 'Fall AirShip'), target_=guids['ship'], pivot_=vec(FALL['pivot']),
                       tiltAxis_=vec(FALL['axis']), tiltAngleDeg_=str(FALL['tilt']), tiltSecs_=str(FALL['tilt_secs']),
                       fallAngleDeg_=str(FALL['fall_angle']), fallDistance_=str(FALL['distance']),
                       fallSecs_=str(FALL['fall_secs']), tiltSinkDistance_=str(FALL['sink']))

    added = [
        once(clone(SRC_LOCK, 'Lock PlayerControl')),
        once(shoot),
        once(camera(SRC_CAMERA_ON, 'Enable Attack Camera', guids['attack'], 105)),
        once(clone(SRC_ROAR, 'Roar Sound')),
        wait('Aim Beat', 1.2),
        once(fireball),
        wait('Fireball Travel', 1.5),
        once_seq(with_params(clone(SRC_IMPACT, 'Impact Particle'), offset_=vec(MAST_HIT)),
                 with_params(clone(SRC_BURNING, 'Burning Particle'), position_=vec(DECK_FIRE), target_=guids['ship']),
                 clone(SRC_SHAKE, 'Impact Shake'),
                 clone(SRC_IMPACT_SOUND, 'Impact Sound')),
    ]
    after_lightning = [
        wait('Impact Beat', 1.0),
        once(camera(SRC_CAMERA_ON, 'Enable Fall Camera', guids['fall'], 106)),
        once(camera(SRC_CAMERA_OFF, 'Disable Attack Camera', guids['attack'], -1)),
        once(fall),
        once(clone(SRC_FALL_SOUND, 'Fall Sound')),
        wait('Fall Watch', 2.0),
    ]
    after_touch_down = [once(camera(SRC_CAMERA_OFF, 'Disable Fall Camera', guids['fall'], -1))]

    order = []
    for c in seq.children:
        name = label(c)
        if name == 'ShootDownAirShip':
            order += added
            continue
        order.append(c)
        if name == 'AirShip Fall Lightning':
            order += after_lightning
        elif name == 'ToTouchDownIsland':
            order += after_touch_down
    seq.children = order

    for node, kv in params:
        bt_edits.set_params(tree, node.guid, kv, cat)
        # Position::mode_ は enum で set_params が触れないので直に 1 (AbsolutePosition) にする
        for key in [k.split('.')[0] for k in kv if k.endswith('.offset_')]:
            pos = node.params[key]
            (pos.body if isinstance(pos, Ver) else pos)["mode_"] = Num.of_int(1)

    auto_layout(tree)
    problems = [p for p in bt_validate(tree, cat=cat) if not p.startswith('note:')]
    new = [p for p in problems if any(n.guid in p for n, _ in params)]
    if new:
        raise SystemExit('BT validation failed:\n  ' + '\n  '.join(new))
    TREE.write_bytes(to_file_bytes(bt_writer.write_tree(tree)))
    print(f'wrote {TREE.name} (State0: {len(seq.children)} children)')


def main():
    guids = build_scene()
    write_route('ToDestroyAirShip', TO_AIRSHIP)
    write_route('TouchDownIsLand', TOUCH_DOWN)
    build_tree(guids)


if __name__ == '__main__':
    main()
