"""狩り場 (GrassLandScene / DesertScene) と拠点の島 (MainIslandScene) の遠景に、ほかの浮島を置く。

    python tools/art/sky_islands.py [grass] [desert] [main] [prologue]      # 省略時はすべて

docs/Story.md §2「見た目」: 狩り場も空に浮かぶ島で、遠景にほかの浮島が見える。
- 草原の空は、拠点の島の周りと同じ丸い Island_Broken / BrickGrassIsland_Broken。
  砂の島の空は草原と違って見えるように、sky_island_shapes_blender.py で作った細長い尾根 / 広い岩盤 / 双子 / 段のある台地 /
  垂れる岩の塔を赤い砂岩と砂にした形 (isLand/Sky/*_Sand、丸い島は SandIsland_Broken) に、台地・城塞のかけら・神殿・ヤシを載せる。遊べる範囲 (250..1250) を囲む山の外、
  中心 (750, 750) から 1500..2300 に置き、山越しに底の岩まで見える高さに浮かべる。
- 島の天辺の中心に親 (FloatingDrift でゆっくり上下・傾く) を置き、その上に木や廃墟を並べる。
  脇には小さな岩のかけらを浮かべる (それぞれ FloatingDrift)。当たり判定はすべて外す。
- 拠点の島の空は、草の島を sky_island_shapes_blender.py の形 (isLand/Sky/*_Grass) と丸い島で混ぜ、
  島々 (中心 (50, 300)) を囲むように、足元より下・同じ高さ・見上げる高さにばらして浮かべる。林・野営地・見張り台・廃墟。
- 序章 (FirstTouchDownMainIsLandScene) は同じ島の襲撃前なので、拠点の島の空と同じ配置・同じ乱数にする。
  空のドームが2枚 (SkyDome と嵐の StormSkyDomeLower) あるので、大きさの比を保ったまま SkyDome を 3.5 にそろえる。
- 島は遠いので、カメラの far を 2300 -> 4000 に、空のドーム (thinSkyDome, 半径 1000 x scale) を
  3.5 倍にする。ドームが近いと、その外の島を隠してしまう。
- 何度実行しても SkyIslands ルートは1組だけになる。
"""
import math
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402

from game_over_prefab import Builder, asset_guid, check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import (StrayVersionStripper, bake_world_matrices, first_versions, quat_axis,  # noqa: E402
                                      rotation, walk)

SCENES = {
    'grass': REPO / 'Assets' / 'Scene' / 'GrassLandScene.scene',
    'desert': REPO / 'Assets' / 'Scene' / 'DesertScene.scene',
    'main': REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene',
    'prologue': REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene',
}
SEEDS = {'grass': 4242, 'desert': 5151, 'main': 6262, 'prologue': 6262}
ROOT_NAME = 'SkyIslands'
MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'isLand'
PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop'
CENTERS = {'grass': (750.0, 750.0), 'desert': (750.0, 750.0), 'main': (50.0, 300.0), 'prologue': (50.0, 300.0)}

CAMERA_FAR = 4000.0
SKY_DOME_SCALE = 3.5

# 島のモデル: (.mv1 (isLand/ 以下), model 単位の天辺の中心, 半幅 (x, z), 深さ[, 天辺の段 (高さ, 内側の割合)])
# Island*_Broken は拠点の島の周りと同じ丸い島 (原点が外れたところにある)。SandIsland_Broken はそのテクスチャを砂と赤い砂岩にしたもの。
# Sky/SkyIsland_*_Sand は sky_island_shapes_blender.py で作った形の砂の島 (原点が天辺の中心)
ROUND_TOP = (4215.0, -14371.0, -3061.0)
MODEL_INFO = {
    'round': ('Island_Broken', ROUND_TOP, (2390.0, 2325.0), 6106.0),
    'brick': ('BrickGrassIsland_Broken', ROUND_TOP, (2390.0, 2325.0), 6106.0),
    'round_sand': ('SandIsland_Broken', ROUND_TOP, (2390.0, 2325.0), 6106.0),
}
for _shape, _half, _depth, *_terrace in (('Long', (918.0, 284.0), 826.0), ('Slab', (740.0, 542.0), 449.0),
                                         ('Twin', (715.0, 402.0), 798.0), ('Mesa', (603.0, 458.0), 730.0, (249.0, 0.52)),
                                         ('Shard', (335.0, 250.0), 1237.0)):
    for _kind in ('Sand', 'Grass'):
        MODEL_INFO[f'{_shape.lower()}_{_kind.lower()}'] = (f'Sky/SkyIsland_{_shape}_{_kind}', (0.0, 0.0, 0.0), _half, _depth,
                                                           *_terrace)

# (方位 deg: +X から +Z へ, 中心からの距離, 天辺の高さ, 長い方の幅 world, 縦の伸び, 向き deg, 島のモデル, 飾りの組)
# 縦の伸びは島の厚みだけに掛ける (1 未満で平たく、1 より大きいと深く垂れる)。飾りの組 None は何も載せない
LAYOUTS = {
    # 草原の空: 拠点の島の周りと同じ丸い島 (灰色の岩に草) に、林・廃墟・見張り台
    'grass': [
        (8.0, 1750.0, 700.0, 574.0, 1.0, 20.0, 'round', 'ruins'),
        (42.0, 2050.0, 860.0, 335.0, 1.0, 110.0, 'round', 'forest'),
        (-28.0, 1680.0, 560.0, 430.0, 1.0, 200.0, 'brick', 'lookout'),
        (-78.0, 1950.0, 760.0, 621.0, 1.0, 60.0, 'round', 'forest'),
        (-128.0, 1720.0, 540.0, 359.0, 1.0, 300.0, 'round', 'rocks'),
        (-104.0, 2300.0, 1050.0, 239.0, 1.0, 150.0, 'brick', 'forest'),
        (172.0, 1880.0, 690.0, 478.0, 1.0, 240.0, 'round', 'forest'),
        (136.0, 2150.0, 940.0, 287.0, 1.0, 20.0, 'brick', 'ruins'),
        (78.0, 1580.0, 470.0, 215.0, 1.0, 80.0, 'round', 'rocks'),
    ],
    # 砂の島の空: 赤い砂岩に砂の島。台地・よその城塞のかけら・神殿・ヤシの泉
    'desert': [
        (12.0, 1800.0, 740.0, 560.0, 1.0, 200.0, 'mesa_sand', 'mesa'),
        (30.0, 1640.0, 560.0, 180.0, 1.1, 40.0, 'shard_sand', None),
        (-35.0, 1720.0, 640.0, 820.0, 1.0, 40.0, 'long_sand', 'fortress'),
        (-82.0, 2000.0, 840.0, 680.0, 0.9, 130.0, 'slab_sand', 'temple'),
        (-100.0, 1760.0, 1000.0, 190.0, 1.0, 10.0, 'shard_sand', 'pillars'),
        (-135.0, 1750.0, 600.0, 600.0, 1.0, 280.0, 'twin_sand', 'oasis'),
        (165.0, 1900.0, 720.0, 480.0, 1.2, 330.0, 'mesa_sand', 'pillars'),
        (125.0, 2250.0, 1000.0, 360.0, 1.0, 60.0, 'round_sand', 'arch'),
        (60.0, 2100.0, 900.0, 760.0, 0.8, 250.0, 'long_sand', 'oasis'),
        (-112.0, 2380.0, 1150.0, 420.0, 1.0, 100.0, 'twin_sand', 'mesa'),
        (95.0, 1600.0, 520.0, 230.0, 1.0, 170.0, 'shard_sand', None),
    ],
    # 拠点の島の空: 灰色の岩に草の島。足元の下に見下ろす島、同じ高さ、見上げる島を混ぜる (広場の床は y 35)
    'main': [
        (-90.0, 1700.0, -260.0, 620.0, 1.0, 30.0, 'slab_grass', 'camp'),
        (-60.0, 2150.0, 420.0, 480.0, 1.0, 120.0, 'mesa_grass', 'lookout'),
        (-120.0, 1600.0, 120.0, 520.0, 1.0, 200.0, 'round', 'forest'),
        (-150.0, 2250.0, 780.0, 300.0, 1.2, 80.0, 'shard_grass', None),
        (-30.0, 1550.0, 60.0, 260.0, 1.2, 250.0, 'shard_grass', 'rocks'),
        (180.0, 1750.0, -120.0, 700.0, 0.9, 160.0, 'long_grass', 'forest'),
        (150.0, 2300.0, 650.0, 560.0, 1.0, 300.0, 'twin_grass', 'ruins'),
        (40.0, 1900.0, 900.0, 400.0, 1.0, 20.0, 'brick', 'forest'),
        (80.0, 1650.0, -380.0, 460.0, 1.0, 340.0, 'twin_grass', 'camp'),
        (120.0, 2000.0, 250.0, 380.0, 1.0, 90.0, 'mesa_grass', 'ruins'),
        (-5.0, 2100.0, 300.0, 520.0, 1.0, 190.0, 'long_grass', 'lookout'),
        (-175.0, 1850.0, -60.0, 330.0, 1.0, 60.0, 'round', 'rocks'),
    ],
}

LAYOUTS['prologue'] = LAYOUTS['main']

# 飾りの組: [(prefab のパス (Prop/ 以下), 大きさ, 数[, 'center'])]。prefab の scale = 大きさ x 島の大きさ
# (島の大きさ = 天辺の面積を丸い Island_Broken の scale に直した値。木は島の幅の 1 割強の高さになる)。
# 'center' は島の中ほどに1つ置く大物
DECOR = {
    'forest': [('Tree/Pine_A', 14.0, 5), ('Tree/Spruce_B', 17.0, 4), ('Tree/Oak_A', 18.0, 3), ('Rock/StylizedRock', 13.0, 2)],
    'ruins': [('Settlement/RuinedStoneHouse', 13.0, 1), ('Settlement/FallenRuinPillar', 14.0, 2),
              ('Tree/Oak_A', 18.0, 3), ('Tree/Birch_A', 15.0, 3)],
    'lookout': [('Settlement/Lookout', 16.0, 1), ('Tree/Pine_A', 14.0, 4), ('Tree/Spruce_B', 17.0, 3)],
    'rocks': [('Rock/StylizedRock1', 12.0, 1), ('Rock/FantasyRock', 16.0, 2), ('Tree/Pine_A', 14.0, 2)],
    'camp': [('Settlement/HideTent', 13.0, 2), ('Settlement/LeanTo', 13.0, 1), ('Settlement/DryingRack', 12.0, 1),
             ('Settlement/TrailBanner', 12.0, 2), ('Tree/Birch_A', 15.0, 3), ('Tree/Pine_A', 14.0, 2)],
    'mesa': [('Desert/DesertMesa', 8.0, 1, 'center'), ('Desert/PalmA', 10.0, 2), ('Desert/CactusTall', 8.0, 3)],
    'fortress': [('Desert/FortressTower', 13.0, 1, 'center'), ('Desert/FortressSpire', 9.0, 1),
                 ('Desert/FortressWallBlock', 9.0, 2), ('Settlement/RubblePile', 11.0, 3)],
    'temple': [('Desert/DesertTemple', 8.0, 1, 'center'), ('Desert/SandstonePillar', 6.0, 2), ('Desert/PalmB', 10.0, 3)],
    'pillars': [('Desert/SandstonePillar', 7.0, 3), ('Desert/SandstoneCliff', 11.0, 1), ('Desert/CactusGroup', 10.0, 2)],
    'oasis': [('Desert/PalmA', 10.0, 4), ('Desert/PalmB', 10.0, 3), ('Desert/PalmBent', 12.0, 1),
              ('Rock/SandyRock', 18.0, 2)],
    'arch': [('Desert/FortressGate', 9.0, 1, 'center'), ('Desert/FortressArch', 10.0, 2), ('Desert/CactusGroup', 10.0, 2)],
}
# 島の脇に浮かぶ岩のかけら
DEBRIS = {
    'grass': ['Rock/StylizedRock', 'Rock/FantasyRock', 'Rock/Rock17'],
    'desert': ['Rock/SandyRock', 'Desert/SandstoneCliff', 'Rock/DesertRockBase'],
    'main': ['Rock/StylizedRock', 'Rock/FantasyRock', 'Rock/Rock17'],
}
DEBRIS['prologue'] = DEBRIS['main']
DEBRIS_SCALE = {'Desert/SandstoneCliff': 0.25}   # 元が大きい岩は小さくする
DEBRIS_COUNT = {'grass': (2, 4), 'desert': (1, 3), 'main': (2, 4), 'prologue': (2, 4)}
DEBRIS_SIZE = (10.0, 24.0)
ROUND_WIDTH = 4780.0          # 飾りの大きさの基準 (丸い Island_Broken の幅 model 単位)
DECOR_RADIUS = 0.72           # 飾りを置く楕円の半径 (半幅に対して)
DECOR_SINK = 0.3              # 飾りの根元を沈める量 (x 飾りの scale)

STRIP_SUFFIXES = ('Collider', '::RigidBody', 'DestructibleObject')


# ---------------------------------------------------------------- 配置
class Island:
    def __init__(self, spec, center):
        az, dist, top, width, stretch, yaw, model, decor = spec
        a = math.radians(az)
        self.pos = (center[0] + math.cos(a) * dist, top, center[1] + math.sin(a) * dist)
        self.stretch, self.yaw, self.model, self.decor = stretch, yaw, model, decor
        self.mv1, self.top, half, depth, *terrace = MODEL_INFO[model]
        self.scale = width / (2.0 * max(half))
        self.half = (half[0] * self.scale, half[1] * self.scale)
        self.depth = depth * self.scale * stretch
        self.terrace = (terrace[0][0] * self.scale, terrace[0][1]) if terrace else None
        # 飾りの大きさの基準: 天辺の面積が同じ丸い島の scale
        self.unit = 2.0 * math.sqrt(self.half[0] * self.half[1]) / ROUND_WIDTH

    @property
    def span(self):
        return 2.0 * math.sqrt(self.half[0] * self.half[1])


def place_decor(rng, island):
    """(prefab, 島からの相対位置, 回転, scale)。互いに重ならないように天辺の楕円の中へ散らす。段のある島は段の上か下に"""
    out, taken = [], []
    if island.decor is None:
        return out
    rx, rz = island.half[0] * DECOR_RADIUS, island.half[1] * DECOR_RADIUS
    for prefab, size, count, *flags in DECOR[island.decor]:
        scale = size * island.unit
        center = 'center' in flags
        for _ in range(count):
            for _try in range(60):
                k = (0.2 if center else 1.0) * math.sqrt(rng.random())
                a = rng.random() * math.tau
                x, z = math.cos(a) * rx * k, math.sin(a) * rz * k
                if island.terrace and abs(k * DECOR_RADIUS - island.terrace[1]) < 0.1:
                    continue
                # 大物の周りは広く空ける
                if all(math.hypot(x - tx, z - tz) > island.span * (0.3 if big or center else 0.08)
                       for tx, tz, big in taken):
                    break
            taken.append((x, z, center))
            y = island.terrace[0] if island.terrace and k * DECOR_RADIUS < island.terrace[1] else 0.0
            s = scale * rng.uniform(0.85, 1.15)
            out.append((prefab, (x, y - DECOR_SINK * s, z), rotation(rng.random() * math.tau), s))
    return out


def place_debris(rng, island, kinds, count):
    out = []
    for i in range(int(rng.integers(count[0], count[1] + 1))):
        name = kinds[i % len(kinds)]
        a = rng.random() * math.tau
        k = rng.uniform(1.15, 1.45)
        y = -island.depth * rng.uniform(0.1, 0.45)
        s = island.unit * rng.uniform(*DEBRIS_SIZE) * DEBRIS_SCALE.get(name, 1.0)
        rot = rotation(rng.random() * math.tau, rng.uniform(-0.6, 0.6), rng.uniform(-0.6, 0.6))
        out.append((name, (math.cos(a) * island.half[0] * k, y, math.sin(a) * island.half[1] * k), rot, s))
    return out


# ---------------------------------------------------------------- シーン
def strip_physics(node):
    for n in walk(node):
        n.components = [c for c in n.components if not c.fqn.endswith(STRIP_SUFFIXES)]


def drift(b, node, bob, period, tilt, tilt_period):
    b.component(node, 'FloatingDrift', bobHeight_=f'{bob:.2f}', bobPeriod_secs_=f'{period:.1f}',
                tiltDegrees_=f'{tilt:.2f}', tiltPeriod_secs_=f'{tilt_period:.1f}')


def num(v):
    return float(v.value if hasattr(v, 'value') else v)


def set_camera_and_sky(scene):
    # NOTE: ドームが複数あるシーン (序章の嵐) は、SkyDome の大きさに対する比を保つ
    base = next((num(r.transform.local_scale.x) for r in scene.roots if r.name == 'SkyDome'), None)
    for root in scene.roots:
        for node in walk(root):
            for comp in node.components:
                if comp.fqn.endswith('::CinemachineCameraBrain') and 'cameraFar_' in comp.data:
                    comp.data['cameraFar_'] = Num.of_float(CAMERA_FAR)
                    print(f'  {node.name}: cameraFar_ {CAMERA_FAR:.0f}')
                if comp.fqn.endswith('::SkyDome3D'):
                    scale = SKY_DOME_SCALE
                    if base:
                        scale *= num(node.transform.local_scale.x) / base
                    node.transform.local_scale = edits._vec3_from_floats((scale,) * 3)
                    bake_world_matrices(node)
                    print(f'  {node.name}: scale {scale:.3f}')


def build(key):
    path = SCENES[key]
    rng = np.random.default_rng(SEEDS[key])
    scene = reader.read_scene_file(path)
    scene.roots = [r for r in scene.roots if r.name != ROOT_NAME]
    set_camera_and_sky(scene)

    b = Builder(scene)
    root = edits.add_gameobject(scene, parent=None, name=ROOT_NAME)
    counts = add_islands(scene, b, root, LAYOUTS[key], CENTERS[key], rng, DEBRIS[key], DEBRIS_COUNT[key])

    for node in walk(root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root)

    text = writer.write_scene(scene)
    tree = loads(text)
    stripper = StrayVersionStripper(catalog_mod.load(), f'/gameObject_{len(scene.roots) - 1}/')
    stripper.run(tree, '')
    text = dumps(tree)
    print(f'  stripped {stripper.stripped} repeat cereal_class_version key(s) inside {ROOT_NAME}')

    versions = first_versions(text)
    # NOTE: ModelRenderer は v4 と v5 で読み方が同じ (序章のシーンは v4 のまま)
    if versions.get('NanamiEngine::Module::Component::ModelRenderer') not in (4, 5):
        raise SystemExit('ModelRenderer: first occurrence is not v4/v5 - nothing written')
    check(text, validate.validate_scene(scene), path.name)
    path.write_bytes(to_file_bytes(text))
    reader.read_scene_file(path)
    print(f'wrote {path.relative_to(REPO)}  {counts}')


def add_islands(scene, b, root, specs, center, rng, debris_kinds, debris_count):
    """specs の島を root の子に組む。タイトル (title_scene.py) も使う"""
    prefabs = {}

    def prefab(name):
        if name not in prefabs:
            prefabs[name] = reader.read_prefab_file(PREFAB / f'{name}.prefab')
        return prefabs[name]

    def put(parent, name, pos, rot, scale):
        node = edits.instantiate_prefab(scene, prefab(name), parent=parent.guid)
        node.transform.local_pos = edits._vec3_from_floats(tuple(float(v) for v in pos))
        node.transform.local_rot = edits._quat_from_floats(tuple(float(v) for v in rot))
        node.transform.local_scale = edits._vec3_from_floats((float(scale),) * 3)
        strip_physics(node)
        return node

    counts = {'islands': 0, 'decor': 0, 'debris': 0}
    for i, spec in enumerate(specs):
        island = Island(spec, center)
        node = edits.add_gameobject(scene, parent=root.guid, name=f'Island{i + 1}_{island.model}_{island.decor or "bare"}')
        node.transform.local_pos = edits._vec3_from_floats(island.pos)
        node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), math.radians(island.yaw)))
        drift(b, node, 3.0 + island.unit * 60.0, rng.uniform(14.0, 22.0), rng.uniform(0.6, 1.1), rng.uniform(20.0, 30.0))

        model = edits.add_gameobject(scene, parent=node.guid, name='Model')
        s = island.scale
        size = (s, s * island.stretch, s)
        model.transform.local_pos = edits._vec3_from_floats(tuple(-k * v for k, v in zip(size, island.top)))
        model.transform.local_scale = edits._vec3_from_floats(size)
        b.component(model, 'ModelRenderer', mv1File_=asset_guid(MODELS / f'{island.mv1}.mv1.meta'))

        for name, pos, rot, scale in place_decor(rng, island):
            put(node, name, pos, rot, scale)
            counts['decor'] += 1
        for name, pos, rot, scale in place_debris(rng, island, debris_kinds, debris_count):
            chunk = put(node, name, pos, rot, scale)
            drift(b, chunk, 4.0 + scale * 2.0, rng.uniform(7.0, 12.0), rng.uniform(3.0, 6.0), rng.uniform(9.0, 15.0))
            counts['debris'] += 1
        counts['islands'] += 1
    return counts


def main():
    keys = sys.argv[1:] or list(SCENES)
    for key in keys:
        if key not in SCENES:
            raise SystemExit(f'unknown scene {key!r} (grass / desert / main / prologue)')
        print(f'[{key}]')
        build(key)


if __name__ == '__main__':
    main()
