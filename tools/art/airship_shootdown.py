"""序章で古竜が飛行船を撃ち落とす演出を組む (docs/Story.md 序章 3.)。

    python tools/art/airship_shootdown.py       # 何度流してもよい (前に足した物は外して足し直す)

流れ (FirstEventDragon の BT の State0。竜が島を回り込んで船の正面に来てから):
  操作ロック → 船の甲板を空ける (ShootDownAirShip: 乗客の NPC を消し、甲板のプレイヤーを桟橋へ) → 襲撃カメラ (島の上から
  船越しに竜を LookAt) → 咆哮 → 火球 → マストに着弾 (爆発・船に付く炎と黒煙・揺れ・雷) → 墜落カメラ (南西から船を横に見る)
  → 船が船首から傾いて雲の下へ落ちる (FallIsland) → 竜が落ちる船の横を急降下して島の下へ潜る → 上昇カメラ (桟橋の階段から
  南の空。縁の向こうから竜がせり上がり、LookAt のカメラへ替えて見上げる) → 着地カメラ (草地の北から桟橋を見る。竜が桟橋の上を
  越えて手前に降りる) → State10 の咆哮で、竜の子の寄りカメラへゆっくり寄る

- シーン (FirstTouchDownMainIsLandScene): DestroyIslandMovie の下に演出カメラ (Heart Dive Camera の写し。向きを固定する
  カメラは LookAt を外す)、桟橋の上に AirShipEvacuatePoint を置き、AirShip (v2) の evacuatePoint_ に繋ぐ。
  カットで切り替えるため、シーンの CineMachineVirtualCamera を v2 (カメラごとの blendIn) に上げる。
- プレハブ (FirstEventDragon): 咆哮のカメラの写しを顔へ寄せた所に置く (blendIn で寄る)。
- ルート: ToDestroyAirShip は船の正面で止まるところまで、急降下は TouchDownIsLand の頭に付ける。
- BT: State0 / State10 の Sequence に "Ship ..." の名前でノードを足す (流し直すと "Ship ..." を外して足し直す)。
  ToTouchDownIsland は同じ GUID・名前の ActionTimeline にして、ルートを飛んでいる間にカメラを時刻で切り替える。
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
from tools.scene import catalog as catalog_mod, edits, mathutil, model as scene_model, reader, validate, writer  # noqa: E402
from tools.bt import catalog as bt_catalog, edits as bt_edits, model as bt_model, reader as bt_reader, writer as bt_writer  # noqa: E402
from tools.bt import compose as bt_compose, meta as bt_meta  # noqa: E402
from tools.bt.layout import auto_layout  # noqa: E402
from tools.bt.validate import validate as bt_validate  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, walk  # noqa: E402
from desert_scene import component, find, set_trs  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
PREFAB = REPO / 'Assets' / 'Prefab' / 'Npc' / 'Enemy' / 'FirstEventDragon.prefab'
TREE =REPO / 'Assets' / 'Data' / 'EnemyBehaviour' / 'FirstEventDragon.enemyBehaviourData'
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
# 桟橋の階段の上から南の空を見上げる。船が消えた桟橋の縁 (2本の街灯の間) の向こうから竜がせり上がる
RISE_CAMERA_POS = (-2.0, 37.5, 12.0)
RISE_CAMERA_ROT = (0.0057978, 0.9911869, 0.1239579, -0.0463602)
# 草地の北から桟橋を見る。竜は北を向いたまま桟橋の上を越えて来て、手前の草地に降りる (奥の桟橋にプレイヤー)
LANDING_CAMERA_POS = (14.0, 40.0, 130.0)
LANDING_CAMERA_ROT = (0.0156476, 0.9827174, 0.0986757, -0.1558356)
CAMERA_SOURCE = 'Heart Dive Camera'
ATTACK_CAMERA = 'AirShip Attack Camera'
FALL_CAMERA = 'AirShip Fall Camera'
RISE_CAMERA = 'Dragon Rise Camera'
RISE_TRACK_CAMERA = 'Dragon Rise Track Camera'     # 上昇カメラと同じ所から LookAt で竜を追う
LANDING_CAMERA = 'Dragon Landing Camera'
STAGING_CAMERAS = (ATTACK_CAMERA, FALL_CAMERA, RISE_CAMERA, RISE_TRACK_CAMERA, LANDING_CAMERA)

# LibCore::EaseType
EASE_IN_OUT_SINE, EASE_SMOOTH_STEP = 6, 10
# NOTE: Brain は blendIn が 0 だと追従の補間で寄っていく。1フレームで終わる長さにしてカットにする
CUT = (0.01, EASE_SMOOTH_STEP)
TRACK_BLEND = (0.6, EASE_SMOOTH_STEP)

# TouchDownIsLand のルート (11 秒) の中の時刻
RISE_CUT_AT = 4.4       # 竜が島の下へ潜った所。桟橋の縁から出てくるのは 6.0 秒ごろ
RISE_TRACK_AT = 6.3
LANDING_CUT_AT = 7.6    # 上がりきって頭上を越え始めた所

# 竜の子のカメラ (竜のローカル。倍率 0.014、正面は -Z)。咆哮の間に元のカメラの位置から顔へ寄る
ROAR_CAMERA = 'FirstTouchDownIsland ProductionCamera'
ROAR_CLOSE_CAMERA = 'FirstTouchDownIsland ProductionCamera Close'
ROAR_CLOSE_POS = (-1500.0, 1250.0, -1970.0)
ROAR_PUSH_IN = (4.0, EASE_IN_OUT_SINE)

EVACUATE_POINT = 'AirShipEvacuatePoint'
EMPTY_SOURCE = 'SampleAppearDragonPos'     # コンポーネントを持たない空の GameObject

# 船首から (z 軸まわり、正で -X が下がる) 少し横にも傾いて落ちる
# 傾きの中心は tools/art/movie_markers.py が置くマーカー (MovieMarkers/AirShipFallPivot)。先にそちらを流しておく
FALL = dict(axis=(0.25, 0.0, 1.0), tilt=12.0, tilt_secs=1.4,
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
STATE10 = '9B6195AE-0F4D-4684-A46B-C3F172764970'
# NOTE: dragon_omen_bt.py / ancient_dragon.py がこの GUID で探すので、ActionTimeline にしても GUID と名前は変えない
TOUCH_DOWN_ROUTE = '80D57EC7-B83A-465B-AFC9-1EC16ACF4AFF'
SRC_ROAR_CAMERA_ON = 'B458295D-A737-4766-BCBD-7EEB12662FED'     # Enable TouchDownIsland ProductionCamera
SRC_ROAR_CAMERA_OFF = '813DAB52-A515-4114-BB57-BC0234BD53B4'    # Disable TouchDownIsland ProductionCamera
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

def upgrade_virtual_cameras(roots):
    """ファイルの中の CineMachineVirtualCamera を全部 v2 にする (版はファイルごとに型で1つ。足す値は読み込み時の既定値)"""
    for root in roots:
        for node in walk(root):
            for comp in node.components:
                if not comp.fqn.endswith('::CineMachineVirtualCamera'):
                    continue
                comp.class_version = 2
                for key, value in (('overrideFov_', False), ('fov_', Num.of_float(60.0)), ('overrideBlendIn_', False),
                                   ('blendIn_secs_', Num.of_float(0.5)), ('blendInEase_', Num.of_int(EASE_SMOOTH_STEP))):
                    if key not in comp.data:
                        comp.data[key] = value


def set_blend_in(node, secs, ease):
    data = component(node, 'CineMachineVirtualCamera').data
    data['overrideBlendIn_'] = True
    data['blendIn_secs_'] = Num.of_float(secs)
    data['blendInEase_'] = Num.of_int(ease)


def build_scene():
    scene = reader.read_scene_file(SCENE)
    movie = find(scene, 'DestroyIslandMovie')
    movie.transform.children = [c for c in movie.transform.children if c.name not in STAGING_CAMERAS]
    scene.roots = [r for r in scene.roots if r.name != EVACUATE_POINT]
    upgrade_virtual_cameras(scene.roots)

    source = find(scene, 'DestroyIslandMovie', CAMERA_SOURCE)

    def staging_camera(name, pos, rot, look_at, blend_in=None):
        camera = copy.deepcopy(source)
        if not look_at:
            # NOTE: 向きは Transform で決めるので LookAt は外す (的が空だと ScenePurposeCamera が竜を追わせる)
            camera.components = [c for c in camera.components if not c.fqn.endswith('::VirtualCameraLookAtBehaviour')]
        edits._remint_guids(camera, {})
        camera.name = name
        set_trs(camera, pos=pos)
        camera.transform.local_rot = edits._quat_from_floats(rot)
        if blend_in:
            set_blend_in(camera, *blend_in)
        return camera

    cameras = [
        staging_camera(ATTACK_CAMERA, ATTACK_CAMERA_POS, ATTACK_CAMERA_ROT, look_at=True),
        staging_camera(FALL_CAMERA, FALL_CAMERA_POS, FALL_CAMERA_ROT, look_at=False, blend_in=CUT),
        staging_camera(RISE_CAMERA, RISE_CAMERA_POS, RISE_CAMERA_ROT, look_at=False, blend_in=CUT),
        staging_camera(RISE_TRACK_CAMERA, RISE_CAMERA_POS, RISE_CAMERA_ROT, look_at=True, blend_in=TRACK_BLEND),
        staging_camera(LANDING_CAMERA, LANDING_CAMERA_POS, LANDING_CAMERA_ROT, look_at=False, blend_in=CUT),
    ]
    index = movie.transform.children.index(source)
    movie.transform.children[index + 1:index + 1] = cameras

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
    guids = dict(attack=cvc(ATTACK_CAMERA), fall=cvc(FALL_CAMERA), rise=cvc(RISE_CAMERA), rise_track=cvc(RISE_TRACK_CAMERA),
                 landing=cvc(LANDING_CAMERA), ship=find(scene, 'AirShip').guid,
                 pivot=find(scene, 'MovieMarkers', 'AirShipFallPivot').guid)
    print(f'placed {", ".join(STAGING_CAMERAS)}, {EVACUATE_POINT} {EVACUATE_POS}')
    return guids


# ---------------------------------------------------------------------------
# プレハブ

def build_prefab():
    prefab = reader.read_prefab_file(PREFAB)
    root = prefab.root
    root.transform.children = [c for c in root.transform.children if c.name != ROAR_CLOSE_CAMERA]
    upgrade_virtual_cameras([root])

    source = next(c for c in root.transform.children if c.name == ROAR_CAMERA)
    close = copy.deepcopy(source)
    edits._remint_guids(close, {})
    close.name = ROAR_CLOSE_CAMERA
    set_trs(close, pos=ROAR_CLOSE_POS)
    set_blend_in(close, *ROAR_PUSH_IN)
    root.transform.children.insert(root.transform.children.index(source) + 1, close)

    for node in walk(root):
        for c in node.components:
            let_writer_place_versions(c.data)
    # NOTE: 他の子の worldMatrix_ は触らない (足したカメラだけ焼く)
    root_trs = edits._node_local_trs(root)
    if not any(root_trs.rot):
        root_trs = mathutil.Trs(root_trs.pos, (0.0, 0.0, 0.0, 1.0), root_trs.scale)
    bake_world_matrices(close, mathutil.IDENTITY.then(root_trs))
    text = writer.write_prefab(prefab)
    check(text, validate.validate_prefab(prefab), PREFAB.name)
    PREFAB.write_bytes(to_file_bytes(text))
    print(f'placed {ROAR_CLOSE_CAMERA} in {PREFAB.name}')


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
    roar_seq = tree.find(STATE10)
    roar_seq.children = [c for c in roar_seq.children if not label(c).startswith(PREFIX)]
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
    fall = with_params(clone(SRC_FALL, 'Fall AirShip'), target_=guids['ship'], pivotPos_=guids['pivot'],
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

    # ルートを飛んでいる間のカメラ切替。Cue の action はツリーに無いので set_params を通さずに書く
    def cue_camera(at_secs, cvc, priority):
        node = bt_edits._clone_node(tree.find(SRC_CAMERA_ON), bt_meta.mint_guid)
        bt_edits._set_field_guid(node.params['purposeCamera_'], cvc)
        node.params['priority_'] = Num.of_int(priority)
        return bt_compose.cue(at_secs, node)

    def touch_down_timeline(node):
        route = node
        if not node.type_fqn.endswith('::MoveEventRoute'):
            # 前に流した時の ActionTimeline。Cue の中のルートを取り出して組み直す
            ptr = next(c['action_'] for c in node.params['cues_'] if c['action_'].fqn.endswith('::MoveEventRoute'))
            route = bt_model.Action(guid=bt_meta.mint_guid(), pos=node.pos, name=node.name, type_fqn=ptr.fqn,
                                    action_version=int(ptr.data.version), params=ptr.data.body)
        return bt_compose.timeline(cat, [
            bt_compose.cue(0.0, route, wait_done=True),
            cue_camera(RISE_CUT_AT, guids['rise'], 107),
            cue_camera(RISE_CUT_AT, guids['fall'], -1),
            cue_camera(RISE_TRACK_AT, guids['rise_track'], 108),
            cue_camera(LANDING_CUT_AT, guids['landing'], 109),
            cue_camera(LANDING_CUT_AT, guids['rise'], -1),
            cue_camera(LANDING_CUT_AT, guids['rise_track'], -1),
        ], once=True, name=node.name, pos=node.pos, guid=node.guid)

    order = []
    for c in seq.children:
        name = label(c)
        if name == 'ShootDownAirShip':
            order += added
            continue
        if c.guid == TOUCH_DOWN_ROUTE:
            c = touch_down_timeline(c)
        order.append(c)
        if name == 'AirShip Fall Lightning':
            order += after_lightning
    seq.children = order

    # State10 (着地の後の咆哮): 着地カメラから竜の子の寄りカメラへ、咆哮の間にゆっくり寄る
    def roar_camera(src, name, priority):
        return with_params(clone(src, name), prefabPurposeCamera_=ROAR_CLOSE_CAMERA, priority_=str(priority))

    order = []
    for c in roar_seq.children:
        order.append(c)
        if label(c) == 'Enable TouchDownIsland ProductionCamera':
            order += [once(roar_camera(SRC_ROAR_CAMERA_ON, 'Enable Roar Close Camera', 101)),
                      once(camera(SRC_CAMERA_OFF, 'Disable Landing Camera', guids['landing'], -1))]
        elif c.guid == SRC_ROAR_CAMERA_OFF:
            order.append(roar_camera(SRC_ROAR_CAMERA_OFF, 'Disable Roar Close Camera', -1))
    roar_seq.children = order

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
    build_prefab()
    write_route('ToDestroyAirShip', TO_AIRSHIP)
    write_route('TouchDownIsLand', TOUCH_DOWN)
    build_tree(guids)


if __name__ == '__main__':
    main()
