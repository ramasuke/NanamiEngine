"""GrassLand の薬草と宝箱のモデル (blib.py の後に exec で読み込む)。単位は m、地面は z=0、正面は +Y。

    g = {'SETTLEMENT_WORK': r'<work>'}
    for f in (r'<repo>/tools/art/settlement/blib.py', r'<repo>/tools/art/loot/assets_loot.py'):
        exec(open(f, encoding='utf-8').read(), g)
    g['reset']()
    for b in g['LOOT_BUILDERS']:
        g['export'](b())                         # <work>/fbx/<Name>.fbx
    # その後 python tools/art/loot/install_models.py <work>

- TreasureChest_Lid は後ろのヒンジが原点。本体(TreasureChest_Base)に対するヒンジ位置は LID_HINGE
- 材質のテクスチャは Settle_Wood.png だけ (<work>/tex に置く)。ほかは単色
"""
import math
import random

import bmesh
from mathutils import Vector

MATERIALS.update({
    'Leaf': (None, 1.0, None, (0.20, 0.50, 0.22)),
    'LeafLight': (None, 1.0, None, (0.46, 0.72, 0.30)),
    'Stem': (None, 1.0, None, (0.24, 0.36, 0.12)),
    'Flower': (None, 1.0, None, (0.96, 0.94, 0.86)),
    'FlowerCore': (None, 1.0, None, (0.95, 0.74, 0.22)),
    'Gold': (None, 1.0, None, (0.80, 0.60, 0.20)),
    'ChestInner': (None, 1.0, None, (0.10, 0.06, 0.04)),
})

CHEST_W, CHEST_D, CHEST_H = 0.95, 0.60, 0.46
LID_R = CHEST_D / 2
LID_HINGE = (0.0, -CHEST_D / 2, CHEST_H)


def leaf(base, yaw, pitch, length, width, mat='Leaf', droop=0.35, segs=6):
    """根元 base から yaw 方向へ伸びる葉。pitch は立ち上がり角(deg)。先へ行くほど droop で垂れる。両面"""
    bm = bmesh.new()
    rows = []
    for i in range(segs + 1):
        t = i / segs
        w = width * math.sin(math.pi * min(1.0, t * 1.05)) ** 0.7 / 2
        rise = math.radians(pitch) - droop * t * t * 2.0
        x = length * t * math.cos(rise)
        z = length * t * math.sin(rise)
        # 葉脈に沿って少し折る
        rows.append((bm.verts.new((x, -w, z - w * 0.25)), bm.verts.new((x, 0, z + w * 0.15)),
                     bm.verts.new((x, w, z - w * 0.25))))
    for a, b in zip(rows, rows[1:]):
        for k in range(2):
            quad = (a[k], b[k], b[k + 1], a[k + 1])
            bm.faces.new(quad)
            bm.faces.new(tuple(bm.verts.new(v.co) for v in reversed(quad)))
    bm.normal_update()
    return _finish(bm, mat, Vector(base), (0, 0, yaw), uv=False, smooth=True)


def blossom(loc, radius, petals=5):
    """小さな白い花 (花びらは平たい玉、中心は黄色)"""
    for k in range(petals):
        a = 2 * math.pi * k / petals + random.uniform(-0.2, 0.2)
        p = Vector(loc) + Vector((math.cos(a) * radius, math.sin(a) * radius, 0))
        stone(p, radius * 1.3, mat='Flower', flat=0.35, rot=(0, 0, math.degrees(a)))
    stone(Vector(loc) + Vector((0, 0, radius * 0.2)), radius * 0.9, mat='FlowerCore', flat=0.6)


def build_herb_patch():
    begin('HerbPatch')
    # 外側の大きい葉 → 内側の明るい若葉
    for k in range(9):
        yaw = k * 40 + random.uniform(-10, 10)
        leaf((0, 0, 0.01), yaw, random.uniform(28, 40), random.uniform(0.34, 0.42), random.uniform(0.13, 0.16))
    for k in range(6):
        yaw = k * 60 + 20 + random.uniform(-12, 12)
        leaf((0, 0, 0.02), yaw, random.uniform(55, 70), random.uniform(0.24, 0.30), random.uniform(0.10, 0.12),
             mat='LeafLight', droop=0.25)
    # 花茎を3本立てて、先に花の房を付ける
    for k in range(3):
        a = math.radians(k * 120 + 35)
        top = Vector((math.cos(a) * 0.08, math.sin(a) * 0.08, random.uniform(0.40, 0.48)))
        pole((0, 0, 0), top, 0.012, mat='Stem', verts=5, taper=0.7)
        for j in range(3):
            b = a + j * 2.1
            blossom(top + Vector((math.cos(b) * 0.035, math.sin(b) * 0.035, random.uniform(-0.01, 0.02))), 0.022)
    return finalize('HerbPatch')


def build_herb_pickup():
    begin('HerbPickup')
    # 葉を4枚束ねて麻ひもで縛った「摘んだ薬草」。横倒しで浮かぶので原点は束の中央
    for k in range(4):
        yaw = -20 + k * 13
        leaf((-0.16, 0, 0), yaw, random.uniform(-6, 10), random.uniform(0.30, 0.36), 0.12,
             mat='Leaf' if k % 2 else 'LeafLight', droop=0.1)
    pole((-0.22, 0, 0), (-0.02, 0, 0.0), 0.018, mat='Stem', verts=6)
    cyl(0.03, 0.035, (-0.12, 0, 0), rot=(0, 90, 0), mat='Rope', verts=8)
    blossom((0.14, 0.02, 0.05), 0.02)
    return finalize('HerbPickup')


def _band(x, mat='Iron'):
    """本体を巻く鉄の帯 (前・後ろ・底は本体から 1cm 浮かせる)"""
    t, w = 0.012, 0.05
    box((w, t, CHEST_H), (x, CHEST_D / 2 + t / 2, 0), mat=mat)
    box((w, t, CHEST_H), (x, -CHEST_D / 2 - t / 2, 0), mat=mat)


def build_chest_base():
    begin('TreasureChest_Base')
    # 上が開いた箱 (底板 + 4枚の壁)。開けた時に中の暗がりと金貨が見える
    t = 0.045
    box((CHEST_W, CHEST_D, 0.06), (0, 0, 0), mat='Wood', grain_axis=0, bevel=0.01)
    for sy in (-1, 1):
        box((CHEST_W, t, CHEST_H), (0, sy * (CHEST_D - t) / 2, 0), mat='Wood', grain_axis=0, bevel=0.008)
    for sx in (-1, 1):
        box((t, CHEST_D - 2 * t, CHEST_H), (sx * (CHEST_W - t) / 2, 0, 0), mat='Wood', grain_axis=1, bevel=0.008)
    box((CHEST_W - 2 * t, CHEST_D - 2 * t, 0.01), (0, 0, 0.06), mat='ChestInner')
    for _ in range(14):
        stone((random.uniform(-0.3, 0.3), random.uniform(-0.18, 0.18), random.uniform(0.20, 0.30)),
              random.uniform(0.06, 0.09), mat='Gold', flat=0.25)
    box((CHEST_W - 2 * t, CHEST_D - 2 * t, 0.2), (0, 0, 0.07), mat='Gold')
    for x in (-CHEST_W * 0.30, CHEST_W * 0.30):
        _band(x)
    # 四隅の金具
    for sx in (-1, 1):
        for sy in (-1, 1):
            box((0.07, 0.07, CHEST_H * 0.98), (sx * (CHEST_W / 2 - 0.02), sy * (CHEST_D / 2 - 0.02), 0), mat='Gold')
    # 錠前の受け
    box((0.12, 0.02, 0.10), (0, CHEST_D / 2 + 0.01, CHEST_H - 0.11), mat='Gold')
    box((0.03, 0.022, 0.035), (0, CHEST_D / 2 + 0.012, CHEST_H - 0.085), mat='ChestInner')
    return finalize('TreasureChest_Base')


def build_chest_lid():
    """かまぼこ形のフタ。ヒンジ(本体の後ろ上端)が原点で、フタは +Y (前) へ伸びる"""
    begin('TreasureChest_Lid')
    # 半円柱: 軸は X、中心は (y=LID_R, z=0)。cyl は +Z へ伸びるので Y 軸回りに 90 度倒す
    arc = cyl(LID_R, CHEST_W, (-CHEST_W / 2, LID_R, 0), rot=(0, 90, 0), mat='Wood', verts=16, smooth=False)
    me = arc.data
    bm = bmesh.new()
    bm.from_mesh(me)
    # 下半分 (ローカル X<0 = 回転後の z<0) を潰して蓋の底にする
    for v in bm.verts:
        if v.co.x > 0:
            v.co.x = 0.0
    bm.to_mesh(me)
    bm.free()
    # 鉄の帯 (半円のアーチ)
    for x in (-CHEST_W * 0.30, CHEST_W * 0.30):
        cyl(LID_R + 0.012, 0.05, (x - 0.025, LID_R, 0), rot=(0, 90, 0), mat='Iron', verts=16, smooth=False)
    for obj in PARTS[-2:]:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        for v in bm.verts:
            if v.co.x > 0:
                v.co.x = 0.0
        bm.to_mesh(obj.data)
        bm.free()
    # 前の錠前金具 (本体側の受けに重なる)
    box((0.14, 0.03, 0.14), (0, CHEST_D + 0.005, -0.07), mat='Gold')
    return finalize('TreasureChest_Lid')


def build_chest_preview():
    """確認用: 本体とフタを組んだ姿 (書き出さない)"""
    base = build_chest_base()
    lid = build_chest_lid()
    lid.location = LID_HINGE
    return base, lid


LOOT_BUILDERS = [build_herb_patch, build_herb_pickup, build_chest_base, build_chest_lid]
