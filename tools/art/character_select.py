"""キャラ選択 UI のスプライトと、実際のゲーム画面に合成したデザインモックを生成する。

    python tools/art/character_select.py --shot <screenshot.png> --out-dir <dir>

Phase A では --shot に実際のゲーム画面を渡して、UI を合成した 1920x1080 の完成イメージを出す。

方向性: 平らな角丸パネルに細い光る縁、という既製品っぽい見た目を避ける。このゲームの既存HUDは
彫り込んだ金属のポートレート枠と紋章のボスゲージなので、UI もその語彙に寄せる。
酒場で仲間を誘う導線なので、羊皮紙・木・鉄・蝋・真鍮の質感を fbm ノイズで焼き、
縁はぎざぎざに削り、部品はわずかに傾けて、機械的な整列を崩す。

合成時は「切り替え中は体力HUDを隠す」という決定に合わせて左上のHUDを消し、
拠点には出ないボスゲージも消してから UI を載せる。

Pillow と numpy が必要。
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

SCREEN_W, SCREEN_H = 1920, 1080

BODY_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'ZenOldMincho-Bold.ttf'
BRUSH_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'onryou.ttf'

INK = (48, 30, 20)
INK_FADE = (104, 78, 54)
STAMP_RED = (146, 38, 30)
PARCH = np.array([0.76, 0.68, 0.52], np.float32)
PARCH_OLD = np.array([0.62, 0.54, 0.40], np.float32)
WAX = np.array([0.46, 0.09, 0.08], np.float32)
BRASS = np.array([0.62, 0.45, 0.17], np.float32)
STEEL = np.array([0.17, 0.18, 0.20], np.float32)
CANDLE = np.array([1.0, 0.74, 0.38], np.float32)

ROSTER = [
    {
        'name': '剣士',
        'reading': 'SWORDMAN',
        'tagline': '斧一本で前に出る',
        'lines': ['斧を振るう近接の要。', '溜め攻撃と回避に長ける。'],
        'stats': [('腕っぷし', 4), ('しぶとさ', 4), ('身軽さ', 3)],
        'locked': False,
        'tilt': -2.4,
    },
    {
        'name': '魔術師',
        'reading': 'MAGIC CASTER',
        'tagline': '離れて撃つ',
        'lines': ['遠くから魔弾を放つ。', '打たれ弱いが手数で押す。'],
        'stats': [('腕っぷし', 3), ('しぶとさ', 2), ('身軽さ', 4)],
        'locked': False,
        'tilt': 1.9,
    },
    {
        'name': '？？？',
        'reading': '',
        'tagline': '',
        'lines': ['まだ雇えない。'],
        'stats': [('腕っぷし', 0), ('しぶとさ', 0), ('身軽さ', 0)],
        'locked': True,
        'tilt': -1.2,
    },
]
SELECTED = 0


# ---------------------------------------------------------------- 質感のもと
def fbm(w, h, seed, octaves=5, base=4):
    rng = np.random.default_rng(seed)
    acc = np.zeros((h, w), np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        gw = max(2, int(base * 2 ** o))
        gh = max(2, int(base * 2 ** o * h / max(w, 1)))
        g = (rng.random((gh, gw)) * 255).astype(np.uint8)
        up = Image.fromarray(g, 'L').resize((w, h), Image.BICUBIC)
        acc += np.asarray(up, np.float32) / 255.0 * amp
        total += amp
        amp *= 0.5
    return acc / total


def rgba(rgb, alpha):
    out = np.concatenate([np.clip(rgb, 0, 1), np.clip(alpha, 0, 1)[..., None]], axis=-1)
    return Image.fromarray((out * 255 + 0.5).astype(np.uint8), 'RGBA')


def grid(w, h):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    return xx, yy


def rect_outside(xx, yy, w, h, inset):
    return np.maximum.reduce([inset - xx, xx - (w - 1 - inset),
                              inset - yy, yy - (h - 1 - inset)])


def soften(mask, r=0.8):
    im = Image.fromarray((np.clip(mask, 0, 1) * 255).astype(np.uint8), 'L')
    return np.asarray(im.filter(ImageFilter.GaussianBlur(r)), np.float32) / 255.0


def bevel(mask, r=3.0):
    """マスクの縁の法線っぽいもの。上が明るく下が暗い立体感を作る"""
    m = soften(mask, r)
    gy, gx = np.gradient(m)
    return np.clip(-gy * 6 - gx * 3, -1, 1)


def drop_shadow(part, blur_r=10, offset=(6, 9), alpha=0.62):
    a = np.asarray(part.split()[-1], np.float32) / 255.0
    pad = blur_r * 3
    canvas = np.zeros((a.shape[0] + pad * 2, a.shape[1] + pad * 2), np.float32)
    canvas[pad + offset[1]:pad + offset[1] + a.shape[0],
           pad + offset[0]:pad + offset[0] + a.shape[1]] = a
    canvas = soften(canvas, blur_r) * alpha
    return rgba(np.zeros(canvas.shape + (3,), np.float32), canvas)


# ---------------------------------------------------------------- 素材
def parchment(w, h, seed, aged=0.0, ragged=13.0, curl=True):
    """羊皮紙。縁をちぎり、繊維と染みを焼き、угол を少し反らせる"""
    xx, yy = grid(w, h)
    n = fbm(w, h, seed, octaves=4, base=3)
    d = rect_outside(xx, yy, w, h, 7.0) + (n - 0.5) * ragged
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)

    fiber = 0.84 + 0.22 * fbm(w, h, seed + 7, octaves=6, base=22)
    stain = 1 - 0.34 * np.clip(fbm(w, h, seed + 13, octaves=3, base=2) * 1.9 - 0.75, 0, 1)
    burn = 1 - 0.50 * np.clip((d + 20) / 20, 0, 1)
    tint = PARCH * (1 - aged) + PARCH_OLD * aged
    shade = fiber * stain * burn
    if curl:
        # 紙の反り。斜めに淡い明暗を乗せて平面に見せない
        shade = shade * (0.90 + 0.18 * np.clip((xx / w + yy / h) / 2 * 1.4, 0, 1))
    rgb = tint[None, None, :] * shade[..., None]
    return rgba(rgb, mask)


def wood_board(w, h, seed, plank=138):
    """酒場の板。木目はノイズで歪ませて、規則的な縞に見えないようにする"""
    xx, yy = grid(w, h)
    warp = fbm(w, h, seed, octaves=5, base=3) * 54
    rings = fbm(w, h, seed + 2, octaves=3, base=2)
    grain = np.sin((yy + warp) * 0.17 + rings * 5.0) * 0.5 + 0.5
    grain = grain * 0.55 + fbm(w, h, seed + 4, octaves=6, base=26) * 0.45
    tone = 0.26 + 0.34 * grain
    seam = np.clip(1 - np.abs((yy % plank) - 2.0) / 3.0, 0, 1)
    tone = tone * (1 - 0.70 * seam)
    knot = np.clip(1 - np.abs(np.hypot(xx - w * 0.72, yy - h * 0.31) - 13) / 11, 0, 1)
    tone = tone * (1 - 0.45 * knot)
    rgb = np.array([0.36, 0.23, 0.13], np.float32)[None, None, :] * tone[..., None] * 2.3
    d = rect_outside(xx, yy, w, h, 2.0)
    return rgba(rgb, soften(np.clip(0.5 - d, 0, 1), 0.6))


def nail(size=22, seed=0):
    xx, yy = grid(size, size)
    r = size / 2
    d = np.hypot(xx - r + 0.5, yy - r + 0.5) - (r - 2)
    mask = soften(np.clip(0.5 - d, 0, 1), 0.5)
    lit = np.clip(((r - xx) + (r - yy)) / (size * 1.0) + 0.45, 0, 1)
    rgb = STEEL[None, None, :] * (1.2 + 3.4 * lit)[..., None]
    rgb = rgb * (1 - 0.6 * np.clip((d + 5) / 5, 0, 1))[..., None]
    return rgba(rgb, mask)


def wax_seal(size=82, seed=5):
    """押しつぶした蝋。縁を不規則にして、押した窪みを陰影で作る"""
    xx, yy = grid(size, size)
    r = size / 2
    n = fbm(size, size, seed, octaves=3, base=5)
    rad = np.hypot(xx - r, yy - r)
    d = rad - (r - 6) + (n - 0.5) * 16
    mask = soften(np.clip(0.5 - d, 0, 1), 0.8)

    press = np.clip(-(d + 12) / 12, 0, 1)                      # 中央の窪み
    ridge = np.clip(1 - np.abs(rad - r * 0.36) / 4.5, 0, 1)    # 押印の輪郭
    b = bevel(mask, 2.5)
    shade = 0.70 + 0.55 * b - 0.30 * press + 0.35 * ridge
    rgb = WAX[None, None, :] * np.clip(shade, 0.15, 2.0)[..., None] * 1.9
    return rgba(rgb, mask)


def iron_plate(w, h, seed=21, notch=26):
    """既存の体力バーに合わせた、両端が絞られた鉄板。彫り込みが見えるよう暗くする"""
    xx, yy = grid(w, h)
    taper = np.clip(np.minimum(xx, w - 1 - xx) / notch, 0, 1)
    d = np.maximum(rect_outside(xx, yy, w, h, 3.0),
                   np.abs(yy - h / 2) - (h / 2 - 3) * (0.42 + 0.58 * taper))
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)
    b = bevel(mask, 3.5)
    hammer = 0.82 + 0.36 * fbm(w, h, seed, octaves=5, base=14)
    rgb = STEEL[None, None, :] * (hammer * (1.0 + 1.9 * b))[..., None] * 2.6
    rim = np.clip(1 - np.abs(d + 3.0) / 2.6, 0, 1)
    rgb = rgb + BRASS[None, None, :] * (rim * 0.85)[..., None]
    return rgba(rgb, mask)


def brass_ring(size, thickness, rivets=10, seed=11):
    """既存HUDのポートレート枠に合わせた真鍮の環"""
    xx, yy = grid(size, size)
    r = size / 2
    rad = np.hypot(xx - r + 0.5, yy - r + 0.5)
    mid = r - thickness / 2 - 2
    d = np.abs(rad - mid) - thickness / 2
    mask = soften(np.clip(0.5 - d, 0, 1), 0.6)
    # 環の断面を丸く見せる
    across = np.clip((rad - (mid - thickness / 2)) / max(thickness, 1), 0, 1)
    round_ = np.sin(np.clip(across, 0, 1) * np.pi)
    light = np.clip(((r - xx) + (r - yy) * 1.4) / (size * 1.2) + 0.5, 0, 1)
    patina = 0.80 + 0.40 * fbm(size, size, seed, octaves=5, base=7)
    rgb = BRASS[None, None, :] * ((0.35 + 1.55 * round_ * (0.35 + light)) * patina)[..., None]

    for k in range(rivets):
        a = k / rivets * 2 * np.pi
        rx, ry = r + np.cos(a) * mid, r + np.sin(a) * mid
        rd = np.hypot(xx - rx, yy - ry) - thickness * 0.24
        rm = soften(np.clip(0.5 - rd, 0, 1), 0.5)
        rb = bevel(rm, 1.6)
        rgb = rgb * (1 - rm)[..., None] + (BRASS[None, None, :] * (0.7 + 2.0 * rb)[..., None]) * rm[..., None]
    return rgba(rgb, mask)


def hanging_rope(w, h, seed=31):
    xx, yy = grid(w, h)
    sag = np.sin(np.clip(xx / max(w - 1, 1), 0, 1) * np.pi) * (h * 0.32)
    d = np.abs(yy - (h * 0.22 + sag)) - 5.0
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)
    twist = 0.55 + 0.45 * (np.sin(xx * 0.42 + yy * 0.3) * 0.5 + 0.5)
    rgb = np.array([0.44, 0.33, 0.19], np.float32)[None, None, :] * twist[..., None] * 1.5
    return rgba(rgb, mask)


# ---------------------------------------------------------------- 文字
def font(path, px):
    return ImageFont.truetype(str(path), px)


def text(base, xy, s, px, color, path=BODY_FONT, anchor='la', shadow=None, angle=0.0):
    if angle == 0.0:
        d = ImageDraw.Draw(base)
        f = font(path, px)
        if shadow:
            d.text((xy[0] + shadow[0], xy[1] + shadow[1]), s, font=f, fill=shadow[2], anchor=anchor)
        d.text(xy, s, font=f, fill=color, anchor=anchor)
        return
    pad = px * 4
    lay = Image.new('RGBA', (pad * 4, pad * 2), (0, 0, 0, 0))
    ImageDraw.Draw(lay).text((pad * 2, pad), s, font=font(path, px), fill=color, anchor='mm')
    lay = lay.rotate(angle, resample=Image.BICUBIC, center=(pad * 2, pad))
    base.alpha_composite(lay, (int(xy[0] - pad * 2), int(xy[1] - pad)))


def engrave(base, xy, s, px, path, anchor='lm', tone=(214, 208, 198)):
    """鉄板の彫り。暗い文字の1px下に明るい縁を置くと彫り込んで見える"""
    d = ImageDraw.Draw(base)
    f = font(path, px)
    d.text((xy[0], xy[1] + 2), s, font=f, fill=(*tone, 120), anchor=anchor)
    d.text(xy, s, font=f, fill=(18, 16, 15, 235), anchor=anchor)


def ink_pips(draw, x, y, filled, total=5, r=9, gap=28, color=INK, empty=INK_FADE):
    for i in range(total):
        cx = x + i * gap
        box = [cx - r, y - r, cx + r, y + r]
        if i < filled:
            draw.ellipse(box, fill=color)
        else:
            draw.ellipse(box, outline=empty, width=2)


# ---------------------------------------------------------------- 実画面の下ごしらえ
def patch_sky(im, box, feather=22, seed=77):
    """既存HUDを消す。周囲の空の色から縦グラデーションを作り直して塗り替える
    (近くを切り貼りすると地形が複製されて不自然になるため)"""
    x0, y0, x1, y1 = box
    w, h = x1 - x0, y1 - y0
    arr = np.asarray(im.convert('RGB'), np.float32) / 255.0
    H = arr.shape[0]
    top = arr[max(y0 - 16, 0):max(y0 - 2, 1), x0:x1].reshape(-1, 3).mean(0)
    bot = arr[min(y1 + 2, H - 1):min(y1 + 16, H), x0:x1].reshape(-1, 3).mean(0)
    t = np.linspace(0, 1, h, dtype=np.float32)[:, None, None]
    grad = top[None, None, :] * (1 - t) + bot[None, None, :] * t
    cloud = fbm(w, h, seed, octaves=4, base=3)[..., None]
    patch = np.clip(grad * (0.93 + 0.14 * cloud), 0, 1)
    part = Image.fromarray((patch * 255).astype(np.uint8), 'RGB')

    m = np.ones((h, w), np.float32)
    m[:feather, :] *= np.linspace(0, 1, feather)[:, None]
    m[-feather:, :] *= np.linspace(1, 0, feather)[:, None]
    m[:, :feather] *= np.linspace(0, 1, feather)[None, :]
    m[:, -feather:] *= np.linspace(1, 0, feather)[None, :]
    im.paste(part, (x0, y0), Image.fromarray((m * 255).astype(np.uint8), 'L'))


def clean_hud(base):
    """切り替え中は体力HUDを隠す決定なので消す。ボスゲージも拠点には出ないので消す。
    塗りつぶしではなく、同じ画面の綺麗な空(右上)を持ってきて貼る"""
    im = base.copy()
    sky = im.crop(SKY_SOURCE)
    for box, flip in [((0, 34, 712, 256), True), ((742, 34, 1206, 304), False), ((1262, 34, 1470, 222), True)]:
        src = sky.transpose(Image.FLIP_LEFT_RIGHT) if flip else sky
        paste_sky(im, box, src)
    return im


SKY_SOURCE = (1466, 58, 1912, 282)


def paste_sky(im, box, src, feather=26):
    x0, y0, x1, y1 = box
    w, h = x1 - x0, y1 - y0
    part = src.resize((w, h), Image.LANCZOS).convert('RGB')
    m = np.ones((h, w), np.float32)
    m[:feather, :] *= np.linspace(0, 1, feather)[:, None]
    m[-feather:, :] *= np.linspace(1, 0, feather)[:, None]
    m[:, :feather] *= np.linspace(0, 1, feather)[None, :]
    m[:, -feather:] *= np.linspace(1, 0, feather)[None, :]
    im.paste(part, (x0, y0), Image.fromarray((m * 255).astype(np.uint8), 'L'))


def tavern_grade(base, darken, lights):
    arr = np.asarray(base.convert('RGB'), np.float32) / 255.0
    xx, yy = grid(SCREEN_W, SCREEN_H)
    lit = np.zeros((SCREEN_H, SCREEN_W), np.float32)
    for cx, cy, r, strength in lights:
        lit = np.maximum(lit, np.clip(1 - np.hypot(xx - cx, yy - cy) / r, 0, 1) ** 1.9 * strength)
    arr = arr * (1 - darken + darken * 0.22)
    arr = arr + CANDLE[None, None, :] * lit[..., None] * 0.17
    arr = arr * np.array([1.07, 0.98, 0.87], np.float32)[None, None, :]
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')


def paste(base, part, x, y):
    base.alpha_composite(part, (int(x), int(y)))


def paste_tilted(base, part, cx, cy, angle, shadow=True):
    if shadow:
        sh = drop_shadow(part).rotate(angle, resample=Image.BICUBIC, expand=True)
        base.alpha_composite(sh, (int(cx - sh.width / 2), int(cy - sh.height / 2)))
    rot = part.rotate(angle, resample=Image.BICUBIC, expand=True)
    base.alpha_composite(rot, (int(cx - rot.width / 2), int(cy - rot.height / 2)))


# ---------------------------------------------------------------- 案1 の部品
def bill(w, h, ch, selected, seed):
    p = parchment(w, h, seed, aged=0.70 if ch['locked'] else 0.10)
    d = ImageDraw.Draw(p)
    d.line([(30, 34), (w - 30, 34)], fill=INK_FADE, width=2)
    text(p, (36, 54), ch['name'], 54, INK, BRUSH_FONT)
    if ch['reading']:
        text(p, (38, 122), ch['reading'], 17, INK_FADE, BODY_FONT)
    if ch['tagline']:
        text(p, (38, h - 36), ch['tagline'], 20, INK_FADE, BODY_FONT, anchor='lm')

    if ch['locked']:
        text(p, (w * 0.62, h * 0.52), '募 集 前', 46, (*STAMP_RED, 175), BRUSH_FONT, anchor='mm', angle=-8)
    if selected:
        paste(p, wax_seal(80, seed + 2), w - 104, h - 100)
        text(p, (w * 0.70, h * 0.33), '選', 76, (*STAMP_RED, 140), BRUSH_FONT, anchor='mm', angle=-11)
    return p


def ledger(w, h, ch, seed):
    """右側の帳面。罫線を引いた見開きの片面"""
    p = parchment(w, h, seed, aged=0.28, ragged=9.0)
    arr = np.asarray(p, np.float32) / 255.0
    arr[:, :64, :3] *= np.linspace(0.62, 1.0, 64)[None, :, None]   # 綴じ側の影
    p = Image.fromarray((arr * 255).astype(np.uint8), 'RGBA')
    d = ImageDraw.Draw(p)

    text(p, (w / 2, 84), ch['name'], 70, INK, BRUSH_FONT, anchor='mm')
    if ch['reading']:
        text(p, (w / 2, 142), ch['reading'], 19, INK_FADE, BODY_FONT, anchor='mm')
    d.line([(76, 180), (w - 76, 180)], fill=INK_FADE, width=3)

    y = 246
    for label, filled in ch['stats']:
        text(p, (82, y), label, 27, INK, BODY_FONT, anchor='lm')
        ink_pips(d, 256, y, filled)
        d.line([(82, y + 30), (w - 82, y + 30)], fill=(160, 136, 104), width=1)
        y += 76

    y += 24
    for line in ch['lines']:
        text(p, (82, y), line, 25, INK, BODY_FONT, anchor='lm')
        y += 44
    return p


def tavern_sign(w, h, label):
    board = wood_board(w, h, 140, plank=9999)
    d = ImageDraw.Draw(board)
    d.rectangle([5, 5, w - 6, h - 6], outline=(126, 88, 42, 235), width=5)
    text(board, (w / 2, h / 2), label, 42, (250, 226, 180, 255), BRUSH_FONT, anchor='mm',
         shadow=(2, 3, (0, 0, 0, 200)))
    return board


def hint_tag(w, h, label):
    xx, yy = grid(w, h)
    d = rect_outside(xx, yy, w, h, 3.0) + (fbm(w, h, 900 + w, octaves=3, base=4) - 0.5) * 5
    mask = soften(np.clip(0.5 - d, 0, 1), 0.6)
    tone = 0.42 + 0.34 * fbm(w, h, 901, octaves=5, base=10)
    rgb = np.array([0.30, 0.21, 0.13], np.float32)[None, None, :] * tone[..., None] * 2.4
    p = rgba(rgb, mask)
    ImageDraw.Draw(p).rectangle([3, 3, w - 4, h - 4], outline=(176, 136, 74, 220), width=2)
    text(p, (w / 2, h / 2), label, 22, (250, 228, 186, 255), BODY_FONT, anchor='mm')
    return p


def hint_strip(base, right_x, y):
    x = right_x
    for glyph, label in reversed([('◀▶', '選ぶ'), ('A', '決める'), ('B', 'やめる')]):
        text(base, (x, y), label, 26, (240, 226, 202, 255), BODY_FONT, anchor='rm',
             shadow=(2, 2, (0, 0, 0, 210)))
        x -= font(BODY_FONT, 26).getlength(label) + 16
        gw = 68 if glyph == '◀▶' else 46
        paste(base, hint_tag(gw, 42, glyph), x - gw, y - 21)
        x -= gw + 30


# ---------------------------------------------------------------- 案1「酒場の貼り紙」
def mock_bills(base):
    im = tavern_grade(clean_hud(base), 0.66, [(392, 580, 640, 1.0), (1552, 560, 560, 0.8)])

    paste_tilted(im, wood_board(664, 690, 101), 392, 588, -0.8)
    for i, ch in enumerate(ROSTER):
        sel = (i == SELECTED)
        w, h = (492, 218) if sel else (454, 188)
        cy = 336 + i * 204 + (6 if sel else 0)
        paste_tilted(im, bill(w, h, ch, sel, 200 + i * 17), 392, cy, ch['tilt'])
        paste(im, nail(22), 392 - w * 0.34, cy - h * 0.40)
        paste(im, nail(22), 392 + w * 0.30, cy - h * 0.42)

    paste_tilted(im, ledger(566, 672, ROSTER[SELECTED], 300), 1552, 556, 0.7)

    paste(im, hanging_rope(300, 80), 810, 8)
    paste_tilted(im, tavern_sign(524, 106, '酒 場  —  仲 間 を 誘 う'), 960, 108, -1.3)

    hint_strip(im, 1856, 1006)
    return im


# ---------------------------------------------------------------- 案2「鉄と真鍮」
def crest(size, ch, selected, seed):
    ring = brass_ring(size, size * 0.13, rivets=10, seed=seed)
    plate = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    xx, yy = grid(size, size)
    r = size / 2
    d = np.hypot(xx - r, yy - r) - (r - size * 0.15)
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)
    glass = np.array([0.07, 0.17, 0.19], np.float32) if not ch['locked'] else np.array([0.10, 0.10, 0.11], np.float32)
    sheen = 0.5 + 1.1 * np.clip(1 - np.hypot(xx - r * 0.68, yy - r * 0.58) / (r * 1.25), 0, 1)
    plate.alpha_composite(rgba(glass[None, None, :] * sheen[..., None] * (2.0 if selected else 1.1), mask))
    plate.alpha_composite(ring)
    text(plate, (r, r + 4), ch['name'][0], int(size * 0.46),
         (255, 214, 136, 255) if selected else (150, 152, 156, 255), BRUSH_FONT, anchor='mm')
    return plate


def mock_crests(base):
    im = tavern_grade(clean_hud(base), 0.60, [(300, 560, 580, 0.85), (1570, 540, 540, 0.8)])

    for i, ch in enumerate(ROSTER):
        sel = (i == SELECTED)
        size = 190 if sel else 154
        cy, cx = 344 + i * 206, 210
        paste_tilted(im, crest(size, ch, sel, 500 + i * 13), cx, cy, 0)
        bw, bh = (384, 74) if sel else (338, 62)
        bar = iron_plate(bw, bh, seed=600 + i)
        engrave(bar, (bw / 2, bh / 2 - 4), ch['name'], 38 if sel else 32, BRUSH_FONT, anchor='mm',
                tone=(236, 206, 150) if sel else (206, 208, 212))
        if ch['reading']:
            engrave(bar, (bw / 2, bh / 2 + 22), ch['reading'], 15, BODY_FONT, anchor='mm')
        paste_tilted(im, bar, cx + 278, cy, 0)

    ch = ROSTER[SELECTED]
    pw, ph = 536, 512
    plaque = iron_plate(pw, ph, seed=700, notch=44)
    dr = ImageDraw.Draw(plaque)
    engrave(plaque, (pw / 2, 78), ch['name'], 62, BRUSH_FONT, anchor='mm', tone=(240, 206, 140))
    dr.line([(88, 136), (pw - 88, 136)], fill=(178, 136, 64), width=3)
    y = 202
    for label, filled in ch['stats']:
        engrave(plaque, (92, y), label, 26, BODY_FONT)
        for k in range(5):
            cx = 276 + k * 32
            box = [cx - 10, y - 10, cx + 10, y + 10]
            if k < filled:
                dr.ellipse(box, fill=(226, 174, 84))
                dr.ellipse([cx - 10, y - 11, cx + 10, y + 9], outline=(120, 88, 34), width=1)
            else:
                dr.ellipse(box, outline=(92, 96, 102), width=2)
        y += 70
    y += 12
    for line in ch['lines']:
        engrave(plaque, (92, y), line, 23, BODY_FONT)
        y += 42
    paste_tilted(im, plaque, 1570, 552, 0)

    paste(im, hanging_rope(300, 80), 810, 8)
    paste_tilted(im, tavern_sign(524, 106, '酒 場  —  仲 間 を 誘 う'), 960, 108, -1.3)
    hint_strip(im, 1856, 1010)
    return im


# ---------------------------------------------------------------- 比較
def contact_sheet(mocks, labels):
    tw = 1140
    th = int(tw * SCREEN_H / SCREEN_W)
    pad, header = 24, 58
    sheet = Image.new('RGBA', (tw + pad * 2, (th + header) * len(mocks) + pad), (20, 15, 11, 255))
    for i, (im, label) in enumerate(zip(mocks, labels)):
        y = pad + i * (th + header)
        text(sheet, (pad + 6, y), label, 34, (250, 214, 150, 255), BRUSH_FONT)
        sheet.alpha_composite(im.convert('RGBA').resize((tw, th), Image.LANCZOS), (pad, y + header - 8))
    return sheet


GEOMETRY = """
GEOMETRY (1920x1080 / 中心ピボット。選ばれた案の数値でプレハブを組む)
  共通   切り替え中は体力HUDを隠す / 操作ガイドとアイテム欄は State 側で自動的に引っ込む
  案1 酒場の貼り紙
    Board      中心 (392, 588)  664x690
    Bill[i]    中心 (392, 336+204i)  非選択 454x188 / 選択 492x218  傾き -2.4/+1.9/-1.2 度
    Nail       各 Bill の中心から (-0.34w, -0.40h) と (+0.30w, -0.42h)
    Ledger     中心 (1552, 556)  566x672  傾き +0.7 度
    Sign       中心 (960, 108)   524x106  傾き -1.3 度 / 吊り紐 左上 (810, 8) 300x80
    Hints      右端 x=1856, y=1006
  案2 鉄と真鍮
    Crest[i]   中心 (210, 344+206i)  非選択 154 / 選択 190
    NameBar[i] 中心 (488, 344+206i)  非選択 338x62 / 選択 384x74
    Plaque     中心 (1570, 552)  536x512
    Hints      右端 x=1856, y=1010
"""



# ---------------------------------------------------------------- 決定案(案1)のスプライト書き出し
# 文字は TextRenderer が描くので、板と紙だけを焼く。3枚とも同じ寸法にして、
# 選択中は CharacterSelectRow::selectedScale_ で持ち上げる
BILL_W, BILL_H = 454, 188


def bill_plate(w, h, seed, aged):
    p = parchment(w, h, seed, aged=aged)
    ImageDraw.Draw(p).line([(30, 34), (w - 30, 34)], fill=INK_FADE, width=2)
    return p


def stamp_sprite(label, px=46, angle=-8, color=STAMP_RED):
    lay = Image.new('RGBA', (px * 7, px * 3), (0, 0, 0, 0))
    text(lay, (px * 3.5, px * 1.5), label, px, (*color, 205), BRUSH_FONT, anchor='mm')
    return lay.rotate(angle, resample=Image.BICUBIC).crop(lay.getbbox() or (0, 0, px, px))


def pip_sprite(filled, size=24):
    xx, yy = grid(size, size)
    r = size / 2
    d = np.hypot(xx - r + 0.5, yy - r + 0.5) - (r - 3)
    if filled:
        mask = soften(np.clip(0.5 - d, 0, 1), 0.6)
        rgb = np.array([0.22, 0.13, 0.08], np.float32)[None, None, :] * np.ones_like(mask)[..., None]
    else:
        mask = soften(np.clip(0.5 - d, 0, 1) * (1 - np.clip(0.5 - (d + 2.4), 0, 1)), 0.6)
        rgb = np.array([0.41, 0.31, 0.21], np.float32)[None, None, :] * np.ones_like(mask)[..., None]
    return rgba(rgb, mask)


def write_sprite(out_dir, name, image):
    from tools.scene import sprite_meta
    png = out_dir / f'{name}.png'
    image.save(png)
    meta = out_dir / f'{name}.png.meta'
    guid = sprite_meta.read_meta(meta)['guid'] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    print(f'  {name}.png  {image.size[0]}x{image.size[1]}  guid={guid}')


def emit(out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {out_dir}')
    write_sprite(out_dir, 'Bill_Unselected', bill_plate(BILL_W, BILL_H, 200, 0.16))
    write_sprite(out_dir, 'Bill_Selected',   bill_plate(BILL_W, BILL_H, 217, 0.00))
    write_sprite(out_dir, 'Bill_Locked',     bill_plate(BILL_W, BILL_H, 234, 0.72))
    write_sprite(out_dir, 'WaxSeal',         wax_seal(80, 7))
    write_sprite(out_dir, 'LockedStamp',     stamp_sprite('募 集 前'))
    write_sprite(out_dir, 'Nail',            nail(22))
    write_sprite(out_dir, 'Board',           wood_board(664, 690, 101))
    write_sprite(out_dir, 'Ledger',          ledger_plate(566, 672, 300))
    write_sprite(out_dir, 'Sign',            tavern_sign(524, 106, '酒 場  —  仲 間 を 誘 う'))
    write_sprite(out_dir, 'SignRope',        hanging_rope(300, 80))
    write_sprite(out_dir, 'Pip_Filled',      pip_sprite(True))
    write_sprite(out_dir, 'Pip_Empty',       pip_sprite(False))
    write_sprite(out_dir, 'HintTag_Move',    hint_tag(68, 42, '◀▶'))
    write_sprite(out_dir, 'HintTag_Confirm', hint_tag(46, 42, 'A'))
    write_sprite(out_dir, 'HintTag_Cancel',  hint_tag(46, 42, 'B'))


def ledger_plate(w, h, seed):
    """帳面の紙だけ。見出しと罫線は焼くが、値は TextRenderer/Pips が描く"""
    p = parchment(w, h, seed, aged=0.28, ragged=9.0)
    arr = np.asarray(p, np.float32) / 255.0
    arr[:, :64, :3] *= np.linspace(0.62, 1.0, 64)[None, :, None]
    p = Image.fromarray((arr * 255).astype(np.uint8), 'RGBA')
    d = ImageDraw.Draw(p)
    d.line([(76, 180), (w - 76, 180)], fill=INK_FADE, width=3)
    y = 276
    for _ in range(3):
        d.line([(82, y), (w - 82, y)], fill=(160, 136, 104), width=1)
        y += 76
    return p


GEOMETRY_EMIT = """
GEOMETRY (案1 / 1920x1080 / 中心ピボット)
  TextRenderer: 文字の大きさは localScale = px / 60 (フォントアセットの size_ が 60)。
  y は常に文字の上端 (中央寄せでも縦は上端) なので、中心で考えた値からは px/2 引く。
  本文は ZenOldMincho-Bold、読み・キャッチコピーの色は (80, 56, 36)
  CharacterSelectRow.prefab   Bill 454x188 (中心が行の原点)
    Bill          (0, 0)              Bill_Unselected / _Selected / _Locked
    NameText      (-191, -40)  56px   onryou
    ReadingText   (-189,  28)  18px   本文
    TaglineText   (-189,  48)  20px   本文
    WaxSeal       ( 143,  54)         WaxSeal(80)      選択中だけ表示
    LockedStamp   (  62,   6)         LockedStamp      未解放だけ表示
    Nail          (-154, -75) / (136, -79)
  CharacterSelectUI.prefab
    Board         (392, 588)   664x690
    Rows          (392, 336)   行間 204px (CharacterSelectUi::rowSpacing_px_)
    Ledger        (1552, 556)  566x672
    DetailName    (1552, 269)  70px onryou 中央寄せ (中心は Ledger 中心から (0, -252))
    DetailReading (1552, 352)  19px 本文 中央寄せ (中心は Ledger 中心から (0, -194))
    StatLabel[i]  (1351, 450 + 76i)  27px 本文
    StatPips[i]   (1525, 464 + 76i)  5個 24px 間隔 28 (ラベルの中心に揃える)
    DescLine[i]   (1351, 706 + 44i)  25px 本文
    Hint*Text     26px 本文
    Sign          (960, 108)   524x106 / SignRope (960, 48) 300x80
    Hints         右端 x=1856, y=1006
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot', default='')
    ap.add_argument('--out-dir', required=True)
    ap.add_argument('--emit', action='store_true', help='決定案のスプライトを Assets/Art/UI/CharacterSelect へ書き出す')
    args = ap.parse_args()

    if args.emit:
        emit(REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect')
        print(GEOMETRY_EMIT)
        return

    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    base = load_base(args.shot)

    plans = [
        ('G_bills', '案1  酒場の貼り紙', mock_bills),
        ('H_iron', '案2  鉄と真鍮', mock_crests),
    ]
    mocks = []
    for key, label, fn in plans:
        im = fn(base.copy())
        path = out / f'charselect_{key}.png'
        im.convert('RGB').save(path)
        mocks.append(im)
        print(f'wrote {path}')

    contact_sheet(mocks, [label for _, label, _ in plans]).convert('RGB').save(out / 'charselect_compare3.png')
    print(f"wrote {out / 'charselect_compare3.png'}")
    print(GEOMETRY)


def load_base(shot_path):
    im = Image.open(shot_path).convert('RGBA')
    if im.size != (SCREEN_W, SCREEN_H):
        im = im.resize((SCREEN_W, SCREEN_H), Image.LANCZOS)
    return im


if __name__ == '__main__':
    main()
