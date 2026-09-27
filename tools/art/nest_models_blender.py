"""古竜の巣のモデル (地形・岩の爪・嵐の壁・竜撃ちの銛) を組み、FBX に書き出す (Blender を -b で動かす)。

    blender -b --factory-startup --python tools/art/nest_models_blender.py -- <work> [Name ...]
    python -m tools.model convert <work>/fbx/<Name>.fbx <out>.mv1 --mode mesh --with-textures

- 単位は m (world = m x 8)。テクスチャは nest_terrain.py が <work>/tex に作ったものを FBX の横へ写す。
- NestTerrain: Blender の (X, Y, Z) = world の (x, z, h) / 8。中心から放射状の格子で、島の縁 (nest_terrain.edge_radius) の外は
  崖になって島の底 (逆さの円錐) へ続く、閉じたメッシュ。シーンでは scale 0.08 で置けば world に戻る。
- NestSpireA/B/C: 外輪に立てる岩の爪。原点は根元の中心、+Y へ反る (巣の中心へ向けて置く)。根元は地面へ沈める分だけ下へ伸ばしてある。
- NestStormWall: 嵐の壁。内向きの筒と、底の渦の雲。原点は島の中心。
- NestHarpoon: 竜撃ちの銛。原点は穂先、柄は +Z へ伸びる (骨や地面に斜めに刺して置く)。
"""
import math
import os
import shutil
import sys

import bmesh
import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import nest_terrain as nt  # noqa: E402

ARGS = sys.argv[sys.argv.index('--') + 1:]
WORK = ARGS[0]
ONLY = set(ARGS[1:])
FBX_DIR = os.path.join(WORK, 'fbx')
TEX_DIR = os.path.join(WORK, 'tex')
REPO_TEX = os.path.join(os.path.dirname(os.path.dirname(HERE)), 'Assets', 'Art', 'Models', 'Settlement')
TO_M = 1.0 / 8.0


def clear():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    for coll in (bpy.data.meshes, bpy.data.materials, bpy.data.images):
        for b in list(coll):
            if b.users == 0:
                coll.remove(b)


def material(name, tex):
    src = os.path.join(TEX_DIR, tex) if os.path.exists(os.path.join(TEX_DIR, tex)) else os.path.join(REPO_TEX, tex)
    shutil.copy2(src, os.path.join(FBX_DIR, tex))
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = next(nd for nd in m.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
    t = m.node_tree.nodes.new('ShaderNodeTexImage')
    t.image = bpy.data.images.load(os.path.join(FBX_DIR, tex))
    m.node_tree.links.new(t.outputs['Color'], bsdf.inputs['Base Color'])
    return m


def flip_normals_for_dxlib(me):
    """DxLibModelViewer は Blender の FBX の法線の上下 (Z) を反転して読むので、先に反転しておく (desert_terrain_blender.py と同じ)"""
    me.update()
    normals = [(n.vector.x, n.vector.y, -n.vector.z) for n in me.corner_normals]
    me.normals_split_custom_set(normals)


def finish(name, verts, faces, face_mat, face_uv, mats, smooth=True, outward=None):
    """face_uv: 面ごとの [(u, v)] (頂点の並びと同じ)。outward: 面の中心 -> 外向きのベクトル (巻きを合わせる)"""
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    me.update()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    for m in mats:
        me.materials.append(m)
    uv = me.uv_layers.new(name='UVMap')
    for poly, mi, uvs in zip(me.polygons, face_mat, face_uv):
        poly.material_index = mi
        poly.use_smooth = smooth
        for li, (u, v) in zip(poly.loop_indices, uvs):
            uv.data[li].uv = (u, v)
    if outward is not None:
        bm = bmesh.new()
        bm.from_mesh(me)
        bm.faces.ensure_lookup_table()
        flips = [f for f in bm.faces if f.normal.dot(outward(f.calc_center_median())) < 0]
        bmesh.ops.reverse_faces(bm, faces=flips)
        bm.to_mesh(me)
        bm.free()
    flip_normals_for_dxlib(me)
    export(obj)
    tris = sum(len(p.vertices) - 2 for p in me.polygons)
    print(f'MODEL {name} verts={len(verts)} tris={tris}')
    return obj


def export(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    units = bpy.context.scene.unit_settings
    units.system, units.scale_length, units.length_unit = 'METRIC', 1.0, 'METERS'
    bpy.ops.export_scene.fbx(filepath=os.path.join(FBX_DIR, f'{obj.name}.fbx'), use_selection=True,
                             object_types={'MESH'}, path_mode='STRIP', embed_textures=False,
                             mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)


# ---------------------------------------------------------------- 地形
class Height:
    def __init__(self):
        d = np.load(nt.NPZ)
        self.h = d['height'].astype(np.float64)
        self.mat = d['material']
        self.n = self.h.shape[0]
        self.step = nt.SIZE / (self.n - 1)

    def at(self, x, z):
        fx = min(max(x / self.step, 0.0), self.n - 1.0001)
        fz = min(max(z / self.step, 0.0), self.n - 1.0001)
        i0, j0 = int(fx), int(fz)
        tx, tz = fx - i0, fz - j0
        g = self.h
        return float((g[j0, i0] * (1 - tx) + g[j0, i0 + 1] * tx) * (1 - tz)
                     + (g[j0 + 1, i0] * (1 - tx) + g[j0 + 1, i0 + 1] * tx) * tz)

    def material(self, x, z):
        i = min(max(int(x / self.step), 0), self.n - 2)
        j = min(max(int(z / self.step), 0), self.n - 2)
        return int(self.mat[j, i])


# 島の底: 縁からの (深さ world, 半径の割合)。縁のすぐ下は切り立った崖
UNDERSIDE = [(-10, 1.005), (-45, 0.99), (-100, 0.95), (-170, 0.88), (-260, 0.79), (-370, 0.67), (-500, 0.53),
             (-640, 0.39), (-780, 0.25), (-900, 0.13), (-990, 0.04)]
TIP_DEPTH = -1040.0


def build_terrain():
    hm = Height()
    A = 480
    ring_step = 6.0
    rings = int(nt.EDGE_RADIUS / ring_step)
    rng = np.random.default_rng(11)
    thetas = np.linspace(0, 2 * math.pi, A, endpoint=False)
    redge = nt.edge_radius(thetas)
    cx, cz = nt.CENTER

    mats = [material('NestTerrain_Basalt', 'NestBasalt.jpg'), material('NestTerrain_Ash', 'NestAsh.jpg'),
            material('NestTerrain_BoneBed', 'NestBoneBed.jpg'), material('NestTerrain_Cliff', 'NestCliff.jpg')]
    reps = [72.0, 56.0, 64.0]
    verts, world = [], []

    def add(x, z, h):
        world.append((x, z, h))
        verts.append((x * TO_M, z * TO_M, h * TO_M))
        return len(verts) - 1

    center = add(cx, cz, hm.at(cx, cz))
    grid = []   # grid[k][a] = 頂点 (k = 1..rings)
    for k in range(1, rings + 1):
        t = k / rings
        row = []
        for a in range(A):
            r = t * redge[a]
            x, z = cx + math.cos(thetas[a]) * r, cz + math.sin(thetas[a]) * r
            row.append(add(x, z, hm.at(x, z)))
        grid.append(row)
    edge_h = [world[grid[-1][a]][2] for a in range(A)]
    under = []
    for depth, frac in UNDERSIDE:
        row = []
        for a in range(A):
            jag = 1.0 + rng.normal(0, 0.035) + 0.04 * math.sin(thetas[a] * 23 + depth * 0.01)
            r = redge[a] * frac * jag
            dy = depth * (1.0 + rng.normal(0, 0.05))
            row.append(add(cx + math.cos(thetas[a]) * r, cz + math.sin(thetas[a]) * r, edge_h[a] + dy))
        under.append(row)
    tip = add(cx + 25, cz - 15, nt.BASE + 34 + TIP_DEPTH)

    faces, face_mat, face_uv = [], [], []

    def top_uv(vi, mi):
        x, z, _ = world[vi]
        return (x / reps[mi], z / reps[mi])

    def top_face(vs):
        xs = [world[v] for v in vs]
        mx, mz = sum(p[0] for p in xs) / len(xs), sum(p[1] for p in xs) / len(xs)
        mi = hm.material(mx, mz)
        faces.append(tuple(vs))
        face_mat.append(mi)
        face_uv.append([top_uv(v, mi) for v in vs])

    for a in range(A):
        b = (a + 1) % A
        top_face((center, grid[0][a], grid[0][b]))
    for k in range(rings - 1):
        for a in range(A):
            b = (a + 1) % A
            top_face((grid[k][a], grid[k + 1][a], grid[k + 1][b], grid[k][b]))

    # 崖と島の底 (UV は周りの長さと深さ)
    perim = [0.0]
    for a in range(A):
        pa, pb = world[grid[-1][a]], world[grid[-1][(a + 1) % A]]
        perim.append(perim[-1] + math.hypot(pb[0] - pa[0], pb[1] - pa[1]))
    rep = 96.0

    def cliff_uv(vi, a):
        return (perim[a] / rep, (world[vi][2] - nt.BASE) / rep)

    rows = [grid[-1]] + under
    for k in range(len(rows) - 1):
        for a in range(A):
            b = (a + 1) % A
            bu = a + 1   # 継ぎ目で UV が巻き戻らないように、閉じる側は周長の最後を使う
            vs = (rows[k][a], rows[k][b], rows[k + 1][b], rows[k + 1][a])
            faces.append(vs)
            face_mat.append(3)
            face_uv.append([cliff_uv(vs[0], a), cliff_uv(vs[1], bu), cliff_uv(vs[2], bu), cliff_uv(vs[3], a)])
    for a in range(A):
        b = (a + 1) % A
        vs = (under[-1][a], under[-1][b], tip)
        faces.append(vs)
        face_mat.append(3)
        face_uv.append([cliff_uv(vs[0], a), cliff_uv(vs[1], a + 1), (perim[a] / rep, (world[tip][2] - nt.BASE) / rep)])

    c3 = np.array([cx * TO_M, cz * TO_M, (nt.BASE - 300) * TO_M])

    def outward(p):
        # 上面は上向き、崖と底は中心の軸から外向き
        q = np.array(p)
        if q[2] > (nt.BASE - 5) * TO_M and math.hypot(q[0] - c3[0], q[1] - c3[1]) < (nt.EDGE_RADIUS - 60) * TO_M:
            return (0.0, 0.0, 1.0)
        d = q - c3
        return tuple(d / max(np.linalg.norm(d), 1e-6))

    finish('NestTerrain', verts, faces, face_mat, face_uv, mats, outward=outward)


# ---------------------------------------------------------------- 岩の爪
SPIRES = {
    # 名前: (高さ m, 根元の半径 m, 反り m, ねじれ rad, 面の数, 種)
    'NestSpireA': (34.0, 4.2, 12.0, 0.6, 7, 1),
    'NestSpireB': (20.0, 6.5, 5.0, 0.2, 8, 2),
    'NestSpireC': (27.0, 3.4, 15.0, 1.4, 6, 3),
}
SPIRE_SINK = 5.0


def build_spire(name, height, radius, bend, twist, sides, seed):
    rng = np.random.default_rng(seed)
    segs = 22
    mat = material(f'{name}_0', 'NestBasalt.jpg')
    verts, faces, face_uv = [], [], []
    rep = 6.0
    noise = rng.normal(0, 0.12, (segs + 1, sides))
    for s in range(segs + 1):
        t = s / segs
        z = -SPIRE_SINK + (height + SPIRE_SINK) * t
        y = bend * t * t
        r = radius * (1 - t) ** 0.85 + 0.05
        for k in range(sides):
            a = 2 * math.pi * k / sides + twist * t
            rr = r * (1 + noise[s, k] * (1 - t * 0.5))
            verts.append((math.cos(a) * rr, y + math.sin(a) * rr, z))
    tip = len(verts)
    verts.append((0.0, bend * 1.02, height + radius * 0.2))
    for s in range(segs):
        for k in range(sides):
            k2 = (k + 1) % sides
            a, b = s * sides + k, s * sides + k2
            c, d = (s + 1) * sides + k2, (s + 1) * sides + k
            faces.append((a, b, c, d))
            u0, u1 = k / sides * 2 * math.pi * radius / rep, (k + 1) / sides * 2 * math.pi * radius / rep
            v0, v1 = verts[a][2] / rep, verts[d][2] / rep
            face_uv.append([(u0, v0), (u1, v0), (u1, v1), (u0, v1)])
    for k in range(sides):
        a, b = segs * sides + k, segs * sides + (k + 1) % sides
        faces.append((a, b, tip))
        face_uv.append([(k / sides, height / rep), ((k + 1) / sides, height / rep), ((k + 0.5) / sides, height / rep + 0.3)])

    def outward(p):
        # 中心線からの向き
        t = min(max((p[2] + SPIRE_SINK) / (height + SPIRE_SINK), 0.0), 1.0)
        axis = np.array((0.0, bend * t * t, p[2]))
        d = np.array(p) - axis
        if np.linalg.norm(d[:2]) < 1e-3:
            return (0.0, 0.0, 1.0)
        return tuple(d / np.linalg.norm(d))

    finish(name, verts, faces, [0] * len(faces), face_uv, [mat], outward=outward)


# ---------------------------------------------------------------- 嵐の壁
STORM_RADIUS = 330.0        # m (world 2640)
STORM_BOTTOM, STORM_TOP = -230.0, 300.0
EYE_RADIUS = 120.0         # m (world 960)。雲の天井に開いた嵐の目


def build_storm_wall():
    rng = np.random.default_rng(21)
    A, rows = 96, 14
    mat = material('NestStormWall_0', 'NestStorm.jpg')
    verts, faces, face_uv, face_mat = [], [], [], []
    for j in range(rows + 1):
        t = j / rows
        z = STORM_BOTTOM + (STORM_TOP - STORM_BOTTOM) * t
        # 上へ開く漏斗 (嵐の目)。途中がうねる
        r = STORM_RADIUS * (0.85 + 0.35 * t * t)
        for a in range(A + 1):
            th = 2 * math.pi * a / A
            rr = r * (1 + 0.04 * math.sin(th * 5 + t * 4) + rng.normal(0, 0.01))
            verts.append((math.cos(th) * rr, math.sin(th) * rr, z))
    for j in range(rows):
        for a in range(A):
            p0 = j * (A + 1) + a
            vs = (p0, p0 + 1, p0 + A + 2, p0 + A + 1)
            faces.append(vs)
            face_mat.append(0)
            face_uv.append([(a / A * 8, j / rows * 2.5), ((a + 1) / A * 8, j / rows * 2.5),
                            ((a + 1) / A * 8, (j + 1) / rows * 2.5), (a / A * 8, (j + 1) / rows * 2.5)])
    # 雲の天井: 壁の上の口から、嵐の目 (EYE_RADIUS) まで内へ閉じる。見上げると目の奥にだけ空が見える
    top_ring = [j_top for j_top in range(rows * (A + 1), (rows + 1) * (A + 1))]
    prev = top_ring
    steps = 5
    for i in range(1, steps + 1):
        t = i / steps
        r = STORM_RADIUS * 1.2 * (1 - t) + EYE_RADIUS * t
        z = STORM_TOP + 35 * math.sin(t * math.pi * 0.8)
        cur = []
        for a in range(A + 1):
            th = 2 * math.pi * a / A
            rr = r * (1 + 0.05 * math.sin(th * 7 + i) + rng.normal(0, 0.015))
            cur.append(len(verts))
            verts.append((math.cos(th) * rr, math.sin(th) * rr, z))
        for a in range(A):
            faces.append((prev[a], prev[a + 1], cur[a + 1], cur[a]))
            face_mat.append(0)
            face_uv.append([(a / A * 8, 2.5 + (i - 1) / steps), ((a + 1) / A * 8, 2.5 + (i - 1) / steps),
                            ((a + 1) / A * 8, 2.5 + i / steps), (a / A * 8, 2.5 + i / steps)])
        prev = cur

    # 底の渦 (下を覗いても雲の渦)
    base = len(verts)
    verts.append((0.0, 0.0, STORM_BOTTOM - 40))
    ring = []
    for a in range(A):
        th = 2 * math.pi * a / A
        ring.append(len(verts))
        verts.append((math.cos(th) * STORM_RADIUS * 0.86, math.sin(th) * STORM_RADIUS * 0.86, STORM_BOTTOM + 2))
    for a in range(A):
        b = (a + 1) % A
        faces.append((base, ring[b], ring[a]))
        face_mat.append(0)
        th0, th1 = 2 * math.pi * a / A, 2 * math.pi * (a + 1) / A
        face_uv.append([(0.5, 0.5), (0.5 + math.cos(th1) * 2, 0.5 + math.sin(th1) * 2),
                        (0.5 + math.cos(th0) * 2, 0.5 + math.sin(th0) * 2)])

    def outward(p):
        # 内向き (中心の軸へ)。底は上向き、天井は下向き
        if p[2] < STORM_BOTTOM + 5:
            return (0.0, 0.0, 1.0)
        if p[2] > STORM_TOP + 1:
            return (0.0, 0.0, -1.0)
        return (-p[0], -p[1], 0.0)

    finish('NestStormWall', verts, faces, face_mat, face_uv, [mat], smooth=True, outward=outward)


# ---------------------------------------------------------------- 竜撃ちの銛
def build_harpoon():
    iron = material('NestHarpoon_Iron', 'NestIron.jpg')
    wood = material('NestHarpoon_Wood', 'Settle_Wood.png')
    verts, faces, face_mat, face_uv = [], [], [], []
    sides = 8

    def tube(z0, z1, r0, r1, mi, rep=2.0):
        base = len(verts)
        for z, r in ((z0, r0), (z1, r1)):
            for k in range(sides + 1):
                a = 2 * math.pi * k / sides
                verts.append((math.cos(a) * r, math.sin(a) * r, z))
        for k in range(sides):
            p = base + k
            faces.append((p, p + 1, p + sides + 2, p + sides + 1))
            face_mat.append(mi)
            face_uv.append([(k / sides, z0 / rep), ((k + 1) / sides, z0 / rep), ((k + 1) / sides, z1 / rep),
                            (k / sides, z1 / rep)])

    def cap(z, r, mi, up):
        c = len(verts)
        verts.append((0.0, 0.0, z))
        ring = []
        for k in range(sides):
            a = 2 * math.pi * k / sides
            ring.append(len(verts))
            verts.append((math.cos(a) * r, math.sin(a) * r, z))
        for k in range(sides):
            faces.append((c, ring[k], ring[(k + 1) % sides]))
            face_mat.append(mi)
            face_uv.append([(0.5, 0.5), (0.5, 0.0), (0.6, 0.0)])

    # 穂先 (原点が尖り)。返しの付いた四角い穂
    tube(0.0, 2.2, 0.02, 0.75, 0)
    tube(2.2, 3.0, 0.75, 0.3, 0)
    tube(3.0, 3.8, 0.3, 0.3, 0)
    # 返し
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        base = len(verts)
        ca, sa = math.cos(a), math.sin(a)
        pa, pb = (-sa * 0.12, ca * 0.12), (sa * 0.12, -ca * 0.12)
        verts += [(ca * 0.5 + pa[0], sa * 0.5 + pa[1], 1.4), (ca * 0.5 + pb[0], sa * 0.5 + pb[1], 1.4),
                  (ca * 1.25, sa * 1.25, 2.9), (ca * 0.3, sa * 0.3, 3.0)]
        for f in ((base, base + 1, base + 2), (base + 1, base + 3, base + 2), (base + 3, base, base + 2)):
            faces.append(f)
            face_mat.append(0)
            face_uv.append([(0.0, 0.0), (1.0, 0.0), (0.5, 1.0)])
    # 柄と鉄の輪
    tube(3.8, 15.0, 0.34, 0.3, 1, rep=3.0)
    for z in (4.2, 9.0, 14.0):
        tube(z, z + 0.45, 0.42, 0.42, 0)
        cap(z, 0.42, 0, False)
        cap(z + 0.45, 0.42, 0, True)
    tube(15.0, 15.6, 0.45, 0.45, 0)
    cap(15.6, 0.45, 0, True)

    def outward(p):
        if p[2] >= 15.59:
            return (0.0, 0.0, 1.0)
        d = (p[0], p[1], 0.0)
        return d if math.hypot(p[0], p[1]) > 1e-4 else (0.0, 0.0, -1.0)

    finish('NestHarpoon', verts, faces, face_mat, face_uv, [iron, wood], smooth=False, outward=outward)


def main():
    os.makedirs(FBX_DIR, exist_ok=True)
    jobs = {'NestTerrain': build_terrain, 'NestStormWall': build_storm_wall, 'NestHarpoon': build_harpoon}
    for name, spec in SPIRES.items():
        jobs[name] = (lambda n=name, s=spec: build_spire(n, *s))
    for name, job in jobs.items():
        if ONLY and name not in ONLY:
            continue
        clear()
        job()


main()
