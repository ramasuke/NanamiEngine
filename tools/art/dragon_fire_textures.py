"""ドラゴンの炎エフェクト (tools/art/dragon_fire_effects.py) 用のテクスチャを生成する。

    python tools/art/dragon_fire_textures.py OUT_DIR [--preview PATH]

magic_fx_textures の白い形 + ノードの色付けと違い、炎は温度のグラデーション (白熱 -> 黄 -> 橙 -> 赤) を
テクスチャに焼き込む。ノードの色は白に近いまま、フェードと冷め具合の調整だけに使う。
* fire_flip  : 立ちのぼる炎の舌。8x4 = 32 コマのループするフリップブック (上へ流れるノイズ)
* fire_puff  : 燃える塊が膨らみながら冷めて千切れる 4x4 = 16 コマ (1 回再生)
* smoke_puff : 陰影のある煙のかたまり 2x2 = 4 種 (StartSheet でランダムに選ぶ)
* ember / hot_glow / flame_ring / scorch / ember_bed
テクスチャごとに固定シードなので結果は決定的。Pillow と numpy が必要。
"""
import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

TAU = 2 * math.pi

# 温度 -> 色。0 は透明、1 は白熱
RAMP = [
    (0.00, (0.10, 0.01, 0.00)),
    (0.18, (0.55, 0.06, 0.01)),
    (0.38, (0.93, 0.26, 0.03)),
    (0.58, (1.00, 0.52, 0.08)),
    (0.78, (1.00, 0.80, 0.34)),
    (1.00, (1.00, 0.97, 0.86)),
]


def _ramp(t):
    t = np.clip(t, 0, 1)
    xs = [p for p, _ in RAMP]
    return np.stack([np.interp(t, xs, [c[i] for _, c in RAMP]) for i in range(3)], axis=-1)


def _smooth(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def _grid(w, h=None):
    h = h or w
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    return (x + 0.5) / w * 2 - 1, (y + 0.5) / h * 2 - 1


def _periodic_noise(cells_x, cells_y, seed):
    """格子 (cells_y, cells_x) の周期的な値ノイズ。返す関数は [0,1) の座標でサンプルし、端でつながる。"""
    g = np.random.default_rng(seed).random((cells_y, cells_x)).astype(np.float32)

    def sample(u, v):
        fx, fy = (u % 1.0) * cells_x, (v % 1.0) * cells_y
        x0, y0 = np.floor(fx).astype(int), np.floor(fy).astype(int)
        tx, ty = fx - x0, fy - y0
        tx, ty = tx * tx * (3 - 2 * tx), ty * ty * (3 - 2 * ty)
        x0, y0 = x0 % cells_x, y0 % cells_y
        x1, y1 = (x0 + 1) % cells_x, (y0 + 1) % cells_y
        a = g[y0, x0] * (1 - tx) + g[y0, x1] * tx
        b = g[y1, x0] * (1 - tx) + g[y1, x1] * tx
        return a * (1 - ty) + b * ty
    return sample


def _fbm_fn(seed, base=4, octaves=5, aspect=1):
    layers = [_periodic_noise(base * 2 ** o, base * aspect * 2 ** o, seed + o * 31) for o in range(octaves)]

    def sample(u, v):
        acc, amp, tot = 0.0, 1.0, 0.0
        for f in layers:
            acc = acc + amp * f(u, v)
            tot += amp
            amp *= 0.5
        return acc / tot
    return sample


def _soften(a, radius):
    """ノイズのギザギザを落とすぼかし (値域はそのまま)"""
    from PIL import ImageFilter
    hi = float(max(a.max(), 1e-6))
    lo = Image.fromarray((np.clip(a / hi, 0, 1) * 255).astype(np.uint8), 'L')
    blurred = lo.filter(ImageFilter.GaussianBlur(radius))
    return np.asarray(blurred, np.float32) / 255 * hi


def _rgba(temp, alpha):
    h, w = temp.shape
    out = np.zeros((h, w, 4), np.float32)
    out[..., :3] = _ramp(temp)
    out[..., 3] = np.clip(alpha, 0, 1)
    return out


def _atlas(cells, cols, rows):
    ch, cw = cells[0].shape[:2]
    sheet = np.zeros((rows * ch, cols * cw, 4), np.float32)
    for i, c in enumerate(cells):
        r, k = divmod(i, cols)
        sheet[r * ch:(r + 1) * ch, k * cw:(k + 1) * cw] = c
    return _to_image(sheet)


def _to_image(arr):
    return Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8), 'RGBA')


# ------------------------------------------------------------------ 炎
def fire_flip(cell=128, cols=8, rows=4):
    """立ちのぼる炎の舌。ノイズを上へ 1 周流すので 32 コマで途切れずにループする"""
    n = cols * rows
    x, y = _grid(cell)
    v = (1 - y) / 2                     # 0 = 下端, 1 = 上端
    fbm = _fbm_fn(101, base=4, octaves=5, aspect=2)
    warp = _fbm_fn(202, base=2, octaves=3, aspect=2)
    cells = []
    for f in range(n):
        t = f / n
        # 横揺れ: 上ほど大きく揺れる
        sway = (warp((x + 1) / 4 + 0.3, v / 2 - t) - 0.5) * 0.7 * v ** 1.3
        xs = x + sway
        # 上へ流れるノイズで削る。上ほど強く削れて、舌が千切れて消える
        detail = fbm((xs + 1) / 2, v * 1.0 - t)
        shape = 1.0 - np.abs(xs) * (1.9 + 1.6 * v)
        temp = shape - v * 1.05 + (detail - 0.5) * (0.9 + 0.9 * v) + 0.12
        temp = temp * _smooth(0.0, 0.3, v + 0.03 - np.abs(x) * 0.35) * _smooth(1.0, 0.8, np.abs(x))
        temp = temp * _smooth(1.0, 0.82, v)
        temp = _soften(np.clip(temp, 0, None) * 1.35, 1.2)
        alpha = _smooth(0.03, 0.45, temp)
        cells.append(_rgba(temp, alpha))
    return _atlas(cells, cols, rows)


def fire_puff(cell=128, cols=4, rows=4):
    """燃える塊: 最初は白熱して詰まっていて、膨らみながら赤く冷めて、ノイズで千切れて消える"""
    n = cols * rows
    x, y = _grid(cell)
    r = np.hypot(x, y)
    ang = np.arctan2(y, x)
    fbm = _fbm_fn(303, base=4, octaves=5)
    fine = _fbm_fn(404, base=8, octaves=4)
    cells = []
    for f in range(n):
        s = f / (n - 1)
        # 角度方向のでこぼこ (もこもこした輪郭)
        lobes = fbm((np.cos(ang) * 0.25 + 0.5) + s * 0.15, (np.sin(ang) * 0.25 + 0.5) + s * 0.1)
        radius = (0.42 + 0.45 * s ** 0.6) * (0.78 + 0.45 * lobes)
        inside = _smooth(1.0, 0.55, r / radius)
        churn = fine((x + 1) / 2 + s * 0.2, (y + 1) / 2 - s * 0.35)
        heat = 1.15 - 1.0 * s ** 0.8
        temp = inside * (heat * (0.6 + 0.6 * churn) + 0.25 * (1 - r / radius)) - s * 0.35 * (1 - churn)
        alpha = _smooth(0.05, 0.3, temp) * _smooth(1.0, 0.75, s)
        cells.append(_rgba(temp, alpha))
    return _atlas(cells, cols, rows)


def smoke_puff(cell=256, cols=2, rows=2):
    """上から光が当たった煙のかたまり。RGB に明暗、アルファにもこもこの輪郭 (色はノードが付ける)"""
    x, y = _grid(cell)
    cells = []
    for k in range(cols * rows):
        fbm = _fbm_fn(500 + k * 7, base=3, octaves=6)
        u, v = (x + 1) / 2, (y + 1) / 2
        n = fbm(u, v)
        # 中心から少しずらしたいくつかの球を重ねて、もこもこさせる
        rng = np.random.default_rng(600 + k)
        blob = np.zeros_like(x)
        for _ in range(5):
            cx, cy, rr = rng.uniform(-0.3, 0.3), rng.uniform(-0.25, 0.25), rng.uniform(0.35, 0.55)
            blob = np.maximum(blob, 1 - np.hypot(x - cx, y - cy) / rr)
        density = _smooth(0.0, 0.55, blob + (n - 0.5) * 0.8) * _smooth(0.95, 0.6, np.hypot(x, y))
        # 上側が明るく、下側と縁は暗い
        shade = np.clip(0.55 - y * 0.3 + (fbm(u + 0.37, v - 0.21) - 0.5) * 0.7, 0.15, 1.0)
        out = np.zeros((cell, cell, 4), np.float32)
        out[..., :3] = shade[..., None]
        out[..., 3] = density * (0.6 + 0.4 * n)
        cells.append(out)
    return _atlas(cells, cols, rows)


def ember(cell=64):
    x, y = _grid(cell)
    r = np.hypot(x, y)
    core = np.exp(-(r / 0.16) ** 2)
    halo = 0.45 * np.exp(-(r / 0.45) ** 2)
    temp = np.clip(core * 1.1 + halo * 0.9, 0, 1)
    return _to_image(_rgba(0.45 + 0.55 * temp, np.clip(core + halo, 0, 1) * _smooth(1.0, 0.8, r)))


def hot_glow(cell=128):
    """白熱した芯から橙に落ちていく光"""
    x, y = _grid(cell)
    r = np.hypot(x, y)
    temp = np.exp(-(r / 0.55) ** 2) * 1.05
    alpha = (0.7 * np.exp(-(r / 0.5) ** 2) + 0.3 * np.exp(-(r / 0.2) ** 2)) * _smooth(1.0, 0.85, r)
    return _to_image(_rgba(0.3 + 0.7 * temp, alpha))


def flame_ring(cell=512):
    """地面を走る炎の輪 (上から見た図)。外縁ほど熱く、内側は煤けて抜ける"""
    x, y = _grid(cell)
    r = np.hypot(x, y)
    ang = np.arctan2(y, x)
    fbm = _fbm_fn(707, base=6, octaves=5)
    n = fbm((ang / TAU) % 1.0, r * 0.8)
    edge = 0.8 + (n - 0.5) * 0.14
    band = _smooth(0.2, 0.0, np.abs(r - edge)) * _smooth(0.0, 0.25, r)
    tongues = _fbm_fn(717, base=12, octaves=4)((ang / TAU) % 1.0, r * 1.5)
    temp = band * (0.2 + 1.1 * tongues) - 0.25 * (1 - tongues)
    temp = np.clip(temp, 0, None) * _smooth(0.55, 0.85, r / edge) * 1.3
    return _to_image(_rgba(temp * 1.1, _smooth(0.05, 0.35, temp)))


def scorch(cell=512):
    """焦げ跡: 不規則な黒い焼け跡。BLEND で重ねる"""
    x, y = _grid(cell)
    r = np.hypot(x, y)
    ang = np.arctan2(y, x)
    fbm = _fbm_fn(808, base=5, octaves=6)
    edge = 0.62 + (fbm((ang / TAU) % 1.0, 0.5) - 0.5) * 0.5
    grain = fbm((x + 1) / 2, (y + 1) / 2)
    density = _smooth(1.0, 0.45, r / edge) * (0.75 + 0.5 * grain)
    out = np.zeros((cell, cell, 4), np.float32)
    shade = 0.10 + 0.25 * grain + 0.3 * _smooth(0.6, 1.0, r / edge)
    out[..., 0] = shade * 0.9
    out[..., 1] = shade * 0.75
    out[..., 2] = shade * 0.6
    out[..., 3] = np.clip(density, 0, 1)
    return _to_image(out)


def ember_bed(cell=512):
    """燃えさし: 焦げ跡の上に加算で重ねる、ひび割れと点々の赤熱"""
    x, y = _grid(cell)
    r = np.hypot(x, y)
    u, v = (x + 1) / 2, (y + 1) / 2
    cracks_n = _fbm_fn(909, base=6, octaves=4)(u, v)
    cracks = _smooth(0.07, 0.0, np.abs(cracks_n - 0.5))
    speck = _smooth(0.66, 0.8, _fbm_fn(919, base=24, octaves=2)(u, v))
    ang = np.arctan2(y, x)
    edge = 0.58 + (_fbm_fn(929, base=5, octaves=4)((ang / TAU) % 1.0, 0.3) - 0.5) * 0.4
    fall = _smooth(1.0, 0.3, r / edge)
    temp = np.clip((cracks * 0.85 + speck * 0.7) * fall, 0, 1)
    return _to_image(_rgba(0.25 + 0.6 * temp, _smooth(0.03, 0.3, temp)))


TEXTURES = {
    'fire_flip': fire_flip,
    'fire_puff': fire_puff,
    'smoke_puff': smoke_puff,
    'ember': ember,
    'hot_glow': hot_glow,
    'flame_ring': flame_ring,
    'scorch': scorch,
    'ember_bed': ember_bed,
}

# フリップブックのコマ割り (列, 行)
SHEETS = {'fire_flip': (8, 4), 'fire_puff': (4, 4), 'smoke_puff': (2, 2)}


def write_all(out_dir: Path) -> dict[str, Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    paths = {}
    for name, fn in TEXTURES.items():
        p = out_dir / f'{name}.png'
        fn().save(p)
        paths[name] = p
    return paths


def render_preview(paths: dict[str, Path], out: Path):
    """暗い背景に加算 (炎) / アルファ (煙・焦げ) で置いた一覧"""
    rows = []
    for name, p in paths.items():
        im = Image.open(p).convert('RGBA')
        scale = 512 / max(im.size)
        im = im.resize((int(im.width * scale), int(im.height * scale)), Image.LANCZOS)
        bg = np.zeros((im.height + 20, 512, 3), np.float32)
        bg[:] = (0.12, 0.13, 0.16)
        a = np.asarray(im, np.float32) / 255
        region = bg[20:20 + im.height, :im.width]
        if name in ('smoke_puff', 'scorch'):
            region[:] = region * (1 - a[..., 3:]) + a[..., :3] * a[..., 3:]
        else:
            region[:] = np.clip(region + a[..., :3] * a[..., 3:], 0, 1)
        img = Image.fromarray((bg * 255).astype(np.uint8))
        ImageDraw.Draw(img).text((4, 4), name, fill=(230, 230, 230))
        rows.append(img)
    sheet = Image.new('RGB', (512, sum(r.height for r in rows)))
    yy = 0
    for r in rows:
        sheet.paste(r, (0, yy))
        yy += r.height
    sheet.save(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out_dir')
    ap.add_argument('--preview')
    args = ap.parse_args()
    paths = write_all(Path(args.out_dir))
    if args.preview:
        render_preview(paths, Path(args.preview))


if __name__ == '__main__':
    main()
