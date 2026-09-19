"""Blender 側のモデリング補助 (exec で読み込む)。単位は m、地面は z=0、正面は +Y (エンジンの +Z)。

GrassLand の「荒れた村」「狩猟民のキャンプ」の建物を作る。手順:
  1. python tools/art/settlement/make_textures.py <work>            # <work>/tex にテクスチャ
  2. Blender (4.5, MCP か Python コンソール) で
         g = {'SETTLEMENT_WORK': r'<work>'}
         for f in ('blib.py', 'assets_ruins.py', 'assets_camp.py'):
             exec(open(r'<repo>/tools/art/settlement/' + f, encoding='utf-8').read(), g)
         g['reset']()
         for b in g['RUIN_BUILDERS'] + g['CAMP_BUILDERS']:
             g['export'](b())                                          # <work>/fbx/<Name>.fbx + .colliders.json
  3. python tools/art/settlement/install_models.py <work>           # .mv1 へ変換して Assets へ、当たり判定を data へ
  4. python tools/art/settlement_prefabs.py && python tools/art/settlement_scatter.py

- 部品はすべてローカル座標で作って UV を張ってから配置し、最後に1オブジェクトへ結合する
- UV は面の法線の主軸で投影し、材質ごとの繰り返し長(m)で割る。木目・藁の向きは grain で指定する
- 当たり判定は COLLIDERS[asset] に (中心, 寸法, 回転行列の3x3) で溜めておき、JSON に書き出す
"""
import bmesh
import bpy
import json
import math
import os
import random
import zlib
from mathutils import Matrix, Vector, Euler

WORK = globals().get('SETTLEMENT_WORK') or os.path.join(os.environ.get('TEMP', '/tmp'), 'nanami_settlement')
TEX = os.path.join(WORK, 'tex')
FBX_DIR = os.path.join(WORK, 'fbx')

# 材質: 名前 -> (テクスチャ, 繰り返し長 m, 木目が画像のどちら向きか 'u'/'v'/None, 基本色)
MATERIALS = {
    'Stone': ('Settle_Stone.png', 2.2, None, None),
    'Wood': ('Settle_Wood.png', 1.8, 'u', None),
    'Burnt': ('Settle_WoodBurnt.png', 1.8, 'u', None),
    'Thatch': ('Settle_Thatch.png', 2.0, 'v', None),
    'Hide': ('Settle_Hide.png', 3.0, None, None),
    'Bark': ('Settle_Bark.png', 1.2, 'v', None),
    'Bone': ('Settle_Bone.png', 1.0, 'v', None),
    'Ash': ('Settle_Ash.png', 1.5, None, None),
    'Meat': (None, 1.0, None, (0.36, 0.09, 0.07)),
    'Rope': (None, 1.0, None, (0.52, 0.42, 0.27)),
    'Iron': (None, 1.0, None, (0.16, 0.15, 0.15)),
}

COLLIDERS = {}
PARTS = []


def material(key, asset):
    name = f'{asset}_{key}'
    mat = bpy.data.materials.get(name)
    if mat:
        return mat
    tex, _, _, color = MATERIALS[key]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Roughness'].default_value = 0.95
    if 'Specular IOR Level' in bsdf.inputs:
        bsdf.inputs['Specular IOR Level'].default_value = 0.1
    if tex:
        img = bpy.data.images.load(os.path.join(TEX, tex), check_existing=True)
        node = mat.node_tree.nodes.new('ShaderNodeTexImage')
        node.image = img
        mat.node_tree.links.new(node.outputs['Color'], bsdf.inputs['Base Color'])
    else:
        bsdf.inputs['Base Color'].default_value = (*color, 1.0)
    return mat


# ---------------------------------------------------------------- scene helpers
def reset():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat)
    PARTS.clear()


def begin(asset):
    PARTS.clear()
    COLLIDERS[asset] = []
    global CURRENT
    CURRENT = asset
    random.seed(zlib.crc32(asset.encode()))


def _uv_project(bm, mat_key, grain_axis):
    """面の法線の主軸で投影。grain_axis(ローカル 0/1/2) を画像の木目方向へ合わせる"""
    tile = MATERIALS[mat_key][1]
    grain = MATERIALS[mat_key][2]
    uv = bm.loops.layers.uv.verify()
    for face in bm.faces:
        n = face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        others = [i for i in range(3) if i != axis]
        if grain_axis in others and grain is not None:
            g = grain_axis
            o = others[0] if others[1] == g else others[1]
            ua, va = (g, o) if grain == 'u' else (o, g)
        else:
            ua, va = others
        for loop in face.loops:
            co = loop.vert.co
            loop[uv].uv = (co[ua] / tile, co[va] / tile)


def _finish(bm, mat_key, loc, rot, grain_axis=2, smooth=False, uv=True):
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.normal_update()
    if uv:
        _uv_project(bm, mat_key, grain_axis)
    me = bpy.data.meshes.new(f'{CURRENT}_part')
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = smooth
    obj = bpy.data.objects.new(me.name, me)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material(mat_key, CURRENT))
    obj.location = loc
    obj.rotation_mode = 'XYZ'
    if isinstance(rot, Matrix):
        obj.rotation_euler = rot.to_euler('XYZ')
    else:
        obj.rotation_euler = Euler([math.radians(a) for a in rot], 'XYZ')
    PARTS.append(obj)
    return obj


def box(size, loc, rot=(0, 0, 0), mat='Wood', grain_axis=0, jitter=0.0, top_jag=None, bevel=0.0):
    """size=(x,y,z)。原点は底面中心。top_jag=(分割数, 振幅) で上端をギザギザに削る"""
    bm = bmesh.new()
    sx, sy, sz = size
    if top_jag:
        segs, amp = top_jag
        # X 方向に segs 分割した箱。上面の頂点の高さを乱す
        xs = [-sx / 2 + sx * i / segs for i in range(segs + 1)]
        hs = [sz - random.uniform(0, amp) for _ in xs]
        verts = []
        for x, h in zip(xs, hs):
            verts.append([bm.verts.new((x, -sy / 2, 0)), bm.verts.new((x, sy / 2, 0)),
                          bm.verts.new((x, sy / 2, h)), bm.verts.new((x, -sy / 2, h))])
        for a, b in zip(verts, verts[1:]):
            bm.faces.new((a[0], b[0], b[3], a[3]))  # front
            bm.faces.new((b[1], a[1], a[2], b[2]))  # back
            bm.faces.new((a[3], b[3], b[2], a[2]))  # top
            bm.faces.new((a[1], b[1], b[0], a[0]))  # bottom
        bm.faces.new((verts[0][1], verts[0][0], verts[0][3], verts[0][2]))
        bm.faces.new((verts[-1][0], verts[-1][1], verts[-1][2], verts[-1][3]))
    else:
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            v.co = Vector((v.co.x * sx, v.co.y * sy, (v.co.z + 0.5) * sz))
    if bevel > 0:
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=bevel, segments=1, affect='EDGES')
    if jitter > 0:
        for v in bm.verts:
            v.co += Vector((random.uniform(-jitter, jitter) for _ in range(3)))
    bm.normal_update()
    return _finish(bm, mat, loc, rot, grain_axis)


def cyl(radius, depth, loc, rot=(0, 0, 0), mat='Bark', verts=8, radius_top=None, cap=True, jitter=0.0, smooth=True):
    """原点は底面中心、+Z へ depth"""
    bm = bmesh.new()
    rt = radius if radius_top is None else radius_top
    bmesh.ops.create_cone(bm, cap_ends=cap, cap_tris=False, segments=verts, radius1=radius, radius2=rt, depth=depth)
    for v in bm.verts:
        v.co.z += depth / 2
        if jitter:
            v.co += Vector((random.uniform(-jitter, jitter), random.uniform(-jitter, jitter), 0))
    bm.normal_update()
    return _finish(bm, mat, loc, rot, 2, smooth=smooth)


def _look_rot(p0, p1):
    d = (Vector(p1) - Vector(p0))
    return d.to_track_quat('Z', 'Y').to_matrix(), d.length


def pole(p0, p1, radius, mat='Bark', verts=7, taper=1.0, sharpen=0.0, smooth=True):
    """p0 から p1 への丸太。sharpen>0 で先端を尖らせる(杭)"""
    rot, length = _look_rot(p0, p1)
    bm = bmesh.new()
    body = length - sharpen
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=verts, radius1=radius,
                          radius2=radius * taper, depth=body)
    for v in bm.verts:
        v.co.z += body / 2
    if sharpen > 0:
        top = [v for v in bm.verts if v.co.z > body - 1e-4]
        tip = bm.verts.new((random.uniform(-0.02, 0.02), random.uniform(-0.02, 0.02), length))
        top_faces = [f for f in bm.faces if all(v in top for v in f.verts)]
        bmesh.ops.delete(bm, geom=top_faces, context='FACES_ONLY')
        ring = sorted(top, key=lambda v: math.atan2(v.co.y, v.co.x))
        for a, b in zip(ring, ring[1:] + ring[:1]):
            bm.faces.new((a, b, tip))
    bm.normal_update()
    return _finish(bm, mat, Vector(p0), rot, 2, smooth=smooth)


def beam(p0, p1, w, h, mat='Burnt', roll=0.0):
    """角材 (断面 w x h)。長さ方向がローカル Z なので木目は Z"""
    rot, length = _look_rot(p0, p1)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector((v.co.x * w, v.co.y * h, (v.co.z + 0.5) * length))
    bm.normal_update()
    if roll:
        rot = rot @ Matrix.Rotation(math.radians(roll), 3, 'Z')
    return _finish(bm, mat, Vector(p0), rot, 2)


def stone(loc, size, mat='Stone', flat=0.7, rot=None):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=1, radius=0.5)
    sx = size * random.uniform(0.8, 1.25)
    sy = size * random.uniform(0.8, 1.25)
    sz = size * flat * random.uniform(0.8, 1.2)
    for v in bm.verts:
        v.co = Vector((v.co.x * sx, v.co.y * sy, v.co.z * sz)) * random.uniform(0.85, 1.12)
    bm.normal_update()
    r = rot if rot is not None else (random.uniform(-15, 15), random.uniform(-15, 15), random.uniform(0, 360))
    return _finish(bm, mat, loc, r, 0)


def sheet(corners, mat='Hide', subdiv=6, sag=0.0, noise=0.0, thickness=0.04, grain_uv=None, holes=()):
    """4隅 (p00, p10, p11, p01) の布/皮。中央を sag だけ垂らす。厚みを付けて両面にする。
    UV は隅の (0,0)-(1,1) を grain_uv=(幅m, 高さm) で割った値"""
    p00, p10, p11, p01 = [Vector(c) for c in corners]
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.verify()
    grid = []
    for j in range(subdiv + 1):
        row = []
        for i in range(subdiv + 1):
            u, v = i / subdiv, j / subdiv
            p = p00.lerp(p10, u).lerp(p01.lerp(p11, u), v)
            p.z -= sag * math.sin(math.pi * u) * math.sin(math.pi * v)
            if noise:
                p += Vector((random.uniform(-noise, noise) for _ in range(3)))
            row.append(bm.verts.new(p))
        grid.append(row)
    w, h = grain_uv or ((p10 - p00).length, (p01 - p00).length)
    tile = MATERIALS[mat][1]
    for j in range(subdiv):
        for i in range(subdiv):
            cu, cv = (i + 0.5) / subdiv, (j + 0.5) / subdiv
            if any(u0 <= cu <= u1 and v0 <= cv <= v1 for (u0, u1, v0, v1) in holes):
                continue
            f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            for loop, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                loop[uvl].uv = (a / subdiv * w / tile, b / subdiv * h / tile)
    bm.normal_update()
    if thickness:
        res = bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=thickness)
    bm.normal_update()
    return _finish(bm, mat, (0, 0, 0), (0, 0, 0), uv=False, smooth=True)


def cone_shell(radius, height, top_radius, loc=(0, 0, 0), mat='Hide', segs=20, door=None, u_repeat=4.0,
               v_range=(0.0, 1.0), thickness=0.05, sag=0.0, tilt=None, noise=0.0, holes=()):
    """テントの皮。door=(開始角 deg, 幅 deg, 高さ割合) で入口を抜く。UV は u=角度, v=高さ割合"""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.verify()
    rings = 6
    grid = []
    for j in range(rings + 1):
        t = j / rings
        r = radius + (top_radius - radius) * t
        z = height * t
        row = []
        for i in range(segs + 1):
            a = 2 * math.pi * i / segs
            rr = r - sag * math.sin(math.pi * t) * (0.5 + 0.5 * math.cos(a * 5)) ** 2
            p = Vector((math.cos(a) * rr, math.sin(a) * rr, z))
            if noise:
                p += Vector((random.uniform(-noise, noise) for _ in range(3)))
            row.append((bm.verts.new(p), i / segs * u_repeat, v_range[0] + (v_range[1] - v_range[0]) * t))
        grid.append(row)
    # 継ぎ目 (i=segs と i=0) は同じ位置だが UV を分けるため別頂点のまま
    for j in range(rings):
        for i in range(segs):
            if door:
                a0, width, hfrac = door
                amid = math.degrees(2 * math.pi * (i + 0.5) / segs)
                d = (amid - a0 + 180) % 360 - 180
                if abs(d) < width / 2 * (1 - (j + 0.5) / rings / hfrac) and (j + 0.5) / rings < hfrac:
                    continue
            amid = math.degrees(2 * math.pi * (i + 0.5) / segs)
            tmid = (j + 0.5) / rings
            if any(a0 <= amid <= a1 and t0 <= tmid <= t1 for (a0, a1, t0, t1) in holes):
                continue
            q = (grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i])
            f = bm.faces.new([c[0] for c in q])
            for loop, c in zip(f.loops, q):
                loop[uvl].uv = (c[1], c[2])
    bm.normal_update()
    if thickness:
        bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=thickness)
    bm.normal_update()
    obj = _finish(bm, mat, loc, (0, 0, 0), uv=False, smooth=True)
    if tilt:
        obj.rotation_euler = Euler([math.radians(a) for a in tilt], 'XYZ')
    return obj


def ring_wall(radius, heights, thickness, loc, mat='Stone', segs=16):
    """円形の石垣。heights は segs 個の高さ"""
    bm = bmesh.new()
    rows = []
    for i in range(segs + 1):
        a = 2 * math.pi * i / segs
        h = heights[i % segs]
        c, s = math.cos(a), math.sin(a)
        ri, ro = radius - thickness / 2, radius + thickness / 2
        rows.append([bm.verts.new((c * ri, s * ri, -0.3)), bm.verts.new((c * ro, s * ro, -0.3)),
                     bm.verts.new((c * ro, s * ro, h)), bm.verts.new((c * ri, s * ri, h))])
    for a, b in zip(rows, rows[1:]):
        bm.faces.new((a[1], b[1], b[2], a[2]))
        bm.faces.new((b[0], a[0], a[3], b[3]))
        bm.faces.new((a[3], a[2], b[2], b[3]))
    bm.normal_update()
    return _finish(bm, mat, loc, (0, 0, 0), 2)


# ---------------------------------------------------------------- colliders
def collider(center, size, rot=(0, 0, 0)):
    """Blender 座標の箱 (中心, 寸法 m, 回転 deg or Matrix)"""
    m = rot if isinstance(rot, Matrix) else Euler([math.radians(a) for a in rot], 'XYZ').to_matrix()
    COLLIDERS[CURRENT].append({'center': list(center), 'size': list(size), 'rot': [list(r) for r in m]})


# ---------------------------------------------------------------- finalize / export
def finalize(asset):
    bpy.ops.object.select_all(action='DESELECT')
    for p in PARTS:
        p.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = asset
    obj.data.name = asset
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    PARTS.clear()
    return obj


def export(obj):
    os.makedirs(FBX_DIR, exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(FBX_DIR, f'{obj.name}.fbx')
    # 木や岩と同じ「m 単位 → .mv1 のフレームに x100」にするため、シーンの単位が cm 等でも m で書き出す
    units = bpy.context.scene.unit_settings
    saved = (units.system, units.scale_length, units.length_unit)
    units.system, units.scale_length, units.length_unit = 'METRIC', 1.0, 'METERS'
    try:
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'MESH'},
                                 path_mode='STRIP', embed_textures=False, mesh_smooth_type='FACE',
                                 use_mesh_modifiers=True, add_leaf_bones=False, bake_anim=False)
    finally:
        units.system, units.scale_length, units.length_unit = saved
    with open(os.path.join(FBX_DIR, f'{obj.name}.colliders.json'), 'w') as f:
        json.dump(COLLIDERS.get(obj.name, []), f, indent=1)
    dims = [round(v, 3) for v in obj.dimensions]
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    return f'{obj.name}: dims {dims}, tris {tris}, mats {[m.name for m in obj.data.materials]}'


def layout_row(objs, spacing=12.0):
    x = 0.0
    for o in objs:
        o.location.x = x
        x += spacing
