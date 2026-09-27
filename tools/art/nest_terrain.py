"""古竜の巣 (終章「嵐の巣」、docs/Story.md) の地形の高さとテクスチャを作る。メッシュは nest_models_blender.py が Blender で組む。

    python tools/art/nest_terrain.py <work>        # data/nest_terrain.npz と <work>/tex/Nest*.png

巣は嵐の目に浮かぶ黒い岩の島。中心 CENTER の周りがすり鉢 (巣) で、外輪は岩の爪 (NestSpire) が内へ反って囲む。
- 範囲は草原・砂漠と同じ world 0..1500 (x, z) で 257x257 の格子。島の縁は edge_radius(θ) (約 560) で、その外は崖になって落ちる。
  npz の height は world 単位そのまま。material はマスごと (0 玄武岩 / 1 灰 / 2 骨の散った灰)。
- 地形の Model は scale 0.08 で置く (Blender = world / 8 の m 単位)。

配置の目安 (world。+z が南):
  着き場 (LEDGE)        (750, 1245)   南の縁の岩棚。拠点の島が横付けする所 (いまはポータルで着く)
  坂道                   着き場から北へ、外輪の切れ目を下って巣の底へ
  巣の底                 半径 330 のすり鉢。古竜と戦う場所
  心臓の山 (MOUND)       (750, 720)    底の中央の小山。奪われた心臓が積まれ、光の柱が立つ
  骸の輪                 心臓の山から半径 190..240。心臓を戻されて起き上がりかけた竜の骨
"""
import math
import os
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
NPZ = HERE / 'data' / 'nest_terrain.npz'

N = 257
SIZE = 1500.0
CENTER = (750.0, 750.0)
BASE = 100.0            # 巣の底の高さ
MOUND = (750.0, 720.0)
MOUND_HEIGHT = 24.0
LEDGE = (750.0, 1245.0)
LEDGE_HEIGHT = BASE + 46.0
BOWL_RADIUS = 330.0
RIM_RADIUS = 450.0      # 外輪の尾根
EDGE_RADIUS = 560.0     # 島の縁 (角度で揺らす)
RAMP_HALF_WIDTH = 42.0

MAT_ROCK, MAT_ASH, MAT_BONES = 0, 1, 2


def fbm(x, z, seed, scale, octaves=4):
    r = np.random.default_rng(seed)
    out = np.zeros_like(x, dtype=np.float64)
    amp, total, s = 1.0, 0.0, scale
    for _ in range(octaves):
        g = r.random((int(SIZE / s) + 3, int(SIZE / s) + 3))
        fx, fz = np.clip(x, 0, SIZE) / s, np.clip(z, 0, SIZE) / s
        i0, j0 = np.floor(fx).astype(int), np.floor(fz).astype(int)
        tx, tz = fx - i0, fz - j0
        tx, tz = tx * tx * (3 - 2 * tx), tz * tz * (3 - 2 * tz)
        a = g[j0, i0] * (1 - tx) + g[j0, i0 + 1] * tx
        b = g[j0 + 1, i0] * (1 - tx) + g[j0 + 1, i0 + 1] * tx
        out += amp * (a * (1 - tz) + b * tz)
        total += amp
        amp *= 0.5
        s /= 2
    return out / total - 0.5


def angular_noise(theta, seed, terms=((3, 0.5), (5, 0.3), (9, 0.2), (17, 0.1))):
    """角度だけの滑らかな揺らぎ (-1..1 くらい)。周期は 2π でつながる"""
    r = np.random.default_rng(seed)
    out = np.zeros_like(theta, dtype=np.float64)
    for k, amp in terms:
        out += amp * np.sin(k * theta + r.uniform(0, 2 * math.pi))
    return out


def edge_radius(theta):
    """島の縁の半径。南 (着き場) は少し張り出す"""
    south = np.exp(-((np.angle(np.exp(1j * (theta - math.pi / 2)))) / 0.35) ** 2)
    return EDGE_RADIUS + angular_noise(theta, 7) * 38.0 + south * 30.0


def smooth(t):
    t = np.clip(t, 0, 1)
    return t * t * (3 - 2 * t)


def smooth_mask(x, z, c, r_in, r_out):
    d = np.hypot(x - c[0], z - c[1])
    return smooth((r_out - d) / (r_out - r_in))


def ramp_mask(x, z):
    """着き場から巣の底へ下りる坂の帯 (中心線は x = 750、z 1210..1060)。1 = 坂の上"""
    across = smooth((RAMP_HALF_WIDTH + 18 - np.abs(x - LEDGE[0])) / 18)
    along = smooth((z - (CENTER[1] + BOWL_RADIUS - 40)) / 30) * smooth((LEDGE[1] + 40 - z) / 30)
    return across * along


def heights(x, z):
    dx, dz = x - CENTER[0], z - CENTER[1]
    r = np.hypot(dx, dz)
    theta = np.arctan2(dz, dx)

    # すり鉢: 底は BASE。外輪へ向けてせり上がり、尾根を越えると縁へ下がる
    rim_h = 78 + angular_noise(theta, 3) * 26 + fbm(x, z, 4, 90) * 30
    t_in = smooth((r - BOWL_RADIUS) / (RIM_RADIUS - BOWL_RADIUS))
    t_out = smooth((r - RIM_RADIUS) / (edge_radius(theta) - RIM_RADIUS))
    h = BASE + fbm(x, z, 5, 60) * 5 + rim_h * t_in * t_in * (1 - t_out) + t_out * (34 + fbm(x, z, 6, 70) * 18)
    # 底はゆるく中央へ下がる
    h -= smooth(1 - r / BOWL_RADIUS) * 8

    # 心臓の山: 天辺は平ら (心臓を積む)
    h += smooth_mask(x, z, MOUND, 34, 95) * (MOUND_HEIGHT + fbm(x, z, 8, 30) * 3) - smooth_mask(x, z, MOUND, 0, 26) * 2.0

    # 着き場の岩棚と坂道
    ledge = smooth_mask(x, z, LEDGE, 62, 100)
    h = h * (1 - ledge) + (LEDGE_HEIGHT + fbm(x, z, 9, 40) * 2) * ledge
    along = np.clip((z - (CENTER[1] + BOWL_RADIUS - 40)) / (LEDGE[1] - 60 - (CENTER[1] + BOWL_RADIUS - 40)), 0, 1)
    ramp_h = BASE + (LEDGE_HEIGHT - BASE) * smooth(along)
    m = ramp_mask(x, z) * (1 - ledge)
    h = h * (1 - m) + ramp_h * m
    return h


def build():
    t = np.linspace(0.0, SIZE, N)
    x, z = np.meshgrid(t, t)          # [j, i] = (z, x)
    h = heights(x, z)

    cc = (t[:-1] + t[1:]) / 2
    cx, cz = np.meshgrid(cc, cc)
    ch = heights(cx, cz)
    gx = (heights(cx + 3, cz) - heights(cx - 3, cz)) / 6
    gz = (heights(cx, cz + 3) - heights(cx, cz - 3)) / 6
    slope = np.degrees(np.arctan(np.hypot(gx, gz)))
    r = np.hypot(cx - CENTER[0], cz - CENTER[1])
    mat = np.full((N - 1, N - 1), MAT_ASH, np.int8)
    mat[(slope > 24) | (r > RIM_RADIUS - 60) & (ramp_mask(cx, cz) < 0.5)] = MAT_ROCK
    bones = ((np.hypot(cx - MOUND[0], cz - MOUND[1]) < 110) | ((r > 150) & (r < 290) & (fbm(cx, cz, 12, 50) > 0.08)))
    mat[bones & (slope < 24)] = MAT_BONES
    ledge = np.hypot(cx - LEDGE[0], cz - LEDGE[1]) < 70
    mat[ledge] = MAT_ROCK
    return h.astype(np.float32), mat, ch


# ---------------------------------------------------------------- テクスチャ (1024px, タイル可能)
TN = 1024


def tile_noise(cells, seed, octaves=4):
    r = np.random.default_rng(seed)
    out = np.zeros((TN, TN))
    amp, total, c = 1.0, 0.0, cells
    for _ in range(octaves):
        g = r.random((c, c))
        y = np.arange(TN) / TN * c
        i0 = np.floor(y).astype(int)
        tt = y - i0
        tt = tt * tt * (3 - 2 * tt)
        i1 = (i0 + 1) % c
        i0 = i0 % c
        a = g[i0][:, i0] * (1 - tt)[None, :] + g[i0][:, i1] * tt[None, :]
        b = g[i1][:, i0] * (1 - tt)[None, :] + g[i1][:, i1] * tt[None, :]
        out += amp * (a * (1 - tt)[:, None] + b * tt[:, None])
        total += amp
        amp *= 0.5
        c *= 2
    return out / total


def voronoi(count, seed):
    r = np.random.default_rng(seed)
    pts = r.random((count, 2)) * TN
    yy, xx = np.mgrid[0:TN, 0:TN].astype(np.float32)
    d1 = np.full((TN, TN), 1e9, np.float32)
    d2 = np.full((TN, TN), 1e9, np.float32)
    for px, py in pts:
        for ox in (-TN, 0, TN):
            for oy in (-TN, 0, TN):
                d = np.hypot(xx - px - ox, yy - py - oy)
                d2 = np.minimum(d2, np.maximum(d, d1))
                d1 = np.minimum(d1, d)
    return d1, d2 - d1


def basalt_tex():
    """黒い玄武岩。柱状の割れ目と、うっすら紫がかった面"""
    base = np.array([0.30, 0.29, 0.33])
    d1, edge = voronoi(70, 51)
    crack = np.clip(1 - edge / 4.0, 0, 1)
    n = tile_noise(6, 52) - 0.5
    fine = tile_noise(48, 53) - 0.5
    face = np.random.default_rng(54).random(70)
    shade = (1 + n * 0.35 + fine * 0.25) * (1 - crack * 0.6) * (0.9 + 0.2 * np.clip(d1 / 60, 0, 1))
    rgb = base[None, None, :] * shade[..., None]
    return rgb


def ash_tex():
    """灰と小石の地面"""
    base = np.array([0.36, 0.34, 0.35])
    n = tile_noise(5, 61) - 0.5
    grain = np.random.default_rng(62).random((TN, TN)) - 0.5
    d1, _ = voronoi(160, 63)
    pebble = np.clip(1 - d1 / 7.0, 0, 1)
    shade = 1 + n * 0.3 + grain * 0.08 + pebble * 0.25
    return base[None, None, :] * shade[..., None]


def bonebed_tex():
    """灰の中に白っぽい骨の欠片が散っている"""
    ash = ash_tex()
    r = np.random.default_rng(71)
    yy, xx = np.mgrid[0:TN, 0:TN].astype(np.float32)
    shard = np.zeros((TN, TN), np.float32)
    for _ in range(90):
        cx, cy = r.random(2) * TN
        ang = r.uniform(0, math.pi)
        ln, wd = r.uniform(18, 70), r.uniform(3, 7)
        for ox in (-TN, 0, TN):
            for oy in (-TN, 0, TN):
                u = (xx - cx - ox) * math.cos(ang) + (yy - cy - oy) * math.sin(ang)
                v = -(xx - cx - ox) * math.sin(ang) + (yy - cy - oy) * math.cos(ang)
                d = (u / ln) ** 2 + (v / wd) ** 2
                shard = np.maximum(shard, np.clip(1.5 - d, 0, 1))
    bone = np.array([0.78, 0.74, 0.64])
    n = tile_noise(24, 72) - 0.5
    bone_rgb = bone[None, None, :] * (1 + n * 0.2)[..., None]
    s = np.clip(shard, 0, 1)[..., None]
    return ash * (1 - s) + bone_rgb * s


def cliff_tex():
    """島の底の崖。横の地層"""
    base = np.array([0.27, 0.26, 0.30])
    y = np.arange(TN)[:, None] / TN
    warp = tile_noise(4, 81) - 0.5
    strata = np.sin((y * 9 + warp * 0.6) * 2 * math.pi) * 0.5 + 0.5
    n = tile_noise(8, 82) - 0.5
    fine = tile_noise(64, 83) - 0.5
    shade = 0.85 + strata * 0.25 + n * 0.3 + fine * 0.2
    return base[None, None, :] * shade[..., None]


def storm_tex():
    """嵐の壁の雲。横へ流れる筋と渦"""
    y = np.arange(TN)[:, None] / TN
    x = np.arange(TN)[None, :] / TN
    warp = (tile_noise(3, 91) - 0.5) * 0.35
    streak = tile_noise(6, 92)
    swirl = np.sin((y * 6 + np.sin(x * 2 * math.pi * 3) * 0.08 + warp) * 2 * math.pi) * 0.5 + 0.5
    n = tile_noise(8, 93)
    v = np.clip(0.25 + streak * 0.45 + swirl * 0.18 + (n - 0.5) * 0.4, 0, 1)
    dark = np.array([0.13, 0.14, 0.19])
    light = np.array([0.48, 0.50, 0.60])
    return dark[None, None, :] * (1 - v[..., None]) + light[None, None, :] * v[..., None]


def iron_tex():
    """錆びた鉄"""
    base = np.array([0.34, 0.30, 0.28])
    rust = np.array([0.55, 0.30, 0.16])
    n = tile_noise(6, 101)
    fine = tile_noise(48, 102) - 0.5
    m = np.clip((n - 0.45) * 4, 0, 1)
    rgb = base[None, None, :] * (1 - m[..., None]) + rust[None, None, :] * m[..., None]
    return rgb * (1 + fine * 0.3)[..., None]


def save(path, rgb):
    from PIL import Image   # NOTE: Blender の Python には PIL が無い (nest_models_blender.py が読み込む)
    Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8), 'RGB').save(path, quality=90)
    print('wrote', path)


def main():
    work = Path(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.environ.get('TEMP', '/tmp'), 'nanami_nest'))
    h, mat, _ = build()
    NPZ.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(NPZ, height=h, material=mat, diag=np.full((N - 1, N - 1), 2, np.int8))
    print(f'wrote {NPZ}  height {h.min():.1f}..{h.max():.1f}  rock {int((mat == MAT_ROCK).sum())} '
          f'bones {int((mat == MAT_BONES).sum())} cells')
    tex = work / 'tex'
    tex.mkdir(parents=True, exist_ok=True)
    save(tex / 'NestBasalt.jpg', basalt_tex())
    save(tex / 'NestAsh.jpg', ash_tex())
    save(tex / 'NestBoneBed.jpg', bonebed_tex())
    save(tex / 'NestCliff.jpg', cliff_tex())
    save(tex / 'NestStorm.jpg', storm_tex())
    save(tex / 'NestIron.jpg', iron_tex())


if __name__ == '__main__':
    main()
