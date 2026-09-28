"""噴水の島 (SecondIsland) の外周と、そこへ渡る橋 (To SecondIsland Bridge) の両脇に、落ちないための見えない壁を立てる (#207)。

    python tools/art/fountain_island_boundary.py [--dry-run]

- 第1島 / 訓練の島と同じく IslandBoundaryColliders の子に、layer 3 (Boundary: Player と Enemy にだけ当たる) の BoxCollider を並べる。
  GameObject は原点・単位回転に置き、offset_ / offsetRotation_ (度) をワールドの値にする。
- 島が戻る前 (ReturningIsland が島と橋を雲の下へ沈めている間) も壁はその場に残るが、第1島側の出口は
  FountainIsland BridgeGate が塞いでいるので、橋の手すりは第1島の壁の外、島の壁は何もない空中に立つだけで邪魔にならない。
  島や橋の子にすると、戻る演出 (島が傾きながら上がる / 段が跳ね上がる) で壁が振り回されて第1島の Player を押すので置かない。
- 島の外周は FirstTouchDownMainIsland.heightGridMap の地面 (GROUND) のセルから、中心まわりの角度ごとの一番遠いセルで取る。
  橋の手すりの延長線と外周が交わる所で外周を切り、その間 (橋の着地点) は開ける。
- 第1島側は、手すりの端から BridgeGate の両端 (第1島の壁の切れ目) までを短い壁でつなぐ。
- BridgeGate の worldMatrix_ が焼かれていない (単位行列のまま) ので、ここで焼き直す。
- 何度実行しても壁は1組だけになる。
"""
import copy
import math
import sys
import uuid
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model, reader, validate, writer  # noqa: E402

from game_over_prefab import check  # noqa: E402
from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices  # noqa: E402
from main_island_props import HEIGHT_GRID, HeightGrid, segment_distance  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
ROOT_NAME = 'IslandBoundaryColliders'
TEMPLATE_NAME = 'To TrainingIsland Bridge Rail'
GATE_NAME = 'FountainIsland BridgeGate'
WALL_NAME = 'FountainIsland BoundaryWall'
RAIL_NAME = 'To SecondIsland Bridge Rail'

LAYER_BOUNDARY = 3
THICKNESS = 3.0
OVERLAP = 2.0

# 島: 地面と建物の高さ (385 前後は高さマップを焼いた時の古い壁、橋の脇の 230..250 も同じ)、探す範囲、壁の高さ
GROUND = (80.0, 220.0)
BRIDGE_CLEARANCE = 25.0      # 橋の中心線からこれより近いセルは島に数えない
# NOTE: x の上限より東は家の島への橋 (To ThirdIsland Bridge、第1章では落ちたまま) の坂なので島に数えない
ISLAND_AREA = dict(x=(-440.0, -138.0), z=(280.0, 640.0))
ISLAND_CENTER = (-263.0, 476.0)
ANGLE_BINS = 48
EDGE_MARGIN = 3.0            # 一番外のセルの中心から外へ出す量 (セルは 4.15 x 5)
WALL_Y = (70.0, 400.0)

# 橋: 段の当たり判定 (Bridge1..4 の Collider) の上面の中心線。手すりは横へ RAIL_HALF_WIDTH
BRIDGE_START = (-83.6, 181.4)
BRIDGE_END = (-190.4, 368.1)
RAIL_HALF_WIDTH = 19.3
RAIL_Y = (5.0, 400.0)
FIRST_ISLAND_CENTER = (-18.79, 79.41)
FIRST_ISLAND_RADIUS = 125.2


# ---------------------------------------------------------------- 形
def island_outline(grid):
    """地面のセルから、中心まわりの角度ごとに一番遠い点を並べた外周 (閉じた折れ線)"""
    cw = (grid.max[0] - grid.min[0]) / grid.nx
    ch = (grid.max[1] - grid.min[1]) / grid.nz
    far = [None] * ANGLE_BINS
    cx, cz = ISLAND_CENTER
    for j in range(grid.nz):
        z = grid.min[1] + (j + 0.5) * ch
        if not ISLAND_AREA['z'][0] <= z <= ISLAND_AREA['z'][1]:
            continue
        for i in range(grid.nx):
            x = grid.min[0] + (i + 0.5) * cw
            if not ISLAND_AREA['x'][0] <= x <= ISLAND_AREA['x'][1]:
                continue
            if not GROUND[0] <= grid.cells[j, i] <= GROUND[1]:
                continue
            if segment_distance((x, z), BRIDGE_START, BRIDGE_END) < BRIDGE_CLEARANCE:
                continue
            a = math.atan2(z - cz, x - cx) % (2.0 * math.pi)
            k = int(a / (2.0 * math.pi) * ANGLE_BINS) % ANGLE_BINS
            r = math.hypot(x - cx, z - cz)
            if far[k] is None or r > far[k][0]:
                far[k] = (r, a)
    if any(f is None for f in far):
        raise SystemExit('island outline: an angle bin has no ground cell - nothing written')
    return [np.array([cx + (r + EDGE_MARGIN) * math.cos(a), cz + (r + EDGE_MARGIN) * math.sin(a)]) for r, a in far]


def cross2(a, b):
    return a[0] * b[1] - a[1] * b[0]


def hit_outline(outline, origin, direction):
    """origin から direction へ伸ばした線が外周と最初に交わる (辺の番号, 点)"""
    best = None
    n = len(outline)
    for i in range(n):
        p, q = outline[i], outline[(i + 1) % n]
        e = q - p
        denom = cross2(direction, e)
        if abs(denom) < 1e-9:
            continue
        t = cross2(p - origin, e) / denom
        u = cross2(p - origin, direction) / denom
        if t > 0.0 and 0.0 <= u <= 1.0 and (best is None or t < best[0]):
            best = (t, i, origin + direction * t)
    if best is None:
        raise SystemExit('bridge rail does not reach the island outline - nothing written')
    return best[1], best[2]


def boxes_along(points, y_range):
    """折れ線の各辺に1枚ずつ (中心, 向きの度, 長さ, 高さの範囲)"""
    return [segment_box(a, b, y_range) for a, b in zip(points, points[1:])]


def segment_box(a, b, y_range):
    d = b - a
    length = float(np.hypot(*d))
    yaw = math.degrees(math.atan2(-d[1], d[0]))
    return (a + b) * 0.5, yaw, length + OVERLAP, y_range


def build_layout(grid, gate_ends):
    start, end = np.array(BRIDGE_START), np.array(BRIDGE_END)
    forward = (end - start) / np.hypot(*(end - start))
    side = np.array([-forward[1], forward[0]])
    outline = island_outline(grid)

    rails, cuts, links = [], [], []
    for sign in (1.0, -1.0):
        origin = start + side * RAIL_HALF_WIDTH * sign
        # 第1島の壁 (円) の上から始める
        rel = origin - np.array(FIRST_ISLAND_CENTER)
        b = float(np.dot(rel, forward))
        c = float(np.dot(rel, rel)) - FIRST_ISLAND_RADIUS ** 2
        t = -b + math.sqrt(b * b - c)
        rail_start = origin + forward * t
        edge, rail_end = hit_outline(outline, rail_start, forward)
        rails.append(segment_box(rail_start, rail_end, RAIL_Y))
        cuts.append((edge, rail_end))
        gate_end = min(gate_ends, key=lambda p: np.hypot(*(p - rail_start)))
        links.append((rail_start, gate_end))

    # 外周を手すりとの交点で切って、橋のない側を回る
    n = len(outline)
    (ea, pa), (eb, pb) = cuts
    around_a = [pa] + [outline[(ea + 1 + k) % n] for k in range((eb - ea) % n)] + [pb]
    around_b = [pb] + [outline[(eb + 1 + k) % n] for k in range((ea - eb) % n)] + [pa]
    wall = max(around_a, around_b, key=len)
    walls = boxes_along(wall, WALL_Y)
    walls += [segment_box(a, b, RAIL_Y) for a, b in links if np.hypot(*(b - a)) > 0.5]
    return walls, rails, wall, links


# ---------------------------------------------------------------- シーン
def set_vec3(obj, values):
    for i, v in enumerate(values):
        obj[f'value{i}'] = Num.of_float(float(v))


def collider_body(comp):
    return comp.data['value0']


def box_component(template, box):
    center, yaw, length, (y0, y1) = box
    comp = copy.deepcopy(template)
    model.set_component_guid(comp, str(uuid.uuid4()).upper())
    body = collider_body(comp)
    set_vec3(body['offset_'], (center[0], (y0 + y1) * 0.5, center[1]))
    set_vec3(body['offsetRotation_'], (0.0, yaw, 0.0))
    body['layer_'] = Num.of_int(LAYER_BOUNDARY)
    set_vec3(comp.data['size_'], (length, y1 - y0, THICKNESS))
    return comp


def gate_ends(gate):
    """BridgeGate の壁の両端 (第1島の壁の切れ目) をワールドの (x, z) で"""
    x, _, z = edits._vec3_floats(gate.transform.local_pos)
    pos = np.array([x, z])
    ends = []
    for comp in gate.components:
        body = collider_body(comp)
        center = pos + np.array([edits._f(body['offset_']['value0']), edits._f(body['offset_']['value2'])])
        yaw = math.radians(edits._f(body['offsetRotation_']['value1']))
        half = edits._f(comp.data['size_']['value0']) * 0.5
        d = np.array([math.cos(yaw), -math.sin(yaw)])
        ends += [center - d * half, center + d * half]
    # NOTE: 壁は少し重ねて並んでいるので、第1島の中心から見て両外側の2点を取る
    center = np.array(FIRST_ISLAND_CENTER)
    angle = lambda p: math.atan2(p[1] - center[1], p[0] - center[0])
    return [min(ends, key=angle), max(ends, key=angle)]


def main():
    dry_run = '--dry-run' in sys.argv[1:]
    scene = reader.read_scene_file(SCENE)
    roots = {r.name: r for r in scene.roots}
    if ROOT_NAME not in roots:
        raise SystemExit(f'{SCENE.name}: no root {ROOT_NAME!r}')
    root = roots[ROOT_NAME]
    root.transform.children = [c for c in root.transform.children if c.name not in (WALL_NAME, RAIL_NAME)]
    children = {c.name: c for c in root.transform.children}
    template = children[TEMPLATE_NAME].components[0]
    gate = children[GATE_NAME]

    ends = gate_ends(gate)
    if len(ends) != 2:
        raise SystemExit(f'{GATE_NAME}: expected 2 open ends, got {len(ends)} - nothing written')
    walls, rails, outline, links = build_layout(HeightGrid(HEIGHT_GRID), ends)

    print(f'island cuts: {[tuple(round(float(v), 1) for v in outline[i]) for i in (0, -1)]}')
    print(f'gate ends: {[tuple(round(float(v), 1) for v in p) for p in ends]}')
    print(f'rail links: {[(tuple(round(float(v), 1) for v in a), round(float(np.hypot(*(b - a))), 1)) for a, b in links]}')
    for label, boxes in (('wall', walls), ('rail', rails)):
        for center, yaw, length, y in boxes:
            print(f'  {label} center=({center[0]:.1f}, {center[1]:.1f}) yaw={yaw:.1f} length={length:.1f} y={y}')
    if dry_run:
        return

    for name, boxes in ((WALL_NAME, walls), (RAIL_NAME, rails)):
        node = edits.add_gameobject(scene, parent=root.guid, name=name)
        node.components = [box_component(template, box) for box in boxes]
    bake_world_matrices(root)

    text = writer.write_scene(scene)
    tree = loads(text)
    stripper = StrayVersionStripper(catalog_mod.load(), (f'/gameObject_{scene.roots.index(root)}/',))
    stripper.run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  {len(walls)} wall + {len(rails)} rail collider(s)')


if __name__ == '__main__':
    main()
