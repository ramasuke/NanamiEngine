"""GrassLandScene に木・岩・茂み・花・倒木を散らし、草(GrassField)を生やす。

    python tools/art/grassland_nature_prefabs.py     # 先にプレハブを組む
    python tools/art/grassland_nature_scatter.py     # シーンの Nature ルートを置き直す + 草を生成

- 地形の高さは data/grassland_terrain_mesh.npz。GrassLand.mv1 の元 GrassLand_reshaped.fbx の 257x257 格子の頂点高さ
  (Blender 単位)と、各マスの三角形の割り方。三角形で補間するので見た目のメッシュと一致する
  (HeightGridMap は作り直し前の地形のままなので使わない)。world = blender x 1.5、y だけ + 29.92。
  遊べる範囲は壁の内側 250..1250 で、外側は景色。
- 置き直すときは前回の Nature ルートを消してから置くので、何度実行しても1組だけになる。乱数は --seed で固定。
- シーンの ColliderBase は v5 で保存されていたので、置く前に v6 の並びへ移す (全て Static なので意味は同じ)。
  StaticMeshCollider も v3 -> v4 (地形の簡略化設定はエンジンの既定値のまま)。
- 草は GrassLandGrass.grassField (.meta に量子化した位置を持つ)。Terrain/Model に Grassable も付けるので、
  プレイ中の配置モードで手で足すこともできる。
"""
import argparse
import base64
import math
import struct
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, mathutil, reader, validate, writer  # noqa: E402

from game_over_prefab import Builder, asset_guid, check, let_writer_place_versions  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'GrassLandScene.scene'
HEIGHTS = Path(__file__).resolve().parent / 'data' / 'grassland_terrain_mesh.npz'
PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop'
GRASS_DIR = REPO / 'Assets' / 'Data' / 'GrassField'
GRASS_NAME = 'GrassLandGrass'
GRASS_VS = asset_guid(REPO / 'Assets/Art/Shaders/Grass/Grass_VS.vso.meta')
GRASS_PS = asset_guid(REPO / 'Assets/Art/Shaders/Grass/Grass_PS.pso.meta')
ROOT_NAME = 'Nature'

PLAY_MIN, PLAY_MAX = 250.0, 1250.0
PORTAL = (378.0, 1201.0)
# 到着演出(ポータルから -Z へ歩いて出てくる + 低いカメラ)と、敵の湧き位置は空けておく
CLEAR_SPAWN = ((380.0, 1170.0), 95.0)
CLEAR_ENEMY = ((565.0, 902.0), 40.0)
# 中央の盆地は戦う場所なので木と大岩を置かない
BASIN = ((820.0, 630.0), (165.0, 245.0))

# 木: (名前, 樹高 world 単位 @scale 1)
TREES = {
    'Oak': ['Oak_A', 'Oak_B'],
    'Birch': ['Birch_A', 'Birch_B'],
    'Pine': ['Pine_A', 'Pine_B'],
    'Spruce': ['Spruce_A', 'Spruce_B', 'Spruce_C'],
}
# 岩: 名前 -> (水平半径, 高さ) world 単位 @scale 1 (Blender の外形 m x 8)
ROCK_SIZE = {
    'RuinedRockFence': (18.0, 15.6),
    'Rock17': (12.0, 13.4),
    'SandyRock': (8.9, 16.0),
    'FantasyRock': (7.6, 17.7),
    'StylizedRock2': (4.6, 14.4),
    'DesertRockBase': (8.0, 10.9),
    'StylizedRock': (14.0, 20.7),
    'StylizedRock1': (12.0, 61.0),
    'CyberpunkRock': (28.0, 46.6),
}
FENCE_LENGTH = 35.0

# 林の中心 (x, z, 半径)。地形図(急斜面・盆地・台地)を見て手で置いた
GROVES = [
    (280, 1080, 60), (330, 900, 70), (440, 760, 60), (330, 640, 70), (470, 540, 60),
    (330, 380, 70), (600, 300, 70), (620, 1150, 70), (780, 1180, 70), (950, 1150, 70),
    (1100, 1180, 70), (1200, 950, 60), (1150, 700, 80), (1100, 500, 70), (1180, 330, 60),
]
MEADOW_SEEDS = [(440.0, 1170.0), (820.0, 620.0), (420.0, 700.0), (1200.0, 930.0)]


# ---------------------------------------------------------------- terrain
class Terrain:
    def __init__(self):
        data = np.load(HEIGHTS)
        self.h = data['height'].astype(np.float64) * 1.5 + 29.92
        if not (data['diag'] == 2).all():
            raise SystemExit('terrain cells are not all split along the (i+1,j)-(i,j+1) diagonal - update height()')
        self.n = self.h.shape[0]
        self.step = 1500.0 / (self.n - 1)

    def height(self, x, z):
        """各マスは (i+1,j)-(i,j+1) の対角線で2枚の三角形に割られている"""
        fx = np.clip(np.asarray(x, float) / self.step, 0, self.n - 1.0001)
        fz = np.clip(np.asarray(z, float) / self.step, 0, self.n - 1.0001)
        i0 = fx.astype(int)
        j0 = fz.astype(int)
        tx = fx - i0
        tz = fz - j0
        g = self.h
        h00, h10, h01, h11 = g[j0, i0], g[j0, i0 + 1], g[j0 + 1, i0], g[j0 + 1, i0 + 1]
        lower = h00 + tx * (h10 - h00) + tz * (h01 - h00)
        upper = h11 + (1 - tx) * (h01 - h11) + (1 - tz) * (h10 - h11)
        return np.where(tx + tz <= 1.0, lower, upper)

    def gradient(self, x, z, d=3.0):
        gx = (self.height(x + d, z) - self.height(x - d, z)) / (2 * d)
        gz = (self.height(x, z + d) - self.height(x, z - d)) / (2 * d)
        return gx, gz

    def slope(self, x, z, d=3.0):
        gx, gz = self.gradient(x, z, d)
        return np.degrees(np.arctan(np.hypot(gx, gz)))

    def relief(self, x, z, radius):
        """足元の円内の高低差。崖の縁をまたぐ場所を避けるのに使う"""
        ys = [float(self.height(x + math.cos(k * math.pi / 4) * radius, z + math.sin(k * math.pi / 4) * radius))
              for k in range(8)] + [float(self.height(x, z))]
        return max(ys) - min(ys)

    def lowest(self, x, z, radius):
        """足元の円周と中心で一番低い地面。斜面でも根元が浮かない"""
        ys = [float(self.height(x, z))]
        for k in range(8):
            a = k * math.pi / 4
            ys.append(float(self.height(x + math.cos(a) * radius, z + math.sin(a) * radius)))
        return min(ys)


# ---------------------------------------------------------------- placement helpers
def inside_play(x, z, margin=0.0):
    return PLAY_MIN + margin <= x <= PLAY_MAX - margin and PLAY_MIN + margin <= z <= PLAY_MAX - margin


def in_outskirts(x, z, band=170.0):
    """壁の外の景色の帯。中から見える壁際だけに置き、島の縁は落ち込むので端は空ける"""
    return (40.0 <= x <= 1460.0 and 40.0 <= z <= 1460.0 and not inside_play(x, z, -12.0)
            and inside_play(x, z, -band))


def in_circle(x, z, circle, extra=0.0):
    (cx, cz), r = circle
    return (x - cx) ** 2 + (z - cz) ** 2 < (r + extra) ** 2


def in_basin(x, z, scale=1.0):
    (cx, cz), (rx, rz) = BASIN
    return ((x - cx) / (rx * scale)) ** 2 + ((z - cz) / (rz * scale)) ** 2 < 1.0


def grove_field(x, z):
    v = 0.0
    for gx, gz, r in GROVES:
        v += math.exp(-((x - gx) ** 2 + (z - gz) ** 2) / (2 * r * r))
    return min(1.0, v)


class Spacing:
    """最小間隔を守るための粗い格子"""

    def __init__(self, cell=16.0):
        self.cell = cell
        self.grid = {}

    def ok(self, x, z, r):
        ci, cj = int(x // self.cell), int(z // self.cell)
        reach = int(math.ceil((r + 40.0) / self.cell))
        for i in range(ci - reach, ci + reach + 1):
            for j in range(cj - reach, cj + reach + 1):
                for (px, pz, pr) in self.grid.get((i, j), ()):
                    if (px - x) ** 2 + (pz - z) ** 2 < max(r, pr) ** 2:
                        return False
        return True

    def add(self, x, z, r):
        self.grid.setdefault((int(x // self.cell), int(z // self.cell)), []).append((x, z, r))


def quat_axis(axis, angle):
    s = math.sin(angle / 2)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(angle / 2))


def rotation(yaw, tilt_x=0.0, tilt_z=0.0):
    q = quat_axis((0.0, 1.0, 0.0), yaw)
    q = mathutil.quat_mul(q, quat_axis((1.0, 0.0, 0.0), tilt_x))
    return mathutil.quat_mul(q, quat_axis((0.0, 0.0, 1.0), tilt_z))


def yaw_for_direction(dx, dz):
    """ローカル +X を (dx, 0, dz) へ向ける Y 回転"""
    return math.atan2(-dz, dx)


class Placement:
    __slots__ = ('group', 'prefab', 'pos', 'rot', 'scale', 'radius')

    def __init__(self, group, prefab, pos, rot, scale, radius):
        self.group, self.prefab, self.pos, self.rot, self.scale, self.radius = group, prefab, pos, rot, scale, radius


# ---------------------------------------------------------------- the plan
def pick_tree(rng, y):
    if y < 110:
        kinds, weights = ['Oak', 'Birch', 'Pine'], [0.45, 0.4, 0.15]
    elif y < 190:
        kinds, weights = ['Oak', 'Birch', 'Pine', 'Spruce'], [0.25, 0.25, 0.3, 0.2]
    else:
        kinds, weights = ['Pine', 'Spruce'], [0.4, 0.6]
    kind = rng.choice(kinds, p=weights)
    return kind, str(rng.choice(TREES[kind]))


def plan(rng, t):
    out = []
    solid = Spacing()      # 木・岩・倒木 (当たり判定のあるもの)
    soft = Spacing(8.0)    # 茂み・花

    def tree_at(x, z, scale, group):
        y = float(t.height(x, z))
        kind, name = pick_tree(rng, y)
        base = t.lowest(x, z, 1.2 * scale) - 0.4 * scale
        out.append(Placement(group, f'Tree/{name}', (x, base, z), rotation(rng.uniform(0, 2 * math.pi)),
                             scale, 3.0 * scale))
        solid.add(x, z, 11.0 * scale)
        return kind

    # --- 木(内側): 林の中心ほど密、急斜面・空ける場所・盆地の底は避ける
    inside_trees = []
    for _ in range(40000):
        if len(inside_trees) >= 85:
            break
        x, z = rng.uniform(PLAY_MIN + 8, PLAY_MAX - 8, 2)
        if rng.random() > 0.08 + 0.92 * grove_field(x, z):
            continue
        if t.slope(x, z) > 33 or in_circle(x, z, CLEAR_SPAWN) or in_circle(x, z, CLEAR_ENEMY) or in_basin(x, z):
            continue
        scale = rng.uniform(1.3, 1.9)
        if t.relief(x, z, 3.0 * scale) > 5.0:
            continue
        if not solid.ok(x, z, 11.0 * scale):
            continue
        kind = tree_at(x, z, scale, 'Trees')
        inside_trees.append((x, z, scale, kind))

    # --- 木(壁の外): 景色としての森。大きめで密
    outer_trees = []
    for _ in range(60000):
        if len(outer_trees) >= 75:
            break
        x, z = rng.uniform(40, 1460, 2)
        if not in_outskirts(x, z) or t.slope(x, z) > 35:
            continue
        if rng.random() > 0.35 + 0.65 * grove_field(x, z):
            continue
        scale = rng.uniform(2.0, 2.8)
        if t.relief(x, z, 3.0 * scale) > 7.0:
            continue
        if not solid.ok(x, z, 12.0 * scale):
            continue
        kind = tree_at(x, z, scale, 'Outskirts')
        outer_trees.append((x, z, scale, kind))

    # --- 崖の岩: 中くらいの斜面には半分埋め、切り立った崖(70-80度)にはその足元へ崩れ落ちた岩として置く
    outcrop_kinds = ['CyberpunkRock', 'StylizedRock', 'Rock17', 'SandyRock']
    outcrop_weights = [0.25, 0.3, 0.2, 0.25]
    outcrop_scale = {'CyberpunkRock': (0.8, 1.3), 'StylizedRock': (1.6, 2.6), 'Rock17': (1.8, 2.8),
                     'SandyRock': (2.2, 3.2)}
    rocks = []
    outer_rocks = 0
    for _ in range(80000):
        if len(rocks) >= 40:
            break
        x, z = rng.uniform(60, 1440, 2)
        inner = inside_play(x, z, 10)
        if not inner and (not in_outskirts(x, z) or outer_rocks >= 10):
            continue
        if in_circle(x, z, CLEAR_SPAWN, 20) or in_circle(x, z, CLEAR_ENEMY, 10):
            continue
        s = float(t.slope(x, z, 5.0))
        embedded = 32 <= s <= 58
        at_cliff_foot = s < 22 and t.relief(x, z, 22.0) > 25.0
        if not embedded and not at_cliff_foot:
            continue
        name = str(rng.choice(outcrop_kinds, p=outcrop_weights))
        scale = rng.uniform(*outcrop_scale[name]) * (1.0 if embedded else 0.7)
        r, h = ROCK_SIZE[name]
        if not solid.ok(x, z, max(r * scale, 30.0)):
            continue
        # 足元の一番低い所より下に底を置けば、どの向きにも浮かない。中心で高さの半分以上は見えるものだけ採る
        low = t.lowest(x, z, r * scale * 0.5)
        if float(t.height(x, z)) - low > h * scale * 0.5:
            continue
        y = low - h * scale * (0.08 if at_cliff_foot else 0.12)
        rot = rotation(rng.uniform(0, 2 * math.pi), math.radians(rng.uniform(-10, 10)), math.radians(rng.uniform(-10, 10)))
        out.append(Placement('Rocks' if inner else 'Outskirts', f'Rock/{name}', (x, y, z), rot, scale, r * scale))
        solid.add(x, z, max(r * scale, 30.0))
        rocks.append((x, z, r * scale))
        outer_rocks += 0 if inner else 1

    # --- 草原の岩: 平らな所にまばらに。一部は小岩を添える
    boulder_kinds = ['SandyRock', 'FantasyRock', 'StylizedRock2', 'DesertRockBase']
    boulder_weights = [0.3, 0.25, 0.3, 0.15]
    boulders = 0
    for _ in range(40000):
        if boulders >= 24:
            break
        x, z = rng.uniform(PLAY_MIN + 15, PLAY_MAX - 15, 2)
        if t.slope(x, z) > 22 or in_circle(x, z, CLEAR_SPAWN, 10) or in_circle(x, z, CLEAR_ENEMY, 10):
            continue
        if in_basin(x, z, 0.7):
            continue
        name = str(rng.choice(boulder_kinds, p=boulder_weights))
        scale = rng.uniform(1.0, 2.0) if name == 'StylizedRock2' else rng.uniform(0.9, 1.8)
        r, h = ROCK_SIZE[name]
        if not solid.ok(x, z, max(r * scale * 2.5, 45.0)):
            continue
        y = t.lowest(x, z, r * scale * 0.6) - h * scale * 0.12
        rot = rotation(rng.uniform(0, 2 * math.pi), math.radians(rng.uniform(-6, 6)), math.radians(rng.uniform(-6, 6)))
        out.append(Placement('Rocks', f'Rock/{name}', (x, y, z), rot, scale, r * scale))
        solid.add(x, z, max(r * scale * 2.5, 45.0))
        rocks.append((x, z, r * scale))
        boulders += 1
        for _ in range(int(rng.choice([0, 0, 1]))):
            a = rng.uniform(0, 2 * math.pi)
            d = r * scale + rng.uniform(4, 10)
            cx, cz = x + math.cos(a) * d, z + math.sin(a) * d
            small = str(rng.choice(['StylizedRock2', 'SandyRock', 'FantasyRock']))
            ss = rng.uniform(0.5, 0.8)
            sr, sh = ROCK_SIZE[small]
            if t.slope(cx, cz) > 28 or not inside_play(cx, cz, 10):
                continue
            cy = t.lowest(cx, cz, sr * ss * 0.6) - sh * ss * 0.15
            out.append(Placement('Rocks', f'Rock/{small}', (cx, cy, cz),
                                 rotation(rng.uniform(0, 2 * math.pi), math.radians(rng.uniform(-8, 8))), ss, sr * ss))
            rocks.append((cx, cz, sr * ss))

    # --- 浮き岩の目印: 盆地の南の縁と、北東の高台
    for (x, z, scale, yaw) in ((830.0, 440.0, 1.25, 0.6), (1205.0, 960.0, 0.9, 2.1)):
        r, h = ROCK_SIZE['StylizedRock1']
        y = t.lowest(x, z, 6.0) - 1.0
        out.append(Placement('Rocks', 'Rock/StylizedRock1', (x, y, z), rotation(yaw), scale, r * scale))
        solid.add(x, z, 30.0)
        rocks.append((x, z, r * scale))

    # --- 崩れた石垣: 等高線に沿って、ところどころ欠けさせる
    ruins = 0
    for (sx, sz, heading, count) in ((420.0, 1120.0, 0.0, 5), (1175.0, 880.0, 1.45, 5), (560.0, 470.0, 1.3, 5)):
        x, z = sx, sz
        for k in range(count):
            gx, gz = t.gradient(x, z, 6.0)
            glen = math.hypot(gx, gz)
            dx, dz = math.cos(heading), math.sin(heading)
            if glen > 1e-3:  # 等高線の向きへ寄せる
                cx, cz = -gz / glen, gx / glen
                if cx * dx + cz * dz < 0:
                    cx, cz = -cx, -cz
                dx, dz = 0.5 * dx + 0.5 * cx, 0.5 * dz + 0.5 * cz
                n = math.hypot(dx, dz)
                dx, dz = dx / n, dz / n
            mx, mz = x + dx * FENCE_LENGTH / 2, z + dz * FENCE_LENGTH / 2
            ends = [float(t.height(x, z)), float(t.height(x + dx * FENCE_LENGTH, z + dz * FENCE_LENGTH))]
            if (abs(ends[0] - ends[1]) < 7 and rng.random() > 0.2 and not in_circle(mx, mz, CLEAR_SPAWN)
                    and inside_play(mx, mz, FENCE_LENGTH / 2 + 5)):
                y = min(ends + [float(t.height(mx, mz))]) - 1.5
                pitch = math.atan2(ends[1] - ends[0], FENCE_LENGTH) * 0.6
                rot = mathutil.quat_mul(rotation(yaw_for_direction(dx, dz)), quat_axis((0.0, 0.0, 1.0), pitch))
                out.append(Placement('Ruins', 'Rock/RuinedRockFence', (mx, y, mz), rot, rng.uniform(0.9, 1.1), 18.0))
                solid.add(mx, mz, 20.0)
                rocks.append((mx, mz, 16.0))
                ruins += 1
            x += dx * (FENCE_LENGTH + rng.uniform(2, 9))
            z += dz * (FENCE_LENGTH + rng.uniform(2, 9))
            heading = math.atan2(dz, dx) + rng.uniform(-0.2, 0.2)

    # --- 倒木と切り株: 林の中
    logs = stumps = 0
    for _ in range(20000):
        if logs >= 10 and stumps >= 14:
            break
        x, z = rng.uniform(PLAY_MIN + 20, PLAY_MAX - 20, 2)
        if grove_field(x, z) < 0.45 or in_circle(x, z, CLEAR_SPAWN) or in_basin(x, z):
            continue
        gx, gz = t.gradient(x, z, 6.0)
        if logs < 10 and rng.random() < 0.5:
            glen = math.hypot(gx, gz)
            dx, dz = ((-gz / glen, gx / glen) if glen > 1e-3 else (1.0, 0.0))
            scale = rng.uniform(0.9, 1.3)
            half = 14.0 * scale
            ends = [float(t.height(x - dx * half, z - dz * half)), float(t.height(x + dx * half, z + dz * half))]
            if abs(ends[0] - ends[1]) > 3 or t.slope(x, z) > 20 or not solid.ok(x, z, 20.0 * scale):
                continue
            y = min(ends + [float(t.height(x, z))]) - 0.8
            out.append(Placement('Deadwood', 'Nature/FallenLog', (x, y, z), rotation(yaw_for_direction(dx, dz)), scale, half))
            solid.add(x, z, 20.0 * scale)
            logs += 1
        elif stumps < 14:
            scale = rng.uniform(0.9, 1.4)
            if t.slope(x, z) > 25 or not solid.ok(x, z, 9.0 * scale):
                continue
            y = t.lowest(x, z, 3.0 * scale) - 0.6
            out.append(Placement('Deadwood', 'Nature/TreeStump', (x, y, z), rotation(rng.uniform(0, 2 * math.pi)), scale, 4.0 * scale))
            solid.add(x, z, 9.0 * scale)
            stumps += 1

    # --- 茂み: 木と岩の根元
    def bush_near(x0, z0, dmin, dmax, fern_bias, group):
        for _ in range(6):
            a = rng.uniform(0, 2 * math.pi)
            d = rng.uniform(dmin, dmax)
            x, z = x0 + math.cos(a) * d, z0 + math.sin(a) * d
            if group != 'Outskirts' and not inside_play(x, z, 6):
                continue
            if t.slope(x, z) > 36 or in_circle(x, z, (PORTAL, 45.0)):
                continue
            name = 'Bush_B' if rng.random() < fern_bias else 'Bush_A'
            scale = rng.uniform(0.9, 1.5)
            if not soft.ok(x, z, 6.0 * scale) or not solid.ok(x, z, 2.0):
                continue
            y = t.lowest(x, z, 3.0 * scale) - 0.4
            out.append(Placement(group, f'Nature/{name}', (x, y, z), rotation(rng.uniform(0, 2 * math.pi)), scale, 6.0 * scale))
            soft.add(x, z, 6.0 * scale)
            return

    for (x, z, scale, kind) in inside_trees:
        for _ in range(int(rng.choice([0, 0, 1, 1, 2]))):
            bush_near(x, z, 5.0 * scale, 14.0 * scale, 0.6 if kind in ('Pine', 'Spruce') else 0.35, 'Bushes')
    for (x, z, r) in rocks:
        if inside_play(x, z) and rng.random() < 0.55:
            bush_near(x, z, r * 0.8 + 2, r * 0.8 + 9, 0.3, 'Bushes')

    # --- 花畑: 平らな所に群れで
    meadows = list(MEADOW_SEEDS)
    for _ in range(4000):
        if len(meadows) >= 13:
            break
        x, z = rng.uniform(PLAY_MIN + 30, PLAY_MAX - 30, 2)
        if t.slope(x, z) < 12 and all((x - mx) ** 2 + (z - mz) ** 2 > 110 ** 2 for mx, mz in meadows):
            meadows.append((x, z))
    flowers = 0
    for (mx, mz) in meadows:
        variant = 'FlowerPatch_A' if rng.random() < 0.55 else 'FlowerPatch_B'
        for _ in range(int(rng.integers(3, 7))):
            for _ in range(10):
                a = rng.uniform(0, 2 * math.pi)
                d = 45.0 * math.sqrt(rng.random())
                x, z = mx + math.cos(a) * d, mz + math.sin(a) * d
                if not inside_play(x, z, 8) or t.slope(x, z) > 18 or in_circle(x, z, (PORTAL, 38.0)):
                    continue
                scale = rng.uniform(0.9, 1.3)
                if not soft.ok(x, z, 9.0 * scale) or not solid.ok(x, z, 3.0):
                    continue
                name = variant if rng.random() < 0.8 else ('FlowerPatch_B' if variant == 'FlowerPatch_A' else 'FlowerPatch_A')
                y = t.lowest(x, z, 6.0 * scale) - 0.2
                out.append(Placement('Flowers', f'Nature/{name}', (x, y, z), rotation(rng.uniform(0, 2 * math.pi)), scale, 9.0 * scale))
                soft.add(x, z, 9.0 * scale)
                flowers += 1
                break
    print(f'plan: trees {len(inside_trees)} inside + {len(outer_trees)} outside, rocks {len(rocks)} '
          f'(boulders {boulders}, ruins {ruins}), logs {logs}, stumps {stumps}, '
          f'bushes {sum(1 for p in out if "Bush" in p.prefab)}, flowers {flowers} in {len(meadows)} meadows')
    return out, meadows


# ---------------------------------------------------------------- grass
def value_noise(rng, x, z, wavelength):
    size = int(1500 / wavelength) + 3
    grid = rng.random((size, size))
    fx, fz = x / wavelength, z / wavelength
    i, j = fx.astype(int), fz.astype(int)
    tx, tz = fx - i, fz - j
    tx, tz = tx * tx * (3 - 2 * tx), tz * tz * (3 - 2 * tz)
    return ((grid[j, i] * (1 - tx) + grid[j, i + 1] * tx) * (1 - tz)
            + (grid[j + 1, i] * (1 - tx) + grid[j + 1, i + 1] * tx) * tz)


def grass_blades(rng, t, placements, meadows):
    step = 1.0
    xs = np.arange(PLAY_MIN - 60, PLAY_MAX + 60, step)
    X, Z = np.meshgrid(xs, xs)
    X = X + rng.uniform(0, step, X.shape)
    Z = Z + rng.uniform(0, step, Z.shape)
    gx = (t.height(X + 1.5, Z) - t.height(X - 1.5, Z)) / 3.0
    gz = (t.height(X, Z + 1.5) - t.height(X, Z - 1.5)) / 3.0
    slope = np.degrees(np.arctan(np.hypot(gx, gz)))
    n = 0.65 * value_noise(rng, X, Z, 140.0) + 0.35 * value_noise(rng, X, Z, 45.0)
    density = (0.05 + 0.56 * n ** 1.6) * np.clip((38.0 - slope) / 10.0, 0.0, 1.0)
    for (mx, mz) in meadows:
        density += 0.28 * np.exp(-((X - mx) ** 2 + (Z - mz) ** 2) / (2 * 55.0 ** 2)) * (slope < 25)
    keep = rng.random(X.shape) < density * step * step
    for p in placements:
        if p.prefab.startswith('Rock/') or p.prefab.endswith(('FallenLog', 'TreeStump')):
            keep &= (X - p.pos[0]) ** 2 + (Z - p.pos[2]) ** 2 > (p.radius * 0.7) ** 2
    x, z = X[keep], Z[keep]
    y = t.height(x, z) - 0.3
    return x, y, z


def encode_chunks(x, y, z, chunk):
    cx = np.floor(x / chunk).astype(int)
    cz = np.floor(z / chunk).astype(int)
    records = []
    for key in sorted(set(zip(cx.tolist(), cz.tolist()))):
        m = (cx == key[0]) & (cz == key[1])
        bx, by, bz = x[m], y[m], z[m]
        lo, hi = float(np.float32(by.min())), float(np.float32(by.max()))
        qx = np.clip(np.rint((bx / chunk - key[0]) * 65535), 0, 65535).astype('<u2')
        qz = np.clip(np.rint((bz / chunk - key[1]) * 65535), 0, 65535).astype('<u2')
        qy = np.clip(np.rint((by - lo) / (hi - lo) * 65535) if hi > lo else np.zeros_like(by), 0, 65535).astype('<u2')
        raw = np.stack([qx, qz, qy], axis=1).astype('<u2').tobytes()
        records.append((key, lo, hi, int(m.sum()), base64.b64encode(raw).decode('ascii')))
    return records


def write_grass_field(records, chunk):
    data_path = GRASS_DIR / f'{GRASS_NAME}.grassField'
    meta_path = GRASS_DIR / f'{GRASS_NAME}.grassField.meta'
    guid = asset_guid(meta_path) if meta_path.exists() else edits.mint_guid().upper()
    data_path.write_bytes(b'')

    def f(v):
        return Num.of_float(float(np.float32(v)))

    def rgb(r, g, b):
        return OrderedObj([('value0', f(r)), ('value1', f(g)), ('value2', f(b))])

    chunks = []
    for i, ((cx, cz), lo, hi, count, blades) in enumerate(records):
        rec = OrderedObj()
        if i == 0:
            rec['cereal_class_version'] = Num.of_int(0)
        rec['cx'] = Num.of_int(cx)
        rec['cz'] = Num.of_int(cz)
        rec['minY'] = f(lo)
        rec['maxY'] = f(hi)
        rec['count'] = Num.of_int(count)
        rec['blades'] = blades
        chunks.append(rec)
    body = OrderedObj([
        ('cereal_class_version', Num.of_int(1)),
        ('value0', OrderedObj([
            ('cereal_class_version', Num.of_int(0)),
            ('value0', OrderedObj([('cereal_class_version', Num.of_int(0))])),
            ('contentPath_', f'Assets\\Data\\GrassField/{GRASS_NAME}.grassField'),
            ('guid_', OrderedObj([('cereal_class_version', Num.of_int(0)), ('value_', guid)])),
        ])),
        ('bladesPerClick_', Num.of_int(400)),
        ('brushRadius_', f(40.0)),
        ('maxSlopeDeg_', f(38.0)),
        ('rayHeight_', f(500.0)),
        ('heightMin_', f(1.8)),
        ('heightMax_', f(5.0)),
        ('widthMin_', f(0.5)),
        ('widthMax_', f(1.1)),
        ('bendAmount_', f(0.35)),
        ('baseColor_', rgb(0.11, 0.26, 0.07)),
        ('tipColor_', rgb(0.5, 0.66, 0.26)),
        ('colorVariation_', f(0.35)),
        ('ambient_', f(0.6)),
        ('windStrength_', f(1.6)),
        ('maxDrawDistance_', f(520.0)),
        ('chunkSize_', f(chunk)),
        ('chunks', chunks),
    ])
    root = OrderedObj([('value0', OrderedObj([
        ('polymorphic_id', Num.of_int(2147483649)),
        ('polymorphic_name', 'NanamiEngine::Module::Asset::GrassField'),
        ('ptr_wrapper', OrderedObj([('id', Num.of_int(2147483649)), ('data', body)])),
    ]))])
    meta_path.write_bytes(to_file_bytes(dumps(root)))
    print(f'wrote {meta_path.relative_to(REPO)}  ({sum(r[3] for r in records)} blades in {len(records)} chunks, guid {guid})')
    return guid


# ---------------------------------------------------------------- scene edits
LEGACY_COLLIDER_KEYS = ('mass_', 'isGravity_', 'emotionType_', 'constraints_')


def migrate_colliders(scene):
    """ColliderBase v5 -> v6 (motion 系を捨てる。このシーンは全て Static)、StaticMeshCollider v3 -> v4"""
    migrated = 0
    for root in scene.roots:
        for node in walk(root):
            for comp in node.components:
                slot = comp.data.get('value0') if hasattr(comp.data, 'get') else None
                body = slot.body if isinstance(slot, Ver) else slot
                # reader は初出だけを Ver (構造の指紋キー) にし、2回目以降は素の OrderedObj で持つ
                if isinstance(body, OrderedObj) and all(k in body for k in LEGACY_COLLIDER_KEYS + ('friction_',)):
                    motion = body.get('emotionType_')
                    if int(motion.value) != 0:
                        raise SystemExit(f'{node.name}: non-static legacy collider - migrate by hand')
                    for key in LEGACY_COLLIDER_KEYS:
                        body.pop(key)
                    if isinstance(slot, Ver):
                        slot.version = 6
                        slot.key = ('type', 'ColliderBase')
                    migrated += 1
                if comp.fqn.endswith('::StaticMeshCollider') and comp.class_version == 3:
                    comp.class_version = 4
                    comp.data.append('simplifyEnabled_', True)
                    comp.data.append('maxSimplifyError_', Num.of_float(5.0))
                    comp.data.append('minTriangleRatio_', Num.of_float(0.05000000074505806))
    print(f'  migrated {migrated} ColliderBase blob(s) to v6')


def walk(node):
    yield node
    for child in node.transform.children:
        yield from walk(child)


def f32(v):
    return struct.unpack('<f', struct.pack('<f', v))[0]


def world_matrix_from_trs(trs):
    x, y, z, w = trs.rot
    r = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
         [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
         [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
    cols = [[r[row][c] * trs.scale[c] for row in range(3)] + [0.0] for c in range(3)]
    cols.append([trs.pos[0], trs.pos[1], trs.pos[2], 1.0])
    obj = OrderedObj()
    for i, col in enumerate(cols):
        obj[f'value{i}'] = OrderedObj((f'value{j}', Num.of_float(f32(v))) for j, v in enumerate(col))
    return obj


def bake_world_matrices(node, parent=mathutil.IDENTITY):
    world = parent.then(edits._node_local_trs(node))
    node.transform.world_matrix = world_matrix_from_trs(world)
    for child in node.transform.children:
        bake_world_matrices(child, world)


class StrayVersionStripper(validate._ClassVersionAudit):
    """prefab から写した初出の版キーのうち、シーンでは2回目以降になったものだけを消す"""

    def __init__(self, cat, owner_path):
        super().__init__(cat)
        self.owner_path = owner_path
        self.stripped = 0

    def _check(self, type_key, node, where):
        if where.startswith(self.owner_path) and type_key in self._first and validate._VER in node:
            node.pop(validate._VER)
            self.stripped += 1
            return
        super()._check(type_key, node, where)


def first_versions(text):
    class Probe(validate._ClassVersionAudit):
        def __init__(self):
            super().__init__(catalog_mod.load())
            self.versions = {}

        def _check(self, type_key, node, where):
            if type_key not in self._first:
                version = node.get(validate._VER)
                self.versions[type_key] = version.value if version is not None else None
            super()._check(type_key, node, where)

    probe = Probe()
    probe.run(loads(text), '')
    return probe.versions


def build_scene(placements, grass_guid):
    scene = reader.read_scene_file(SCENE)
    scene.roots = [r for r in scene.roots if r.name != ROOT_NAME]
    migrate_colliders(scene)

    terrain_model = next(c for r in scene.roots if r.name == 'Terrain' for c in r.transform.children if c.name == 'Model')
    b = Builder(scene)
    if not any(c.fqn.endswith('::Grassable') for c in terrain_model.components):
        b.component(terrain_model, 'Grassable')

    root = edits.add_gameobject(scene, parent=None, name=ROOT_NAME)
    grass = edits.add_gameobject(scene, parent=root.guid, name='Grass')
    if grass_guid:
        b.component(grass, 'GrassRenderer', grassField_=grass_guid, vsFile_=GRASS_VS, psFile_=GRASS_PS)
    groups = {}
    for name in ('Trees', 'Rocks', 'Ruins', 'Deadwood', 'Bushes', 'Flowers', 'Outskirts'):
        groups[name] = edits.add_gameobject(scene, parent=root.guid, name=name)

    prefabs = {}
    for p in placements:
        if p.prefab not in prefabs:
            prefabs[p.prefab] = reader.read_prefab_file(PREFAB / f'{p.prefab}.prefab')
        node = edits.instantiate_prefab(scene, prefabs[p.prefab], parent=groups[p.group].guid)
        node.transform.local_pos = edits._vec3_from_floats(tuple(float(v) for v in p.pos))
        node.transform.local_rot = edits._quat_from_floats(tuple(float(v) for v in p.rot))
        node.transform.local_scale = edits._vec3_from_floats((float(p.scale),) * 3)
        if p.group == 'Outskirts':
            for n in walk(node):
                n.components = [c for c in n.components if not c.fqn.endswith('Collider')]
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
    for type_key, want in (('ColliderBase', 6), ('NanamiEngine::Module::Component::StaticMeshCollider', 4),
                           ('NanamiEngine::Module::Component::BoxCollider', 6),
                           ('NanamiEngine::Module::Component::ModelRenderer', 5)):
        if versions.get(type_key) != want:
            raise SystemExit(f'{type_key}: first occurrence is v{versions.get(type_key)}, expected v{want} - nothing written')
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({len(placements)} objects under {ROOT_NAME})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seed', type=int, default=20260918)
    ap.add_argument('--no-grass', action='store_true', help='草(GrassField)を生成しない')
    ap.add_argument('--dry-run', action='store_true', help='配置の数だけ表示して何も書かない')
    args = ap.parse_args()

    rng = np.random.default_rng(args.seed)
    t = Terrain()
    placements, meadows = plan(rng, t)
    if args.dry_run:
        return
    grass_guid = None
    if not args.no_grass:
        chunk = 200.0
        x, y, z = grass_blades(rng, t, placements, meadows)
        grass_guid = write_grass_field(encode_chunks(x, y, z, chunk), chunk)
    build_scene(placements, grass_guid)


if __name__ == '__main__':
    main()
