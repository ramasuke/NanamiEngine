"""スポーン地点から山の棚の野営地へ導く道しるべ。blib.py と assets_camp.py を exec した後に exec する。
一族の目印は赤土で染めた布 (Settle_Cloth)。遠くからでも見える旗竿と、その間を埋める石積み・踏み石、分かれ道の立て札。"""


def trail_banner():
    """一族の旗竿 6m: 横木から赤い布の旗を垂らし、先に角の生えた頭骨を載せる。根元は石で固める。旗は +X 側"""
    begin('TrailBanner')
    top = 6.2
    pole((0, 0, -0.5), (0, 0, top), 0.1, 'Bark', verts=8, taper=0.8)
    # 横木と結び目
    pole((-0.2, 0, 5.7), (1.35, 0, 5.72), 0.05, 'Bark', verts=6)
    lashing((0, 0, 5.71), 0.14, 0.18)
    # 旗: 上を横木に留め、風で少しはらんで裾が燕尾に割れる
    sheet([(0.12, 0.0, 3.35), (1.22, 0.0, 3.3), (1.25, 0.02, 5.66), (0.1, 0.02, 5.66)], 'Cloth', subdiv=6,
          sag=-0.08, noise=0.02, thickness=0.03, grain_uv=(1.1, 2.4))
    for x0, x1, drop in ((0.12, 0.64, 0.55), (0.7, 1.22, 0.7)):
        sheet([(x0 + 0.05, 0.0, 3.35 - drop), (x1 - 0.05, 0.0, 3.35 - drop * 0.6), (x1, 0.0, 3.36), (x0, 0.0, 3.36)],
              'Cloth', subdiv=2, noise=0.015, thickness=0.03, grain_uv=(0.5, 0.6))
    # 横木の端から垂らす吹き流し
    for x in (1.3, -0.15):
        sheet([(x - 0.05, 0.03, 5.65), (x + 0.05, 0.03, 5.65), (x + 0.07, 0.08, 4.6), (x - 0.06, 0.08, 4.6)], 'Cloth',
              subdiv=2, thickness=0.02, grain_uv=(0.1, 1.0))
    # 先の頭骨と角
    box((0.34, 0.42, 0.26), (0, 0.04, top - 0.05), mat='Bone', jitter=0.03)
    box((0.24, 0.3, 0.12), (0, 0.3, top - 0.08), (12, 0, 0), mat='Bone', jitter=0.02)
    for sx in (-1, 1):
        box((0.07, 0.05, 0.07), (sx * 0.1, 0.24, top + 0.1), mat='Iron')
        horn((sx * 0.16, 0.0, top + 0.18), (sx * 1.0, 0.1, 0.4), 0.6, 0.06, 0.6, 'Bone')
    # 根元の石
    for i in range(6):
        a = 2 * math.pi * i / 6 + random.uniform(-0.2, 0.2)
        stone((math.cos(a) * 0.4, math.sin(a) * 0.4, 0.05), random.uniform(0.5, 0.62), flat=0.6)
    stone((0.12, -0.2, 0.3), 0.45, flat=0.6)
    collider((0, 0, 0.25), (1.1, 1.1, 0.5))
    collider((0, 0, 3.0), (0.3, 0.3, 6.0))
    return finalize('TrailBanner')


def trail_cairn():
    """石積みの目印 1m。赤い布を結んだ枝を差す"""
    begin('TrailCairn')
    layers = ((4, 0.3, 0.62, 0.1), (3, 0.16, 0.5, 0.36), (1, 0.0, 0.46, 0.6), (1, 0.0, 0.32, 0.8))
    for count, radius, size, z in layers:
        for i in range(count):
            a = 2 * math.pi * i / count + random.uniform(-0.3, 0.3)
            stone((math.cos(a) * radius, math.sin(a) * radius, z), size, flat=0.6)
    stick_top = Vector((0.14, 0.05, 1.75))
    pole((0.02, 0.0, 0.6), stick_top, 0.03, 'Bark', verts=5)
    sheet([(0.1, 0.06, 1.68), (0.2, 0.06, 1.7), (0.27, 0.14, 1.0), (0.15, 0.12, 1.0)], 'Cloth', subdiv=2,
          noise=0.01, thickness=0.02, grain_uv=(0.12, 0.7))
    lashing((0.13, 0.05, 1.69), 0.045, 0.07)
    collider((0, 0, 0.5), (1.1, 1.1, 1.0))
    return finalize('TrailCairn')


def _arrow_board(length, width, mat, tip):
    """+X を向いた矢羽形の板 (先端が尖る)。原点は板の根元の中心"""
    bm = bmesh.new()
    t = 0.05
    body = length - tip
    outline = [(0.0, -width / 2), (body, -width / 2), (length, 0.0), (body, width / 2), (0.0, width / 2)]
    lower = [bm.verts.new((x, -t / 2, z)) for x, z in outline]
    upper = [bm.verts.new((x, t / 2, z)) for x, z in outline]
    bm.faces.new(lower)
    bm.faces.new(list(reversed(upper)))
    for i in range(len(outline)):
        j = (i + 1) % len(outline)
        bm.faces.new((lower[i], lower[j], upper[j], upper[i]))
    return _finish(bm, mat, (0, 0, 0), (0, 0, 0), 0)


def trail_sign():
    """分かれ道の立て札。+X の板は野営地 (赤い布を巻く)、-53 度の板は村の跡 (焼けて爪痕が走り、傾いている)"""
    begin('TrailSign')
    pole((0, 0, -0.4), (0, 0, 2.9), 0.09, 'Bark', verts=8, taper=0.85)
    camp = _arrow_board(1.4, 0.3, 'Wood', 0.3)
    camp.location = (0.05, 0.0, 2.35)
    lashing((0, 0, 2.35), 0.13, 0.36)
    # 野営地の板の先に巻いた赤い布と、垂らした吹き流し
    cyl(0.07, 0.34, (1.0, 0.0, 2.35), (0, 90, 0), mat='Cloth', verts=8)
    sheet([(0.98, 0.07, 2.3), (1.1, 0.07, 2.3), (1.14, 0.12, 1.55), (1.0, 0.12, 1.6)], 'Cloth', subdiv=2,
          thickness=0.02, grain_uv=(0.12, 0.75))
    # 村の跡の板: 焼けて割れ、爪痕が3本
    village = _arrow_board(1.25, 0.28, 'Burnt', 0.28)
    village.location = (0.05, 0.0, 1.8)
    village.rotation_euler = Euler((math.radians(10), math.radians(-9), math.radians(-53)), 'XYZ')
    rot = village.rotation_euler.to_matrix()
    for k in range(3):
        a = Vector((0.35 + k * 0.12, 0.03, -0.1)) + Vector((0, 0, 0))
        b = a + Vector((0.3, 0.0, 0.2))
        pa = rot @ a + Vector((0.05, 0.0, 1.8))
        pb = rot @ b + Vector((0.05, 0.0, 1.8))
        beam(pa, pb, 0.035, 0.012, 'Iron')
    lashing((0, 0, 1.8), 0.12, 0.3)
    # 根元の石
    for i in range(4):
        a = 2 * math.pi * i / 4 + 0.4
        stone((math.cos(a) * 0.32, math.sin(a) * 0.32, 0.04), 0.45, flat=0.6)
    collider((0, 0, 1.3), (0.35, 0.35, 2.7))
    return finalize('TrailSign')


def trail_stones():
    """踏み石: 平たい石を 3m ほど並べて半分埋める。当たり判定なし。並ぶ向きは X"""
    begin('TrailStones')
    x = -1.3
    while x < 1.4:
        stone((x, random.uniform(-0.3, 0.3), 0.0), random.uniform(0.75, 0.95), flat=0.25,
              rot=(random.uniform(-4, 4), random.uniform(-4, 4), random.uniform(0, 360)))
        x += random.uniform(0.8, 1.0)
    return finalize('TrailStones')


TRAIL_BUILDERS = [trail_banner, trail_cairn, trail_sign, trail_stones]
