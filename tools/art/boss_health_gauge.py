"""Generate the boss HP gauge sprites (Dauntless-inspired "crystal crown") drawn by GamePlay::Ui::BossHealthGauge.

    python tools/art/boss_health_gauge.py [--out-dir Assets/Art/UI/BossHealth] [--preview PATH]

Writes BossHealthCrest / BossHealthCrestGlow / BossHealthShard_{Fill,FillDanger,Trail,Empty}.png and a
SpriteFile .png.meta for each (an existing .meta keeps its GUID, so regenerating never breaks references).
One shard sprite serves all eight shards: it is drawn pointing up (tip at the top, base at the bottom) and
BossHealthGauge rotates it around the crown pivot with DrawRectRotaGraph2F, clipping the fill from the base.
The layout values the component has to agree with are printed at the end (GEOMETRY).
--preview also composites the crown at 100% / 60% / 22% HP the same way the component draws it.

Shapes are authored in "design units" and rasterised at PX_PER_UNIT, SDF-based at 4x supersampling.
Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.scene import sprite_meta  # noqa: E402

S = 4
PX_PER_UNIT = 0.8
TAU = 2 * math.pi

SHARD_COUNT = 8
ARC_DEG = 140.0
SHARD_W_PX, SHARD_H_PX = 48, 96
SHARD_BASE_U = 118.0      # pivot -> shard base, design units
SHARD_LEN_U = 94.0        # base -> tip
SHARD_WIDEST_U = 34.0     # base -> widest point
SHARD_HALF_W_U = 17.0
SHARD_PAD_PX = (SHARD_H_PX - SHARD_LEN_U * PX_PER_UNIT) / 2

CREST_W_PX, CREST_H_PX = 128, 152
CREST_OFFSET_U = -44.0    # pivot -> crest centre
SKULL_OFFSET_U = 2.0      # crest centre -> skull centre
ROOT_TO_PIVOT_PX = 84.8   # BossHealthGauge transform -> pivot
NAME_TOP_PX = 126.0       # BossHealthGauge transform -> BossName text top

rng = np.random.default_rng(7)


def hexc(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)], dtype=np.float32)


INK = hexc('#0b0a0c')
TRAIL = hexc('#fbe4cf')
NORMAL = [(0, '#5a0810'), (0.45, '#c8141e'), (0.8, '#ff5a3c'), (1, '#a0121a')]
DANGER = [(0, '#2a0004'), (0.45, '#8a000c'), (0.8, '#ff7a2a'), (1, '#5a0008')]


# ---------------------------------------------------------------- raster helpers
def grid(w_px, h_px):
    ys, xs = np.mgrid[0:h_px * S, 0:w_px * S].astype(np.float32)
    return (xs + 0.5) / S / PX_PER_UNIT, (ys + 0.5) / S / PX_PER_UNIT


def cov(sd):
    return np.clip(0.5 - sd * PX_PER_UNIT * S, 0, 1).astype(np.float32)


def blur_cov(c, radius_u):
    im = Image.fromarray((np.clip(c, 0, 1) * 255).astype(np.uint8), 'L')
    im = im.filter(ImageFilter.GaussianBlur(radius_u * PX_PER_UNIT * S))
    return np.asarray(im, np.float32) / 255


def over(canvas, rgb, a):
    a = np.asarray(a, np.float32)[..., None]
    canvas[..., :3] = rgb * a + canvas[..., :3] * (1 - a)
    canvas[..., 3:] = a + canvas[..., 3:] * (1 - a)


def resolve(canvas, w_px, h_px):
    c = canvas.reshape(h_px, S, w_px, S, 4).mean(axis=(1, 3))
    a = c[..., 3:]
    rgb = np.where(a > 1e-5, c[..., :3] / np.maximum(a, 1e-5), 0)
    out = np.concatenate([np.clip(rgb, 0, 1), np.clip(a, 0, 1)], axis=-1)
    return Image.fromarray((out * 255 + 0.5).astype(np.uint8), 'RGBA')


def ramp(t, stops):
    t = np.clip(t, 0, 1)
    rgb = np.zeros(np.shape(t) + (3,), np.float32)
    for (t0, c0), (t1, c1) in zip(stops[:-1], stops[1:]):
        m = (t >= t0) & (t <= t1)
        k = ((t - t0) / (t1 - t0))[..., None]
        rgb = np.where(m[..., None], hexc(c0) * (1 - k) + hexc(c1) * k, rgb)
    return rgb


# ---------------------------------------------------------------- SDF helpers
def sd_polygon(x, y, verts):
    v = np.array(verts, dtype=np.float32)
    n = len(v)
    d = (x - v[0, 0]) ** 2 + (y - v[0, 1]) ** 2
    s = np.ones_like(x)
    j = n - 1
    for i in range(n):
        ex, ey = v[j, 0] - v[i, 0], v[j, 1] - v[i, 1]
        wx, wy = x - v[i, 0], y - v[i, 1]
        t = np.clip((wx * ex + wy * ey) / (ex * ex + ey * ey), 0, 1)
        bx, by = wx - ex * t, wy - ey * t
        d = np.minimum(d, bx * bx + by * by)
        c1 = y >= v[i, 1]
        c2 = y < v[j, 1]
        c3 = ex * wy > ey * wx
        s = np.where((c1 & c2 & c3) | (~c1 & ~c2 & ~c3), -s, s)
        j = i
    return s * np.sqrt(d)


def sd_circle(x, y, cx, cy, r):
    return np.hypot(x - cx, y - cy) - r


def sd_rbox(x, y, x0, y0, x1, y1, r):
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2 - r, (y1 - y0) / 2 - r
    qx, qy = np.abs(x - cx) - hx, np.abs(y - cy) - hy
    return np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - r


def metal(X, Y, d_shape, top, bottom):
    rgb = ramp((Y - top) / (bottom - top), [(0.0, '#c2cad3'), (0.16, '#88909a'), (0.5, '#4b515a'),
                                            (0.84, '#2c3036'), (1.0, '#1b1e22')])
    rim = (np.clip(1 - (-d_shape) / 1.6, 0, 1) * (d_shape < 0))[..., None]
    upper = (Y < (top + bottom) / 2)[..., None]
    rgb = np.where(upper, rgb + rim * 0.28, rgb - rim * 0.12)
    streak = rng.normal(0, 0.025, size=(1, X.shape[1] // S)).repeat(S, axis=1)
    rows = rng.normal(0, 0.02, size=(X.shape[0], 1))
    return np.clip(rgb + (streak * 0.6 + rows)[..., None], 0, 1)


# ---------------------------------------------------------------- crest + skull
def crest_sd(X, Y, cx, cy):
    w, h = 50.0, 64.0
    pts = [(cx, cy - h), (cx + w, cy - h * 0.55), (cx + w, cy + h * 0.45), (cx, cy + h),
           (cx - w, cy + h * 0.45), (cx - w, cy - h * 0.55)]
    return sd_polygon(X, Y, pts) - 6, h


def horn_pts(cx, cy, side):
    pts = [(-12, -20), (-21, -30), (-29, -36), (-27, -24), (-19, -9)]
    return [(cx + side * px, cy + py) for px, py in pts]


def skull_sd(X, Y, cx, cy):
    head = np.minimum(sd_circle(X, Y, cx, cy - 6, 19), sd_rbox(X, Y, cx - 11, cy + 2, cx + 11, cy + 21, 4))
    horns = np.minimum(sd_polygon(X, Y, horn_pts(cx, cy, 1)), sd_polygon(X, Y, horn_pts(cx, cy, -1)))
    eyes = np.minimum(sd_polygon(X, Y, [(cx - 15, cy - 8), (cx - 3, cy - 4), (cx - 5, cy + 3), (cx - 13, cy + 1)]),
                      sd_polygon(X, Y, [(cx + 15, cy - 8), (cx + 3, cy - 4), (cx + 5, cy + 3), (cx + 13, cy + 1)]))
    return np.minimum(head, horns), eyes


def render_crest():
    X, Y = grid(CREST_W_PX, CREST_H_PX)
    img = np.zeros((CREST_H_PX * S, CREST_W_PX * S, 4), np.float32)
    cx, cy = CREST_W_PX / PX_PER_UNIT / 2, CREST_H_PX / PX_PER_UNIT / 2
    sil, h = crest_sd(X, Y, cx, cy)

    over(img, INK, cov(sil))
    over(img, metal(X, Y, sil, cy - h, cy + h), cov(sil + 3))
    over(img, hexc('#ffffff'), cov(sil + 3) * (1 - cov(sil + 5)) * np.clip((cy - Y) / h, 0, 1) * 0.5)
    over(img, INK, cov(sil + 9))
    field = cov(sil + 11)
    t = np.clip((Y - (cy - h)) / (2 * h), 0, 1)[..., None]
    over(img, hexc('#2c151a') * (1 - t) + hexc('#0b0507') * t, field)
    over(img, hexc('#d0202a'), field * (1 - cov(sil + 13)) * 0.55)

    sx, sy = cx, cy + SKULL_OFFSET_U
    shape, eyes = skull_sd(X, Y, sx, sy)
    over(img, hexc('#ff4a2a'), blur_cov(cov(shape), 4) * 0.3)
    over(img, INK, cov(shape - 2.5))
    t = np.clip((Y - (sy - 36)) / 58, 0, 1)[..., None]
    over(img, hexc('#f4ead8') * (1 - t) + hexc('#a8927a') * t, cov(shape))
    over(img, INK, cov(eyes))
    over(img, hexc('#ff5a2a'), cov(eyes + 1.6) * 0.75)
    over(img, INK, cov(sd_polygon(X, Y, [(sx, sy + 5), (sx - 3, sy + 11), (sx + 3, sy + 11)])))
    for tx in (-6, 0, 6):
        over(img, INK, cov(sd_rbox(X, Y, sx + tx - 0.8, sy + 14, sx + tx + 0.8, sy + 21, 0.4)))
    return resolve(img, CREST_W_PX, CREST_H_PX)


def render_crest_glow():
    X, Y = grid(CREST_W_PX, CREST_H_PX)
    img = np.zeros((CREST_H_PX * S, CREST_W_PX * S, 4), np.float32)
    cx, cy = CREST_W_PX / PX_PER_UNIT / 2, CREST_H_PX / PX_PER_UNIT / 2
    sil, _ = crest_sd(X, Y, cx, cy)
    shape, eyes = skull_sd(X, Y, cx, cy + SKULL_OFFSET_U)
    rim = blur_cov(cov(sil) * (1 - cov(sil + 9)), 6)
    halo = np.maximum(rim * 1.4, blur_cov(cov(shape), 4) * 0.8)
    halo = np.maximum(halo, blur_cov(cov(eyes), 3) * 1.6)
    over(img, hexc('#ff5a2a'), np.clip(halo, 0, 1))
    return resolve(img, CREST_W_PX, CREST_H_PX)


# ---------------------------------------------------------------- shards
def shard_frame():
    X, Y = grid(SHARD_W_PX, SHARD_H_PX)
    cx = SHARD_W_PX / PX_PER_UNIT / 2
    base_y = (SHARD_H_PX - SHARD_PAD_PX) / PX_PER_UNIT
    tip_y = base_y - SHARD_LEN_U
    wide_y = base_y - SHARD_WIDEST_U
    d = sd_polygon(X, Y, [(cx, tip_y), (cx + SHARD_HALF_W_U, wide_y), (cx, base_y), (cx - SHARD_HALF_W_U, wide_y)])
    U = SHARD_BASE_U + (base_y - Y)
    V = X - cx
    img = np.zeros((SHARD_H_PX * S, SHARD_W_PX * S, 4), np.float32)
    return X, Y, d, U, V, img


def render_shard_fill(stops, glow):
    X, Y, d, U, V, img = shard_frame()
    over(img, hexc('#ff3a20'), blur_cov(cov(d), 5) * glow)
    over(img, INK, cov(d - 2.5))
    col = ramp((U - SHARD_BASE_U) / SHARD_LEN_U, stops)
    col = np.where((V > 0)[..., None], col * 0.72, np.clip(col * 1.12, 0, 1))
    over(img, col, cov(d + 1))
    over(img, hexc('#ffe6d0'), cov(d + 1) * np.clip(1 - np.abs(V + 3) / 1.2, 0, 1) * 0.55)
    return resolve(img, SHARD_W_PX, SHARD_H_PX)


def render_shard_trail():
    X, Y, d, U, V, img = shard_frame()
    over(img, TRAIL, cov(d + 1) * 0.9)
    return resolve(img, SHARD_W_PX, SHARD_H_PX)


def render_shard_empty():
    X, Y, d, U, V, img = shard_frame()
    over(img, INK, cov(d - 2.5))
    over(img, hexc('#1a0f13'), cov(d))
    over(img, hexc('#3a2a30'), cov(d) * (V > 0) * 0.5)
    crack = np.abs(V - 5 * np.sin(U * 0.21)) < 0.9
    over(img, hexc('#6a5a62'), cov(d + 2) * crack * 0.6)
    return resolve(img, SHARD_W_PX, SHARD_H_PX)


# ---------------------------------------------------------------- output
SPRITES = {
    'BossHealthCrest': render_crest,
    'BossHealthCrestGlow': render_crest_glow,
    'BossHealthShard_Fill': lambda: render_shard_fill(NORMAL, 0.45),
    'BossHealthShard_FillDanger': lambda: render_shard_fill(DANGER, 0.8),
    'BossHealthShard_Trail': render_shard_trail,
    'BossHealthShard_Empty': render_shard_empty,
}

GEOMETRY = {
    'shardCount_': SHARD_COUNT,
    'arcAngle_deg_': ARC_DEG,
    'shardBaseDistance_': SHARD_BASE_U * PX_PER_UNIT,
    'shardPadding_px_': SHARD_PAD_PX,
    'pivotOffset_': (0.0, ROOT_TO_PIVOT_PX),
    'crestOffset_': (0.0, CREST_OFFSET_U * PX_PER_UNIT),
    'BossName localPos': (0.0, NAME_TOP_PX),
}


def write_sprite(out_dir, name, image):
    png = out_dir / f"{name}.png"
    image.save(png)
    meta = out_dir / f"{name}.png.meta"
    if meta.exists():
        guid = sprite_meta.read_meta(meta)["guid"]
    else:
        guid = sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    return guid


# ---------------------------------------------------------------- preview (mirrors BossHealthGauge::OnUserInterfaceRender)
def premul(im):
    a = np.asarray(im, np.float32) / 255
    a[..., :3] *= a[..., 3:]
    return a


def composite_rotated(canvas, sprite, src_y, pivot, angle_rad):
    w, h = sprite.size
    layer = Image.new('RGBA', (canvas.shape[1], canvas.shape[0]), (0, 0, 0, 0))
    part = sprite.crop((0, src_y, w, h))
    pivot_in_sprite_y = h - SHARD_PAD_PX + SHARD_BASE_U * PX_PER_UNIT
    layer.paste(part, (int(round(pivot[0] - w / 2)), int(round(pivot[1] - pivot_in_sprite_y + src_y))))
    layer = layer.rotate(-math.degrees(angle_rad), resample=Image.BICUBIC, center=pivot)
    src = premul(layer)
    canvas[:] = src + canvas * (1 - src[..., 3:])


def shard_src_y(fill):
    if fill >= 1.0:
        return 0
    top = (SHARD_H_PX - SHARD_PAD_PX) - fill * (SHARD_H_PX - 2 * SHARD_PAD_PX)
    return int(math.floor(top))


def render_preview(sprites, rate, trail, danger, name):
    W, H = 420, 290
    root = (W / 2, 116.0)
    pivot = (root[0], root[1] + ROOT_TO_PIVOT_PX)
    canvas = np.zeros((H, W, 4), np.float32)
    canvas[..., :3] = hexc('#161a22')
    canvas[..., 3] = 1
    for i in range(SHARD_COUNT):
        angle = math.radians(-ARC_DEG / 2 + ARC_DEG * i / (SHARD_COUNT - 1))
        fill = float(np.clip(rate * SHARD_COUNT - i, 0, 1))
        trail_fill = float(np.clip(trail * SHARD_COUNT - i, 0, 1))
        composite_rotated(canvas, sprites['BossHealthShard_Empty'], 0, pivot, angle)
        if trail_fill > fill:
            composite_rotated(canvas, sprites['BossHealthShard_Trail'], shard_src_y(trail_fill), pivot, angle)
        if fill > 0:
            key = 'BossHealthShard_FillDanger' if danger else 'BossHealthShard_Fill'
            composite_rotated(canvas, sprites[key], shard_src_y(fill), pivot, angle)
    crest = sprites['BossHealthCrest']
    crest_pos = (int(round(pivot[0] - crest.width / 2)), int(round(pivot[1] + CREST_OFFSET_U * PX_PER_UNIT - crest.height / 2)))
    layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    layer.paste(crest, crest_pos)
    src = premul(layer)
    canvas[:] = src + canvas * (1 - src[..., 3:])
    if danger:
        glow_layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        glow_layer.paste(sprites['BossHealthCrestGlow'], crest_pos)
        canvas[..., :3] = np.clip(canvas[..., :3] + premul(glow_layer)[..., :3] * (90 / 255), 0, 1)

    out = Image.fromarray((np.clip(canvas, 0, 1) * 255).astype(np.uint8), 'RGBA')
    font = ImageFont.truetype(str(REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'onryou.ttf'), 34)
    draw = ImageDraw.Draw(out)
    top = (root[0], root[1] + NAME_TOP_PX)
    for ox, oy in ((-1.7, 0), (1.7, 0), (0, -1.7), (0, 1.7), (-1.2, -1.2), (1.2, -1.2), (-1.2, 1.2), (1.2, 1.2), (0, 2.2)):
        draw.text((top[0] + ox, top[1] + oy), name, font=font, fill=(6, 20, 26, 255), anchor='mt')
    draw.text(top, name, font=font, fill=(245, 232, 214, 255), anchor='mt')
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/BossHealth')
    ap.add_argument('--preview', help='also write a 100%%/60%%/22%% HP composite to this PNG path')
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    sprites = {}
    for name, fn in SPRITES.items():
        sprites[name] = fn()
        guid = write_sprite(out_dir, name, sprites[name])
        print(f"{name:28s} {sprites[name].size[0]}x{sprites[name].size[1]}  {guid}")

    print("GEOMETRY (BossHealthGauge / prefab):")
    for key, value in GEOMETRY.items():
        print(f"  {key} = {value}")

    if args.preview:
        states = [(1.0, 1.0, False), (0.6, 0.72, False), (0.22, 0.3, True)]
        panels = [render_preview(sprites, *s, 'ボス名') for s in states]
        sheet = Image.new('RGBA', (sum(p.width for p in panels), panels[0].height))
        for i, p in enumerate(panels):
            sheet.paste(p, (i * p.width, 0))
        sheet.convert('RGB').save(args.preview)
        print(f"preview -> {args.preview}")


if __name__ == '__main__':
    main()
