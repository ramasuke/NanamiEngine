"""砂漠ステージのシーン (Assets/Scene/DesertScene.scene) を組む。

    python tools/art/desert_scene.py            # 作り直す (シーンの .meta の GUID は保つ)
    python tools/art/desert_scene.py --dry-run  # 置く数だけ表示

- 土台は GrassLandScene.scene の複製。カメラの Brain・空・NetworkRunner・外周の壁・到着カメラ・スポーン地点など
  草原と同じ仕組みの物だけを残し、GameObject / Component の GUID はすべて振り直す (草原とは別の物にする)。
  草原の自然物・村・野営地・宝箱・柱のギミックと、敵の湧き地点の中身は捨てる。
- 地形は desert_terrain.py の DesertTerrain.mv1。Terrain のルート (scale 0.5, y 29.92) は外周の壁の親なので触らず、
  Model 子を scale 0.16 / y -59.84 にして world = m x 8、地面の高さ = npz の height にそろえる。
- 小物は desert_prefabs.py のプレハブ (Assets/Prefab/Prop/Desert)。配置の意図は desert_terrain.py の先頭を参照。
  城塞は神殿前の広場を囲む壁の輪で、南が崩れた門、東が割れ目 (はぐれた護衛の台詞)。壁は砂に半分沈める。
- 光の浮遊石 (LightFloatingStone.prefab) は広場の中央。骸竜はその傍に湧き、DesertCleared の後は湧かない。
- 隊商の人たちとクノイチは desert_caravan.py place / story_npcs.py が置く (このスクリプトは Caravan ルートを残す)。
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

from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, reader, validate, writer  # noqa: E402

from game_over_prefab import asset_guid, check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import (StrayVersionStripper, bake_world_matrices, quat_axis, rotation,  # noqa: E402
                                      walk, yaw_for_direction)
import desert_terrain as dt  # noqa: E402

SOURCE = REPO / 'Assets' / 'Scene' / 'GrassLandScene.scene'
SCENE = REPO / 'Assets' / 'Scene' / 'DesertScene.scene'
PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop'
TERRAIN_MV1 = REPO / 'Assets' / 'Art' / 'Models' / 'Terrain' / 'Desert' / 'DesertTerrain.mv1.meta'
LIGHT_STONE = PREFAB / 'Story' / 'LightFloatingStone.prefab'

KEEP_ROOTS = ['CameraBrain', 'Terrain', 'SkyDome', 'NetworkRunner', 'Canvas', 'PlayerSpawn Pos', 'EnemySpawnPoints',
              'WindZone', 'ArrivalCamera', 'GreenStoneCamera']
TERRAIN_ROOT_Y, TERRAIN_ROOT_SCALE = 29.92, 0.5
M = 8.0

# EnemyKind / StoryFlag (Assets/Scripts/Core/Game/Npc/Enemy/Type/EnemyKind.h, Story/Story_StoryFlag.h)
KIND_SCORPION, KIND_WORM, KIND_SKELETON_DRAGON = 4, 5, 6
FLAG_DESERT_CLEARED = 5

FORTRESS_RADIUS = 175.0


class Terrain:
    """data/desert_terrain.npz の world 高さ。各マスは (i+1,j)-(i,j+1) の対角線で2枚"""

    def __init__(self):
        data = np.load(dt.NPZ)
        self.h = data['height'].astype(np.float64)
        self.n = self.h.shape[0]
        self.step = dt.SIZE / (self.n - 1)

    def height(self, x, z):
        fx = min(max(x / self.step, 0.0), self.n - 1.0001)
        fz = min(max(z / self.step, 0.0), self.n - 1.0001)
        i0, j0 = int(fx), int(fz)
        tx, tz = fx - i0, fz - j0
        g = self.h
        h00, h10, h01, h11 = g[j0, i0], g[j0, i0 + 1], g[j0 + 1, i0], g[j0 + 1, i0 + 1]
        if tx + tz <= 1.0:
            return float(h00 + tx * (h10 - h00) + tz * (h01 - h00))
        return float(h11 + (1 - tx) * (h01 - h11) + (1 - tz) * (h10 - h11))

    def slope(self, x, z, d=3.0):
        gx = (self.height(x + d, z) - self.height(x - d, z)) / (2 * d)
        gz = (self.height(x, z + d) - self.height(x, z - d)) / (2 * d)
        return math.degrees(math.atan(math.hypot(gx, gz)))

    def lowest(self, x, z, radius):
        ys = [self.height(x, z)] + [self.height(x + math.cos(k * math.pi / 4) * radius,
                                                z + math.sin(k * math.pi / 4) * radius) for k in range(8)]
        return min(ys)


class Placement:
    def __init__(self, group, prefab, pos, yaw=0.0, scale=1.0, tilt=(0.0, 0.0)):
        self.group, self.prefab, self.pos, self.yaw, self.scale, self.tilt = group, prefab, pos, yaw, scale, tilt


# ---------------------------------------------------------------- 配置
def plan(rng, t):
    out = []
    taken = []   # (x, z, r) 重ならないように

    def free(x, z, r):
        return all(math.hypot(x - ox, z - oz) > r + orr for ox, oz, orr in taken)

    def put(group, prefab, x, z, r, yaw=None, scale=1.0, sink=0.0, tilt=(0.0, 0.0), footprint=None):
        yaw = rng.uniform(0, 2 * math.pi) if yaw is None else yaw
        y = t.lowest(x, z, footprint if footprint is not None else r * 0.6) - sink
        out.append(Placement(group, prefab, (x, y, z), yaw, scale, tilt))
        taken.append((x, z, r))

    px, pz = dt.PLAZA

    # --- 城塞: 広場を囲む壁の輪。南 (+z) に崩れた門、東 (+x) に割れ目
    walls = ['FortressWallTall', 'FortressWallBlock', 'FortressWallAngled', 'FortressWallBlockC', 'FortressWallAngledB',
             'FortressWallBlockB']
    gap_east, gate_south = 0.0, math.pi / 2
    k = 0
    for deg in range(0, 360, 26):
        th = math.radians(deg + 8)
        if abs(math.remainder(th - gap_east, 2 * math.pi)) < math.radians(18):
            continue   # 東の割れ目 (護衛が教える抜け道)
        if abs(math.remainder(th - gate_south, 2 * math.pi)) < math.radians(18):
            continue
        x, z = px + math.cos(th) * FORTRESS_RADIUS, pz + math.sin(th) * FORTRESS_RADIUS
        yaw = yaw_for_direction(-math.sin(th), math.cos(th))
        sink = rng.uniform(6, 26)
        tilt = (rng.uniform(-0.05, 0.05), rng.uniform(-0.06, 0.06))
        put('Fortress', walls[k % len(walls)], x, z, 40, yaw=yaw, sink=sink, tilt=tilt, footprint=30)
        k += 1
    for deg in (45, 135, 225, 315):
        th = math.radians(deg)
        put('Fortress', 'FortressTower', px + math.cos(th) * (FORTRESS_RADIUS + 10), pz + math.sin(th) * (FORTRESS_RADIUS + 10),
            18, yaw=rng.uniform(0, 6.28), sink=rng.uniform(4, 14), footprint=12)
    # 南の門 (半分崩れて砂に埋もれている)。門の前に崩れた岩
    gx, gz = px, pz + FORTRESS_RADIUS + 10
    # NOTE: FortressGate (元モデルの Entrance_Frame) は丸いドームで門に見えないので、装飾のアーチを大きくして門にする
    put('Fortress', 'FortressArch', gx, gz, 45, yaw=yaw_for_direction(1, 0), scale=2.2, sink=10, tilt=(0.03, -0.02),
        footprint=25)
    put('Rocks', 'SandstoneCliff', gx - 60, gz + 45, 30, yaw=rng.uniform(0, 6.28), scale=0.6, sink=6)
    # 神殿 (北)、その前に日輪、塔とアーチ
    put('Fortress', 'DesertTemple', px, pz - 135, 70, yaw=yaw_for_direction(1, 0), scale=0.55, sink=4, footprint=40)
    put('Fortress', 'SunRing', px, pz - 55, 40, yaw=yaw_for_direction(1, 0), scale=0.7, sink=6, footprint=10)
    put('Fortress', 'FortressSpire', px - 205, pz - 110, 30, yaw=0.4, sink=40, footprint=15)
    put('Fortress', 'FortressBalcony', px + 215, pz - 95, 25, yaw=2.1, sink=48, footprint=15)
    put('Fortress', 'FortressArch', px - 95, pz + 95, 30, yaw=yaw_for_direction(1, -0.6), sink=10, footprint=15)
    put('Fortress', 'FortressArchSmall', px + 110, pz + 70, 28, yaw=yaw_for_direction(1, 0.7), sink=12, footprint=15)
    put('Fortress', 'FortressWallEnd', px + 60, pz + 120, 14, yaw=1.2, sink=10, tilt=(0.25, 0.1), footprint=6)
    put('Fortress', 'FortressWallEnd', px - 70, pz + 130, 14, yaw=2.4, sink=12, tilt=(-0.3, 0.05), footprint=6)
    # 城塞の外にも崩れた壁の名残
    for x, z in ((650, 330), (1120, 520), (700, 610), (1060, 250)):
        put('Fortress', walls[int(rng.integers(len(walls)))], x, z, 45, sink=rng.uniform(20, 40),
            tilt=(rng.uniform(-0.1, 0.1), rng.uniform(-0.1, 0.1)), footprint=30)
    taken.append((px, pz, FORTRESS_RADIUS - 30))   # 広場の中は空ける (骸竜と戦う場所)

    # --- 竜の骨 (東の尾根)。翼を尾根に沿って広げ、胴のあった跡を城塞 (北西) へ向ける
    bx, bz = dt.BONES
    put('Props', 'DragonBones', bx, bz, 140, yaw=yaw_for_direction(px - bx, pz - bz) + math.pi / 2, scale=1.0, sink=6,
        footprint=60)

    # --- オアシス: 泥のくぼ地を囲むヤシ。野営地はその西
    ox, oz = dt.OASIS
    taken.append((ox, oz, 85))
    for i in range(11):
        th = i / 11 * 2 * math.pi + rng.uniform(-0.15, 0.15)
        r = rng.uniform(95, 135)
        x, z = ox + math.cos(th) * r, oz + math.sin(th) * r
        if free(x, z, 12):
            put('Plants', ['PalmA', 'PalmB', 'PalmBent'][i % 3], x, z, 12, scale=rng.uniform(0.9, 1.25), footprint=3,
                tilt=(rng.uniform(-0.08, 0.08), rng.uniform(-0.08, 0.08)))
    cx, cz = dt.CAMP
    for name, dx, dz, yaw in (('HideTent', -35, -25, 0.4), ('HideTent', 30, -40, 2.4), ('LeanTo', -45, 30, 1.2),
                              ('Campfire', 0, 0, 0.0), ('SupplyPile', 38, 22, 3.5), ('BrokenCart', -10, 55, 0.9),
                              ('DryingRack', 55, -5, 1.9)):
        put('Camp', f'Settlement/{name}', cx + dx, cz + dz, 16, yaw=yaw, footprint=6)
    for name, dx, dz in (('PalmA', -70, -60), ('PalmB', 70, 55), ('PalmBent', -80, 60)):
        put('Plants', name, cx + dx, cz + dz, 12, scale=rng.uniform(0.9, 1.2), footprint=3)
    # 到着の場所は空ける
    taken.append((dt.SPAWN[0], dt.SPAWN[1] - 25, 55))

    # --- 砂嵐でなくした荷車 (砂に埋もれた荷の依頼の目印)
    for x, z in ((760, 880), (520, 760), (980, 1080)):
        if free(x, z, 20):
            put('Props', 'Settlement/BrokenCart', x, z, 20, sink=5, tilt=(rng.uniform(-0.2, 0.2), rng.uniform(-0.15, 0.15)),
                footprint=6)

    # --- 外周: 台地と崖 (遊べる範囲の外側から、境目を隠す)
    for x, z, s in ((110, 300, 1.1), (140, 760, 0.9), (120, 1250, 1.2), (650, 110, 1.0), (1320, 140, 1.1),
                    (1390, 690, 0.9), (1360, 1210, 1.0), (820, 1400, 1.1), (300, 1410, 0.9), (1050, 1390, 0.8)):
        put('Rocks', 'DesertMesa', x, z, 110 * s, scale=s, footprint=60 * s)
    for i in range(22):
        side = i % 4
        u = rng.uniform(280, 1220)
        d = rng.uniform(1235, 1290)
        x, z = [(u, 1500 - d), (d, u), (u, d), (1500 - d, u)][side]
        if free(x, z, 40):
            put('Rocks', 'SandstoneCliff', x, z, 40, scale=rng.uniform(0.8, 1.3), sink=8, footprint=25)

    # --- 砂丘の中の岩の柱とサボテン
    def scatter(prefab, count, r, scale, group, avoid_worm=False, sink=0.0):
        n = tries = 0
        while n < count and tries < count * 60:
            tries += 1
            x, z = rng.uniform(dt.PLAY_MIN + 20, dt.PLAY_MAX - 20), rng.uniform(dt.PLAY_MIN + 20, dt.PLAY_MAX - 20)
            if avoid_worm and math.hypot(x - dt.WORM_SEA[0], z - dt.WORM_SEA[1]) < 170:
                continue
            if 250 < z < 600 and 560 < x < 1250:
                continue   # 城塞の帯
            if not free(x, z, r):
                continue
            put(group, prefab, x, z, r, scale=rng.uniform(*scale), sink=sink, footprint=r * 0.3)
            n += 1

    scatter('SandstonePillar', 7, 40, (0.6, 1.1), 'Rocks', sink=6)
    for name, count, scale in (('CactusCluster', 7, (0.45, 0.7)), ('CactusGroup', 9, (0.6, 0.9)),
                               ('CactusSmall', 16, (0.7, 1.1)), ('CactusTall', 10, (0.55, 0.85)),
                               ('CactusArm', 10, (0.55, 0.85))):
        scatter(name, count, 10, scale, 'Plants', avoid_worm=True)
    return out


def enemy_spawns(rng, t):
    """(名前, [(x, z)], kind, skip_flag)。骸竜の広場の近くには、骸竜のほかは置かない"""
    packs = [
        ('ScorpionPack_Oasis', [(660, 950), (700, 975), (640, 905)]),
        ('ScorpionPack_Dunes', [(820, 830), (860, 810), (790, 800)]),
        ('ScorpionPack_Ridge', [(990, 800), (1030, 760), (1150, 820)]),
        # NOTE: 城塞の近くに置くと骸竜の広場まで追ってきて混ざるので、広場 (PLAZA) から 350 以上離す
        ('ScorpionPack_WestDunes', [(560, 820), (600, 790), (520, 770)]),
        ('SandWorm_WestSea', [(430, 640)]),
        ('SandWorm_SouthSea', [(470, 470)]),
    ]
    out = []
    for name, spots in packs:
        kind = KIND_WORM if name.startswith('SandWorm') else KIND_SCORPION
        out.append((name, [(x, t.height(x, z) + 6.0, z) for x, z in spots], kind, -1))
    px, pz = dt.PLAZA
    out.append(('SkeletonDragon', [(px, t.height(px, pz - 40) + 6.0, pz - 40)], KIND_SKELETON_DRAGON, FLAG_DESERT_CLEARED))
    return out


# ---------------------------------------------------------------- シーン
def remint_all(scene):
    remap = {}
    for root in scene.roots:
        edits._remint_guids(root, remap)
    for root in scene.roots:
        edits._remap_guid_references(root, remap)
    return remap


def find(scene, *path):
    nodes = scene.roots
    node = None
    for name in path:
        node = next(n for n in nodes if n.name == name)
        nodes = node.transform.children
    return node


def set_trs(node, pos=None, yaw=None, scale=None, tilt=(0.0, 0.0)):
    if pos is not None:
        node.transform.local_pos = edits._vec3_from_floats(tuple(float(v) for v in pos))
    if yaw is not None:
        node.transform.local_rot = edits._quat_from_floats(tuple(float(v) for v in rotation(yaw, tilt[0], tilt[1])))
    if scale is not None:
        s = scale if isinstance(scale, tuple) else (scale,) * 3
        node.transform.local_scale = edits._vec3_from_floats(tuple(float(v) for v in s))


def field_guid(comp, key, guid):
    edits._set_field_guid(comp.data[key], guid)


def component(node, leaf):
    return next(c for c in node.components if c.fqn.endswith('::' + leaf))


def build(placements, spawns, t, dry_run):
    scene = reader.read_scene_file(SOURCE)
    # NOTE: 草原の緑の浮遊石は捨てる村 (Settlement) の中にあるので、ここで光の石に置き換える
    scene.roots = [r for r in scene.roots if r.name in KEEP_ROOTS]
    scene.name = 'Desert'
    remint_all(scene)

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

    # スポーン地点と到着カメラ (草原と同じずらし方)
    sx, sz = dt.SPAWN
    sy = t.height(sx, sz) + 5.0
    set_trs(find(scene, 'PlayerSpawn Pos'), pos=(sx, sy, sz))
    set_trs(find(scene, 'ArrivalCamera'), pos=(sx + 9.0, sy + 2.0, sz - 24.0))

    # 光の浮遊石と、それを追うカメラ
    stone_cam = find(scene, 'GreenStoneCamera')
    stone_cam.name = 'LightStoneCamera'
    px, pz = dt.PLAZA
    set_trs(stone_cam, pos=(px + 80.0, t.height(px + 80, pz + 140) + 30.0, pz + 140.0))
    stone_prefab = reader.read_prefab_file(LIGHT_STONE)
    stone = edits.instantiate_prefab(scene, stone_prefab)
    set_trs(stone, pos=(px, t.lowest(px, pz, 12) - 1.0, pz), yaw=0.0)

    # 敵の湧き地点: 草原のハイエナの地点を1つ写して使う
    root = find(scene, 'EnemySpawnPoints')
    template = next(n for pack in root.transform.children for n in walk(pack)
                    if any(c.fqn.endswith('::EnemySpawnPoint') for c in n.components))
    template = copy.deepcopy(template)
    template.transform.children = []
    root.transform.children = []
    for name, spots, kind, skip in spawns:
        group = edits.add_gameobject(scene, parent=root.guid, name=name)
        for i, pos in enumerate(spots):
            node = copy.deepcopy(template)
            edits._remint_guids(node, {})
            node.name = f'{name.split("_")[0]}{i + 1}' if len(spots) > 1 else name.split('_')[0]
            sp = component(node, 'EnemySpawnPoint')
            sp.data['kind_'] = Num.of_int(kind)
            sp.data['skipIfStoryFlag_'] = Num.of_int(skip)
            set_trs(node, pos=pos, yaw=0.0)
            group.transform.children.append(node)

    # 小物
    groups = {}
    desert = edits.add_gameobject(scene, parent=None, name='Desert')
    for g in ('Fortress', 'Rocks', 'Plants', 'Props', 'Camp'):
        groups[g] = edits.add_gameobject(scene, parent=desert.guid, name=g)
    edits.add_gameobject(scene, parent=None, name='Caravan')
    prefabs = {}
    for p in placements:
        path = PREFAB / (p.prefab + '.prefab' if '/' in p.prefab else f'Desert/{p.prefab}.prefab')
        if path not in prefabs:
            prefabs[path] = reader.read_prefab_file(path)
        node = edits.instantiate_prefab(scene, prefabs[path], parent=groups[p.group].guid)
        set_trs(node, pos=p.pos, yaw=p.yaw, scale=p.scale, tilt=p.tilt)

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
        content_path = scene_meta.content_path_for(scene_meta.SCENE_SPEC, 'DesertScene', SCENE.parent, REPO)
        scene_meta.write_meta(scene_meta.SCENE_SPEC, meta, 'DesertScene', guid, content_path)
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  (asset guid {asset_guid(meta)}, {len(placements)} props)')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seed', type=int, default=20260924)
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()
    rng = np.random.default_rng(args.seed)
    t = Terrain()
    placements = plan(rng, t)
    counts = {}
    for p in placements:
        counts[p.prefab] = counts.get(p.prefab, 0) + 1
    print(f'{len(placements)} props: ' + ', '.join(f'{k} {v}' for k, v in sorted(counts.items())))
    build(placements, enemy_spawns(rng, t), t, args.dry_run)


if __name__ == '__main__':
    main()
