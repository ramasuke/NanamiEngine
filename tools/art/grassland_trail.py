"""GrassLandScene に、スポーン地点から山の棚の野営地 (狩人の一族) へ導く道しるべを置く。

    python tools/art/settlement_prefabs.py TrailBanner TrailCairn TrailSign TrailStones   # 先にプレハブ
    python tools/art/campfire_smoke_fx.py --build-dir <tmp> --install                      # 焚き火の炎と煙
    python tools/art/grassland_trail.py [--plan <png>]                                      # シーンの Wayfinding ルートを置き直す
    python tools/art/grassland_heightgrid_bake.py                                          # 当たり判定が増えたので焼き直す

スポーン地点は北西の低地で、野営地は東の棚 (高さ ~186) の上。そこへ上れるのは北の壁沿いの坂だけで、
何もしないとプレイヤーは南の盆地 (ハイエナ・大顎) へ降りていく。そこで:
- 道筋は地形から探す (傾き 20 度までで歩ける最短路を均したもの)。スポーンの東の分かれ道に立て札
  (赤い布の板 = 野営地、焼けた板 = 村の跡)、道沿いに一族の赤い旗竿、その間に石積み、道の真ん中に踏み石を並べる。
- 道の上と両脇の Nature (木・岩・茂み・花・倒木) を取り除き、草を抜いて踏み跡に見せる。
- 野営地の焚き火に炎と煙の柱 (CampfireSmoke) を付けて、遠くからでも場所が分かるようにする。
- 野営地から盆地へ降りる道 (見張り台の脇) にも旗と石積みを置く。盆地から見上げても野営地が分かる。
- プレイヤーのスポーン地点 (PlayerSpawn Pos) を道の方へ向ける (GrassLandScene がマーカーの向きで出す)。
- 何度実行しても Wayfinding ルートは1組だけになる。Nature から抜いた物・抜いた草は戻らない
  (戻すなら grassland_nature_scatter / settlement_scatter をやり直す)。
"""
import argparse
import base64
import heapq
import math
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, mathutil, reader, validate, writer  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import (SCENE, StrayVersionStripper, Terrain, bake_world_matrices, first_versions,  # noqa: E402
                                      quat_axis, walk)

PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Settlement'
SMOKE_PREFAB = REPO / 'Assets' / 'Prefab' / 'Particle' / 'CampfireSmoke.prefab'
GRASS_META = REPO / 'Assets' / 'Data' / 'GrassField' / 'GrassLandGrass.grassField.meta'
ROOT_NAME = 'Wayfinding'
SPAWN_NAME = 'PlayerSpawn Pos'

# 道筋を探す範囲 (壁の内側) と、歩ける傾き
GRID_MIN, GRID_MAX, GRID_STEP = 250.0, 1250.0, 5.0
CAMP_ROUTE = ((430.0, 1200.0), (955.0, 1172.0), 20.0)   # スポーンの東 -> 野営地の西の端
DOWN_ROUTE = ((992.0, 1095.0), (975.0, 800.0), 25.0)    # 見張り台の脇 -> 盆地

SIGN_AT = 455.0                        # 立て札を置く道の x
SIGN_SIDE = 9.0                        # 道の右 (南、盆地側) へずらす量
# 旗竿: 道のこの点に一番近い所から、左右 (+ 左 / - 右) へずらす。旗は手前 (一つ前の旗) を向く
BANNERS = [((500.0, 1200.0), 10.0), ((635.0, 1195.0), -10.0), ((735.0, 1145.0), -11.0),
           ((795.0, 1215.0), 10.0), ((865.0, 1222.0), 10.0)]
DOWN_BANNER = ((995.0, 1070.0), 9.0)   # 野営地から降りる坂の上。盆地を向く
CAIRN_EVERY, CAIRN_SIDE, CAIRN_CLEAR = 40.0, 6.5, 18.0
DOWN_CAIRN_EVERY = 45.0
STONES_EVERY = 16.0
PATH_CLEAR = {'Trees': 9.0, 'Rocks': 9.0}   # 道の中心線からこの距離の Nature を抜く (他の種類は PATH_CLEAR_OTHER)
PATH_CLEAR_OTHER = 5.0
PROP_CLEAR = 6.0                            # 置いた物からこの距離の Nature を抜く
GRASS_CLEAR, GRASS_THIN = 4.5, 7.5          # 草を全部抜く幅 / 半分抜く幅 (中心線から)

# 名前: 足元を沈める量 world
SINK = {'TrailBanner': 0.6, 'TrailCairn': 0.5, 'TrailSign': 0.6, 'TrailStones': 0.9, 'CampfireSmoke': 0.0}


# ---------------------------------------------------------------- 道筋
def find_route(t, start, goal, limit_deg):
    n = int((GRID_MAX - GRID_MIN) / GRID_STEP) + 1
    xs = GRID_MIN + np.arange(n) * GRID_STEP
    gx, gz = np.meshgrid(xs, xs)
    h = t.height(gx, gz)

    def idx(p):
        return int(round((p[1] - GRID_MIN) / GRID_STEP)), int(round((p[0] - GRID_MIN) / GRID_STEP))

    s, g = idx(start), idx(goal)
    dist, prev, pq = {s: 0.0}, {}, [(0.0, s)]
    tan_limit = math.tan(math.radians(limit_deg))
    while pq:
        d, u = heapq.heappop(pq)
        if u == g:
            break
        if d > dist[u]:
            continue
        for dj in (-1, 0, 1):
            for di in (-1, 0, 1):
                v = (u[0] + dj, u[1] + di)
                if (dj == 0 and di == 0) or not (0 <= v[0] < n and 0 <= v[1] < n):
                    continue
                run = GRID_STEP * math.hypot(dj, di)
                rise = h[v] - h[u]
                if abs(rise) > run * tan_limit:
                    continue
                nd = d + math.hypot(run, rise)
                if nd < dist.get(v, math.inf):
                    dist[v], prev[v] = nd, u
                    heapq.heappush(pq, (nd, v))
    if g not in dist:
        raise SystemExit(f'no walkable route from {start} to {goal} at {limit_deg} deg')
    cells = [g]
    while cells[-1] != s:
        cells.append(prev[cells[-1]])
    cells.reverse()
    pts = np.array([(xs[i], xs[j]) for j, i in cells])
    return smooth(pts)


def resample(pts, step):
    seg = np.hypot(*np.diff(pts, axis=0).T)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    at = np.arange(0.0, s[-1], step)
    return np.stack([np.interp(at, s, pts[:, 0]), np.interp(at, s, pts[:, 1])], axis=1)


def smooth(pts, passes=6):
    """格子の階段を均す。端は動かさない"""
    pts = resample(pts, 4.0)
    for _ in range(passes):
        inner = (pts[:-2] + pts[1:-1] * 2 + pts[2:]) / 4
        pts = np.concatenate([pts[:1], inner, pts[-1:]])
    return resample(pts, 2.0)


class Path2D:
    def __init__(self, pts):
        self.pts = pts
        self.s = np.concatenate([[0.0], np.cumsum(np.hypot(*np.diff(pts, axis=0).T))])
        self.length = float(self.s[-1])

    def at(self, s):
        x = float(np.interp(s, self.s, self.pts[:, 0]))
        z = float(np.interp(s, self.s, self.pts[:, 1]))
        a, b = self.at_raw(s - 3.0), self.at_raw(s + 3.0)
        dx, dz = b[0] - a[0], b[1] - a[1]
        d = math.hypot(dx, dz) or 1.0
        return x, z, dx / d, dz / d

    def at_raw(self, s):
        s = min(max(s, 0.0), self.length)
        return float(np.interp(s, self.s, self.pts[:, 0])), float(np.interp(s, self.s, self.pts[:, 1]))

    def nearest_s(self, p):
        d = np.hypot(self.pts[:, 0] - p[0], self.pts[:, 1] - p[1])
        return float(self.s[int(np.argmin(d))])

    def distance(self, x, z):
        return float(np.min(np.hypot(self.pts[:, 0] - x, self.pts[:, 1] - z)))

    def distances(self, x, z):
        """点列 (x, z) それぞれの中心線までの距離"""
        out = np.full(len(x), np.inf)
        for px, pz in self.pts[::2]:
            out = np.minimum(out, np.hypot(x - px, z - pz))
        return out


def left_of(dx, dz):
    """進む向き (dx, dz) の左。x 東、z 北で見た左"""
    return -dz, dx


# ---------------------------------------------------------------- 配置
class Placement:
    __slots__ = ('prefab', 'x', 'z', 'rot', 'y')

    def __init__(self, prefab, x, z, rot):
        self.prefab, self.x, self.z, self.rot, self.y = prefab, x, z, rot, 0.0


def yaw_x_along(dx, dz):
    """ローカル +X を (dx, dz) へ向ける Y 回転 (+X -> (cos a, -sin a))"""
    return quat_axis((0.0, 1.0, 0.0), math.atan2(-dz, dx))


def yaw_z_toward(x, z, tx, tz):
    """ローカル +Z を (tx, tz) の方へ向ける Y 回転 (+Z -> (sin a, cos a))"""
    return quat_axis((0.0, 1.0, 0.0), math.atan2(tx - x, tz - z))


def tilt_to_ground(t, x, z, q):
    """踏み石を斜面に寝かせる: 上 (0,1,0) を地面の法線へ回す回転を yaw の前に掛ける"""
    gx, gz = t.gradient(x, z, 4.0)
    n = np.array([-float(gx), 1.0, -float(gz)])
    n /= np.linalg.norm(n)
    angle = math.acos(max(-1.0, min(1.0, n[1])))
    if angle < 1e-4:
        return q
    axis = np.cross([0.0, 1.0, 0.0], n)
    axis /= np.linalg.norm(axis)
    return mathutil.quat_mul(quat_axis(tuple(axis), angle), q)


def plan(t):
    main = Path2D(find_route(t, *CAMP_ROUTE))
    down = Path2D(find_route(t, *DOWN_ROUTE))
    out = []

    # 分かれ道の立て札: 板の +X が道の先 (野営地)
    s = main.nearest_s((SIGN_AT, CAMP_ROUTE[0][1]))
    x, z, dx, dz = main.at(s)
    lx, lz = left_of(dx, dz)
    sign = Placement('TrailSign', x - lx * SIGN_SIDE, z - lz * SIGN_SIDE, yaw_x_along(dx, dz))
    out.append(sign)
    anchors = [(sign.x, sign.z)]

    # 旗竿: 一つ前の旗 (最初は立て札) の方へ旗を向ける
    prev = (sign.x, sign.z)
    for target, side in BANNERS:
        x, z, dx, dz = main.at(main.nearest_s(target))
        lx, lz = left_of(dx, dz)
        bx, bz = x + lx * side, z + lz * side
        out.append(Placement('TrailBanner', bx, bz, yaw_z_toward(bx, bz, *prev)))
        anchors.append((bx, bz))
        prev = (bx, bz)

    # 石積み: 旗・立て札から離れた所に、左右交互に
    side = 1.0
    s = CAIRN_EVERY / 2
    while s < main.length - 10.0:
        x, z, dx, dz = main.at(s)
        if all(math.hypot(x - ax, z - az) > CAIRN_CLEAR for ax, az in anchors):
            lx, lz = left_of(dx, dz)
            cx, cz = x + lx * CAIRN_SIDE * side, z + lz * CAIRN_SIDE * side
            out.append(Placement('TrailCairn', cx, cz, yaw_z_toward(cx, cz, *main.at_raw(s - 20.0))))
            side = -side
        s += CAIRN_EVERY

    # 踏み石: 道の真ん中。並ぶ向きを道に合わせて斜面に寝かせる
    s = 20.0
    while s < main.length - 4.0:
        x, z, dx, dz = main.at(s)
        if math.hypot(x - sign.x, z - sign.z) > 8.0:
            out.append(Placement('TrailStones', x, z, tilt_to_ground(t, x, z, yaw_x_along(dx, dz))))
        s += STONES_EVERY

    # 野営地から盆地へ降りる道: 坂の上に旗 (盆地の方を向く) と、坂を下る石積み
    x, z, dx, dz = down.at(down.nearest_s(DOWN_BANNER[0]))
    lx, lz = left_of(dx, dz)
    bx, bz = x + lx * DOWN_BANNER[1], z + lz * DOWN_BANNER[1]
    out.append(Placement('TrailBanner', bx, bz, yaw_z_toward(bx, bz, *DOWN_ROUTE[1])))
    side = -1.0
    s = DOWN_CAIRN_EVERY
    while s < down.length - 10.0:
        x, z, dx, dz = down.at(s)
        if math.hypot(x - bx, z - bz) > CAIRN_CLEAR:
            lx, lz = left_of(dx, dz)
            cx, cz = x + lx * CAIRN_SIDE * side, z + lz * CAIRN_SIDE * side
            out.append(Placement('TrailCairn', cx, cz, yaw_z_toward(cx, cz, *down.at_raw(s + 20.0))))
            side = -side
        s += DOWN_CAIRN_EVERY

    for p in out:
        if p.prefab == 'TrailStones':
            p.y = float(t.height(p.x, p.z)) - SINK[p.prefab]
        else:
            p.y = t.lowest(p.x, p.z, 3.0) - SINK[p.prefab]
    counts = {}
    for p in out:
        counts[p.prefab] = counts.get(p.prefab, 0) + 1
    print(f'  route to camp {main.length:.0f}, down to the basin {down.length:.0f}; placed {counts}')
    return main, down, out


# ---------------------------------------------------------------- Nature / 草
def remove_nature(scene, main, placements):
    nature = next((r for r in scene.roots if r.name == 'Nature'), None)
    if nature is None:
        return
    solid = [p for p in placements if p.prefab != 'TrailStones']
    removed = {}
    for group in nature.transform.children:
        if group.name == 'Grass':
            continue
        keep = []
        for obj in group.transform.children:
            x, _, z = edits._node_local_trs(obj).pos
            on_path = main.distance(x, z) < PATH_CLEAR.get(group.name, PATH_CLEAR_OTHER)
            near_prop = any(math.hypot(x - p.x, z - p.z) < PROP_CLEAR for p in solid)
            if on_path or near_prop:
                removed[group.name] = removed.get(group.name, 0) + 1
            else:
                keep.append(obj)
        group.transform.children = keep
    print(f'  removed from Nature: {removed}')


def clear_grass(main):
    root = loads(GRASS_META.read_bytes().decode('utf-8-sig'))
    body = root['value0']['ptr_wrapper']['data']
    chunk = float(body['chunkSize_'].value)
    x_min, z_min = main.pts.min(axis=0) - GRASS_THIN - chunk
    x_max, z_max = main.pts.max(axis=0) + GRASS_THIN
    rng = np.random.default_rng(1207)
    total = removed = 0
    for rec in body['chunks']:
        cx, cz = int(rec['cx'].value), int(rec['cz'].value)
        x0, z0 = cx * chunk, cz * chunk
        count = int(rec['count'].value)
        total += count
        if not (x_min <= x0 <= x_max and z_min <= z0 <= z_max) or count == 0:
            continue
        raw = np.frombuffer(base64.b64decode(rec['blades']), dtype='<u2').reshape(-1, 3)
        x = (raw[:, 0] / 65535.0 + cx) * chunk
        z = (raw[:, 1] / 65535.0 + cz) * chunk
        d = main.distances(x, z)
        drop = (d < GRASS_CLEAR) | ((d < GRASS_THIN) & (rng.random(len(d)) < 0.5))
        if not drop.any():
            continue
        removed += int(drop.sum())
        kept = raw[~drop]
        rec['count'] = Num.of_int(len(kept))
        rec['blades'] = base64.b64encode(kept.astype('<u2').tobytes()).decode('ascii')
    GRASS_META.write_bytes(to_file_bytes(dumps(root)))
    print(f'  grass: removed {removed} of {total} blades along the route')


# ---------------------------------------------------------------- シーン
def find_node(scene, name, under=None):
    roots = scene.roots if under is None else [r for r in scene.roots if r.name == under]
    for root in roots:
        for node in walk(root):
            if node.name == name:
                return node
    return None


def face_spawn_to_route(scene, main):
    spawn = find_node(scene, SPAWN_NAME)
    if spawn is None:
        raise SystemExit(f'{SPAWN_NAME} not found')
    sx, _, sz = edits._node_local_trs(spawn).pos
    tx, tz = main.at_raw(main.nearest_s((SIGN_AT + 45.0, CAMP_ROUTE[0][1])))
    # プレイヤーは -Z を向いて出る: R_y(a) (0,0,-1) = (-sin a, 0, -cos a)
    angle = math.atan2(-(tx - sx), -(tz - sz))
    spawn.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), angle))
    bake_world_matrices(spawn)
    print(f'  {SPAWN_NAME}: faces ({tx:.0f}, {tz:.0f}), yaw {math.degrees(angle):.1f}')


def build_scene(main, placements):
    scene = reader.read_scene_file(SCENE)
    scene.roots = [r for r in scene.roots if r.name != ROOT_NAME]
    remove_nature(scene, main, placements)
    face_spawn_to_route(scene, main)

    campfire = find_node(scene, 'Campfire', under='Settlement')
    if campfire is None:
        raise SystemExit('Settlement/Camp/Campfire not found')
    fire_pos = edits._node_local_trs(campfire).pos

    root = edits.add_gameobject(scene, parent=None, name=ROOT_NAME)
    groups = {name: edits.add_gameobject(scene, parent=root.guid, name=name)
              for name in ('Signs', 'Banners', 'Cairns', 'Stones', 'Campfire')}
    group_of = {'TrailSign': 'Signs', 'TrailBanner': 'Banners', 'TrailCairn': 'Cairns', 'TrailStones': 'Stones'}
    prefabs = {}
    for p in placements:
        if p.prefab not in prefabs:
            prefabs[p.prefab] = reader.read_prefab_file(PREFAB / f'{p.prefab}.prefab')
        node = edits.instantiate_prefab(scene, prefabs[p.prefab], parent=groups[group_of[p.prefab]].guid)
        node.transform.local_pos = edits._vec3_from_floats((p.x, p.y, p.z))
        node.transform.local_rot = edits._quat_from_floats(p.rot)
    smoke = edits.instantiate_prefab(scene, reader.read_prefab_file(SMOKE_PREFAB), parent=groups['Campfire'].guid)
    smoke.transform.local_pos = edits._vec3_from_floats(tuple(fire_pos))
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
    for type_key, want in (('ColliderBase', 6), ('NanamiEngine::Module::Component::BoxCollider', 6),
                           ('NanamiEngine::Module::Component::ModelRenderer', 5)):
        if versions.get(type_key) != want:
            raise SystemExit(f'{type_key}: first occurrence is v{versions.get(type_key)}, expected v{want} - nothing written')
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({len(placements) + 1} objects under {ROOT_NAME})')


def draw_plan(t, main, down, placements, out):
    from PIL import Image, ImageDraw
    n = 1000
    xs = np.linspace(GRID_MIN, GRID_MAX, n)
    gx, gz = np.meshgrid(xs, xs)
    h = t.height(gx, gz)
    g = ((h - h.min()) / (np.ptp(h) or 1) * 180 + 50).astype(np.uint8)
    img = np.stack([g, g, g], -1)
    img[t.slope(gx, gz) > 40] = [170, 70, 60]
    im = Image.fromarray(img[::-1])
    d = ImageDraw.Draw(im)

    def px(x, z):
        return ((x - GRID_MIN) / (GRID_MAX - GRID_MIN) * n, (GRID_MAX - z) / (GRID_MAX - GRID_MIN) * n)

    for path, col in ((main, (0, 220, 255)), (down, (255, 255, 255))):
        d.line([px(x, z) for x, z in path.pts], fill=col, width=2)
    style = {'TrailSign': ((255, 255, 0), 6), 'TrailBanner': ((255, 40, 40), 6), 'TrailCairn': ((255, 150, 60), 3),
             'TrailStones': ((120, 120, 120), 2)}
    for p in placements:
        col, r = style[p.prefab]
        cx, cz = px(p.x, p.z)
        d.ellipse([cx - r, cz - r, cx + r, cz + r], fill=col)
    im.save(out)
    print(f'  plan image: {out}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--plan', help='配置案を png に描くだけでシーンは触らない')
    ap.add_argument('--no-grass', action='store_true', help='草を抜かない')
    args = ap.parse_args()

    t = Terrain()
    main_path, down_path, placements = plan(t)
    if args.plan:
        draw_plan(t, main_path, down_path, placements, args.plan)
        return
    build_scene(main_path, placements)
    if not args.no_grass:
        clear_grass(main_path)


if __name__ == '__main__':
    main()
