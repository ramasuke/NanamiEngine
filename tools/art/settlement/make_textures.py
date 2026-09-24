"""荒れた村・狩猟民のキャンプ用のタイル可能テクスチャ (1024px)。numpy + PIL だけで描く。

Settle_Stone      : 乱石積み(石 + 目地)
Settle_Wood       : 風化した板(U 方向に木目、V 方向に板の継ぎ目)
Settle_WoodBurnt  : 焼けた木(亀甲状の割れ + 炭の黒)
Settle_Thatch     : 茅葺き(V 方向の藁、段ごとの影)
Settle_Hide       : なめし革(まだら + 縫い目 + 赤土の文様の帯)
Settle_Bark       : 丸太の樹皮(V 方向の縦筋)
Settle_Bone       : 骨・角(黄ばんだ白 + 細かな筋)
Settle_Ash        : 焚き火跡の灰と炭

    python tools/art/settlement/make_textures.py <work>     # <work>/tex/*.png
"""
import os
import sys
from pathlib import Path

import numpy as np
from PIL import Image

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.environ.get('TEMP', '/tmp'), 'nanami_settlement')) / 'tex'
OUT.mkdir(parents=True, exist_ok=True)
N = 1024
rng = np.random.default_rng(7)


def tile_noise(size, cells, seed, octaves=4):
    """タイル可能な value noise (周期 cells)"""
    r = np.random.default_rng(seed)
    out = np.zeros((size, size))
    amp, total = 1.0, 0.0
    c = cells
    for _ in range(octaves):
        g = r.random((c, c))
        y = np.arange(size) / size * c
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


def save(name, rgb):
    arr = (np.clip(rgb, 0, 1) * 255).astype(np.uint8)
    Image.fromarray(arr, 'RGB').save(OUT / f'{name}.png')
    print('wrote', name)


def voronoi_tile(size, count, seed):
    """周期境界の Voronoi: (最近点距離, 2番目との差, セルID)"""
    r = np.random.default_rng(seed)
    pts = r.random((count, 2)) * size
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    d1 = np.full((size, size), 1e9, np.float32)
    d2 = np.full((size, size), 1e9, np.float32)
    idx = np.zeros((size, size), np.int32)
    for k, (px, py) in enumerate(pts):
        dx = np.abs(xx - px)
        dx = np.minimum(dx, size - dx)
        dy = np.abs(yy - py)
        dy = np.minimum(dy, size - dy)
        d = np.sqrt(dx * dx * 0.75 + dy * dy * 1.25)  # 横長の石
        closer = d < d1
        d2 = np.where(closer, d1, np.minimum(d2, d))
        idx = np.where(closer, k, idx)
        d1 = np.where(closer, d, d1)
    return d1, d2 - d1, idx


# ------------------------------------------------------------ 石
def stone():
    d1, edge, idx = voronoi_tile(N, 70, 11)
    r = np.random.default_rng(12)
    tone = r.uniform(0.78, 1.12, 70)[idx]
    hue = r.uniform(-0.04, 0.04, 70)[idx]
    base = np.array([0.56, 0.53, 0.47])
    n1 = tile_noise(N, 16, 3)
    n2 = tile_noise(N, 64, 4)
    col = base[None, None, :] * tone[..., None] * (0.8 + 0.35 * n1[..., None]) * (0.9 + 0.2 * n2[..., None])
    col[..., 0] += hue
    col[..., 2] -= hue * 0.5
    # 石の縁を少し暗く、目地は暗い土色。縁からの距離の勾配で上から光を当てたような膨らみを付ける
    rim = np.clip(edge / 14.0, 0, 1)
    col *= (0.7 + 0.3 * rim)[..., None]
    bulge = np.sqrt(np.clip(edge / 22.0, 0, 1))
    gy = np.roll(bulge, -2, axis=0) - np.roll(bulge, 2, axis=0)
    gx = np.roll(bulge, -2, axis=1) - np.roll(bulge, 2, axis=1)
    light = np.clip(1.0 - 2.2 * gy + 0.8 * gx, 0.55, 1.35)
    col *= light[..., None]
    mortar = edge < 4.5
    mcol = np.array([0.30, 0.27, 0.22]) * (0.8 + 0.4 * n2[..., None])
    col = np.where(mortar[..., None], mcol, col)
    # 苔と煤
    moss = np.clip((tile_noise(N, 8, 5) - 0.58) * 4, 0, 1)
    col = col * (1 - 0.45 * moss[..., None]) + np.array([0.22, 0.30, 0.12]) * 0.45 * moss[..., None]
    soot = np.clip((tile_noise(N, 6, 9) - 0.6) * 3, 0, 1)
    col *= (1 - 0.35 * soot)[..., None]
    save('Settle_Stone', col)


# ------------------------------------------------------------ 木
def plank_field(seed, planks=5, burnt=False):
    r = np.random.default_rng(seed)
    y = np.arange(N)
    x = np.arange(N)
    X, Y = np.meshgrid(x, y)
    band = (Y * planks // N)
    tone = r.uniform(0.82, 1.15, planks)[band]
    shift = r.uniform(0, 1, planks)[band]
    # 木目: U 方向に伸びる縞、板ごとに位相をずらす
    warp = tile_noise(N, 8, seed + 1) * 6.0
    grain = np.sin((Y / N * planks * 18.0 + warp + shift * 10) * np.pi * 2) * 0.5 + 0.5
    fine = tile_noise(N, 128, seed + 2)
    streak = np.repeat(tile_noise(N, 256, seed + 3)[:, :1], N, axis=1)
    base = np.array([0.50, 0.38, 0.26])
    col = base[None, None, :] * tone[..., None] * (0.78 + 0.18 * grain[..., None] + 0.14 * fine[..., None])
    col *= (0.9 + 0.2 * streak[..., None])
    # 風化で灰色寄せ
    gray = col.mean(-1, keepdims=True)
    weather = 0.35 + 0.3 * tile_noise(N, 6, seed + 4)[..., None]
    col = col * (1 - weather) + gray * 1.05 * weather
    # 継ぎ目
    seam = (Y % (N // planks)) < 5
    col = np.where(seam[..., None], col * 0.35, col)
    # 節
    for _ in range(9):
        cx, cy = r.uniform(0, N, 2)
        dx = np.minimum(np.abs(X - cx), N - np.abs(X - cx))
        dy = np.minimum(np.abs(Y - cy), N - np.abs(Y - cy))
        d = np.sqrt((dx / 2.2) ** 2 + dy ** 2)
        k = np.clip(1 - d / 10, 0, 1)
        col *= (1 - 0.55 * k)[..., None]
    if burnt:
        cr, cedge, cidx = voronoi_tile(N, 260, seed + 9)
        crack = np.clip(1 - cedge / 3.0, 0, 1)
        char = np.array([0.07, 0.06, 0.055]) * (0.8 + 0.6 * tile_noise(N, 32, seed + 5)[..., None])
        amount = np.clip(0.55 + 0.6 * (tile_noise(N, 5, seed + 6) - 0.5), 0.25, 1.0)[..., None]
        col = col * (1 - amount) * 0.6 + char * amount
        col = col * (1 - 0.8 * crack[..., None])
        ember = np.clip((tile_noise(N, 12, seed + 7) - 0.72) * 5, 0, 1) * (1 - crack)
        col += np.array([0.22, 0.07, 0.02]) * ember[..., None] * 0.6
    return col


def bark():
    X, Y = np.meshgrid(np.arange(N), np.arange(N))
    warp = tile_noise(N, 6, 21) * 30
    ridges = np.abs(np.sin((X + warp) / N * 40 * np.pi))
    ridges = ridges ** 0.5
    n = tile_noise(N, 48, 22)
    base = np.array([0.36, 0.28, 0.20])
    col = base[None, None, :] * (0.55 + 0.45 * ridges[..., None]) * (0.85 + 0.3 * n[..., None])
    lichen = np.clip((tile_noise(N, 10, 23) - 0.62) * 4, 0, 1)
    col = col * (1 - 0.4 * lichen[..., None]) + np.array([0.45, 0.48, 0.36]) * 0.4 * lichen[..., None]
    save('Settle_Bark', col)


# ------------------------------------------------------------ 茅葺き
def thatch():
    r = np.random.default_rng(31)
    X, Y = np.meshgrid(np.arange(N), np.arange(N))
    rows = 8
    rowh = N // rows
    col = np.zeros((N, N, 3))
    base = np.array([0.66, 0.55, 0.32])
    strands = np.zeros((N, N))
    for layer in range(2):
        # 列ごとの乱数を横に少しだけぼかした藁の束。ノイズで横に揺らす
        cols = r.random(N // 4)
        cols = np.repeat(cols, 4)
        cols = (cols + np.roll(cols, 1) + np.roll(cols, -1)) / 3
        w = (tile_noise(N, 12, 40 + layer) * 18).astype(int)
        xi = (X + w) % N
        strands += cols[xi]
    strands /= 2
    strands = np.clip((strands - 0.2) * 1.4, 0, 1)
    within = (Y % rowh) / rowh  # 0 上端 -> 1 下端(先端)
    shade = 0.55 + 0.45 * within
    tone = r.uniform(0.85, 1.1, rows)[(Y // rowh) % rows]
    n = tile_noise(N, 32, 44)
    col = base[None, None, :] * (0.65 + 0.4 * strands[..., None]) * shade[..., None] * tone[..., None] * (0.85 + 0.3 * n[..., None])
    # 古びた灰色 + 黒ずみ
    gray = col.mean(-1, keepdims=True)
    old = 0.2 + 0.25 * tile_noise(N, 5, 45)[..., None]
    col = col * (1 - old) + gray * old
    rot = np.clip((tile_noise(N, 8, 46) - 0.55) * 3, 0, 1)
    col *= (1 - 0.5 * rot)[..., None]
    save('Settle_Thatch', col)


# ------------------------------------------------------------ 獣皮
def hide():
    X, Y = np.meshgrid(np.arange(N), np.arange(N))
    n1 = tile_noise(N, 6, 51)
    n2 = tile_noise(N, 40, 52)
    base = np.array([0.66, 0.52, 0.36])
    col = base[None, None, :] * (0.8 + 0.3 * n1[..., None]) * (0.92 + 0.14 * n2[..., None])
    # 皮を継いだ縫い目 (V 方向に4枚)
    seam_x = (X % (N // 4))
    seam = (np.abs(seam_x - 2) < 3)
    stitch = seam & ((Y // 14) % 2 == 0)
    col = np.where(seam[..., None], col * 0.72, col)
    col = np.where(stitch[..., None], np.array([0.25, 0.18, 0.12]), col)
    # 赤土の文様: V の中ほどに帯 + 三角の連続 + 点
    ochre = np.array([0.55, 0.20, 0.10])
    dark = np.array([0.16, 0.12, 0.10])
    v = Y / N
    band1 = (np.abs(v - 0.30) < 0.012) | (np.abs(v - 0.42) < 0.012)
    tri_phase = (X % 128) / 128.0
    tri = (v > 0.315) & (v < 0.405) & (np.abs(tri_phase - 0.5) * 2 < (v - 0.315) / 0.09)
    dots_c = ((X % 64) - 32) ** 2 + ((Y - int(0.52 * N)) ** 2)
    dots = dots_c < 9 ** 2
    zig = np.abs(((X / 48.0) % 2) - 1) * 0.03 + 0.62
    zigzag = np.abs(v - zig) < 0.008
    paint = band1 | tri
    wear = tile_noise(N, 24, 53) > 0.35
    col = np.where((paint & wear)[..., None], col * 0.35 + ochre * 0.65, col)
    col = np.where(((dots | zigzag) & wear)[..., None], col * 0.3 + dark * 0.7, col)
    # 下端の汚れ
    dirt = np.clip((v - 0.8) * 4, 0, 1) * (0.6 + 0.4 * n2)
    col *= (1 - 0.4 * dirt)[..., None]
    save('Settle_Hide', col)


def bone():
    X, Y = np.meshgrid(np.arange(N), np.arange(N))
    n1 = tile_noise(N, 8, 61)
    streak = tile_noise(N, 128, 62)
    streak = np.repeat(streak[:1, :], N, axis=0) * 0.5 + tile_noise(N, 64, 63) * 0.5
    base = np.array([0.86, 0.81, 0.68])
    col = base[None, None, :] * (0.78 + 0.22 * n1[..., None]) * (0.9 + 0.15 * streak[..., None])
    stain = np.clip((tile_noise(N, 10, 64) - 0.55) * 3, 0, 1)
    col = col * (1 - 0.35 * stain[..., None]) + np.array([0.45, 0.36, 0.22]) * 0.35 * stain[..., None]
    save('Settle_Bone', col)


def ash():
    n1 = tile_noise(N, 10, 71)
    n2 = tile_noise(N, 80, 72)
    base = np.array([0.28, 0.27, 0.26])
    col = base[None, None, :] * (0.55 + 0.6 * n1[..., None]) * (0.8 + 0.4 * n2[..., None])
    coal = np.clip((tile_noise(N, 40, 73) - 0.62) * 6, 0, 1)
    col = col * (1 - coal[..., None]) + np.array([0.05, 0.045, 0.04]) * coal[..., None]
    save('Settle_Ash', col)


if __name__ == '__main__':
    stone()
    save('Settle_Wood', plank_field(81))
    save('Settle_WoodBurnt', plank_field(82, burnt=True))
    bark()
    thatch()
    hide()
    bone()
    ash()
