"""山の上の狩猟民のキャンプ。blib.py を exec した後に exec する。正面(入口・見張りの向き)は +Y"""


def lashing(p, radius=0.11, height=0.18):
    """柱の結び目 (縄を巻いた太り)"""
    cyl(radius, height, (p[0], p[1], p[2] - height / 2), mat='Rope', verts=8)


def hide_tent():
    """皮張りの円錐テント。直径 4.6m、頂点から柱が突き出す。入口は +Y"""
    begin('HideTent')
    r, h = 2.3, 4.3
    cone_shell(r, h, 0.32, mat='Hide', segs=22, door=(90, 44, 0.42), u_repeat=4.0, v_range=(0.0, 1.0),
               thickness=0.05, sag=0.07, noise=0.015)
    # 入口の垂れ幕: 横へめくって柱に留めた三角
    sheet([(-0.55, 2.05, 0.0), (-1.25, 1.7, 0.0), (-0.95, 1.7, 1.2), (-0.3, 2.15, 1.75)], 'Hide', subdiv=3,
          sag=-0.05, thickness=0.04)
    top = Vector((0, 0, h - 0.25))
    for i in range(11):
        a = 2 * math.pi * (i + 0.3) / 11
        base = Vector((math.cos(a) * (r + 0.05), math.sin(a) * (r + 0.05), -0.1))
        d = (top - base).normalized()
        end = top + d * random.uniform(0.9, 1.4) + Vector((random.uniform(-0.1, 0.1), random.uniform(-0.1, 0.1), 0))
        pole(base, end, 0.055, 'Bark', verts=6, taper=0.6)
    lashing(top + Vector((0, 0, 0.1)), 0.18, 0.25)
    # 裾の杭
    for i in range(14):
        a = 2 * math.pi * (i + 0.5) / 14
        if abs(math.degrees(a) - 90) < 25:
            continue
        p = Vector((math.cos(a) * (r + 0.25), math.sin(a) * (r + 0.25), -0.2))
        pole(p, p + Vector((math.cos(a) * 0.12, math.sin(a) * 0.12, 0.45)), 0.035, 'Wood', verts=5)
    collider((0, 0, 1.4), (3.2, 3.2, 2.8))
    return finalize('HideTent')


def lean_to():
    """差し掛け小屋: 二股の柱に棟木を渡し、後ろへ地面まで皮と枝を葺く。中に毛皮の寝床"""
    begin('LeanTo')
    w = 3.6
    for x in (-w / 2, w / 2):
        pole((x, 0.0, -0.2), (x, 0.0, 2.1), 0.08, 'Bark')
        pole((x, 0.0, 1.8), (x - 0.2 if x < 0 else x + 0.2, 0.05, 2.3), 0.05, 'Bark')
        lashing((x, 0.0, 1.95), 0.11)
    pole((-w / 2 - 0.4, 0.0, 2.08), (w / 2 + 0.4, 0.0, 2.1), 0.07, 'Bark')
    for i in range(7):
        x = -w / 2 + i * w / 6
        pole((x, 0.1, 2.15), (x + random.uniform(-0.1, 0.1), -2.5, -0.15), 0.045, 'Bark', verts=6)
    sheet([(-w / 2 - 0.1, -2.3, 0.05), (w / 2 + 0.1, -2.3, 0.05), (w / 2 + 0.1, 0.05, 2.12), (-w / 2 - 0.1, 0.05, 2.12)],
          'Hide', subdiv=6, sag=0.12, thickness=0.05)
    # 屋根の上に押さえの枝
    for i in range(3):
        y = -0.6 - i * 0.6
        z = 2.12 * (1 - (-y) / 2.35)
        pole((-w / 2 - 0.3, y, z + 0.12), (w / 2 + 0.3, y + random.uniform(-0.1, 0.1), z + 0.12), 0.04, 'Bark', verts=5)
    # 寝床の毛皮と、枕元の丸めた皮
    sheet([(-1.4, -1.5, 0.04), (0.2, -1.6, 0.04), (0.3, -0.1, 0.05), (-1.3, 0.0, 0.05)], 'Hide', subdiv=3, noise=0.03,
          thickness=0.06)
    cyl(0.2, 1.2, (0.9, -1.2, 0.2), (0, 90, 10), mat='Hide', verts=10)
    collider((0, -1.15, 0.9), (w + 0.4, 2.4, 1.8))
    return finalize('LeanTo')


def campfire():
    """石組みの炉 + 組んだ薪 + 三脚に吊った鍋 + 腰掛けの丸太2本"""
    begin('Campfire')
    for i in range(12):
        a = 2 * math.pi * i / 12
        stone((math.cos(a) * 0.72, math.sin(a) * 0.72, 0.02), 0.32, flat=0.7)
    sheet([(-0.6, -0.6, 0.03), (0.6, -0.6, 0.03), (0.6, 0.6, 0.03), (-0.6, 0.6, 0.03)], 'Ash', subdiv=3,
          sag=-0.06, thickness=0.02)
    for i in range(5):
        a = 2 * math.pi * i / 5 + 0.3
        p0 = (math.cos(a) * 0.55, math.sin(a) * 0.55, 0.05)
        pole(p0, (math.cos(a) * 0.05, math.sin(a) * 0.05, 0.55), 0.06, 'Burnt', verts=6)
    # 三脚と鍋
    apex = Vector((0, 0, 1.7))
    for i in range(3):
        a = 2 * math.pi * i / 3 + 0.5
        base = Vector((math.cos(a) * 1.0, math.sin(a) * 1.0, -0.1))
        pole(base, apex + (apex - base).normalized() * 0.25, 0.045, 'Bark', verts=6)
    lashing(apex, 0.08, 0.14)
    pole((0, 0, 1.65), (0, 0, 1.05), 0.012, 'Rope', verts=4, smooth=False)
    cyl(0.28, 0.32, (0, 0, 0.72), mat='Iron', verts=12, radius_top=0.33)
    # 腰掛けの丸太
    for (x, y, a) in ((0.0, -1.9, 5), (1.8, 0.4, 80)):
        r = math.radians(a)
        dx, dy = math.cos(r) * 0.95, math.sin(r) * 0.95
        pole((x - dx, y - dy, 0.18), (x + dx, y + dy, 0.2), 0.2, 'Bark', verts=9)
    # 焼き串の肉
    pole((-0.9, 0.9, 0.0), (-0.2, 0.2, 0.75), 0.02, 'Wood', verts=4)
    for k in range(3):
        t = 0.55 + k * 0.12
        p = Vector((-0.9, 0.9, 0.0)).lerp(Vector((-0.2, 0.2, 0.75)), t)
        stone(p, 0.16, 'Meat', flat=0.9)
    return finalize('Campfire')


def drying_rack():
    """皮なめしの干し台: A 字の脚に横木を渡し、枠に張った皮と干し肉を吊る"""
    begin('DryingRack')
    for x in (-1.6, 1.6):
        pole((x, -0.7, -0.15), (x, 0.0, 2.2), 0.06, 'Bark')
        pole((x, 0.7, -0.15), (x, 0.0, 2.2), 0.06, 'Bark')
        lashing((x, 0.0, 2.05), 0.1)
    pole((-2.0, 0.0, 2.08), (2.0, 0.0, 2.1), 0.055, 'Bark')
    pole((-1.7, 0.0, 1.2), (1.7, 0.0, 1.22), 0.045, 'Bark')
    # 干し肉と毛皮
    for i in range(9):
        x = -1.35 + i * 0.34
        l = random.uniform(0.35, 0.6)
        box((0.09, 0.03, l), (x, 0.02, 2.05 - l), (0, random.uniform(-6, 6), 0), mat='Meat')
    sheet([(-1.3, 0.05, 0.25), (0.1, 0.05, 0.2), (0.15, 0.05, 1.15), (-1.35, 0.05, 1.18)], 'Hide', subdiv=3, noise=0.03,
          thickness=0.03)
    # 枠に張った皮 (立て掛け)
    for (x, y, a) in ((2.6, 0.9, 15), (-2.7, 1.0, -20)):
        rot = Euler((math.radians(-18), 0, math.radians(a)), 'XYZ').to_matrix()
        o = Vector((x, y, 0.0))
        c = [o + rot @ Vector(v) for v in ((-0.7, 0, 0.1), (0.7, 0, 0.1), (0.7, 0, 1.7), (-0.7, 0, 1.7))]
        for p0, p1 in zip(c, c[1:] + c[:1]):
            pole(p0 + (p0 - p1).normalized() * 0.15, p1 + (p1 - p0).normalized() * 0.15, 0.035, 'Bark', verts=5)
        inset = [o + rot @ Vector(v) for v in ((-0.55, 0.02, 0.25), (0.55, 0.02, 0.25), (0.55, 0.02, 1.55), (-0.55, 0.02, 1.55))]
        sheet(inset, 'Hide', subdiv=3, sag=-0.04, thickness=0.02)
    collider((0, 0, 1.0), (3.6, 1.5, 2.1))
    return finalize('DryingRack')


def horn(base, direction, length, radius, curl, mat='Bone'):
    """曲がった角: 短い丸太を曲げながら継ぐ"""
    p = Vector(base)
    d = Vector(direction).normalized()
    up = Vector((0, 0, 1))
    n = 5
    for k in range(n):
        r0 = radius * (1 - k / n)
        seg = length / n
        nd = (d + up * curl).normalized()
        q = p + nd * seg
        pole(p, q, max(r0, 0.015), mat, verts=6, taper=(1 - (k + 1) / n) / max(1 - k / n, 1e-3) if k < n - 1 else 0.2)
        p, d = q, nd


def totem():
    """見張りのトーテム: 彫った柱の上に角の生えた頭骨、ティラノの鉤爪と皮の吹き流しを吊る"""
    begin('Totem')
    pole((0, 0, -0.3), (0, 0, 4.2), 0.26, 'Wood', verts=10, taper=0.85)
    # 彫り (段と張り出し)
    for z, s in ((0.9, 0.34), (1.9, 0.33), (2.9, 0.31)):
        cyl(s, 0.22, (0, 0, z), mat='Burnt', verts=10)
        box((0.26, 0.2, 0.28), (0, 0.27, z + 0.35), mat='Burnt')  # 鼻
        for sx in (-0.13, 0.13):
            box((0.1, 0.1, 0.08), (sx, 0.29, z + 0.66), mat='Hide')  # 目 (赤土)
    # 横木と吹き流し
    pole((-1.1, 0, 3.4), (1.1, 0, 3.45), 0.06, 'Bark')
    lashing((0, 0, 3.42), 0.3, 0.2)
    for x in (-0.9, -0.5, 0.55, 0.95):
        sheet([(x - 0.08, 0.02, 3.4), (x + 0.08, 0.02, 3.4), (x + 0.1 + random.uniform(-0.1, 0.1), 0.1, 2.3),
               (x - 0.1, 0.1, 2.3)], 'Hide', subdiv=2, thickness=0.02)
    # 鉤爪 (大きく曲がった円錐)
    horn((1.0, 0.0, 3.3), (0.1, 0.2, -1.0), 0.9, 0.14, -0.35, 'Bone')
    # 頭骨: 箱を歪ませた顔 + 顎 + 角
    box((0.6, 0.75, 0.45), (0, 0.1, 4.15), mat='Bone', jitter=0.04)
    box((0.42, 0.55, 0.22), (0, 0.55, 4.05), (12, 0, 0), mat='Bone', jitter=0.03)
    for sx in (-1, 1):
        box((0.12, 0.08, 0.12), (sx * 0.17, 0.46, 4.35), mat='Iron')  # 眼窩
        horn((sx * 0.28, 0.0, 4.4), (sx * 1.0, 0.2, 0.3), 1.0, 0.1, 0.6, 'Bone')
    collider((0, 0, 2.0), (0.7, 0.7, 4.6))
    return finalize('Totem')


def palisade():
    """尖らせた杭の防柵 6m。外(-Y)へ少し傾け、横木2本で縛る"""
    begin('Palisade')
    n = 15
    for i in range(n):
        x = -3.0 + 6.0 * i / (n - 1) + random.uniform(-0.05, 0.05)
        h = random.uniform(2.1, 2.8)
        lean = math.radians(random.uniform(10, 16))
        top = (x + random.uniform(-0.05, 0.05), -math.sin(lean) * h, math.cos(lean) * h)
        pole((x, 0.0, -0.4), top, random.uniform(0.11, 0.15), 'Bark', verts=7, sharpen=0.35)
    for z in (0.6, 1.6):
        off = -math.tan(math.radians(13)) * z
        pole((-3.2, off + 0.15, z), (3.2, off + 0.15, z + 0.03), 0.06, 'Bark')
    # 支え (内側 +Y)
    for x in (-2.0, 0.5, 2.6):
        pole((x, 1.3, -0.2), (x, -0.1, 1.5), 0.07, 'Bark')
    collider((0, -0.2, 1.1), (6.3, 0.5, 2.4), (-13, 0, 0))
    return finalize('Palisade')


def lookout():
    """見張り台: 4本柱に 2.4m 四方の床 (高さ 2.8m)、手摺、梯子、皮の日除け。見張る向きは -Y (盆地側へ向ける)"""
    begin('Lookout')
    hgt = 2.8
    s = 1.2
    for (x, y) in ((-s, -s), (s, -s), (s, s), (-s, s)):
        pole((x * 1.15, y * 1.15, -0.3), (x, y, hgt + 1.1), 0.1, 'Bark')
    # 床
    for i in range(9):
        y = -s - 0.1 + i * (2 * s + 0.2) / 8
        pole((-s - 0.25, y, hgt), (s + 0.25, y, hgt + 0.02), 0.075, 'Bark', verts=6)
    pole((-s, -s, hgt - 0.12), (-s, s, hgt - 0.12), 0.07, 'Bark')
    pole((s, -s, hgt - 0.12), (s, s, hgt - 0.12), 0.07, 'Bark')
    # 筋交い
    for (a, b) in (((-s * 1.12, -s * 1.12, 0.3), (s * 1.05, -s * 1.05, hgt - 0.3)),
                   ((s * 1.12, s * 1.12, 0.3), (-s * 1.05, s * 1.05, hgt - 0.3)),
                   ((-s * 1.12, s * 1.12, 0.3), (-s * 1.05, -s * 1.05, hgt - 0.3))):
        pole(a, b, 0.05, 'Bark', verts=5)
    # 手摺 (梯子側の +Y は空ける)
    for z in (hgt + 0.55, hgt + 1.0):
        pole((-s, -s, z), (s, -s, z), 0.04, 'Bark', verts=5)
        pole((-s, -s, z), (-s, s, z), 0.04, 'Bark', verts=5)
        pole((s, -s, z), (s, s, z), 0.04, 'Bark', verts=5)
    # 梯子
    lx, ly = 0.35, s + 1.0
    pole((-lx, ly, -0.1), (-lx, s + 0.05, hgt + 0.6), 0.05, 'Bark', verts=6)
    pole((lx, ly, -0.1), (lx, s + 0.05, hgt + 0.6), 0.05, 'Bark', verts=6)
    for k in range(7):
        t = (k + 0.7) / 8
        y = ly + (s + 0.05 - ly) * t
        z = -0.1 + (hgt + 0.6 + 0.1) * t
        pole((-lx - 0.05, y, z), (lx + 0.05, y, z), 0.03, 'Wood', verts=5)
    # 日除け: 後ろの柱の上から前へ
    for (x, y) in ((-s, -s), (s, -s), (s, s), (-s, s)):
        pass
    pole((-s - 0.2, s, hgt + 2.2), (s + 0.2, s, hgt + 2.2), 0.05, 'Bark')
    pole((-s, s, hgt + 1.1), (-s, s, hgt + 2.3), 0.07, 'Bark')
    pole((s, s, hgt + 1.1), (s, s, hgt + 2.3), 0.07, 'Bark')
    sheet([(-s - 0.3, -s - 0.4, hgt + 1.55), (s + 0.3, -s - 0.4, hgt + 1.55), (s + 0.3, s + 0.05, hgt + 2.25),
           (-s - 0.3, s + 0.05, hgt + 2.25)], 'Hide', subdiv=4, sag=0.12, thickness=0.04)
    # 立て掛けた槍
    pole((s + 0.1, -s - 0.3, hgt + 0.02), (s - 0.3, -s + 0.1, hgt + 2.0), 0.025, 'Wood', verts=5, sharpen=0.25)
    collider((0, 0, hgt + 0.05), (2 * s + 0.5, 2 * s + 0.4, 0.2))
    for (x, y) in ((-s, -s), (s, -s), (s, s), (-s, s)):
        collider((x * 1.07, y * 1.07, hgt / 2), (0.25, 0.25, hgt))
    return finalize('Lookout')


def supply_pile():
    """物資: 籠、丸めた毛皮、槍の束、薪の山"""
    begin('SupplyPile')
    for (x, y, r, h) in ((-1.2, 0.3, 0.38, 0.6), (-0.5, 0.8, 0.3, 0.5)):
        cyl(r, h, (x, y, -0.05), mat='Thatch', verts=12, radius_top=r * 1.2)
        cyl(r * 1.22, 0.06, (x, y, h - 0.08), mat='Wood', verts=12)
    for (x, y, a) in ((0.4, 0.9, 5), (0.5, 0.45, -8), (0.45, 0.68, 0)):
        cyl(0.22, 1.1, (x - 0.55, y, 0.2 if y != 0.68 else 0.55), (0, 90, a), mat='Hide', verts=10)
    # 薪の山
    for k in range(4):
        for j in range(5 - k):
            y = -0.9 + j * 0.3 + k * 0.15
            pole((0.3, y, 0.15 + k * 0.26), (1.5, y + random.uniform(-0.05, 0.05), 0.15 + k * 0.26), 0.13, 'Bark', verts=7)
    # 槍の束 (立て掛け)
    for k in range(5):
        pole((-1.6 + k * 0.08, -0.7, -0.1), (-1.2 + k * 0.1, -0.2 + random.uniform(-0.1, 0.1), 2.3), 0.025, 'Wood',
             verts=5, sharpen=0.22)
    lashing((-1.26, -0.3, 1.8), 0.1, 0.12)
    collider((0.0, 0.0, 0.5), (3.2, 2.2, 1.1))
    return finalize('SupplyPile')


CAMP_BUILDERS = [hide_tent, lean_to, campfire, drying_rack, totem, palisade, lookout, supply_pile]
