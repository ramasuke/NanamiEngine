"""遠景の浮島のメッシュを形ごとに組み、FBX に書き出す (Blender を -b で動かす)。

    blender -b --factory-startup --python tools/art/sky_island_shapes_blender.py -- <work> <textures dir>
    python tools/art/sky_island_models.py <work>        # FBX -> .mv1 (草 / 砂の2種) と .meta

- 形: Long (細長い尾根) / Slab (広く薄い岩盤、下に小さな岩の房) / Twin (2つがくっついた島) /
  Mesa (段になった台地と高い崖) / Shard (細く長く垂れる岩の塔)。
- 極座標の格子で組む: 天辺 (輪郭の内側、ほぼ平ら) -> 縁の崖 -> 底 (輪郭から内へ、垂れる岩の先へ深くなる高さ場)。
  どれも閉じたメッシュ。1 m = .mv1 の 100 単位 (FBX の既定)。
- 材質は2つ: SkyTop (上を向いた面) と SkyRock (それ以外)。UV は面の向きで箱投影。
  テクスチャは SkyTop_Grass.jpg / SkyRock_Grey.jpg。砂の島はファイル名だけ差し替える (sky_island_models.py)。
"""
import math
import os
import shutil
import sys

import bpy
import bmesh
import numpy as np

ARGS = sys.argv[sys.argv.index('--') + 1:]
WORK, TEX_DIR = ARGS[0], ARGS[1]
FBX_DIR = os.path.join(WORK, 'fbx')

SEGMENTS = 144
TOP_RINGS = 10
BOTTOM_RINGS = 22
TOP_TILE, ROCK_TILE = 3.0, 4.0     # テクスチャの繰り返し長 m
MATERIALS = [('SkyTop', 'SkyTop_Grass.jpg'), ('SkyRock', 'SkyRock_Grey.jpg')]

# 形: outline = 輪郭 (ellipse: 半径 A, B / twin: 円2つの和 (中心 ±c, 半径 R))、cliff = 縁の崖の高さ、
# sag = 底が縁から中へ下がる量、tips = 垂れる岩 [(x, y, 半径, 深さ, 尖り)]、terrace = 天辺の段 (高さ, 内側の割合)
SHAPES = {
    'Long': dict(outline=('ellipse', 9.0, 2.8), cliff=0.8, sag=1.5, noise=0.10, seed=1,
                 tips=[(-5.0, 0.3, 3.2, 5.5, 1.5), (0.2, -0.2, 3.6, 7.5, 1.4), (5.2, 0.2, 3.0, 4.5, 1.6)]),
    'Slab': dict(outline=('ellipse', 7.5, 5.5), cliff=0.6, sag=0.8, noise=0.08, seed=2,
                 tips=[(-4.0, -1.5, 1.8, 2.8, 1.8), (-1.5, 2.5, 1.6, 2.2, 1.8), (1.0, -2.8, 1.7, 3.0, 1.8),
                       (3.5, 1.0, 1.9, 3.4, 1.8), (-0.5, 0.0, 2.4, 3.8, 1.6), (5.2, -1.8, 1.2, 1.8, 1.8),
                       (-5.5, 1.8, 1.3, 2.0, 1.8), (2.0, 3.2, 1.2, 1.6, 1.8)]),
    'Twin': dict(outline=('twin', 3.4, 3.8), cliff=0.9, sag=1.2, noise=0.09, seed=3,
                 tips=[(-3.4, 0.0, 3.4, 7.0, 1.4), (3.4, 0.3, 3.1, 5.8, 1.5)]),
    'Mesa': dict(outline=('ellipse', 6.0, 4.6), cliff=2.6, sag=0.9, noise=0.07, seed=4, terrace=(2.4, 0.52),
                 tips=[(0.0, 0.0, 4.6, 4.5, 0.9), (2.5, -1.0, 2.0, 2.5, 1.5)]),
    'Shard': dict(outline=('ellipse', 3.4, 2.4), cliff=0.7, sag=1.0, noise=0.12, seed=5,
                  tips=[(0.2, 0.0, 3.0, 12.0, 1.1), (-1.5, 0.8, 1.2, 3.0, 1.6)]),
}


def outline_radius(kind, a, b, theta):
    c, s = np.cos(theta), np.sin(theta)
    if kind == 'ellipse':
        return 1.0 / np.sqrt((c / a) ** 2 + (s / b) ** 2)
    # 円2つ (中心 (±a, 0)、半径 b) の和の輪郭: 原点からの半直線と円の交点の遠い方
    out = np.zeros_like(theta)
    for cx in (-a, a):
        proj = c * cx
        disc = proj ** 2 - (cx ** 2 - b ** 2)
        out = np.maximum(out, np.where(disc >= 0, proj + np.sqrt(np.maximum(disc, 0.0)), 0.0))
    return out


class Noise:
    def __init__(self, seed):
        r = np.random.default_rng(seed)
        self.waves = [(r.uniform(0.3, 1.6), r.uniform(0, math.tau), r.uniform(0, math.tau), r.uniform(0, math.tau))
                      for _ in range(7)]
        self.ring = [(k, r.uniform(0, math.tau), r.uniform(0.3, 1.0) / k) for k in range(2, 9)]

    def field(self, x, y):
        v = np.zeros_like(x)
        for f, ph, px, py in self.waves:
            v += np.sin(f * (x * math.cos(ph) + y * math.sin(ph)) + px) * np.sin(f * 0.8 * (y * math.cos(ph) - x * math.sin(ph)) + py)
        return v / len(self.waves) * 2.0

    def edge(self, theta):
        v = np.zeros_like(theta)
        for k, ph, amp in self.ring:
            v += np.sin(k * theta + ph) * amp
        return v


def build_shape(name, spec):
    noise = Noise(spec['seed'])
    theta = np.linspace(0.0, math.tau, SEGMENTS, endpoint=False)
    kind, a, b = spec['outline']
    radius = outline_radius(kind, a, b, theta) * (1.0 + spec['noise'] * noise.edge(theta))
    dirs = np.stack([np.cos(theta), np.sin(theta)], axis=1)

    def ring(t):
        return dirs * (radius * t)[:, None]

    def top_height(p, t):
        h = 0.12 * noise.field(p[:, 0] * 0.9, p[:, 1] * 0.9)
        if 'terrace' in spec:
            height, inner = spec['terrace']
            edge = inner + 0.05 * noise.field(p[:, 0] * 0.5 + 7, p[:, 1] * 0.5)
            h += height * np.clip((edge - t) / 0.06 + 0.5, 0.0, 1.0)
        return h

    # 大きな岩の先の周りに、小さな岩の房を足す (円錐がそのまま並ぶと作り物に見える)
    rng = np.random.default_rng(spec['seed'] + 100)
    tips = list(spec['tips'])
    for tx, ty, rad, depth, sharp in spec['tips']:
        for _ in range(4):
            a, r = rng.uniform(0, math.tau), rad * rng.uniform(0.45, 0.9)
            tips.append((tx + math.cos(a) * r, ty + math.sin(a) * r, rad * rng.uniform(0.3, 0.5),
                         depth * rng.uniform(0.3, 0.6), sharp + 0.3))

    def bottom_depth(p, t):
        d = spec['cliff'] + spec['sag'] * (1.0 - t ** 2)
        for tx, ty, rad, depth, sharp in tips:
            dist = np.hypot(p[:, 0] - tx, p[:, 1] - ty) / rad
            d = np.maximum(d, spec['cliff'] + depth * np.clip(1.0 - dist, 0.0, 1.0) ** sharp)
        rough = 0.16 * noise.field(p[:, 0] * 1.7 + 3, p[:, 1] * 1.7) + 0.08 * noise.field(p[:, 0] * 4.1, p[:, 1] * 4.3 + 5)
        return d * (1.0 + rough)

    bm = bmesh.new()
    rows = []

    def add_row(points, heights):
        rows.append([bm.verts.new((float(x), float(y), float(z))) for (x, y), z in zip(points, heights)])

    # 天辺: 中心 -> 縁
    center_top = bm.verts.new((0.0, 0.0, float(top_height(np.zeros((1, 2)), np.zeros(1))[0])))
    for i in range(1, TOP_RINGS + 1):
        t = i / TOP_RINGS
        pts = ring(t)
        add_row(pts, top_height(pts, np.full(SEGMENTS, t)))
    # 縁の崖: 縁からまっすぐ下へ、少し内へ入りながら
    rim = ring(1.0)
    for k, (inset, drop) in enumerate(((0.99, 0.35), (0.97, 0.7), (0.95, 1.0))):
        pts = rim * inset
        add_row(pts, -spec['cliff'] * drop + 0.1 * noise.field(pts[:, 0] * 2 + k, pts[:, 1] * 2))
    # 底: 輪郭の内側を外から中へ
    for i in range(1, BOTTOM_RINGS + 1):
        t = 0.95 * (1.0 - i / (BOTTOM_RINGS + 1))
        pts = ring(t)
        add_row(pts, -bottom_depth(pts, np.full(SEGMENTS, t)))
    center_bottom = bm.verts.new((0.0, 0.0, float(-bottom_depth(np.zeros((1, 2)), np.zeros(1))[0])))

    n = SEGMENTS
    for j in range(n):
        bm.faces.new((center_top, rows[0][j], rows[0][(j + 1) % n]))
    for r0, r1 in zip(rows[:-1], rows[1:]):
        for j in range(n):
            bm.faces.new((r0[j], r1[j], r1[(j + 1) % n], r0[(j + 1) % n]))
    for j in range(n):
        bm.faces.new((rows[-1][(j + 1) % n], rows[-1][j], center_bottom))

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    me = bpy.data.meshes.new(f'SkyIsland_{name}')
    bm.to_mesh(me)
    bm.free()
    return me


def assign_materials_and_uv(me):
    uv = me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        nx, ny, nz = poly.normal
        is_top = nz > 0.55
        poly.material_index = 0 if is_top else 1
        poly.use_smooth = False
        tile = TOP_TILE if is_top else ROCK_TILE
        ax = max(range(3), key=lambda k: abs(poly.normal[k]))
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            if ax == 2:
                u, v = co.x, co.y
            elif ax == 0:
                u, v = co.y, co.z
            else:
                u, v = co.x, co.z
            uv.data[li].uv = (u / tile, v / tile)


def flip_normals_for_dxlib(me):
    """DxLibModelViewer は Blender の FBX の法線の上下 (Blender の Z) を反転して読む。角ばった岩に見せたいので面の法線で"""
    me.update()
    loops = [None] * len(me.loops)
    for poly in me.polygons:
        n = poly.normal
        for li in poly.loop_indices:
            loops[li] = (n.x, n.y, -n.z)
    me.normals_split_custom_set(loops)


def main():
    os.makedirs(FBX_DIR, exist_ok=True)
    mats = []
    for mname, tex in MATERIALS:
        shutil.copy2(os.path.join(TEX_DIR, tex), os.path.join(FBX_DIR, tex))
        m = bpy.data.materials.new(mname)
        m.use_nodes = True
        bsdf = next(nd for nd in m.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        t = m.node_tree.nodes.new('ShaderNodeTexImage')
        t.image = bpy.data.images.load(os.path.join(FBX_DIR, tex))
        m.node_tree.links.new(t.outputs['Color'], bsdf.inputs['Base Color'])
        mats.append(m)

    units = bpy.context.scene.unit_settings
    units.system, units.scale_length, units.length_unit = 'METRIC', 1.0, 'METERS'
    for name, spec in SHAPES.items():
        for o in list(bpy.data.objects):
            bpy.data.objects.remove(o)
        me = build_shape(name, spec)
        for m in mats:
            me.materials.append(m)
        assign_materials_and_uv(me)
        flip_normals_for_dxlib(me)
        obj = bpy.data.objects.new(f'SkyIsland_{name}', me)
        bpy.context.scene.collection.objects.link(obj)
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.export_scene.fbx(filepath=os.path.join(FBX_DIR, f'SkyIsland_{name}.fbx'), use_selection=True,
                                 object_types={'MESH'}, path_mode='STRIP', embed_textures=False,
                                 mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)
        xs = [v.co for v in me.vertices]
        print(f'SHAPE {name}: verts={len(me.vertices)} tris={len(me.polygons)} '
              f'x={min(c.x for c in xs):.1f}..{max(c.x for c in xs):.1f} y={min(c.y for c in xs):.1f}..{max(c.y for c in xs):.1f} '
              f'z={min(c.z for c in xs):.1f}..{max(c.z for c in xs):.1f}')


main()
