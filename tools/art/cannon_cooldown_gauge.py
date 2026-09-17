"""Generate the cannon cooldown gauge sprites (steel medallion + bomb) drawn by GamePlay::Ui::CannonCooldownGauge.

    python tools/art/cannon_cooldown_gauge.py [--out-dir Assets/Art/UI/CannonCooldown] [--preview PATH]

Writes CannonGauge* / CannonBomb* / CannonPrompt* PNGs and a SpriteFile .png.meta for each (an existing .meta keeps
its GUID, so regenerating never breaks references). Every sprite is authored in screen pixels around the gauge root
(the medallion centre); the offsets the component has to agree with are printed at the end (GEOMETRY).
The fill rings are drawn with DxLib DrawCircleGaugeF, so their images keep a transparent border.
--preview composites cooling / ready-burst / ready-idle / shooting frames the same way the component draws them.

SDF-based at 4x supersampling. Requires Pillow + numpy.
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
TAU = 2 * math.pi

GAUGE_RADIUS = 67.0
FRAME_PX = 184
FILL_PX = 148
HALO_PX = 232
SHOCK_PX = 232
SHOCK_RADIUS = 100.0
TIP_PX = 24
BOMB_W_PX, BOMB_H_PX = 64, 72
BOMB_BALL_IN_SPRITE = (32.0, 40.0)
BOMB_BALL_R = 25.0
BOMB_PIVOT_IN_SPRITE = (BOMB_BALL_IN_SPRITE[0], BOMB_BALL_IN_SPRITE[1] + BOMB_BALL_R)
BOMB_BALL_OFFSET = (-2.0, 5.0)
FUSE_TIP_FROM_BALL = (25.0, -38.0)
SPARK_PX = 64
EMBER_PX = 12
EMBER_R = 3.6
PILL_W_PX, PILL_H_PX = 212, 80
PILL_OFFSET = (-150.0, 0.0)
MOUSE_W_PX, MOUSE_H_PX = 28, 36
MOUSE_OFFSET = (-210.0, 0.0)
PROMPT_TEXT_OFFSET = (-138.0, -1.0)
COUNT_TEXT_OFFSET = (0.0, 2.0)


def hexc(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)], dtype=np.float32)


INK = hexc('#0b0d10')
TEAL = [(0, '#146b5c'), (0.6, '#2fb592'), (1, '#6fe8c6')]
GOLD = [(0, '#b86a12'), (0.6, '#ffb23a'), (1, '#ffe7a0')]
STEEL = [(0.0, '#c2cad3'), (0.16, '#88909a'), (0.5, '#4b515a'), (0.84, '#2c3036'), (1.0, '#1b1e22')]


# ---------------------------------------------------------------- raster helpers
def grid(w_px, h_px):
    ys, xs = np.mgrid[0:h_px * S, 0:w_px * S].astype(np.float32)
    return (xs + 0.5) / S, (ys + 0.5) / S


def canvas(w_px, h_px):
    return np.zeros((h_px * S, w_px * S, 4), np.float32)


def cov(sd):
    return np.clip(0.5 - sd * S, 0, 1).astype(np.float32)


def blur_cov(c, radius_px):
    im = Image.fromarray((np.clip(c, 0, 1) * 255).astype(np.uint8), 'L')
    im = im.filter(ImageFilter.GaussianBlur(radius_px * S))
    return np.asarray(im, np.float32) / 255


def over(img, rgb, a):
    a = np.clip(np.asarray(a, np.float32), 0, 1)[..., None]
    img[..., :3] = np.asarray(rgb, np.float32) * a + img[..., :3] * (1 - a)
    img[..., 3:] = a + img[..., 3:] * (1 - a)


def resolve(img, w_px, h_px):
    c = img.reshape(h_px, S, w_px, S, 4).mean(axis=(1, 3))
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


def metal(Y, top, bottom):
    return ramp((Y - top) / (bottom - top), STEEL)


# ---------------------------------------------------------------- SDF helpers
def sd_circle(x, y, cx, cy, r):
    return np.hypot(x - cx, y - cy) - r


def sd_rbox(x, y, x0, y0, x1, y1, r):
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2 - r, (y1 - y0) / 2 - r
    qx, qy = np.abs(x - cx) - hx, np.abs(y - cy) - hy
    return np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - r


def sd_segment(x, y, ax, ay, bx, by, r):
    pax, pay, bax, bay = x - ax, y - ay, bx - ax, by - ay
    h = np.clip((pax * bax + pay * bay) / (bax * bax + bay * bay), 0, 1)
    return np.hypot(pax - bax * h, pay - bay * h) - r


# ---------------------------------------------------------------- medallion
def render_frame():
    X, Y = grid(FRAME_PX, FRAME_PX)
    img = canvas(FRAME_PX, FRAME_PX)
    c = FRAME_PX / 2
    d = np.hypot(X - c, Y - c)
    over(img, (0, 0, 0), blur_cov(cov(d - 80), 6) * 0.55)
    over(img, INK, cov(d - 80))
    over(img, metal(Y, c - 80, c + 80), cov(np.maximum(d - 78, 56 - d)))
    over(img, hexc('#ffffff'), (cov(d - 78) - cov(d - 76.5)) * np.clip((c - Y) / 80, 0, 1) * 0.6)
    over(img, hexc('#0a0e11'), cov(np.abs(d - GAUGE_RADIUS) - 6.5))
    over(img, INK, cov(d - 56))
    over(img, ramp(d / 54, [(0, '#23353a'), (1, '#070b0d')]), cov(d - 53))
    return resolve(img, FRAME_PX, FRAME_PX)


def render_fill(stops):
    X, Y = grid(FILL_PX, FILL_PX)
    img = canvas(FILL_PX, FILL_PX)
    c = FILL_PX / 2
    d = np.hypot(X - c, Y - c)
    a = np.arctan2(X - c, -(Y - c)) % TAU
    rgb = ramp(a / TAU * 0.6 + 0.4, stops) if stops else np.ones(X.shape + (3,), np.float32)
    over(img, rgb, cov(np.abs(d - GAUGE_RADIUS) - 4.5))
    return resolve(img, FILL_PX, FILL_PX)


def render_tip():
    X, Y = grid(TIP_PX, TIP_PX)
    img = canvas(TIP_PX, TIP_PX)
    c = TIP_PX / 2
    over(img, hexc('#c8fff0'), blur_cov(cov(sd_circle(X, Y, c, c, 4)), 4) * 0.9)
    return resolve(img, TIP_PX, TIP_PX)


def render_halo():
    X, Y = grid(HALO_PX, HALO_PX)
    img = canvas(HALO_PX, HALO_PX)
    c = HALO_PX / 2
    over(img, hexc('#ffb23a'), blur_cov(cov(sd_circle(X, Y, c, c, 82)), 9))
    return resolve(img, HALO_PX, HALO_PX)


def render_shockwave():
    X, Y = grid(SHOCK_PX, SHOCK_PX)
    img = canvas(SHOCK_PX, SHOCK_PX)
    c = SHOCK_PX / 2
    ring = np.abs(np.hypot(X - c, Y - c) - SHOCK_RADIUS)
    over(img, hexc('#ffd88a'), blur_cov(cov(ring - 2.0), 2.5))
    over(img, hexc('#fff6dc'), cov(ring - 1.0) * 0.8)
    return resolve(img, SHOCK_PX, SHOCK_PX)


# ---------------------------------------------------------------- bomb + spark
def render_bomb():
    X, Y = grid(BOMB_W_PX, BOMB_H_PX)
    img = canvas(BOMB_W_PX, BOMB_H_PX)
    bx, by = BOMB_BALL_IN_SPRITE
    ball = sd_circle(X, Y, bx, by, BOMB_BALL_R)
    fuse = sd_segment(X, Y, bx + 15, by - 24, bx + 24, by - 36, 2.2)
    over(img, hexc('#7a5a34'), cov(fuse))
    over(img, INK, cov(ball - 2))
    hl = np.clip(1 - np.hypot(X - (bx - 10), Y - (by - 10)) / 37.5, 0, 1) ** 1.6
    rgb = hexc('#15181b')[None, None] * (1 - hl[..., None]) + hexc('#a4adb5')[None, None] * hl[..., None]
    over(img, rgb, cov(ball))
    cap = sd_rbox(X, Y, bx + 9, by - 26, bx + 19, by - 17, 2)
    over(img, INK, cov(cap - 1))
    over(img, metal(Y, by - 26, by - 17), cov(cap))
    return resolve(img, BOMB_W_PX, BOMB_H_PX)


def render_spark():
    X, Y = grid(SPARK_PX, SPARK_PX)
    img = canvas(SPARK_PX, SPARK_PX)
    c = SPARK_PX / 2
    over(img, hexc('#ff8a20'), blur_cov(cov(sd_circle(X, Y, c, c, 6)), 7) * 0.9)
    for k in range(4):
        a = k * TAU / 8
        dx, dy = math.cos(a) * 12, math.sin(a) * 12
        over(img, hexc('#ffd27a'), cov(sd_segment(X, Y, c - dx, c - dy, c + dx, c + dy, 1.1)) * 0.9)
    over(img, hexc('#fffbe8'), cov(sd_circle(X, Y, c, c, 3.4)))
    return resolve(img, SPARK_PX, SPARK_PX)


def render_ember():
    X, Y = grid(EMBER_PX, EMBER_PX)
    img = canvas(EMBER_PX, EMBER_PX)
    c = EMBER_PX / 2
    over(img, hexc('#ffc452'), cov(sd_circle(X, Y, c, c, EMBER_R)))
    return resolve(img, EMBER_PX, EMBER_PX)


# ---------------------------------------------------------------- prompt
def pill_sd(X, Y):
    cx, cy = PILL_W_PX / 2, PILL_H_PX / 2
    return sd_rbox(X, Y, cx - 90, cy - 24, cx + 90, cy + 24, 24)


def render_pill():
    X, Y = grid(PILL_W_PX, PILL_H_PX)
    img = canvas(PILL_W_PX, PILL_H_PX)
    p = pill_sd(X, Y)
    over(img, (0, 0, 0), blur_cov(cov(p), 5) * 0.5)
    over(img, INK, cov(p) * 0.88)
    over(img, hexc('#6b737c'), cov(p) - cov(p + 1.6))
    return resolve(img, PILL_W_PX, PILL_H_PX)


def render_pill_glow():
    X, Y = grid(PILL_W_PX, PILL_H_PX)
    img = canvas(PILL_W_PX, PILL_H_PX)
    p = pill_sd(X, Y)
    over(img, hexc('#ffb23a'), blur_cov(cov(p) - cov(p + 2), 3))
    return resolve(img, PILL_W_PX, PILL_H_PX)


def render_mouse(lit):
    X, Y = grid(MOUSE_W_PX, MOUSE_H_PX)
    img = canvas(MOUSE_W_PX, MOUSE_H_PX)
    x, y = MOUSE_W_PX / 2, MOUSE_H_PX / 2
    color = hexc('#d6dde2') * (1.0 if lit else 0.6)
    accent = hexc('#ffb23a') if lit else hexc('#5a6168')
    m = sd_rbox(X, Y, x - 9, y - 14, x + 9, y + 14, 8)
    over(img, color, cov(m) - cov(m + 2))
    over(img, color, cov(np.abs(X - x) - 0.8) * (Y > y - 13) * (Y < y - 1) * cov(m + 2))
    over(img, color, cov(np.abs(Y - (y - 1)) - 0.8) * cov(m + 2))
    over(img, accent, cov(m + 2.5) * (X < x - 1.6) * (Y < y - 2.6))
    return resolve(img, MOUSE_W_PX, MOUSE_H_PX)


# ---------------------------------------------------------------- output
SPRITES = {
    'CannonGaugeFrame': render_frame,
    'CannonGaugeFill_Teal': lambda: render_fill(TEAL),
    'CannonGaugeFill_Gold': lambda: render_fill(GOLD),
    'CannonGaugeFill_Flash': lambda: render_fill(None),
    'CannonGaugeTip': render_tip,
    'CannonGaugeHalo': render_halo,
    'CannonGaugeShockwave': render_shockwave,
    'CannonBomb': render_bomb,
    'CannonBombSpark': render_spark,
    'CannonBombEmber': render_ember,
    'CannonPromptPill': render_pill,
    'CannonPromptPillGlow': render_pill_glow,
    'CannonPromptMouse': lambda: render_mouse(False),
    'CannonPromptMouseLit': lambda: render_mouse(True),
}

GEOMETRY = {
    'gaugeRadius_': GAUGE_RADIUS,
    'shockwaveSpriteRadius_': SHOCK_RADIUS,
    'emberSpriteRadius_': EMBER_R,
    'bombPivotOffset_': (BOMB_BALL_OFFSET[0], BOMB_BALL_OFFSET[1] + BOMB_BALL_R),
    'bombPivotInSprite_': BOMB_PIVOT_IN_SPRITE,
    'sparkOffsetFromBombPivot_': (FUSE_TIP_FROM_BALL[0], FUSE_TIP_FROM_BALL[1] - BOMB_BALL_R),
    'pillOffset_': PILL_OFFSET,
    'mouseOffset_': MOUSE_OFFSET,
    'promptTextOffset_': PROMPT_TEXT_OFFSET,
    'countTextOffset_': COUNT_TEXT_OFFSET,
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


# ---------------------------------------------------------------- preview (mirrors CannonCooldownGauge::OnUserInterfaceRender)
def premul(im):
    a = np.asarray(im, np.float32) / 255
    a[..., :3] *= a[..., 3:]
    return a


def paste(cv, sprite, centre, scale=1.0, angle_rad=0.0, pivot=None, alpha=1.0, bright=1.0, add=False):
    w, h = sprite.size
    px, py = pivot if pivot else (w / 2, h / 2)
    im = sprite
    if bright != 1.0:
        arr = np.asarray(im, np.float32).copy()
        arr[..., :3] *= bright
        im = Image.fromarray(arr.clip(0, 255).astype(np.uint8), 'RGBA')
    ow, oh = max(1, round(w * scale)), max(1, round(h * scale))
    im = im.resize((ow, oh), Image.BICUBIC)
    layer = Image.new('RGBA', (cv.shape[1], cv.shape[0]), (0, 0, 0, 0))
    ox, oy = centre[0] - px * scale, centre[1] - py * scale
    layer.paste(im, (int(round(ox)), int(round(oy))))
    if angle_rad:
        layer = layer.rotate(-math.degrees(angle_rad), resample=Image.BICUBIC, center=centre)
    src = premul(layer) * alpha
    if add:
        cv[..., :3] = np.clip(cv[..., :3] + src[..., :3], 0, 1)
    else:
        cv[:] = src + cv * (1 - src[..., 3:])


def circle_gauge(cv, sprite, centre, percent, scale=1.0, alpha=1.0, add=False):
    w, h = sprite.size
    ys, xs = np.mgrid[0:h, 0:w].astype(np.float32)
    a = np.arctan2(xs + 0.5 - w / 2, -(ys + 0.5 - h / 2)) % TAU
    arr = np.asarray(sprite).copy()
    arr[..., 3] = (arr[..., 3] * (a <= percent / 100 * TAU)).astype(np.uint8)
    paste(cv, Image.fromarray(arr, 'RGBA'), centre, scale, alpha=alpha, add=add)


def text(img, xy, s, size, rgb, alpha=1.0):
    layer = Image.new('RGBA', img.size, (0, 0, 0, 0))
    ImageDraw.Draw(layer).text(xy, s, font=ImageFont.truetype(str(REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'onryou.ttf'), size),
                               fill=rgb + (255,), anchor='mm', stroke_width=max(1, round(3 * size / 60)), stroke_fill=(6, 20, 26))
    arr = np.asarray(layer).copy()
    arr[..., 3] = (arr[..., 3] * alpha).astype(np.uint8)
    return Image.alpha_composite(img, Image.fromarray(arr))


def render_preview(sprites, state):
    W, H = 480, 260
    root = (330.0, 130.0)
    cv = np.zeros((H, W, 4), np.float32)
    cv[..., :3] = hexc('#3a4a36')
    cv[..., 3] = 1
    sc = state.get('scale', 1.0)
    add = lambda o: (root[0] + o[0], root[1] + o[1])  # noqa: E731

    paste(cv, sprites['CannonPromptPill'], add(PILL_OFFSET))
    if state.get('pill_flash', 0) > 0:
        paste(cv, sprites['CannonPromptPillGlow'], add(PILL_OFFSET), alpha=state['pill_flash'])
    lit = state['ready']
    paste(cv, sprites['CannonPromptMouseLit' if lit else 'CannonPromptMouse'], add(MOUSE_OFFSET))
    if state.get('glow', 0) > 0:
        paste(cv, sprites['CannonGaugeHalo'], root, sc, alpha=min(1.0, state['glow']))
    if 'shock' in state:
        r, a = state['shock']
        paste(cv, sprites['CannonGaugeShockwave'], root, r / SHOCK_RADIUS, alpha=a, add=True)
    paste(cv, sprites['CannonGaugeFrame'], root, sc)
    circle_gauge(cv, sprites['CannonGaugeFill_Gold' if state['gold'] else 'CannonGaugeFill_Teal'], root, state['percent'], sc)
    if state.get('flash', 0) > 0:
        circle_gauge(cv, sprites['CannonGaugeFill_Flash'], root, state['percent'], sc, alpha=state['flash'])
    if not state['gold'] and state['percent'] > 1:
        ang = state['percent'] / 100 * TAU
        paste(cv, sprites['CannonGaugeTip'], (root[0] + math.sin(ang) * GAUGE_RADIUS * sc, root[1] - math.cos(ang) * GAUGE_RADIUS * sc))
    pivot = (root[0] + BOMB_BALL_OFFSET[0] * sc, root[1] + (BOMB_BALL_OFFSET[1] + BOMB_BALL_R) * sc)
    paste(cv, sprites['CannonBomb'], pivot, sc, state.get('rot', 0.0), BOMB_PIVOT_IN_SPRITE, bright=1 - state.get('dim', 0.8))
    if state.get('spark', 0) > 0:
        lx, ly = FUSE_TIP_FROM_BALL[0], FUSE_TIP_FROM_BALL[1] - BOMB_BALL_R
        r = state.get('rot', 0.0)
        sx = pivot[0] + (lx * math.cos(r) - ly * math.sin(r)) * sc
        sy = pivot[1] + (lx * math.sin(r) + ly * math.cos(r)) * sc
        paste(cv, sprites['CannonBombSpark'], (sx, sy), state['spark'] * sc, 0.4)
    out = Image.fromarray((np.clip(cv, 0, 1) * 255).astype(np.uint8), 'RGBA')
    out = text(out, add(PROMPT_TEXT_OFFSET), '発射' if lit else '装填中', 26 if lit else 24,
               (255, 214, 130) if lit else (170, 180, 186))
    if 'count' in state:
        out = text(out, add(COUNT_TEXT_OFFSET), state['count'], 58, (255, 255, 255))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/CannonCooldown')
    ap.add_argument('--preview', help='also write a cooling / burst / ready / shot composite to this PNG path')
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    sprites = {}
    for name, fn in SPRITES.items():
        sprites[name] = fn()
        guid = write_sprite(out_dir, name, sprites[name])
        print(f"{name:24s} {sprites[name].size[0]}x{sprites[name].size[1]}  {guid}")

    print("GEOMETRY (CannonCooldownGauge):")
    for key, value in GEOMETRY.items():
        print(f"  {key} = {value}")

    if args.preview:
        states = [
            dict(ready=False, gold=False, percent=38, count='7'),
            dict(ready=True, gold=True, percent=100, flash=0.6, scale=1.1, dim=0.4, glow=0.8, pill_flash=0.6,
                 shock=(110, 0.6), spark=1.2),
            dict(ready=True, gold=True, percent=100, dim=0.0, glow=0.6, spark=1.0, rot=math.radians(4)),
            dict(ready=False, gold=False, percent=1, count='10'),
        ]
        panels = [render_preview(sprites, s) for s in states]
        sheet = Image.new('RGBA', (panels[0].width * 2, panels[0].height * 2))
        for i, p in enumerate(panels):
            sheet.paste(p, ((i % 2) * p.width, (i // 2) * p.height))
        sheet.convert('RGB').save(args.preview)
        print(f"preview -> {args.preview}")


if __name__ == '__main__':
    main()
