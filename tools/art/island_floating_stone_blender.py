"""ほかの島の底に埋まった浮遊石。島の尖った底から結晶が突き出して、底そのものが結晶になっている形。

"C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --python tools/art/island_floating_stone_blender.py -- \\
    Assets/Art/Models/IslandHeart/_Source/IslandFloatingStone.blend Assets/Art/Models/IslandHeart/_Source

色ごとに IslandFloatingStone_<Color>.fbx を書く (形は色ごとに少し変える)。原点の上 (+Z = エンジンの上) は島の岩に
埋める所で、根元は軸に集まっているので、岩の中から結晶が斜め下へ突き抜けて見える。単位はエンジン単位。
テクスチャは COLORS のもの。Sky / Pearl / Rose の Core_Crystal_<Color>.png は Core_Crystal.png (緑) の色違い (tools/art/island_floating_stones.py --textures)。
FBX の隣にそのテクスチャを置いてから tools.model convert --mode mesh --with-textures -> install -> set-emissive
(Green=0.25,0.7,0.4 / Light=0.7,0.6,0.25 / Sky=0.3,0.6,0.8 / Pearl=0.62,0.64,0.72 / Rose=0.8,0.32,0.45)。
"""
import bpy
import bmesh
import math
import random
import sys
from mathutils import Vector

REPO = r"C:/Users/e29sw/OneDrive/ドキュメント/GitHub/NanamiEngine"
TEX = REPO + "/Assets/Art/Models/IslandHeart/"
args = sys.argv[sys.argv.index("--") + 1:]
OUT_BLEND, OUT_DIR = args[0], args[1]
# 色 -> (形の乱数の種, テクスチャ)。Green / Light は草原・砂漠へ飛んでいく物語の石 (序章で落ちる島から抜かれる)
COLORS = {
    "Green": (11, "Core_Crystal.png"),
    "Light": (23, "Core_Crystal_Light.png"),
    "Sky": (7, "Core_Crystal_Sky.png"),
    "Pearl": (31, "Core_Crystal_Pearl.png"),
    "Rose": (41, "Core_Crystal_Rose.png"),
}

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)


def material(name, image):
    mat = bpy.data.materials.new(name)
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(TEX + image)
    nt.links.new(tex.outputs[0], bsdf.inputs[0])
    return mat


def add_crystal(bm, rng, base, direction, length, radius, sides=6, tip=0.3):
    """base から direction へ伸びる、先の尖った角柱"""
    d = Vector(direction).normalized()
    up = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
    u = d.cross(up).normalized()
    v = d.cross(u).normalized()
    twist = rng.uniform(0, math.pi)
    body = length * (1.0 - tip)
    rings = []
    for t, r in ((0.0, radius * 0.8), (body, radius)):
        ring = []
        for i in range(sides):
            a = twist + 2 * math.pi * i / sides
            p = Vector(base) + d * t + (u * math.cos(a) + v * math.sin(a)) * r * (1.0 + rng.uniform(-0.12, 0.12))
            ring.append(bm.verts.new(p))
        rings.append(ring)
    apex = bm.verts.new(Vector(base) + d * length + u * rng.uniform(-0.1, 0.1) * radius)
    for i in range(sides):
        j = (i + 1) % sides
        bm.faces.new((rings[0][i], rings[0][j], rings[1][j], rings[1][i]))
        bm.faces.new((rings[1][i], rings[1][j], apex))
    bm.faces.new(list(reversed(rings[0])))


def outward(rng, a, lean):
    return (math.cos(a) * lean, math.sin(a) * lean, -1.0)


def build(color, seed, texture):
    rng = random.Random(seed)
    bm = bmesh.new()
    # 底の尖りを伸ばす太い1本。上半分は岩の中
    add_crystal(bm, rng, (0, 0, 30), (rng.uniform(-0.08, 0.08), rng.uniform(-0.08, 0.08), -1), 78, 6.5)
    # 岩の中の軸から斜め下へ突き抜ける房。上ほど寝かせて長く、下ほど立てて短く
    rings = [(6, 24.0, (0.9, 1.3), (34, 44), (3.8, 5.0)),
             (5, 10.0, (0.55, 0.85), (34, 42), (3.4, 4.4)),
             (4, -6.0, (0.3, 0.5), (18, 26), (2.2, 3.0))]
    for count, z, lean, length, radius in rings:
        offset = rng.uniform(0, 2 * math.pi)
        for i in range(count):
            a = offset + 2 * math.pi * i / count + rng.uniform(-0.25, 0.25)
            base = (math.cos(a) * 1.5, math.sin(a) * 1.5, z + rng.uniform(-3, 3))
            add_crystal(bm, rng, base, outward(rng, a, rng.uniform(*lean)), rng.uniform(*length), rng.uniform(*radius))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

    name = f"IslandFloatingStone_{color}"
    mesh = bpy.data.meshes.new(name)
    uv = bm.loops.layers.uv.new("UVMap")
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        for loop in f.loops:
            co = loop.vert.co
            loop[uv].uv = ((co.y, co.z), (co.x, co.z), (co.x, co.y))[ax]
            loop[uv].uv = (loop[uv].uv[0] / 20.0, loop[uv].uv[1] / 20.0)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(material(f"{color}Crystal", texture))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    print("DIMS", name, tuple(round(v, 2) for v in obj.dimensions), "FACES", len(mesh.polygons))
    return obj


objects = [build(color, seed, texture) for color, (seed, texture) in COLORS.items()]
for i, obj in enumerate(objects):
    obj.location.x = i * 80.0
bpy.ops.wm.save_as_mainfile(filepath=OUT_BLEND)

for obj in objects:
    location = obj.location.copy()
    obj.location = (0, 0, 0)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=f"{OUT_DIR}/{obj.name}.fbx", use_selection=True, apply_unit_scale=False, bake_space_transform=True,
        global_scale=0.01, path_mode="STRIP", object_types={"MESH"}, mesh_smooth_type="FACE")
    obj.location = location
