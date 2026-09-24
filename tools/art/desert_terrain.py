"""砂漠ステージの地形の高さとテクスチャを作る。メッシュは desert_terrain_blender.py が Blender で組む。

    python tools/art/desert_terrain.py <work>        # data/desert_terrain.npz と <work>/tex/Desert*.png

- 範囲は草原と同じ world 0..1500 (x, z)、257x257 の格子。遊べるのは 250..1250 で、外側は台地へせり上がる景色。
- npz の height は world 単位そのまま (草原の npz と違い、倍率も底上げも掛けない)。
  各マスは (i+1,j)-(i,j+1) の対角線で2枚に割る (diag = 2)。material はマスごとの材質 (0 砂 / 1 干上がった泥 / 2 石畳)。
- 地形の Model は scale 0.08 で置く (Blender = world / 8 の m 単位で書き出すので、ほかの小物と同じ world = m x 8)。

配置の目安 (world):
  ポータルと到着      (380, 1200)
  隊商の野営地        (420, 1000)   オアシスの西の平地
  オアシス            (600, 1040)   干上がりかけのくぼ地。中心に小さな水たまり
  竜の骨              (1100, 900)   東の砂丘の尾根
  サソリの砂丘        (800, 800)    中央
  ワームの砂海        (430, 640)    西の大きな砂丘
  城塞 (故郷の島)     (650..1150, 280..560) 北。砂に半分埋もれた壁
  神殿前の広場        (880, 420)    骸竜と光の浮遊石。石畳
"""
import math
import os
import sys
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
NPZ = HERE / 'data' / 'desert_terrain.npz'

N = 257
SIZE = 1500.0
PLAY_MIN, PLAY_MAX = 250.0, 1250.0
BASE = 60.0

SPAWN = (380.0, 1200.0)
CAMP = (420.0, 1000.0)
OASIS = (600.0, 1040.0)
BONES = (1100.0, 900.0)
PLAZA = (880.0, 420.0)
WORM_SEA = (430.0, 640.0)

MAT_SAND, MAT_MUD, MAT_PAVING = 0, 1, 2


def fbm(x, z, seed, scale, octaves=4):
    """格子上の滑らかなノイズ (周期は気にしない。地形は1枚なので)"""
    r = np.random.default_rng(seed)
    out = np.zeros_like(x)
    amp, total, s = 1.0, 0.0, scale
    for _ in range(octaves):
        g = r.random((int(SIZE / s) + 3, int(SIZE / s) + 3))
        fx, fz = x / s, z / s
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


def smooth_mask(x, z, c, r_in, r_out):
    """中心 c から r_in までは 1、r_out で 0 へ滑らかに落ちる"""
    d = np.hypot(x - c[0], z - c[1])
    t = np.clip((r_out - d) / (r_out - r_in), 0, 1)
    return t * t * (3 - 2 * t)


def dunes(x, z):
    """風下側が急な砂丘。向きの違う3本の波を足し、ノイズで稜線を曲げる"""
    warp = fbm(x, z, 11, 220) * 90
    h = np.zeros_like(x)
    for ang, wl, amp in ((20, 210, 1.0), (65, 140, 0.55), (-30, 90, 0.3)):
        a = math.radians(ang)
        p = (x * math.cos(a) + z * math.sin(a) + warp) / wl
        f = p - np.floor(p)
        # のこぎり波を丸めて、ゆるい風上 (0..0.75) と急な風下 (0.75..1)
        prof = np.where(f < 0.75, f / 0.75, (1 - f) / 0.25)
        h += amp * prof * prof * (3 - 2 * prof)
    return h


def heights():
    t = np.linspace(0.0, SIZE, N)
    x, z = np.meshgrid(t, t)          # [j, i] = (z, x)
    h = BASE + dunes(x, z) * 16 + fbm(x, z, 3, 180) * 18

    # 西のワームの砂海は大きくうねる
    h += smooth_mask(x, z, WORM_SEA, 60, 260) * (dunes(x * 0.7 + 300, z * 0.7) * 22 - 8)
    # 竜の骨が横たわる尾根
    h += smooth_mask(x, z, BONES, 40, 200) * 28

    # 平らにする所: (中心, 内, 外, 目標の高さ)
    for c, r_in, r_out, target in ((SPAWN, 60, 130, BASE + 6), (CAMP, 70, 150, BASE + 4),
                                   (OASIS, 90, 180, BASE + 2), (PLAZA, 150, 260, BASE + 10)):
        m = smooth_mask(x, z, c, r_in, r_out)
        h = h * (1 - m) + target * m
    # オアシスは浅いすり鉢。真ん中に水たまりが残っている
    h -= smooth_mask(x, z, OASIS, 10, 95) * 12

    # 城塞の帯は砂が吹き溜まって少し高い
    band = np.clip(1 - np.abs(z - 420) / 160, 0, 1) * np.clip((x - 560) / 80, 0, 1) * np.clip((1250 - x) / 80, 0, 1)
    h += band * 8 * (1 - smooth_mask(x, z, PLAZA, 150, 260))

    # 遊べる範囲の外は台地へせり上がる (崖の手前の斜面。崖そのものは Mesa/Cliff の小物で作る)
    ox = np.maximum(PLAY_MIN - x, 0) + np.maximum(x - PLAY_MAX, 0)
    oz = np.maximum(PLAY_MIN - z, 0) + np.maximum(z - PLAY_MAX, 0)
    out = np.hypot(ox, oz)
    rise = np.clip(out / 200, 0, 1)
    h += rise * rise * (170 + fbm(x, z, 5, 120) * 80)

    mat = np.full((N - 1, N - 1), MAT_SAND, np.int8)
    cc = (t[:-1] + t[1:]) / 2
    cx, cz = np.meshgrid(cc, cc)
    mat[np.hypot(cx - OASIS[0], cz - OASIS[1]) < 85] = MAT_MUD
    plaza = (np.hypot(cx - PLAZA[0], cz - PLAZA[1]) < 125) & (fbm(cx, cz, 9, 40) < 0.12)
    mat[plaza] = MAT_PAVING
    return h.astype(np.float32), mat


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
        t = y - i0
        t = t * t * (3 - 2 * t)
        i1 = (i0 + 1) % c
        i0 = i0 % c
        a = g[i0][:, i0] * (1 - t)[None, :] + g[i0][:, i1] * t[None, :]
        b = g[i1][:, i0] * (1 - t)[None, :] + g[i1][:, i1] * t[None, :]
        out += amp * (a * (1 - t)[:, None] + b * t[:, None])
        total += amp
        amp *= 0.5
        c *= 2
    return out / total


def voronoi(count, seed):
    """周期境界の Voronoi の (最近点距離, 2番目との差)"""
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


def sand_tex():
    base = np.array([0.80, 0.63, 0.42])
    n = tile_noise(4, 21) - 0.5
    grain = np.random.default_rng(22).random((TN, TN)) - 0.5
    y = np.arange(TN)[:, None] / TN
    xw = np.arange(TN)[None, :] / TN + (tile_noise(6, 23) - 0.5) * 0.08
    ripple = np.sin((y * 14 + xw * 2) * 2 * math.pi + (tile_noise(8, 24) - 0.5) * 3)
    shade = 1 + n * 0.22 + grain * 0.07 + ripple * 0.045
    return base[None, None, :] * shade[..., None]


def mud_tex():
    base = np.array([0.52, 0.40, 0.29])
    d1, edge = voronoi(90, 31)
    crack = np.clip(1 - edge / 5.0, 0, 1)
    n = tile_noise(8, 32) - 0.5
    grain = np.random.default_rng(33).random((TN, TN)) - 0.5
    shade = (1 + n * 0.25 + grain * 0.05) * (1 - crack * 0.55) * (1 - np.clip(d1 / 90, 0, 1) * 0.08)
    return base[None, None, :] * shade[..., None]


def paving_tex():
    sand = sand_tex()
    stone = np.array([0.76, 0.62, 0.46])
    k = 4
    yy, xx = np.mgrid[0:TN, 0:TN]
    cell = TN // k
    row = yy // cell
    xs = (xx + (row % 2) * cell // 2) % TN
    fx, fy = (xs % cell) / cell, (yy % cell) / cell
    joint = np.minimum.reduce([fx, 1 - fx, fy, 1 - fy])
    j = np.clip(joint / 0.035, 0, 1)
    tid = (xs // cell) + row * k
    tone = np.random.default_rng(41).random(k * k * 2)[tid % (k * k * 2)] * 0.18 - 0.09
    n = tile_noise(8, 42) - 0.5
    stone_rgb = stone[None, None, :] * (1 + tone + n * 0.2)[..., None]
    stone_rgb = stone_rgb * (0.55 + 0.45 * j)[..., None]
    drift = np.clip((tile_noise(3, 43) - 0.42) * 5, 0, 1)
    return stone_rgb * (1 - drift[..., None]) + sand * drift[..., None]


def save(path, rgb):
    Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8), 'RGB').save(path, quality=90)
    print('wrote', path)


def main():
    work = Path(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.environ.get('TEMP', '/tmp'), 'nanami_desert'))
    h, mat = heights()
    NPZ.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(NPZ, height=h, material=mat, diag=np.full((N - 1, N - 1), 2, np.int8))
    print(f'wrote {NPZ}  height {h.min():.1f}..{h.max():.1f}  mud {int((mat == MAT_MUD).sum())} '
          f'paving {int((mat == MAT_PAVING).sum())} cells')
    tex = work / 'tex'
    tex.mkdir(parents=True, exist_ok=True)
    save(tex / 'DesertSand.jpg', sand_tex())
    save(tex / 'DesertMud.jpg', mud_tex())
    save(tex / 'DesertPaving.jpg', paving_tex())


if __name__ == '__main__':
    main()
