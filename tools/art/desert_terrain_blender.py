"""data/desert_terrain.npz から地形のメッシュを組み、FBX に書き出す (Blender を -b で動かす)。

    blender -b --factory-startup --python tools/art/desert_terrain_blender.py -- <work>
    python -m tools.model convert <work>/fbx/DesertTerrain.fbx <out>.mv1 --mode mesh --with-textures

- Blender の (X, Y, Z) = world の (x, z, h) / 8 の m 単位。シーンでは Model を scale 0.08 で置けば world に戻る。
- 各マスは (i+1,j)-(i,j+1) の対角線で割る (npz の diag と同じ)。
- 材質はマスごと: 砂 / 干上がった泥 / 石畳。UV は world 座標をそれぞれの繰り返し長で割る。
- テクスチャ (<work>/tex/Desert*.jpg、desert_terrain.py が作る) を FBX の横へ写す。
"""
import os
import shutil
import sys

import bpy
import numpy as np

WORK = sys.argv[sys.argv.index('--') + 1]
NPZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data', 'desert_terrain.npz')
FBX_DIR = os.path.join(WORK, 'fbx')
SIZE = 1500.0
TO_M = 1.0 / 8.0

# 材質: (名前, テクスチャ, 繰り返し長 world)
MATERIALS = [('DesertTerrain_Sand', 'DesertSand.jpg', 64.0),
             ('DesertTerrain_Mud', 'DesertMud.jpg', 48.0),
             ('DesertTerrain_Paving', 'DesertPaving.jpg', 96.0)]


def flip_normals_for_dxlib(me):
    """DxLibModelViewer は Blender の FBX の法線の上下 (Blender の Z) を反転して読む。先に反転しておくと、上向きの面が
    正しく光を受ける (反転しないと地面は自己発光だけで暗く、壁の上面も黒くなる)"""
    me.update()
    normals = [(-0.0 + v.normal.x, v.normal.y, -v.normal.z) for v in me.vertices]
    me.normals_split_custom_set_from_vertices(normals)


def main():
    os.makedirs(FBX_DIR, exist_ok=True)
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    d = np.load(NPZ)
    h, mat = d['height'].astype(np.float64), d['material']
    n = h.shape[0]
    step = SIZE / (n - 1)

    verts = [(i * step * TO_M, j * step * TO_M, h[j, i] * TO_M) for j in range(n) for i in range(n)]
    faces, face_mat = [], []
    for j in range(n - 1):
        for i in range(n - 1):
            a, b, c, e = j * n + i, j * n + i + 1, (j + 1) * n + i, (j + 1) * n + i + 1
            faces += [(a, b, c), (b, e, c)]
            face_mat += [int(mat[j, i])] * 2

    me = bpy.data.meshes.new('DesertTerrain')
    me.from_pydata(verts, [], faces)
    me.update()
    obj = bpy.data.objects.new('DesertTerrain', me)
    bpy.context.scene.collection.objects.link(obj)

    for name, tex, _ in MATERIALS:
        shutil.copy2(os.path.join(WORK, 'tex', tex), os.path.join(FBX_DIR, tex))
        m = bpy.data.materials.new(name)
        m.use_nodes = True
        bsdf = next(nd for nd in m.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        t = m.node_tree.nodes.new('ShaderNodeTexImage')
        t.image = bpy.data.images.load(os.path.join(FBX_DIR, tex))
        m.node_tree.links.new(t.outputs['Color'], bsdf.inputs['Base Color'])
        me.materials.append(m)

    uv = me.uv_layers.new(name='UVMap')
    for poly, mi in zip(me.polygons, face_mat):
        poly.material_index = mi
        poly.use_smooth = True
        rep = MATERIALS[mi][2] * TO_M
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            uv.data[li].uv = (co.x / rep, co.y / rep)

    flip_normals_for_dxlib(me)

    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    units = bpy.context.scene.unit_settings
    units.system, units.scale_length, units.length_unit = 'METRIC', 1.0, 'METERS'
    bpy.ops.export_scene.fbx(filepath=os.path.join(FBX_DIR, 'DesertTerrain.fbx'), use_selection=True,
                             object_types={'MESH'}, path_mode='STRIP', embed_textures=False,
                             mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)
    print(f'TERRAIN verts={len(verts)} tris={len(faces)}')


main()
