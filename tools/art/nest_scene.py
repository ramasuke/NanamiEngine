"""古竜の巣 (終章「嵐の巣」) のシーン (Assets/Scene/DragonNestScene.scene) を組む。

    python tools/art/nest_scene.py            # 作り直す (シーンの .meta の GUID は保つ。GameObject の GUID は毎回新しい)
    python tools/art/nest_scene.py --dry-run  # 置く数だけ表示

docs/Story.md 終章: 巣は奪った心臓と仲間の骨を積み上げた **竜の墓場**。奪われた心臓が仲間の骨に戻され、起き上がりかけた骸がいくつもある。
- 嵐の目に浮かぶ黒い岩の島 (地形は nest_terrain.py / nest_models_blender.py)。周りを嵐の壁 (NestStormWall) がゆっくり回る。
  見上げると嵐の目の空。
- 外輪には岩の爪 (NestSpire*) が内へ反って並び、島そのものが巨大な肋骨か爪のように巣を抱える。南の切れ目が着き場からの坂道で、
  坂の上には爪が門のように左右から覆いかぶさる。
- 巣の底の中央、心臓の山に色とりどりの心臓が積まれている (拠点の島の緑・光と同じ結晶に、よその島の潮 (青)・宵 (紫)・火)。
  「島の心臓は引き合い、ものを引き寄せ、持ち上げる」ので、山の周りでは岩や、心臓ごと持ち去られた島の瓦礫が宙に浮いている。
- 山を囲んで、心臓を胸に戻されて起き上がりかけた竜の骸 (NestSkeleton) が輪になる。まだ心臓の無い骨 (DragonBones) は伏せたまま。
  骨と地面には、古い竜撃ちの銛 (NestHarpoon) が刺さったまま残っている = ハンターがもとは竜狩りだったことの物証。
- 敵の湧き地点は空 (古竜の戦いは【未設計】)。土台は草原のシーンの複製 (desert_scene.py と同じ仕組みの物だけ残す)。
- 嵐は序章のシーンの WeatherService 一式 (空のドームの下、閃光、突風、雷の音) を写し、強さ INITIAL_STORM で始める。
  嵐の壁の回転も WeatherService が嵐の強さで速める。
"""
import argparse
import copy
import math
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, model as scene_model, reader, validate, writer  # noqa: E402

from game_over_prefab import Builder, asset_guid, check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, rotation, walk  # noqa: E402
from desert_scene import component, field_guid, find, remint_all, set_trs  # noqa: E402
import nest_terrain as nt  # noqa: E402

SOURCE = REPO / 'Assets' / 'Scene' / 'GrassLandScene.scene'
WEATHER_SOURCE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
SCENE = REPO / 'Assets' / 'Scene' / 'DragonNestScene.scene'
PREFAB = REPO / 'Assets' / 'Prefab'
TERRAIN_MV1 = REPO / 'Assets' / 'Art' / 'Models' / 'Nest' / 'NestTerrain.mv1.meta'

KEEP_ROOTS = ['CameraBrain', 'Terrain', 'SkyDome', 'NetworkRunner', 'Canvas', 'PlayerSpawn Pos', 'EnemySpawnPoints',
              'WindZone', 'ArrivalCamera']
TERRAIN_ROOT_Y, TERRAIN_ROOT_SCALE = 29.92, 0.5
M = 8.0
CAMERA_FAR = 5200.0
SKY_DOME_SCALE = 4.6        # 嵐の壁 (半径 2640、上の口は 3500 近く) より外へ
STRIP_SUFFIXES = ('Collider', '::RigidBody', 'DestructibleObject')
# 序章から写す WeatherService の部品 (SkyDome の WeatherService が参照する)
WEATHER_ROOTS = ['StormSkyDomeLower', 'LightningFlash', 'StormSky']
LOWER_SKY_DOME_SCALE = 4.4
INITIAL_STORM = 0.6
WALL_ROTATE_SPEED = (1.5, 6.0)   # 晴れ / 嵐 1.0 の deg/s

# 心臓の色の重み (よその島から奪った物が多い)
HEART_COLORS = [('NestHeartTide', 3), ('NestHeartDusk', 3), ('NestHeartFire', 2), ('NestHeartGreen', 2), ('NestHeartLight', 2)]
# 骸の輪: (心臓の山からの方位 deg: +X から +Z へ, 距離, 大きさ, 沈める world, 傾き (x, z) rad, 胸の心臓)
# 南 (90°) は坂道から山への通り道なので空ける
SKELETONS = [
    (-90.0, 235.0, 1.05, 0.0, (0.0, 0.0), 'NestHeartTide'),      # 北: いちばん起き上がっている
    (-150.0, 245.0, 0.9, 8.0, (0.08, -0.05), 'NestHeartDusk'),
    (-30.0, 245.0, 0.95, 6.0, (-0.06, 0.07), 'NestHeartFire'),
    (170.0, 230.0, 0.8, 20.0, (0.18, 0.1), 'NestHeartTide'),      # 西: まだ半分埋もれている
    (15.0, 225.0, 0.85, 18.0, (-0.12, -0.16), 'NestHeartDusk'),   # 東
]
# 心臓の無い骨 (伏せたまま): (方位, 距離, 大きさ, 沈める)
BARE_BONES = [(130.0, 250.0, 0.7, 8.0), (45.0, 270.0, 0.8, 10.0), (-120.0, 300.0, 0.6, 12.0)]
# 胸の心臓の位置 (骸のローカル、scale 1 の world 単位)。NestSkeleton の胸 (Blender で (4.4, -3.4, 7) m) を world に直した値
CHEST_OFFSET = (35.0, 56.0, 27.0)
HARPOON_SCALE = (0.45, 0.65)   # 銛の長さ 15.6 m x これ


class Terrain:
    """data/nest_terrain.npz の world 高さ"""

    def __init__(self):
        self.h = np.load(nt.NPZ)['height'].astype(np.float64)
        self.n = self.h.shape[0]
        self.step = nt.SIZE / (self.n - 1)

    def height(self, x, z):
        fx = min(max(x / self.step, 0.0), self.n - 1.0001)
        fz = min(max(z / self.step, 0.0), self.n - 1.0001)
        i0, j0 = int(fx), int(fz)
        tx, tz = fx - i0, fz - j0
        g = self.h
        return float((g[j0, i0] * (1 - tx) + g[j0, i0 + 1] * tx) * (1 - tz)
                     + (g[j0 + 1, i0] * (1 - tx) + g[j0 + 1, i0 + 1] * tx) * tz)

    def lowest(self, x, z, radius):
        ys = [self.height(x, z)] + [self.height(x + math.cos(k * math.pi / 4) * radius,
                                                z + math.sin(k * math.pi / 4) * radius) for k in range(8)]
        return min(ys)


def yaw_facing(dx, dz):
    """ローカル +Z を (dx, 0, dz) へ向ける Y 回転"""
    return math.atan2(dx, dz)


def yaw_bend(dx, dz):
    """岩の爪の反り (Blender の +Y = ローカル -Z) を (dx, 0, dz) へ向ける Y 回転"""
    return math.atan2(-dx, -dz)


def polar(center, deg, dist):
    a = math.radians(deg)
    return center[0] + math.cos(a) * dist, center[1] + math.sin(a) * dist


class Put:
    def __init__(self, group, prefab, pos, rot, scale, strip=False, drift=None):
        self.group, self.prefab, self.pos, self.rot, self.scale = group, prefab, pos, rot, scale
        self.strip, self.drift = strip, drift


# ---------------------------------------------------------------- 配置
def plan(rng, t):
    out = []
    cx, cz = nt.CENTER
    mx, mz = nt.MOUND

    # --- 外輪の岩の爪。内へ反らせる。南の坂道の切れ目は空ける
    spires = ['NestSpireA', 'NestSpireC', 'NestSpireB']
    k = 0
    for deg in np.arange(0, 360, 17.0):
        th = deg + rng.uniform(-5, 5)
        if abs(math.remainder(math.radians(th - 90), 2 * math.pi)) < math.radians(15):
            continue
        r = rng.uniform(430, 490)
        x, z = polar(nt.CENTER, th, r)
        s = rng.uniform(0.75, 1.25)
        yaw = yaw_bend(cx - x, cz - z) + rng.uniform(-0.25, 0.25)
        tilt = (rng.uniform(-0.12, 0.05), rng.uniform(-0.1, 0.1))
        out.append(Put('Spires', f'Prop/Nest/{spires[k % 3]}', (x, t.lowest(x, z, 20) - 2, z), rotation(yaw, *tilt), s))
        k += 1
        # 外輪の内側に小さい爪を足して、牙が並んだように
        if rng.random() < 0.55:
            x2, z2 = polar(nt.CENTER, th + rng.uniform(4, 9), r - rng.uniform(45, 70))
            out.append(Put('Spires', f'Prop/Nest/{spires[(k + 1) % 3]}', (x2, t.lowest(x2, z2, 12) - 2, z2),
                           rotation(yaw_bend(cx - x2, cz - z2), rng.uniform(-0.1, 0.1), 0.0), rng.uniform(0.45, 0.8)))

    # --- 坂の上の門: 左右の大きな爪が覆いかぶさる
    lx, lz = nt.LEDGE
    for side in (-1, 1):
        x, z = lx + side * 92, lz - 110
        out.append(Put('Spires', 'Prop/Nest/NestSpireA', (x, t.lowest(x, z, 20) - 4, z),
                       rotation(yaw_bend(-side, -0.25), 0.0, 0.0), 1.15))
        x, z = lx + side * 72, lz - 185
        out.append(Put('Spires', 'Prop/Nest/NestSpireC', (x, t.lowest(x, z, 15) - 3, z),
                       rotation(yaw_bend(-side, -0.6), 0.0, 0.0), 0.9))

    # --- 着き場から見上げる大きな頭骨 (坂の下、巣の入口)
    x, z = cx + 150, cz + 250
    out.append(Put('Bones', 'Prop/Nest/NestSkull', (x, t.lowest(x, z, 30) - 18, z),
                   rotation(yaw_facing(-0.5, 1.0), -0.25, 0.12), 2.4))
    for deg, dist, s in ((200, 320, 1.1), (-40, 340, 0.9), (60, 310, 1.3)):
        x, z = polar(nt.CENTER, deg, dist)
        out.append(Put('Bones', 'Prop/Nest/NestSkull', (x, t.lowest(x, z, 15) - 10 * s, z),
                       rotation(rng.uniform(0, 6.28), rng.uniform(-0.4, 0.2), rng.uniform(-0.3, 0.3)), s))

    # --- 心臓の山
    top = t.height(mx, mz)
    names, weights = zip(*HEART_COLORS)
    p = np.array(weights, float) / sum(weights)
    for i in range(5):   # 真ん中の大きな5つ
        a = i * 1.26 + 0.3
        x, z = mx + math.cos(a) * 9, mz + math.sin(a) * 9
        out.append(Put('Hearts', f'Prop/Nest/{names[i]}', (x, top - 6, z),
                       rotation(rng.uniform(0, 6.28), rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25)),
                       rng.uniform(3.6, 4.4)))
    for i in range(75):
        rr = 10 + 34 * math.sqrt(rng.random())
        a = rng.uniform(0, 2 * math.pi)
        x, z = mx + math.cos(a) * rr, mz + math.sin(a) * rr
        s = rng.uniform(1.0, 2.6) * (1.25 - rr / 55)
        out.append(Put('Hearts', f'Prop/Nest/{rng.choice(names, p=p)}', (x, t.height(x, z) - 3 * s, z),
                       rotation(rng.uniform(0, 6.28), rng.uniform(-0.6, 0.6), rng.uniform(-0.6, 0.6)), s))
    for name in ('Particle/LightCoreAura', 'Particle/GreenCoreAura'):
        out.append(Put('Hearts', name, (mx, top + 8, mz), rotation(0.0), 3.0))

    # --- 心臓に引かれて浮いている岩と瓦礫 (山の上をゆっくり回る)
    debris = ['Prop/Rock/StylizedRock', 'Prop/Rock/FantasyRock', 'Prop/Rock/Rock17', 'Prop/Nest/NestSpireB',
              'Prop/Settlement/FallenRuinPillar', 'Prop/Desert/FortressWallEnd', 'Prop/Settlement/RubblePile']
    for i in range(22):
        a = rng.uniform(0, 2 * math.pi)
        rr = rng.uniform(45, 170)
        x, z = mx + math.cos(a) * rr, mz + math.sin(a) * rr
        y = top + rng.uniform(35, 190) * (1.1 - rr / 300)
        name = debris[i % len(debris)]
        s = {'Prop/Nest/NestSpireB': 0.35, 'Prop/Desert/FortressWallEnd': 0.5}.get(name, 1.0) * rng.uniform(0.6, 1.4)
        out.append(Put('Floating', name, (x, y, z),
                       rotation(rng.uniform(0, 6.28), rng.uniform(-1.2, 1.2), rng.uniform(-1.2, 1.2)), s,
                       strip=True, drift=(rng.uniform(4, 9), rng.uniform(7, 13), rng.uniform(4, 9), rng.uniform(9, 16))))
    for i in range(7):   # 宙に浮いたままの心臓
        a = rng.uniform(0, 2 * math.pi)
        rr = rng.uniform(30, 110)
        x, z = mx + math.cos(a) * rr, mz + math.sin(a) * rr
        out.append(Put('Floating', f'Prop/Nest/{rng.choice(names, p=p)}', (x, top + rng.uniform(30, 110), z),
                       rotation(rng.uniform(0, 6.28), rng.uniform(-0.5, 0.5), 0.0), rng.uniform(0.9, 1.6),
                       drift=(rng.uniform(3, 6), rng.uniform(5, 9), rng.uniform(6, 12), rng.uniform(7, 12))))

    # --- 骸の輪。正面 (+Z) を心臓の山へ向け、胸に心臓
    for deg, dist, s, sink, tilt, heart in SKELETONS:
        x, z = polar(nt.MOUND, deg, dist)
        yaw = yaw_facing(mx - x, mz - z)
        y = t.lowest(x, z, 40) - sink
        rot = rotation(yaw, *tilt)
        out.append(Put('Bones', 'Prop/Nest/NestSkeleton', (x, y, z), rot, s))
        # 胸の心臓 (骸と一緒に回す)
        ox, oy, oz = (v * s for v in CHEST_OFFSET)
        cy, sy = math.cos(yaw), math.sin(yaw)
        hx, hz = x + ox * cy + oz * sy, z - ox * sy + oz * cy
        out.append(Put('Bones', f'Prop/Nest/{heart}', (hx, y + oy, hz), rotation(yaw, 0.3, 0.0), 1.2 * s / 0.7))
        # 骸の傍に刺さったままの竜撃ちの銛 (骸の方へ傾けて、骸を射た向き)
        for j in range(3):
            a = rng.uniform(0, 2 * math.pi)
            rr = rng.uniform(25, 60) * s / 0.7
            hx2, hz2 = x + math.cos(a) * rr, z + math.sin(a) * rr
            hs = rng.uniform(*HARPOON_SCALE)
            out.append(Put('Harpoons', 'Prop/Nest/NestHarpoon', (hx2, t.height(hx2, hz2) - 14 * hs, hz2),
                           rotation(yaw_facing(hx2 - x, hz2 - z), rng.uniform(0.35, 0.75), rng.uniform(-0.2, 0.2)), hs))
    for deg, dist, s, sink in BARE_BONES:
        x, z = polar(nt.MOUND, deg, dist)
        out.append(Put('Bones', 'Prop/Desert/DragonBones', (x, t.lowest(x, z, 40) - sink, z),
                       rotation(rng.uniform(0, 6.28), rng.uniform(-0.1, 0.1), rng.uniform(-0.1, 0.1)), s))
        for j in range(2):
            a = rng.uniform(0, 2 * math.pi)
            hx2, hz2 = x + math.cos(a) * rng.uniform(20, 60), z + math.sin(a) * rng.uniform(20, 60)
            hs = rng.uniform(*HARPOON_SCALE)
            out.append(Put('Harpoons', 'Prop/Nest/NestHarpoon', (hx2, t.height(hx2, hz2) - 14 * hs, hz2),
                           rotation(rng.uniform(0, 6.28), rng.uniform(0.3, 0.9), 0.0), hs))
    # 坂道の脇にも折れた銛 (着いてすぐ目に入る)
    for side, dz in ((-1, 0), (1, 40), (-1, 90)):
        x, z = lx + side * rng.uniform(50, 62), lz - 160 - dz
        out.append(Put('Harpoons', 'Prop/Nest/NestHarpoon', (x, t.height(x, z) - 8, z),
                       rotation(rng.uniform(0, 6.28), rng.uniform(0.5, 1.0), 0.0), 0.6))
    return out


# ---------------------------------------------------------------- シーン
def drift_component(b, node, params):
    bob, period, tilt, tilt_period = params
    b.component(node, 'FloatingDrift', bobHeight_=f'{bob:.2f}', bobPeriod_secs_=f'{period:.1f}',
                tiltDegrees_=f'{tilt:.2f}', tiltPeriod_secs_=f'{tilt_period:.1f}')


def set_camera_and_sky(scene):
    for root in scene.roots:
        for node in walk(root):
            for comp in node.components:
                if comp.fqn.endswith('::CinemachineCameraBrain') and 'cameraFar_' in comp.data:
                    comp.data['cameraFar_'] = Num.of_float(CAMERA_FAR)
                if comp.fqn.endswith('::SkyDome3D'):
                    node.transform.local_scale = edits._vec3_from_floats((SKY_DOME_SCALE,) * 3)


def field_guid_of(blob):
    """Field<T> の参照先 GUID (edits._set_field_guid の逆)"""
    unwrap = lambda b: b.body if isinstance(b, Ver) else b
    holder = unwrap(blob)['value0'].data
    return unwrap(unwrap(holder)['value0'])['value_']


def color32_from_vec3(blob):
    """WeatherService v1 の glm::vec3 色 -> Color32 (Color32::FromVec3 と同じ丸め)"""
    body = blob.body if isinstance(blob, Ver) else blob
    r, g, b = (int(round(float(body[f'value{i}'].value) * 255.0)) for i in range(3))
    return edits.color32_blob(r, g, b)


def add_weather(scene):
    """序章の WeatherService 一式を写す。remint_all の前に呼ぶ (参照ごと GUID を振り直す)"""
    src = reader.read_scene_file(WEATHER_SOURCE)
    sky, src_sky = find(scene, 'SkyDome'), find(src, 'SkyDome')
    src_lower = find(src, 'StormSkyDomeLower')
    # 序章の空のドーム (上) とカメラへの参照を、巣のものへ付け替える
    remap = {
        scene_model.find_component_guid(component(src_sky, 'SkyDome3D')): scene_model.find_component_guid(component(sky, 'SkyDome3D')),
        scene_model.find_component_guid(component(src_sky, 'Rotator')): scene_model.find_component_guid(component(sky, 'Rotator')),
        field_guid_of(component(src_lower, 'SkyDome3D').data['mainCamera_']):
            field_guid_of(component(sky, 'SkyDome3D').data['mainCamera_']),
    }
    roots = [copy.deepcopy(find(src, name)) for name in WEATHER_ROOTS]
    for root in roots:
        edits._remap_guid_references(root, remap)
    weather = copy.deepcopy(component(src_sky, 'WeatherService'))
    edits._remap_guid_blob(weather.data, remap)

    assert weather.class_version == 1, weather.class_version
    weather.class_version = 3
    data = weather.data
    # NOTE: 巣のシーンでは SkyDome が先に書き出されるので、ここが ParticleSystem / SoundFile の Field の初出になる。
    #       序章では2回目以降の形 (版キー無し) だったので、版キー付きの形に包み直す
    outer = data['skyDomeUpper_']
    for key in ('gustParticle_', 'thunderNearSound_'):
        holder_ptr = data[key]['value0']
        holder_ptr.data = Ver(outer.body['value0'].data.key, 0, holder_ptr.data, literal_presence=True)
        data[key] = Ver(outer.key, 0, data[key], literal_presence=True)

    # v1 (色が glm::vec3) -> v3
    for key in ('clearSkyTint_', 'stormSkyTint_', 'stormFogColor_', 'stormLightColor_'):
        data[key] = color32_from_vec3(data[key])
    data['wallRotator_'] = copy.deepcopy(data['lowerRotator_'])
    data['initialStormIntensity_'] = Num.of_float(INITIAL_STORM)
    data['clearWallRotateSpeedDeg_'] = Num.of_float(WALL_ROTATE_SPEED[0])
    data['stormWallRotateSpeedDeg_'] = Num.of_float(WALL_ROTATE_SPEED[1])
    sky.components.append(weather)
    scene.roots.extend(roots)
    return weather


def build(placements, t, dry_run):
    scene = reader.read_scene_file(SOURCE)
    scene.roots = [r for r in scene.roots if r.name in KEEP_ROOTS]
    scene.name = 'DragonNest'
    weather = add_weather(scene)
    remint_all(scene)
    set_camera_and_sky(scene)
    set_trs(find(scene, 'StormSkyDomeLower'), scale=LOWER_SKY_DOME_SCALE)
    b = Builder(scene)

    # 地形
    model = find(scene, 'Terrain', 'Model')
    model.components = [c for c in model.components if not c.fqn.endswith('::Grassable')]
    field_guid(component(model, 'ModelRenderer'), 'mv1File_', asset_guid(TERRAIN_MV1))
    collider = component(model, 'StaticMeshCollider')
    body = collider.data['value0']
    body = body.body if hasattr(body, 'body') else body
    for i, v in enumerate((90.0, 0.0, 0.0)):
        body['offsetRotation_'][f'value{i}'] = Num.of_float(v)
    collider.data['maxSimplifyError_'] = Num.of_float(2.0)
    set_trs(model, pos=(0.0, -TERRAIN_ROOT_Y / TERRAIN_ROOT_SCALE, 0.0), scale=M * 0.01 / TERRAIN_ROOT_SCALE)

    # スポーン地点と到着カメラ (着き場。北の巣を向いて歩き出す)
    sx, sz = nt.LEDGE
    sz += 25.0
    sy = t.height(sx, sz) + 5.0
    set_trs(find(scene, 'PlayerSpawn Pos'), pos=(sx, sy, sz))
    set_trs(find(scene, 'ArrivalCamera'), pos=(sx + 9.0, sy + 2.0, sz - 24.0))

    # 敵の湧き地点は空 (古竜との戦いは未設計)
    find(scene, 'EnemySpawnPoints').transform.children = []

    # 嵐の壁 (ゆっくり回る)
    storm = edits.add_gameobject(scene, parent=None, name='StormWall')
    set_trs(storm, pos=(nt.CENTER[0], nt.BASE, nt.CENTER[1]), yaw=0.0)
    b.component(storm, 'Rotator', rotateSpeedDegPerSec_=str(WALL_ROTATE_SPEED[0]), rotateAxis_='0,1,0')
    field_guid(weather, 'wallRotator_', scene_model.find_component_guid(component(storm, 'Rotator')))
    wall = edits.instantiate_prefab(scene, reader.read_prefab_file(PREFAB / 'Prop' / 'Nest' / 'NestStormWall.prefab'),
                                    parent=storm.guid)
    set_trs(wall, pos=(0.0, 0.0, 0.0), yaw=0.0)

    groups = {}
    nest = edits.add_gameobject(scene, parent=None, name='Nest')
    for g in ('Spires', 'Hearts', 'Floating', 'Bones', 'Harpoons'):
        groups[g] = edits.add_gameobject(scene, parent=nest.guid, name=g)
    prefabs = {}
    for p in placements:
        path = PREFAB / f'{p.prefab}.prefab'
        if path not in prefabs:
            prefabs[path] = reader.read_prefab_file(path)
        node = edits.instantiate_prefab(scene, prefabs[path], parent=groups[p.group].guid)
        node.transform.local_pos = edits._vec3_from_floats(tuple(float(v) for v in p.pos))
        node.transform.local_rot = edits._quat_from_floats(tuple(float(v) for v in p.rot))
        node.transform.local_scale = edits._vec3_from_floats((float(p.scale),) * 3)
        if p.strip:
            for n in walk(node):
                n.components = [c for c in n.components if not c.fqn.endswith(STRIP_SUFFIXES)]
        if p.drift:
            drift_component(b, node, p.drift)

    # NOTE: 序章から写した部品は2回目以降の出現の形 (版キー無し) なので、同じ型が先に出る巣の物より後ろへ置く
    scene.roots.sort(key=lambda r: r.name in WEATHER_ROOTS)
    for r in scene.roots:
        for node in walk(r):
            for comp in node.components:
                let_writer_place_versions(comp.data)
        bake_world_matrices(r)

    text = writer.write_scene(scene)
    tree = loads(text)
    stripper = StrayVersionStripper(catalog_mod.load(), '/')
    stripper.run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    if dry_run:
        print('dry run: nothing written')
        return
    SCENE.write_bytes(to_file_bytes(text))
    meta = Path(str(SCENE) + '.meta')
    if not meta.exists():
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.SCENE_SPEC, 'DragonNestScene', SCENE.parent, REPO)
        scene_meta.write_meta(scene_meta.SCENE_SPEC, meta, 'DragonNestScene', guid, content_path)
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  (asset guid {asset_guid(meta)}, {len(placements)} props)')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seed', type=int, default=20260927)
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()
    rng = np.random.default_rng(args.seed)
    t = Terrain()
    placements = plan(rng, t)
    counts = {}
    for p in placements:
        counts[p.prefab] = counts.get(p.prefab, 0) + 1
    print(f'{len(placements)} props: ' + ', '.join(f'{k.split("/")[-1]} {v}' for k, v in sorted(counts.items())))
    build(placements, t, args.dry_run)


if __name__ == '__main__':
    main()
