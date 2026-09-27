# Builds the GrassLand island cliff skirt in Blender (GrassLandCliff.blend -> GrassLandCliff.mv1).
# grassland_cliff_ring.json = the terrain's perimeter vertices in engine units, dumped from GrassLand.mv1.
# Engine coords (X, Y up, Z) are placed in Blender at (X, Z, Y); DxLib's import mirrors it back.
import bpy, bmesh, json, math, os
from mathutils import Vector, noise

S = os.path.dirname(os.path.abspath(__file__))
ring = json.load(open(S + "/grassland_cliff_ring.json"))
pts = ring["points"]; svals = ring["s"]
N = len(pts)
C = 750.0
HALF = 750.0

DEPTHS = [0, 1.5, 4.5] + [8 + 7 * i for i in range(40)] + [300 + 40 * i for i in range(16)]
TOP_BAND_ROWS = 2          # rows using the terrain texture
ROCK_TILE = 55.0           # world units per rock texture repeat
TIP_Y = -1050.0


def smooth(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def outward(x, z):
    # gradient of a superellipse: edge normals on the sides, diagonal at the corners
    m = 8.0
    dx = (x - C) / HALF; dz = (z - C) / HALF
    gx = math.copysign(abs(dx) ** (m - 1), dx); gz = math.copysign(abs(dz) ** (m - 1), dz)
    l = math.hypot(gx, gz) or 1.0
    return gx / l, gz / l


def superellipse_r(dirx, dirz, a, m):
    # radius of |x/a|^m + |z/a|^m = 1 along the unit direction
    return a / ((abs(dirx) ** m + abs(dirz) ** m) ** (1.0 / m))


def place(i, d):
    x, y, z = pts[i]
    ny = y - d
    nx, nz = outward(x, z)
    # rounded outline further down
    rx, rz = x - C, z - C
    r = math.hypot(rx, rz)
    ux, uz = rx / r, rz / r
    w = smooth(30, 420, d)
    r_round = superellipse_r(ux, uz, HALF * 1.03, 3.2)
    r_mix = r * (1 - w) + r_round * w
    # taper toward the underside tip
    taper = 1.0 - 0.93 * smooth(120, 900, d) ** 1.2
    r_mix *= taper
    px, pz = C + ux * r_mix, C + uz * r_mix
    # small outward bulge just below the rim
    bulge = 14.0 * math.sin(math.pi * min(d / 140.0, 1.0)) if d < 140 else 0.0
    # rocky displacement
    amp = 20.0 * smooth(3, 25, d) * (1.0 - 0.5 * smooth(400, 900, d))
    p = Vector((x * 0.011, ny * 0.02, z * 0.011))
    cols = Vector((x * 0.035, ny * 0.0045, z * 0.035))
    n = noise.fractal(cols, 0.7, 2.1, 4) * 0.8 + noise.fractal(p, 0.8, 2.2, 3) * 0.45
    strata = 0.18 * math.sin(ny * 0.12 + noise.noise(p * 0.5) * 3.0)
    disp = bulge + amp * (n + strata)
    # blend displacement direction: rim normal near the top, radial further down
    ox = nx * (1 - w) + ux * w; oz = nz * (1 - w) + uz * w
    ol = math.hypot(ox, oz) or 1.0
    ox /= ol; oz /= ol
    if d == 0:
        return x, y, z
    if d < 5:
        lip = 3.5 if d < 2 else 2.5
        return x + nx * lip, ny, z + nz * lip
    return px + ox * disp, ny + amp * 0.25 * noise.noise(p * 2.0), pz + oz * disp


# ---- build
for o in [o for o in bpy.data.objects if o.name.startswith("GrassLandCliff")]:
    bpy.data.objects.remove(o, do_unlink=True)

me = bpy.data.meshes.new("GrassLandCliff")
ob = bpy.data.objects.new("GrassLandCliff", me)
bpy.context.scene.collection.objects.link(ob)
bm = bmesh.new()
uv = bm.loops.layers.uv.new("UVMap")

rows = []
for d in DEPTHS:
    row = []
    for i in range(N):
        X, Y, Z = place(i, d)
        row.append(bm.verts.new((X, Z, Y)))
    rows.append(row)
tip = bm.verts.new((C, C, TIP_Y))

mat_top = bpy.data.materials.get("CliffTop") or bpy.data.materials.new("CliffTop")
mat_rock = bpy.data.materials.get("CliffRock") or bpy.data.materials.new("CliffRock")
me.materials.append(mat_top)
me.materials.append(mat_rock)

cu = ring["uv_u"]; cv = ring["uv_v"]
PERIM = 6000.0


def terrain_uv(v):
    # v is the Blender vertex: (engine X, engine Z, engine Y)
    return (min(max(v.co.x / 1500.0, 0.0005), 0.9995), min(max(v.co.y / 1500.0, 0.0005), 0.9995))


def rock_uv(i, k, wrap):
    s = svals[i] if not wrap else PERIM
    return (s / ROCK_TILE, -DEPTHS[k] / ROCK_TILE)


for k in range(len(DEPTHS) - 1):
    for i in range(N):
        j = (i + 1) % N
        wrap = j == 0
        # engine-outward winding; the Y/Z swap mirrors, so check normals below
        f = bm.faces.new((rows[k][i], rows[k][j], rows[k + 1][j], rows[k + 1][i]))
        top = k < TOP_BAND_ROWS or (k == TOP_BAND_ROWS and noise.noise(Vector((i * 0.21, 0, 0))) > 0.1)
        f.material_index = 0 if top else 1
        for loop, (vi, kk, wr) in zip(f.loops, [(i, k, False), (j, k, wrap), (j, k + 1, wrap), (i, k + 1, False)]):
            loop[uv].uv = terrain_uv(loop.vert) if top else rock_uv(vi, kk, wr)
last = len(DEPTHS) - 1
for i in range(N):
    j = (i + 1) % N
    f = bm.faces.new((rows[last][i], rows[last][j], tip))
    f.material_index = 1
    for loop, (vi, wr) in zip(f.loops, [(i, False), (j, j == 0), (i, False)]):
        loop[uv].uv = rock_uv(vi, last, wr)
    f.loops[2][uv].uv = ((svals[i] + 3) / ROCK_TILE, -(DEPTHS[last] + 150) / ROCK_TILE)

bm.normal_update()
# make every face point away from the island's vertical axis (Blender XY = engine XZ)
flipped = 0
for f in bm.faces:
    c = f.calc_center_median()
    radial = Vector((c.x - C, c.y - C, 0.0))
    if f.normal.dot(radial) < 0:
        f.normal_flip(); flipped += 1
bm.to_mesh(me)
bm.free()
print("verts", len(me.vertices), "faces", len(me.polygons), "flipped", flipped)
