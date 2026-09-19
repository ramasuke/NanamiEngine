"""荒れた村 (ティラノに踏み荒らされ、焼けた集落)。blib.py を exec した後に exec する"""


def rubble_scatter(cx, cy, rx, ry, count, smin, smax, zmax=0.0, mat='Stone'):
    for _ in range(count):
        a = random.uniform(0, 2 * math.pi)
        d = math.sqrt(random.random())
        x, y = cx + math.cos(a) * rx * d, cy + math.sin(a) * ry * d
        s = random.uniform(smin, smax)
        z = zmax * (1 - d) * random.uniform(0.3, 1.0)
        stone((x, y, z - s * 0.2), s, mat)


def ruined_stone_house():
    """8 x 6 m の石造りの家。右の壁は崩れ落ち、屋根は焼け落ちて垂木と茅の塊だけが残る"""
    begin('RuinedStoneHouse')
    t = 0.6
    # 奥の壁 (y=+3): 一番高く残る。左寄りに切妻の名残
    box((8.0, t, 3.3), (0, 3.0, -0.4), mat='Stone', top_jag=(10, 1.4))
    box((2.6, t, 1.5), (-1.6, 3.0, 2.3), mat='Stone', top_jag=(5, 1.2))
    # 左の壁 (x=-4): 奥から手前へ低くなる
    box((6.0, t, 2.8), (-4.0, 0.0, -0.4), (0, 0, 90), mat='Stone', top_jag=(8, 1.6))
    # 手前の壁 (y=-3): 真ん中に戸口。右側は低く崩れる
    box((3.0, t, 2.9), (-2.5, -3.0, -0.4), mat='Stone', top_jag=(5, 1.0))
    box((2.8, t, 1.7), (2.6, -3.0, -0.4), mat='Stone', top_jag=(5, 1.1))
    beam((-1.0, -3.0, 2.2), (0.9, -3.0, 1.7), 0.25, 0.3, 'Burnt')  # 落ちかけた楣
    # 右の壁 (x=+4): 腰の高さまで崩れ、外側に石が崩れ落ちている
    box((6.0, t, 1.3), (4.0, 0.0, -0.4), (0, 0, 90), mat='Stone', top_jag=(8, 1.0))
    rubble_scatter(5.2, 0.2, 1.4, 3.0, 26, 0.25, 0.6, 0.7)
    rubble_scatter(1.5, -0.5, 1.8, 1.8, 10, 0.2, 0.45, 0.3)
    # 焼けた柱と梁
    beam((-3.4, 2.4, -0.2), (-3.4, 2.4, 3.4), 0.3, 0.3, 'Burnt')
    beam((3.4, 2.4, -0.2), (3.4, 2.4, 2.1), 0.3, 0.3, 'Burnt')
    beam((-3.4, -2.4, -0.2), (-3.35, -2.5, 2.6), 0.3, 0.3, 'Burnt')
    beam((-3.6, 2.5, 3.1), (1.2, 2.6, 2.2), 0.28, 0.32, 'Burnt')  # 片側が落ちた桁
    beam((3.0, -1.8, 0.1), (-1.5, 1.6, 0.25), 0.3, 0.3, 'Burnt', roll=20)  # 床に落ちた梁
    # 奥の壁から床へ落ちた垂木 + 茅の塊
    for i, x in enumerate((-2.6, -1.5, -0.3, 0.9)):
        beam((x, 2.7, 3.0 - i * 0.15), (x + random.uniform(-0.4, 0.4), -0.6, 0.1), 0.16, 0.2, 'Burnt')
    sheet([(-2.9, 0.4, 0.3), (-0.1, 0.8, 0.35), (-0.3, 2.4, 2.3), (-3.1, 2.4, 2.7)], 'Thatch', subdiv=6,
          sag=0.3, noise=0.08, thickness=0.18, holes=[(0.5, 1.0, 0.0, 0.35), (0.0, 0.3, 0.6, 0.85)])
    sheet([(1.6, -2.4, 0.15), (3.3, -2.2, 0.1), (3.2, -0.8, 0.6), (1.4, -0.9, 0.5)], 'Thatch', subdiv=4,
          sag=0.1, noise=0.08, thickness=0.14)
    # 当たり判定: 壁4面 (低い所に合わせる)
    collider((0, 3.0, 1.1), (8.0, t, 3.0))
    collider((-4.0, 0.0, 0.9), (t, 6.0, 2.6))
    collider((-2.5, -3.0, 0.9), (3.0, t, 2.6))
    collider((2.6, -3.0, 0.4), (2.8, t, 1.6))
    collider((4.0, 0.0, 0.2), (t, 6.0, 1.2))
    return finalize('RuinedStoneHouse')


def ruined_timber_house():
    """6 x 5 m の木造の家。石の基礎の上に柱と板壁が一部残り、屋根の片面が前へずり落ちている"""
    begin('RuinedTimberHouse')
    # 基礎
    box((6.4, 0.5, 0.9), (0, 2.5, -0.4), mat='Stone', top_jag=(6, 0.3))
    box((6.4, 0.5, 0.9), (0, -2.5, -0.4), mat='Stone', top_jag=(6, 0.35))
    box((5.0, 0.5, 0.9), (-3.0, 0, -0.4), (0, 0, 90), mat='Stone', top_jag=(5, 0.3))
    box((5.0, 0.5, 0.9), (3.0, 0, -0.4), (0, 0, 90), mat='Stone', top_jag=(5, 0.45))
    # 柱
    for (x, y, h, m) in ((-3.0, 2.5, 3.4, 'Wood'), (0.0, 2.5, 3.3, 'Burnt'), (3.0, 2.5, 2.2, 'Burnt'),
                         (-3.0, -2.5, 3.2, 'Wood'), (3.0, -2.5, 1.2, 'Burnt')):
        beam((x, y, 0.2), (x, y, h), 0.26, 0.26, m)
    beam((-3.0, 2.5, 3.3), (0.1, 2.5, 3.3), 0.24, 0.28, 'Wood')  # 桁
    beam((0.0, 2.5, 3.2), (2.8, 2.5, 2.0), 0.24, 0.28, 'Burnt')
    # 奥の板壁 (歯抜け)
    for i in range(14):
        x = -2.8 + i * 0.4
        if i in (5, 9, 10, 12):
            continue
        top = 3.2 if x < 0 else 3.2 - (x) * 0.45
        top -= random.uniform(0, 0.5)
        box((0.36, 0.06, top - 0.45), (x, 2.62, 0.45), mat='Wood' if x < 0.5 else 'Burnt', grain_axis=2)
    # 左の妻壁: 板と三角の妻
    for i in range(11):
        y = -2.3 + i * 0.44
        if i in (3, 7):
            continue
        top = 3.3 + (2.5 - abs(y)) * 0.55 - random.uniform(0, 0.3)
        box((0.4, 0.06, top - 0.45), (-3.1, y, 0.45), (0, 0, 90), mat='Wood', grain_axis=2)
    beam((-3.0, -2.6, 3.3), (-3.0, 0.0, 4.7), 0.22, 0.22, 'Wood')
    beam((-3.0, 2.6, 3.3), (-3.0, 0.0, 4.7), 0.22, 0.22, 'Wood')
    # 手前へずり落ちた屋根 (板 + 茅)。穴が開いている
    roof = [(-3.3, -4.2, 0.2), (2.4, -4.0, 0.1), (2.2, 0.6, 3.0), (-3.2, 0.4, 3.6)]
    sheet(roof, 'Thatch', subdiv=8, sag=0.25, noise=0.07, thickness=0.22,
          holes=[(0.3, 0.62, 0.4, 0.85), (0.72, 1.0, 0.12, 0.38), (0.0, 0.14, 0.7, 1.0)])
    for i in range(6):
        x = -3.0 + i * 1.0
        beam((x, -4.0, 0.25), (x + 0.1, 0.5, 3.3 - i * 0.1), 0.14, 0.18, 'Burnt')
    # 落ちた棟木と、室内に散らばる焼けた板
    beam((-2.8, 0.2, 0.1), (2.5, -0.8, 0.9), 0.26, 0.26, 'Burnt', roll=10)
    for _ in range(7):
        x, y = random.uniform(-2.3, 2.3), random.uniform(-1.8, 2.0)
        a = random.uniform(0, math.pi)
        l = random.uniform(1.0, 2.2)
        box((l, 0.35, 0.05), (x, y, 0.1 + random.uniform(0, 0.15)), (random.uniform(-10, 10), random.uniform(-8, 8), math.degrees(a)), mat='Burnt')
    rubble_scatter(3.6, -2.4, 1.0, 1.0, 8, 0.2, 0.45, 0.3)
    collider((0, 2.55, 1.4), (6.4, 0.5, 3.6))
    collider((-3.05, 0, 1.4), (0.5, 5.4, 3.6))
    collider((0, -2.5, 0.0), (6.4, 0.5, 0.9))
    collider((3.0, 0, 0.0), (0.5, 5.4, 0.9))
    # ずり落ちた屋根: 手前へ傾いた板
    d = Vector(roof[3]) - Vector(roof[0])
    ang = math.degrees(math.atan2(d.z, d.y))
    collider(((roof[0][0] + roof[1][0]) / 2, (roof[0][1] + roof[3][1]) / 2, (roof[0][2] + roof[3][2]) / 2 - 0.1),
             (5.6, d.length, 0.3), (ang, 0, 0))
    return finalize('RuinedTimberHouse')


def crushed_hut():
    """直径 4.5m の丸い小屋。円い石垣の上の円錐屋根が片側へ潰れている"""
    begin('CrushedHut')
    hs = [random.uniform(0.6, 1.3) for _ in range(18)]
    for k in range(5, 9):
        hs[k] = random.uniform(0.1, 0.35)
    ring_wall(2.2, hs, 0.5, (0, 0, 0), 'Stone', segs=18)
    rubble_scatter(1.6, 2.4, 1.2, 0.9, 12, 0.2, 0.45, 0.4)
    # 潰れた屋根: 傾けて片側を地面に着ける
    cone_shell(3.1, 1.6, 0.3, loc=(-0.3, -0.2, -0.25), mat='Thatch', segs=18, u_repeat=6.0,
               v_range=(0.0, 1.0), thickness=0.22, sag=0.55, tilt=(24, -12, 0), noise=0.1,
               holes=[(190, 290, 0.25, 1.0), (40, 75, 0.5, 1.0)])
    sheet([(1.8, -2.6, 0.05), (3.4, -2.2, 0.1), (3.0, -1.0, 0.2), (1.6, -1.3, 0.12)], 'Thatch', subdiv=3,
          sag=-0.15, noise=0.1, thickness=0.2)
    for a in range(0, 360, 45):
        r = math.radians(a)
        p0 = (math.cos(r) * 2.7 - 0.4, math.sin(r) * 2.7 - 0.3, 0.3 + (0.9 if a in (90, 135) else 0.0))
        p1 = (-0.9 + random.uniform(-0.3, 0.3), -0.6 + random.uniform(-0.3, 0.3), 1.6 + random.uniform(0, 0.9))
        pole(p0, p1, 0.07, 'Burnt', verts=6)
    collider((0, 0, 0.8), (4.6, 4.6, 1.9))
    return finalize('CrushedHut')


def broken_fence():
    """6m の柵。一本は傾き、一本は折れ、横木の片方は途中で折れて垂れている"""
    begin('BrokenFence')
    posts = [(-3.0, 1.25, 0), (-1.5, 1.2, 0), (0.0, 0.75, 0), (1.5, 1.25, 22), (3.0, 1.1, -8)]
    tops = []
    for x, h, lean in posts:
        p0 = (x, 0, -0.3)
        p1 = (x + math.sin(math.radians(lean)) * h, 0.05 * lean / 20, h)
        pole(p0, p1, 0.09, 'Bark', verts=7, sharpen=0.12 if h > 1.0 else 0.0)
        tops.append(p1)
    pole((-3.1, 0.1, 1.0), (-1.4, 0.1, 0.95), 0.06, 'Bark')
    pole((-1.4, 0.1, 0.95), (0.05, 0.1, 0.62), 0.06, 'Bark')
    pole((-3.1, 0.1, 0.5), (0.1, 0.1, 0.48), 0.06, 'Bark')
    pole((0.1, 0.12, 0.62), (1.2, 0.3, 0.02), 0.06, 'Bark')  # 折れて垂れた横木
    pole((1.75, 0.1, 0.9), (3.1, 0.1, 0.92), 0.06, 'Bark')
    pole((1.6, 0.1, 0.45), (3.1, 0.1, 0.5), 0.06, 'Bark')
    pole((0.3, -0.6, 0.08), (1.9, -0.1, 0.12), 0.07, 'Bark')  # 地面に落ちた横木
    collider((0, 0.05, 0.55), (6.2, 0.3, 1.3))
    return finalize('BrokenFence')


def rubble_pile():
    """崩れた石と焼けた梁の山 (直径 3m)"""
    begin('RubblePile')
    for _ in range(26):
        a = random.uniform(0, 2 * math.pi)
        d = math.sqrt(random.random())
        s = random.uniform(0.25, 0.65)
        stone((math.cos(a) * 1.5 * d, math.sin(a) * 1.3 * d, (1 - d) * 0.9 - s * 0.2), s)
    beam((-1.4, -0.6, 0.0), (0.8, 0.9, 1.4), 0.26, 0.26, 'Burnt', roll=12)
    beam((1.5, -0.5, 0.0), (-0.3, 0.3, 1.0), 0.22, 0.24, 'Burnt')
    for _ in range(4):
        box((random.uniform(0.9, 1.6), 0.3, 0.05), (random.uniform(-1, 1), random.uniform(-1, 1), random.uniform(0.2, 0.7)),
            (random.uniform(-25, 25), random.uniform(-25, 25), random.uniform(0, 180)), mat='Burnt')
    collider((0, 0, 0.3), (2.6, 2.2, 0.9))
    return finalize('RubblePile')


def wheel(center, radius, axis_rot, mat='Wood'):
    """車輪: 輪 + 8本のスポーク + ハブ。axis_rot はローカル Z(車軸) の向き"""
    rot = Euler([math.radians(a) for a in axis_rot], 'XYZ').to_matrix()
    c = Vector(center)
    segs = 12
    pts = []
    for i in range(segs):
        a = 2 * math.pi * i / segs
        pts.append(c + rot @ Vector((math.cos(a) * radius, math.sin(a) * radius, 0)))
    for a, b in zip(pts, pts[1:] + pts[:1]):
        beam(a, b, 0.12, 0.1, mat)
    for i in range(0, segs, 2):
        beam(c, pts[i], 0.06, 0.06, mat)
    hub = rot @ Vector((0, 0, 0.12))
    pole(c - hub, c + hub, 0.1, mat, verts=8)


def broken_cart():
    """横倒しの荷車。片輪は外れて地面に倒れ、梶棒は折れている"""
    begin('BrokenCart')
    # 荷台を長手(X)まわりに 75 度倒す
    tilt = Euler((math.radians(-75), 0, math.radians(8)), 'XYZ').to_matrix()
    base = Vector((0, 0.1, 0.62))

    def at(v):
        return base + tilt @ Vector(v)
    for (sx, sy, sz, x, y, z) in ((2.4, 1.3, 0.08, 0, 0, 0), (2.4, 0.07, 0.55, 0, -0.62, 0.05), (2.4, 0.07, 0.45, 0, 0.62, 0.05),
                                  (0.07, 1.3, 0.5, -1.18, 0, 0.05)):
        box((sx, sy, sz), at((x, y, z)), tilt, mat='Wood')
    wheel(at((0.5, -0.8, -0.1)), 0.55, (math.degrees(-75) + 90, 0, 8))
    wheel((1.6, -1.6, 0.08), 0.55, (0, 0, 20))
    pole(at((-1.2, 0.3, -0.05)), (-2.6, 0.5, 0.05), 0.06, 'Wood')
    pole(at((-1.2, -0.3, -0.05)), (-2.0, -0.4, 0.4), 0.06, 'Wood')
    pole((-2.1, -0.5, 0.05), (-3.0, -0.9, 0.02), 0.06, 'Wood')
    # こぼれた木箱と袋
    box((0.6, 0.6, 0.5), (0.6, 1.3, 0.0), (0, 0, 25), mat='Wood', jitter=0.02)
    box((0.55, 0.5, 0.45), (-0.4, 1.5, 0.0), (8, -12, 60), mat='Burnt')
    for (x, y) in ((1.4, 0.9), (1.1, 1.8)):
        stone((x, y, 0.1), 0.55, 'Rope', flat=0.5)
    collider((0, 0.1, 0.65), (2.6, 1.0, 1.35), (0, 0, 8))
    return finalize('BrokenCart')


RUIN_BUILDERS = [ruined_stone_house, ruined_timber_house, crushed_hut, broken_fence, rubble_pile, broken_cart]
