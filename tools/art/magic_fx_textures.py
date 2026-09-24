"""MagicCaster の魔法エフェクト (tools/art/magic_spell_effects.py) が色を付ける元の白い RGBA テクスチャを生成する。

    python tools/art/magic_fx_textures.py OUT_DIR [--preview PATH]

どのテクスチャも白で形はアルファに持つので、1枚で全属性の色に使える (色はエフェクトノードが付ける)。
lightning.png は横4コマのフリップブック (UVAnimation FrameCountX=4)。テクスチャごとに固定シードなので結果は決定的。

Pillow と numpy が必要。
"""
import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

TAU = 2 * math.pi


def _grid(w, h=None):
    h = h or w
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    return (x + 0.5) / w * 2 - 1, (y + 0.5) / h * 2 - 1


def _smooth(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def _to_image(alpha, rgb=None):
    a = np.clip(alpha, 0, 1)
    h, w = a.shape
    out = np.zeros((h, w, 4), np.uint8)
    if rgb is None:
        out[..., :3] = 255
    else:
        out[..., :3] = np.clip(rgb * 255, 0, 255).astype(np.uint8)
    out[..., 3] = (a * 255).astype(np.uint8)
    return Image.fromarray(out, 'RGBA')


def _value_noise(w, h, cells, seed):
    rng = np.random.default_rng(seed)
    g = rng.random((cells + 1, cells + 1)).astype(np.float32)
    g[-1, :] = g[0, :]
    g[:, -1] = g[:, 0]
    img = Image.fromarray((g * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)
    return np.asarray(img, np.float32) / 255


def _fbm(w, h, seed, octaves=5, base=4):
    acc = np.zeros((h, w), np.float32)
    amp, tot = 1.0, 0.0
    for o in range(octaves):
        acc += amp * _value_noise(w, h, base * 2 ** o, seed + o * 17)
        tot += amp
        amp *= 0.5
    return acc / tot


def _blur(a, radius):
    img = Image.fromarray((np.clip(a, 0, 1) * 255).astype(np.uint8))
    return np.asarray(img.filter(ImageFilter.GaussianBlur(radius)), np.float32) / 255


def _lines(size, segments, width, supersample=4):
    """アンチエイリアス付きポリライン -> アルファ (0..1)。"""
    s = size * supersample
    img = Image.new('L', (s, s), 0)
    d = ImageDraw.Draw(img)
    for pts, wmul in segments:
        d.line([(x * s, y * s) for x, y in pts], fill=255, width=max(1, int(width * wmul * supersample)), joint='curve')
    return np.asarray(img.resize((size, size), Image.LANCZOS), np.float32) / 255


# ------------------------------------------------------------------ テクスチャ
def glow():
    x, y = _grid(128)
    r = np.hypot(x, y)
    return _to_image(np.exp(-(r / 0.42) ** 2) * _smooth(1.0, 0.8, r))


def glow_core():
    x, y = _grid(128)
    r = np.hypot(x, y)
    return _to_image((0.55 * np.exp(-(r / 0.5) ** 2) + 0.9 * np.exp(-(r / 0.16) ** 2)) * _smooth(1.0, 0.85, r))


def spark():
    x, y = _grid(128)
    r = np.hypot(x, y)
    ray = lambda u, v, width: np.exp(-(v / width) ** 2) * np.clip(1 - np.abs(u), 0, 1) ** 2.2
    a = np.exp(-(r / 0.13) ** 2)
    a = np.maximum(a, ray(x, y, 0.035))
    a = np.maximum(a, ray(y, x, 0.035))
    d1, d2 = (x + y) / math.sqrt(2), (x - y) / math.sqrt(2)
    a = np.maximum(a, 0.45 * ray(d1, d2, 0.025) * _smooth(0.75, 0.2, r))
    a = np.maximum(a, 0.45 * ray(d2, d1, 0.025) * _smooth(0.75, 0.2, r))
    return _to_image(a)


def ring():
    x, y = _grid(256)
    r = np.hypot(x, y)
    a = np.exp(-((r - 0.84) / 0.05) ** 2) + 0.35 * np.exp(-((r - 0.84) / 0.14) ** 2)
    return _to_image(a * _smooth(1.0, 0.95, r))


def smoke():
    x, y = _grid(128)
    r = np.hypot(x, y)
    n = _fbm(128, 128, 11)
    a = _smooth(0.95, 0.25, r + (n - 0.5) * 0.55) * (0.55 + 0.45 * n)
    return _to_image(a)


def flame():
    # 炎の舌: 下は丸く、上は尖り、縁はノイズで揺らす
    x, y = _grid(128)
    n = _fbm(128, 128, 23, octaves=4, base=3)
    width = 0.62 * np.clip((y + 1.0) / 1.55, 0, 1) ** 0.75          # 先端 (上) で 0、下ほど広い
    bottom = _smooth(0.95, 0.55, y)
    edge = np.abs(x) / np.maximum(width, 1e-3) + (n - 0.5) * 0.7
    a = _smooth(1.0, 0.35, edge) * bottom * _smooth(-1.0, -0.72, y)
    core = _smooth(0.55, 0.0, np.abs(x) / np.maximum(width, 1e-3)) * _smooth(0.9, 0.1, y) * _smooth(-0.5, 0.1, y)
    return _to_image(np.clip(a * 0.85 + core * 0.35, 0, 1))


def rune_circle():
    size = 512
    rng = np.random.default_rng(5)
    segs = []
    def circle(r, n=180):
        return [(0.5 + r * math.cos(t), 0.5 + r * math.sin(t)) for t in np.linspace(0, TAU, n + 1)]
    segs += [(circle(0.47), 1.3), (circle(0.44), 0.7), (circle(0.34), 1.0), (circle(0.155), 0.8)]
    # 0.35 から 0.43 の間にルーン
    count = 24
    for i in range(count):
        t0 = TAU * (i + 0.15) / count
        t1 = TAU * (i + 0.85) / count
        rr = [0.365, 0.39, 0.415]
        for _ in range(3):
            a0 = t0 + (t1 - t0) * rng.random()
            a1 = t0 + (t1 - t0) * rng.random()
            r0, r1 = rng.choice(rr), rng.choice(rr)
            segs.append(([(0.5 + r0 * math.cos(a0), 0.5 + r0 * math.sin(a0)),
                          (0.5 + r1 * math.cos(a1), 0.5 + r1 * math.sin(a1))], 0.6))
    # 六芒星
    for k in range(2):
        pts = [(0.5 + 0.34 * math.cos(TAU * j / 3 + k * math.pi + math.pi / 2),
                0.5 + 0.34 * math.sin(TAU * j / 3 + k * math.pi + math.pi / 2)) for j in range(4)]
        segs.append((pts, 0.9))
    # 外側の帯に目盛り
    for i in range(72):
        t = TAU * i / 72
        l = 0.02 if i % 3 else 0.035
        segs.append(([(0.5 + 0.44 * math.cos(t), 0.5 + 0.44 * math.sin(t)),
                      (0.5 + (0.44 - l) * math.cos(t), 0.5 + (0.44 - l) * math.sin(t))], 0.5))
    line = _lines(size, segs, 2.6)
    a = np.clip(line + _blur(line, 5) * 0.8, 0, 1)
    return _to_image(a)


def _bolt_path(rng, x0, y0, x1, y1, depth, jitter):
    pts = [(x0, y0), (x1, y1)]
    for _ in range(depth):
        nxt = [pts[0]]
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            mx, my = (ax + bx) / 2, (ay + by) / 2
            length = math.hypot(bx - ax, by - ay)
            mx += (rng.random() - 0.5) * jitter * length
            nxt += [(mx, my), (bx, by)]
        pts = nxt
    return pts


def lightning():
    frames = []
    for f in range(4):
        rng = np.random.default_rng(100 + f)
        size = 512
        main = _bolt_path(rng, 0.5, 0.0, 0.5 + (rng.random() - 0.5) * 0.2, 1.0, 7, 0.55)
        segs = [(main, 1.0)]
        for _ in range(3):
            i = int(rng.integers(len(main) // 5, len(main) * 3 // 4))
            sx, sy = main[i]
            ex = sx + (rng.random() - 0.5) * 0.5
            ey = min(1.0, sy + 0.12 + rng.random() * 0.25)
            segs.append((_bolt_path(rng, sx, sy, ex, ey, 5, 0.6), 0.45))
        line = _lines(size, segs, 9.0)
        a = np.clip(line + _blur(line, 8) * 1.1 + _blur(line, 24) * 0.7, 0, 1)
        img = Image.fromarray((a * 255).astype(np.uint8)).resize((128, 512), Image.LANCZOS)
        frames.append(np.asarray(img, np.float32) / 255)
    return _to_image(np.concatenate(frames, axis=1))


def crescent():
    x, y = _grid(256)
    outer = np.hypot(x, y + 0.25) - 0.72
    inner = np.hypot(x, y + 0.58) - 0.78
    d = np.maximum(outer, -inner)                 # 月: 外側の円から下にずらした円を引く
    a = _smooth(0.03, -0.03, d)
    glow_ = _smooth(0.25, -0.02, d) * 0.45
    tips = _smooth(0.9, 0.2, np.abs(x))           # 角の先に向かってフェード
    edge = _smooth(0.12, 0.0, -outer)             # 前縁 (外側) ほど明るい
    return _to_image(np.clip((a * (0.55 + 0.45 * edge) + glow_) * tips, 0, 1))


def beam():
    # v はビームの横方向、u は沿う方向。u 方向にタイルする
    w, h = 256, 64
    y = (np.arange(h, dtype=np.float32) + 0.5) / h * 2 - 1
    n = _fbm(w, h, 31, octaves=3, base=4)
    profile = np.exp(-(y / 0.22) ** 2) * 0.9 + np.exp(-(y / 0.6) ** 2) * 0.35
    a = profile[:, None] * (0.75 + 0.25 * n)
    return _to_image(np.clip(a, 0, 1))


def streak():
    w, h = 64, 256
    x, y = _grid(w, h)
    a = np.exp(-(x / 0.28) ** 2) * _smooth(1.0, 0.2, np.abs(y)) * np.clip(1 - np.abs(y), 0, 1) ** 0.6
    return _to_image(a)


def shard():
    w, h = 64, 128
    img = Image.new('L', (w * 4, h * 4), 0)
    d = ImageDraw.Draw(img)
    d.polygon([(w * 2, 4), (w * 3.6, h * 2.6), (w * 2, h * 4 - 4), (w * 0.4, h * 1.7)], fill=255)
    a = np.asarray(img.resize((w, h), Image.LANCZOS), np.float32) / 255
    x, y = _grid(w, h)
    facet = np.where(x > 0.1 * y, 1.0, 0.72)
    return _to_image(np.clip(a * facet + _blur(a, 3) * 0.3, 0, 1))


def spike():
    # 底面で立つ岩の棘: 頂点が上、左から照らす
    w, h = 64, 128
    img = Image.new('L', (w * 4, h * 4), 0)
    ImageDraw.Draw(img).polygon([(w * 2, 6), (w * 4 - 6, h * 4), (6, h * 4)], fill=255)
    a = np.asarray(img.resize((w, h), Image.LANCZOS), np.float32) / 255
    x, y = _grid(w, h)
    n = _fbm(w, h, 57, octaves=4, base=5)
    shade = np.where(x < 0.0, 0.95, 0.62) + (n - 0.5) * 0.3 - (y + 1) * 0.08
    rgb = np.repeat(np.clip(shade, 0.2, 1.0)[..., None], 3, axis=2)
    return _to_image(a, rgb)


def cross():
    x, y = _grid(128)
    bar = lambda u, v: _smooth(0.2, 0.14, np.abs(u)) * _smooth(0.72, 0.62, np.abs(v))
    a = np.maximum(bar(x, y), bar(y, x))
    return _to_image(np.clip(a * 0.9 + _blur(a, 6) * 0.5, 0, 1))


def chevron():
    x, y = _grid(128)
    d = np.abs(-y + 0.1 - (0.6 - np.abs(x) * 0.9)) - 0.1
    a = _smooth(0.04, -0.04, d) * _smooth(0.78, 0.68, np.abs(x))
    return _to_image(np.clip(a + _blur(a, 5) * 0.5, 0, 1))


def snowflake():
    segs = []
    for k in range(6):
        t = TAU * k / 6 - math.pi / 2
        cx, cy = 0.5 + 0.44 * math.cos(t), 0.5 + 0.44 * math.sin(t)
        segs.append(([(0.5, 0.5), (cx, cy)], 1.0))
        for f in (0.45, 0.7):
            bx, by = 0.5 + 0.44 * f * math.cos(t), 0.5 + 0.44 * f * math.sin(t)
            for s in (-1, 1):
                ang = t + s * math.pi / 4
                segs.append(([(bx, by), (bx + 0.13 * math.cos(ang), by + 0.13 * math.sin(ang))], 0.7))
    line = _lines(128, segs, 5)
    return _to_image(np.clip(line + _blur(line, 3) * 0.6, 0, 1))


def rock():
    # 陰影付きの塊 (陰影は RGB に持つ。Blend モードの破片用)
    size = 128
    rng = np.random.default_rng(41)
    pts = []
    n = 9
    for i in range(n):
        t = TAU * i / n + rng.random() * 0.4
        r = 0.34 + rng.random() * 0.12
        pts.append((0.5 + r * math.cos(t), 0.5 + r * math.sin(t)))
    img = Image.new('L', (size * 4, size * 4), 0)
    ImageDraw.Draw(img).polygon([(x * size * 4, y * size * 4) for x, y in pts], fill=255)
    a = np.asarray(img.resize((size, size), Image.LANCZOS), np.float32) / 255
    x, y = _grid(size)
    n2 = _fbm(size, size, 43, octaves=4, base=6)
    shade = np.clip(0.62 + 0.35 * (-x * 0.6 - y * 0.8) + (n2 - 0.5) * 0.35, 0.25, 1.0)
    return _to_image(a, np.repeat(shade[..., None], 3, axis=2))


def crack():
    size = 512
    rng = np.random.default_rng(77)
    segs = []
    for i in range(11):
        t = TAU * i / 11 + rng.random() * 0.3
        pts = [(0.5, 0.5)]
        r, ang = 0.0, t
        while r < 0.47:
            r += 0.03 + rng.random() * 0.05
            ang += (rng.random() - 0.5) * 0.5
            pts.append((0.5 + r * math.cos(ang), 0.5 + r * math.sin(ang)))
            if rng.random() < 0.18 and r < 0.35:
                b = ang + (rng.random() - 0.5) * 1.6
                segs.append(([pts[-1], (pts[-1][0] + 0.12 * math.cos(b), pts[-1][1] + 0.12 * math.sin(b))], 0.5))
        segs.append((pts, 1.0))
    line = _lines(size, segs, 3.4)
    x, y = _grid(size)
    fade = _smooth(1.0, 0.7, np.hypot(x, y))
    return _to_image(np.clip((line + _blur(line, 4) * 0.5) * fade, 0, 1))


TEXTURES = {
    'glow': glow, 'glow_core': glow_core, 'spark': spark, 'ring': ring, 'smoke': smoke, 'flame': flame,
    'rune_circle': rune_circle, 'lightning': lightning, 'crescent': crescent, 'beam': beam, 'streak': streak,
    'shard': shard, 'spike': spike, 'cross': cross, 'chevron': chevron, 'snowflake': snowflake, 'rock': rock, 'crack': crack,
}


def write_all(out_dir: Path) -> dict[str, Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    paths = {}
    for name, fn in TEXTURES.items():
        p = out_dir / f'{name}.png'
        fn().save(p)
        paths[name] = p
    return paths


def render_preview(paths: dict[str, Path], out: Path):
    cell = 160
    cols = 6
    rows = math.ceil(len(paths) / cols)
    sheet = Image.new('RGBA', (cols * cell, rows * (cell + 16)), (24, 26, 34, 255))
    d = ImageDraw.Draw(sheet)
    for i, (name, p) in enumerate(paths.items()):
        im = Image.open(p).convert('RGBA')
        im.thumbnail((cell - 8, cell - 8))
        x, y = (i % cols) * cell, (i // cols) * (cell + 16)
        sheet.alpha_composite(im, (x + 4, y + 4))
        d.text((x + 4, y + cell), name, fill=(220, 220, 220, 255))
    sheet.save(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out_dir')
    ap.add_argument('--preview')
    args = ap.parse_args()
    paths = write_all(Path(args.out_dir))
    for name, p in paths.items():
        print(f'{name:12s} {Image.open(p).size}')
    if args.preview:
        render_preview(paths, Path(args.preview))


if __name__ == '__main__':
    main()
