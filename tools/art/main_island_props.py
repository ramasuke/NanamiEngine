"""拠点の島とその周りの島の地面に小物を置く。序章 (襲撃の前) はにぎやかに、第1章 (襲撃のあと) は荒れた姿にする。

    python tools/art/main_island_props.py [prologue] [main]      # 省略時は両方

- docs/Story.md: 序章 (FirstTouchDownMainIsLandScene) で竜に心臓を奪われ、第1章 (MainIslandScene) の島は半壊して沈みつつある
  (瓦礫、傾いた建物、消えた灯り)。だから序章の方が栄えていて、第1章は焼け残りと、落ちた島から逃げてきた人の仮住まいにする。
  第1章の島の中心 (心臓が抜けた所) には、地面の裂けた跡 (CoreShardCrater) を残す。
- 島ごとに置く。噴水の島 (SecondIsland) と家の島 (ThirdIsland) は序章で雲の下へ落ち、噴水の島は第1章で浮かび上がってくるので、
  その島の小物は島の子 (IslandProps) にして一緒に動かす。第1章の家の島は落ちたまま (隠してある) なので置かない。
  第1章の噴水の島は、狩人の一族が移り住む所 (一族の目印は赤い旗)。一族の家の建つ場所 (RestorationSite_ClanHouse) は空ける。
- 高さは FirstTouchDownMainIsland.heightGridMap (同じ島々) から取る。当たり判定の上面なので、根元を GROUND_LIFT だけ持ち上げる。
  置いた場所がその島の地面 (ISLANDS の ground) から外れていたら (屋根や壁の上)、何も書かずに止める。
- 通り道 (PATHS) の近くには当たり判定のある物を置かない (花は置いてよい)。広場 -> 転移台 -> 2本の橋、
  広場から訓練の島への橋へ斜めに行く道、広場 -> 屋台。近すぎたら何も書かずに止める。
- 大きさは草原の野営地 (GrassLandScene の Settlement / Nature) と同じ。人の scale も同じ (0.1 前後)。
- (8, 178) の花は草 (GrassRenderer) と重なって上から見ると黒い四角が出たので、そこには花を置かない。
- 序章のシーンは ColliderBase が v5 のまま保存されている (初出の版で全部読まれる)。prefab の当たり判定は v6 なので、
  置いた分だけ v5 の並び (mass_ / isGravity_ / emotionType_ / constraints_ を足す。全て Static) に戻す。
  シーン全体を v6 に上げないのは、序章には動く当たり判定 (木箱など) もあるから。
- 何度実行しても IslandProps は1組だけになる。
"""
import math
import re
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
from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, first_versions, rotation, walk  # noqa: E402

SCENES = {
    'prologue': REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene',
    'main': REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene',
}
PREFAB = REPO / 'Assets' / 'Prefab'
CRATER = REPO / 'Assets' / 'Art' / 'Models' / 'IslandHeart' / 'CoreShardCrater.mv1.meta'
HEIGHT_GRID = REPO / 'Assets' / 'Data' / 'HeightGridMap' / 'FirstTouchDownMainIsland.heightGridMap.meta'
ROOT_NAME = 'IslandProps'
GROUND_LIFT = 0.8

# 島: 地面の高さ (高さマップ)、小物の親 (None はシーン直下の IslandProps)。y を持つ島は高さマップを使わない
ISLANDS = {
    'First': dict(ground=(20.0, 24.0), parent=None),
    'Fountain': dict(ground=(83.0, 86.0), parent='SecondIsland'),
    # NOTE: 家の島の高さマップは家と合っていない (家の下は地面の高さのまま、東の芝生に 217..240 の箱がある) ので y で決め打つ
    'House': dict(ground=None, y=176.0, parent='ThirdIsland'),
    'Cannon': dict(ground=(253.0, 256.0), parent=None),
    'Training': dict(ground=(4.0, 7.0), parent=None),
}

# 通り道 (x, z) の折れ線。当たり判定のある物は PATH_HALF_WIDTH + 物の半径より離す
PATH_HALF_WIDTH = 8.0
PATHS = {
    'First': [
        [(-10.0, 15.0), (-24.0, 130.0)],             # 広場の階段 -> 転移台
        [(-24.0, 138.0), (-88.0, 190.0)],            # 転移台 -> 噴水の島への橋
        [(-24.0, 138.0), (86.0, 190.0)],             # 転移台 -> 訓練の島への橋
        [(-10.0, 15.0), (86.0, 190.0)],              # 広場の階段 -> 訓練の島への橋 (斜めに行く)
        [(-10.0, 15.0), (45.0, 90.0)],               # 広場の階段 -> 屋台
    ],
    'Fountain': [[(-190.0, 375.0), (-277.0, 470.0)], [(-190.0, 375.0), (-205.0, 432.0)]],
    'Cannon': [[(10.0, 405.0), (-90.0, 305.0)]],
    'Training': [[(100.0, 225.0), (100.0, 300.0)], [(100.0, 225.0), (180.0, 235.0)], [(100.0, 225.0), (215.0, 345.0)]],
}
# 当たり判定の大きさ (半径)。無い物 (花・煙・地割れ) は None
RADIUS = {
    'Campfire': 4.0, 'CampfireSmoke': None, 'FallenLog': 8.0, 'DryingRack': 6.0, 'SupplyPile': 6.0,
    'TrailBanner': 2.0, 'TrailSign': 2.0, 'Birch_A': 3.0, 'Birch_B': 3.0, 'Oak_A': 3.0, 'Oak_B': 3.0,
    'Bush_A': 3.0, 'TreeStump': 2.0, 'FlowerPatch_A': None, 'HideTent': 10.0, 'LeanTo': 10.0,
    'RubblePile': 8.0, 'BrokenFence': 8.0, 'BrokenCart': 8.0, 'CrushedHut': 14.0,
}

# (島, グループ, prefab (Prefab/ 以下), x, z, 向き deg, scale)。x, z はワールド座標
# 序章: 襲撃の前のにぎやかな島々
LIVELY = [
    # 拠点の島: 焚き火の集まり場は西の芝生 (酒場の前)。東は訓練の島への道なので縁の木と茂みだけ
    ('First', 'Gathering', 'Prop/Settlement/Campfire', -56.0, 76.0, 0.0, 1.0),
    ('First', 'Gathering', 'Particle/CampfireSmoke', -56.0, 76.0, 0.0, 8.0),
    ('First', 'Gathering', 'Prop/Nature/FallenLog', -68.0, 66.0, 30.0, 1.0),
    ('First', 'Gathering', 'Prop/Nature/FallenLog', -44.0, 90.0, 120.0, 0.95),
    ('First', 'Gathering', 'Prop/Settlement/DryingRack', -68.0, 100.0, 200.0, 1.0),
    ('First', 'Gathering', 'Prop/Settlement/SupplyPile', -40.0, 58.0, 30.0, 0.9),
    ('First', 'Wayfinding', 'Prop/Settlement/TrailBanner', -58.0, 192.0, 30.0, 1.0),
    ('First', 'Wayfinding', 'Prop/Settlement/TrailBanner', 80.0, 144.0, 150.0, 1.0),
    ('First', 'Wayfinding', 'Prop/Settlement/TrailSign', -24.0, 172.0, 60.0, 1.0),
    ('First', 'Greenery', 'Prop/Tree/Birch_A', -104.0, 126.0, 20.0, 1.8),
    ('First', 'Greenery', 'Prop/Tree/Oak_A', -96.0, 150.0, 140.0, 1.6),
    ('First', 'Greenery', 'Prop/Tree/Birch_B', 74.0, 126.0, 260.0, 1.7),
    ('First', 'Greenery', 'Prop/Nature/Bush_A', -114.0, 112.0, 0.0, 1.2),
    ('First', 'Greenery', 'Prop/Nature/Bush_A', -84.0, 164.0, 70.0, 1.1),
    ('First', 'Greenery', 'Prop/Nature/Bush_A', 82.0, 138.0, 190.0, 1.1),
    ('First', 'Greenery', 'Prop/Nature/Bush_A', -58.0, 180.0, 120.0, 1.0),
    ('First', 'Greenery', 'Prop/Nature/TreeStump', -90.0, 120.0, 0.0, 1.0),
    ('First', 'Flowers', 'Prop/Nature/FlowerPatch_A', -46.0, 40.0, 0.0, 1.1),
    ('First', 'Flowers', 'Prop/Nature/FlowerPatch_A', 22.0, 62.0, 90.0, 1.0),
    ('First', 'Flowers', 'Prop/Nature/FlowerPatch_A', -56.0, 152.0, 200.0, 1.0),
    ('First', 'Flowers', 'Prop/Nature/FlowerPatch_A', 18.0, 112.0, 300.0, 0.9),
    ('First', 'Flowers', 'Prop/Nature/FlowerPatch_A', -30.0, 176.0, 150.0, 1.0),
    ('First', 'Flowers', 'Prop/Nature/FlowerPatch_A', 32.0, 120.0, 60.0, 0.9),
    # 噴水の島: 町の広場。噴水の周りに花、市場の荷、洗濯棚、橋の着く所に旗
    ('Fountain', 'Square', 'Prop/Nature/FlowerPatch_A', -246.0, 478.0, 0.0, 1.2),
    ('Fountain', 'Square', 'Prop/Nature/FlowerPatch_A', -300.0, 512.0, 120.0, 1.1),
    ('Fountain', 'Square', 'Prop/Nature/FlowerPatch_A', -268.0, 520.0, 240.0, 1.0),
    ('Fountain', 'Square', 'Prop/Nature/FlowerPatch_A', -300.0, 450.0, 60.0, 1.0),
    ('Fountain', 'Market', 'Prop/Settlement/SupplyPile', -252.0, 560.0, 200.0, 1.0),
    ('Fountain', 'Market', 'Prop/Settlement/DryingRack', -228.0, 548.0, 160.0, 1.0),
    ('Fountain', 'Wayfinding', 'Prop/Settlement/TrailBanner', -220.0, 388.0, 30.0, 1.0),
    ('Fountain', 'Wayfinding', 'Prop/Settlement/TrailBanner', -176.0, 408.0, 210.0, 1.0),
    ('Fountain', 'Greenery', 'Prop/Tree/Birch_A', -192.0, 540.0, 40.0, 1.7),
    ('Fountain', 'Greenery', 'Prop/Tree/Oak_A', -240.0, 588.0, 200.0, 1.5),
    ('Fountain', 'Greenery', 'Prop/Nature/Bush_A', -182.0, 498.0, 0.0, 1.1),
    ('Fountain', 'Greenery', 'Prop/Nature/Bush_A', -330.0, 470.0, 90.0, 1.1),
    # 家の島: 住まいの島。家が西半分を占めるので、東の芝生と木箱の脇に花と木、洗濯棚と荷物
    ('House', 'Yard', 'Prop/Nature/FlowerPatch_A', 322.0, 688.0, 0.0, 1.1),
    ('House', 'Yard', 'Prop/Nature/FlowerPatch_A', 290.0, 748.0, 90.0, 1.0),
    ('House', 'Yard', 'Prop/Settlement/DryingRack', 334.0, 622.0, 200.0, 1.0),
    ('House', 'Yard', 'Prop/Settlement/SupplyPile', 262.0, 728.0, 30.0, 0.9),
    ('House', 'Greenery', 'Prop/Tree/Birch_A', 346.0, 652.0, 20.0, 1.6),
    ('House', 'Greenery', 'Prop/Tree/Oak_A', 352.0, 700.0, 140.0, 1.5),
    # 大砲の島: 弾薬の荷と旗 (大砲への道は空ける)
    ('Cannon', 'Battery', 'Prop/Settlement/SupplyPile', -60.0, 296.0, 60.0, 1.0),
    ('Cannon', 'Battery', 'Prop/Settlement/TrailBanner', -10.0, 330.0, 200.0, 1.0),
    ('Cannon', 'Battery', 'Prop/Nature/Bush_A', -128.0, 344.0, 0.0, 1.0),
    ('Cannon', 'Battery', 'Prop/Nature/FlowerPatch_A', -30.0, 390.0, 0.0, 1.0),
    # 訓練の島: 旗と荷物 (真ん中の稽古場は空ける。石畳なので草花は置かない)
    ('Training', 'Yard', 'Prop/Settlement/TrailBanner', 138.0, 244.0, 180.0, 1.0),
    ('Training', 'Yard', 'Prop/Settlement/SupplyPile', 150.0, 340.0, 20.0, 0.9),
]

# 第1章: 襲撃のあと。焼け落ちた物、倒れた木、落ちた島から逃げてきた人の仮住まい。花は無い
RUINED = [
    # 瓦礫: 落ちた噴水の島への橋のたもと、壊れた家の脇。まだ燻っている
    ('First', 'Rubble', 'Prop/Settlement/RubblePile', -50.0, 192.0, 40.0, 1.1),
    ('First', 'Rubble', 'Particle/CampfireSmoke', -50.0, 192.0, 0.0, 5.0),
    ('First', 'Rubble', 'Prop/Settlement/BrokenFence', -30.0, 190.0, 0.0, 1.0),
    ('First', 'Rubble', 'Prop/Settlement/BrokenFence', -114.0, 124.0, 90.0, 1.0),
    ('First', 'Rubble', 'Prop/Settlement/RubblePile', 24.0, 28.0, 200.0, 0.9),
    ('First', 'Rubble', 'Particle/CampfireSmoke', 24.0, 28.0, 0.0, 5.0),
    ('First', 'Rubble', 'Prop/Settlement/BrokenCart', -66.0, 116.0, 45.0, 1.0),
    ('First', 'Rubble', 'Prop/Settlement/CrushedHut', -100.0, 142.0, 20.0, 1.0),
    # 倒れた木と切り株。残った木は1本
    ('First', 'Trees', 'Prop/Nature/FallenLog', -72.0, 128.0, 70.0, 1.2),
    ('First', 'Trees', 'Prop/Nature/TreeStump', -90.0, 118.0, 0.0, 1.0),
    ('First', 'Trees', 'Prop/Nature/TreeStump', 70.0, 120.0, 90.0, 1.1),
    ('First', 'Trees', 'Prop/Nature/TreeStump', -96.0, 166.0, 200.0, 0.9),
    ('First', 'Trees', 'Prop/Tree/Birch_B', 74.0, 132.0, 260.0, 1.6),
    ('First', 'Trees', 'Prop/Nature/Bush_A', -114.0, 108.0, 0.0, 1.0),
    ('First', 'Wayfinding', 'Prop/Settlement/TrailSign', -24.0, 172.0, 60.0, 1.0),
    # 噴水の島 (草原のあと雲の下から戻る): 狩人の一族の野営地。赤い旗が一族の目印。一族の家の場所 (-205, 432) は空ける
    ('Fountain', 'ClanCamp', 'Prop/Settlement/HideTent', -252.0, 560.0, 200.0, 1.0),
    ('Fountain', 'ClanCamp', 'Prop/Settlement/HideTent', -222.0, 566.0, 160.0, 1.0),
    ('Fountain', 'ClanCamp', 'Prop/Settlement/Campfire', -240.0, 530.0, 0.0, 1.0),
    ('Fountain', 'ClanCamp', 'Particle/CampfireSmoke', -240.0, 530.0, 0.0, 8.0),
    ('Fountain', 'ClanCamp', 'Prop/Settlement/DryingRack', -206.0, 528.0, 250.0, 1.0),
    ('Fountain', 'ClanCamp', 'Prop/Settlement/TrailBanner', -220.0, 388.0, 30.0, 1.0),
    ('Fountain', 'ClanCamp', 'Prop/Settlement/TrailBanner', -270.0, 540.0, 90.0, 1.0),
    ('Fountain', 'Rubble', 'Prop/Settlement/RubblePile', -318.0, 450.0, 0.0, 1.0),
    ('Fountain', 'Rubble', 'Prop/Settlement/BrokenFence', -330.0, 474.0, 90.0, 1.0),
    ('Fountain', 'Rubble', 'Prop/Nature/TreeStump', -192.0, 540.0, 0.0, 1.0),
    # 大砲の島: 瓦礫と壊れた柵
    ('Cannon', 'Rubble', 'Prop/Settlement/RubblePile', -60.0, 296.0, 60.0, 1.0),
    ('Cannon', 'Rubble', 'Prop/Settlement/BrokenFence', -126.0, 350.0, 90.0, 1.0),
    ('Cannon', 'Rubble', 'Prop/Nature/TreeStump', -10.0, 330.0, 0.0, 1.0),
    # 訓練の島: 稽古は続けている。縁に瓦礫と壊れた柵
    ('Training', 'Rubble', 'Prop/Settlement/RubblePile', 150.0, 340.0, 20.0, 0.9),
    ('Training', 'Rubble', 'Prop/Settlement/BrokenFence', 118.0, 340.0, 0.0, 1.0),
    ('Training', 'Rubble', 'Prop/Nature/TreeStump', 138.0, 244.0, 0.0, 1.0),
]
# 心臓が抜けた跡: (x, y, z, scale)。IslandHeartStones の真上 (序章で竜が爪を立てる所)
HEART_SCAR = (-18.0, 22.0, 72.0, 1.3)
PROPS = {'prologue': (LIVELY, None), 'main': (RUINED, HEART_SCAR)}
# ColliderBase が v5 で保存されているシーン
LEGACY_COLLIDER_SCENES = {'prologue'}


# ---------------------------------------------------------------- 高さと通り道
class HeightGrid:
    """HeightGridMap の .meta (runLen_i / runHeight_i の連長圧縮) を読む"""

    def __init__(self, path):
        text = path.read_text(encoding='utf-8-sig')
        ints = {k: float(v) for k, v in re.findall(r'"(divisions[XZ]_)":\s*([-0-9.e+]+)', text)}
        self.nx, self.nz = int(ints['divisionsX_']), int(ints['divisionsZ_'])
        area = {k: (float(a), float(b)) for k, a, b in re.findall(
            r'"(area(?:Min|Max)_)":\s*\{\s*"value0":\s*([-0-9.e+]+),\s*"value1":\s*([-0-9.e+]+)', text)}
        self.min, self.max = area['areaMin_'], area['areaMax_']
        lens = {int(i): int(n) for i, n in re.findall(r'"runLen_(\d+)":\s*(\d+)', text)}
        heights = {int(i): float(h) for i, h in re.findall(r'"runHeight_(\d+)":\s*([-0-9.e+]+)', text)}
        cells = np.concatenate([np.full(lens[i], heights[i]) for i in range(len(lens))])
        self.cells = cells.reshape(self.nz, self.nx)

    def at(self, x, z):
        i = int((x - self.min[0]) / (self.max[0] - self.min[0]) * self.nx)
        j = int((z - self.min[1]) / (self.max[1] - self.min[1]) * self.nz)
        if not (0 <= i < self.nx and 0 <= j < self.nz):
            return None
        h = self.cells[j, i]
        return None if h < -1e30 else float(h)


def segment_distance(p, a, b):
    ax, az = a
    dx, dz = b[0] - ax, b[1] - az
    t = max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - az) * dz) / (dx * dx + dz * dz)))
    return math.hypot(p[0] - ax - dx * t, p[1] - az - dz * t)


def check_layout(props, grid):
    """地面から外れた物と、通り道をふさぐ物を集める"""
    errors, heights = [], []
    for island, _group, name, x, z, _yaw, scale in props:
        if ISLANDS[island]['ground'] is None:
            heights.append(ISLANDS[island]['y'])
        else:
            lo, hi = ISLANDS[island]['ground']
            h = grid.at(x, z)
            if h is None or not lo <= h <= hi:
                errors.append(f'{island} {name} ({x}, {z}): ground {h} is not in {lo}..{hi}')
            heights.append(h)
        radius = RADIUS[name.split('/')[-1]]
        if radius is None:
            continue
        for a, b in PATHS.get(island, []):
            gap = segment_distance((x, z), a, b) - PATH_HALF_WIDTH - radius * scale
            if gap < 0.0:
                errors.append(f'{island} {name} ({x}, {z}): blocks the path {a} -> {b} by {-gap:.1f}')
    return errors, heights


# ---------------------------------------------------------------- シーン
def to_legacy_collider(node):
    """ColliderBase v6 -> v5 (Static の motion 系を足す)"""
    count = 0
    for n in walk(node):
        for comp in n.components:
            slot = comp.data.get('value0') if hasattr(comp.data, 'get') else None
            body = slot.body if isinstance(slot, Ver) else slot
            if not isinstance(body, OrderedObj) or 'friction_' not in body or 'mass_' in body:
                continue
            legacy = OrderedObj()
            for key in body:
                if key == 'offset_':
                    legacy['mass_'] = Num.of_float(1.0)
                    legacy['isGravity_'] = True
                legacy[key] = body[key]
                if key == 'offsetRotation_':
                    legacy['emotionType_'] = Num.of_int(0)
                if key == 'layer_':
                    legacy['constraints_'] = Num.of_int(0)
            if isinstance(slot, Ver):
                slot.body = legacy
                slot.version = 5
            else:
                comp.data['value0'] = legacy
            count += 1
    return count


def world_trs(node):
    """ルート直下の島の TRS (位置, 回転 xyzw, 一様 scale)"""
    trs = edits._node_local_trs(node)
    rot = tuple(trs.rot)
    # NOTE: 島の回転は (0, 0, 0, 0) で保存されていることがある (エンジンは単位回転として扱う)
    if sum(v * v for v in rot) < 1e-12:
        rot = (0.0, 0.0, 0.0, 1.0)
    return np.array(trs.pos), rot, trs.scale[0]


def to_local(parent, pos, rot, scale):
    """ワールドの (位置, 回転, scale) を親の中の値にする"""
    if parent is None:
        return pos, rot, scale
    p_pos, p_rot, p_scale = parent
    inv = (-p_rot[0], -p_rot[1], -p_rot[2], p_rot[3])
    local = mathutil.quat_rotate(inv, tuple(np.array(pos) - p_pos)) if hasattr(mathutil, 'quat_rotate') \
        else rotate(inv, np.array(pos) - p_pos)
    return tuple(float(v) / p_scale for v in local), mathutil.quat_mul(inv, rot), scale / p_scale


def rotate(q, v):
    x, y, z, w = q
    u = np.array([x, y, z])
    return 2.0 * np.dot(u, v) * u + (w * w - np.dot(u, u)) * v + 2.0 * w * np.cross(u, v)


def build(key, grid):
    path = SCENES[key]
    props, scar = PROPS[key]
    errors, heights = check_layout(props, grid)
    if errors:
        raise SystemExit('\n'.join([f'{path.name}: nothing written'] + errors))

    scene = reader.read_scene_file(path)
    scene.roots = [r for r in scene.roots if r.name != ROOT_NAME]
    roots = {r.name: r for r in scene.roots}
    for island in ISLANDS.values():
        if island['parent'] in roots:
            node = roots[island['parent']]
            node.transform.children = [c for c in node.transform.children if c.name != ROOT_NAME]

    prefabs, groups = {}, {}
    root = edits.add_gameobject(scene, parent=None, name=ROOT_NAME)
    owners = [root]
    for (island, group, name, x, z, yaw, scale), h in zip(props, heights):
        parent_name = ISLANDS[island]['parent']
        if parent_name and parent_name not in roots:
            raise SystemExit(f'{path.name}: no root {parent_name!r} for {island}')
        parent_node = roots[parent_name] if parent_name else None
        if (island, group) not in groups:
            if parent_node:
                holder = next((c for c in parent_node.transform.children if c.name == ROOT_NAME), None)
                if holder is None:
                    holder = edits.add_gameobject(scene, parent=parent_node.guid, name=ROOT_NAME)
                    owners.append(parent_node)
            else:
                holder = next((c for c in root.transform.children if c.name == island), None) \
                    or edits.add_gameobject(scene, parent=root.guid, name=island)
            groups[(island, group)] = edits.add_gameobject(scene, parent=holder.guid, name=group)
        if name not in prefabs:
            prefabs[name] = reader.read_prefab_file(PREFAB / f'{name}.prefab')

        pos, rot, s = (x, h + GROUND_LIFT, z), rotation(math.radians(yaw)), scale
        if parent_node:
            pos, rot, s = to_local(world_trs(parent_node), pos, rot, scale)
        node = edits.instantiate_prefab(scene, prefabs[name], parent=groups[(island, group)].guid)
        node.transform.local_pos = edits._vec3_from_floats(tuple(float(v) for v in pos))
        node.transform.local_rot = edits._quat_from_floats(tuple(float(v) for v in rot))
        node.transform.local_scale = edits._vec3_from_floats((float(s),) * 3)

    if scar:
        x, y, z, scale = scar
        node = edits.add_gameobject(scene, parent=root.guid, name='HeartScar')
        node.transform.local_pos = edits._vec3_from_floats((x, y, z))
        node.transform.local_scale = edits._vec3_from_floats((scale,) * 3)
        Builder(scene).component(node, 'ModelRenderer', mv1File_=asset_guid(CRATER))

    for owner in owners:
        if key in LEGACY_COLLIDER_SCENES:
            print(f'  {owner.name}: {to_legacy_collider(owner)} collider(s) written as ColliderBase v5')
        for node in walk(owner):
            for comp in node.components:
                let_writer_place_versions(comp.data)
        bake_world_matrices(owner)

    text = writer.write_scene(scene)
    tree = loads(text)
    indices = [scene.roots.index(o) for o in owners]
    stripper = StrayVersionStripper(catalog_mod.load(), tuple(f'/gameObject_{i}/' for i in indices))
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
    print(f'wrote {path.relative_to(REPO)}  {len(props)} props')


def main():
    keys = sys.argv[1:] or list(SCENES)
    grid = HeightGrid(HEIGHT_GRID)
    for key in keys:
        if key not in SCENES:
            raise SystemExit(f'unknown scene {key!r} (prologue / main)')
        print(f'[{key}]')
        build(key, grid)


if __name__ == '__main__':
    main()
