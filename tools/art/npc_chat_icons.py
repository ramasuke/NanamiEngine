"""Generate the NPC head icons (Dauntless-inspired crest) drawn by BillBoardNpcChatIcon.

    python tools/art/npc_chat_icons.py [--out-dir Assets/Art/UI]
    python tools/art/npc_chat_icons.py --rim-sweep [--out-dir DIR]

Writes SurpriseMark.png / ChattableIcon.png / ChatIcon.png. The file names match the
existing sprites so their .meta GUIDs (and every prefab/scene reference) stay valid.
All three share one 256x312 canvas and crest silhouette; only the glyph and the rim
accent colour differ. Rendering is SDF-based at 4x supersampling, then box-filtered
down with premultiplied alpha. Requires Pillow + numpy.

--rim-sweep instead writes only SurpriseMarkRimSweep.png: an 18-frame 6x3 sprite sheet
(256x312 cells) of two light streaks running down the crest rim from the top vertex and
merging with a flash at the bottom tip. It is straight-alpha (constant colour, intensity
in alpha) and is drawn additively over SurpriseMark.png by NanamiUi::BillboardAnimation3D
via SurpriseMarkRimSweep.spriteAnimation.
"""
import argparse
import math
import os

import numpy as np
from PIL import Image, ImageFilter

W, H = 256, 312
S = 4
OY = 10
GLYPH_SCALE = 0.82
GLYPH_CENTER = (128.0, 140.0)


def hexc(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)], dtype=np.float32)


ys, xs = np.mgrid[0:H * S, 0:W * S].astype(np.float32)
X = (xs + 0.5) / S
Y = (ys + 0.5) / S


# ---------------------------------------------------------------- SDF helpers
def sd_circle(x, y, cx, cy, r):
    return np.hypot(x - cx, y - cy) - r


def sd_segment(x, y, ax, ay, bx, by):
    pax, pay = x - ax, y - ay
    bax, bay = bx - ax, by - ay
    h = np.clip((pax * bax + pay * bay) / (bax * bax + bay * bay), 0, 1)
    return np.hypot(pax - bax * h, pay - bay * h)


def sd_polygon(x, y, verts):
    v = [(float(a), float(b)) for a, b in verts]
    n = len(v)
    d = (x - v[0][0]) ** 2 + (y - v[0][1]) ** 2
    s = np.ones_like(x)
    j = n - 1
    for i in range(n):
        ex, ey = v[j][0] - v[i][0], v[j][1] - v[i][1]
        wx, wy = x - v[i][0], y - v[i][1]
        t = np.clip((wx * ex + wy * ey) / (ex * ex + ey * ey), 0, 1)
        bx, by = wx - ex * t, wy - ey * t
        d = np.minimum(d, bx * bx + by * by)
        c1 = y >= v[i][1]
        c2 = y < v[j][1]
        c3 = ex * wy > ey * wx
        s = np.where((c1 & c2 & c3) | (~c1 & ~c2 & ~c3), -s, s)
        j = i
    return s * np.sqrt(d)


def cov(sdf):
    return np.clip(0.5 - sdf * S, 0, 1)


def over(dst, color, a):
    a = a[..., None]
    col = color if color.ndim == 3 else color[None, None, :]
    dst[..., :3] = col * a + dst[..., :3] * (1 - a)
    dst[..., 3:] = a + dst[..., 3:] * (1 - a)


def vgrad(c0, c1, y0, y1):
    t = np.clip((Y - y0) / (y1 - y0), 0, 1)[..., None]
    return c0 * (1 - t) + c1 * t


# ---------------------------------------------------------------- shapes
CREST = [(128, 14 + OY), (214, 70 + OY), (214, 170 + OY), (128, 276 + OY), (42, 170 + OY), (42, 70 + OY)]


def crest_sd(x, y):
    return sd_polygon(x, y, CREST) - 8


def glyph_exclaim(x, y):
    bar = sd_polygon(x, y, [(108, 64), (148, 58), (139, 164), (117, 164)]) - 5
    return np.minimum(bar, sd_circle(x, y, 128, 202, 17))


def glyph_chevrons(x, y):
    c1 = np.minimum(sd_segment(x, y, 88, 92, 128, 130), sd_segment(x, y, 128, 130, 168, 92)) - 12
    c2 = np.minimum(sd_segment(x, y, 88, 146, 128, 184), sd_segment(x, y, 128, 184, 168, 146)) - 12
    return np.minimum(c1, c2)


def glyph_dots(x, y):
    return np.minimum(np.minimum(sd_circle(x, y, 88, 144, 15), sd_circle(x, y, 128, 144, 15)),
                      sd_circle(x, y, 168, 144, 15))


def scaled(fn, k):
    cx, cy = GLYPH_CENTER

    def f(x, y):
        return fn(cx + (x - cx) / k, cy + (y - cy) / k) * k
    return f


# ---------------------------------------------------------------- render
ACCENTS = {
    'gold': dict(top=hexc('#FFE08A'), bot=hexc('#E8892B'), glow=hexc('#FFB347')),
    'silver': dict(top=hexc('#FFFFFF'), bot=hexc('#9FB6C4'), glow=hexc('#CFEFFF')),
}
INK = hexc('#0B1218')
FIELD_TOP = hexc('#34505E')
FIELD_BOT = hexc('#121C24')
GLYPH_TOP = hexc('#FFFFFF')
GLYPH_BOT = hexc('#EADFCB')
WHITE = hexc('#FFFFFF')


def render(glyph_fn, accent):
    ac = ACCENTS[accent]
    img = np.zeros((H * S, W * S, 4), dtype=np.float32)
    sil = crest_sd(X, Y)

    glow = Image.fromarray((cov(sil) * 255).astype(np.uint8), 'L').filter(ImageFilter.GaussianBlur(9 * S))
    over(img, ac['glow'], np.asarray(glow, np.float32) / 255 * 0.75)

    over(img, INK, cov(sil))
    over(img, vgrad(ac['top'], ac['bot'], 16, 294), cov(sil + 3))
    over(img, WHITE, cov(sil + 3) * (1 - cov(sil + 5)) * np.clip((120 - Y) / 80, 0, 1) * 0.6)
    over(img, INK, cov(sil + 11))

    field = cov(sil + 13)
    over(img, vgrad(FIELD_TOP, FIELD_BOT, 30, 280), field)
    over(img, WHITE, field * (X < 128) * 0.05)
    over(img, WHITE, field * np.clip((110 - Y) / 90, 0, 1) * 0.08)
    over(img, ac['top'], field * (1 - cov(sil + 15)) * 0.35)

    g = glyph_fn(X, Y)
    over(img, INK, cov(g - 4.5) * 0.9)
    over(img, vgrad(GLYPH_TOP, GLYPH_BOT, 60, 220), cov(g))
    over(img, ac['bot'], cov(g) * (1 - cov(glyph_fn(X, Y + 4))) * 0.55)

    small = img.reshape(H, S, W, S, 4).mean(axis=(1, 3))
    a = small[..., 3:]
    rgb = np.where(a > 1e-6, small[..., :3] / np.maximum(a, 1e-6), 0)
    out = np.concatenate([rgb, a], axis=-1)
    return Image.fromarray((np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8), 'RGBA')


ICONS = (
    ('SurpriseMark', glyph_exclaim, 'gold'),     # surpriseIcon_  : event / quest
    ('ChattableIcon', glyph_chevrons, 'silver'),  # chattableIcon_ : talkable NPC beacon
    ('ChatIcon', glyph_dots, 'silver'),           # chattingIcon_  : player in talk range
)


# ---------------------------------------------------------------- rim sweep sheet
RIM_SWEEP_NAME = 'SurpriseMarkRimSweep'
RIM_SWEEP_FRAMES = 18
RIM_SWEEP_COLUMNS = 6
RIM_SWEEP_COLOR = hexc('#FFF3C4')
RIM_SWEEP_WIDTH = 0.075


def crest_perimeter_param():
    """Per-pixel position along the crest outline: 0 at the top vertex, increasing clockwise."""
    verts = [(float(a), float(b)) for a, b in CREST]
    n = len(verts)
    lens = [math.dist(verts[i], verts[(i + 1) % n]) for i in range(n)]
    starts = np.cumsum([0.0] + lens[:-1])
    total = sum(lens)
    best = np.full(X.shape, np.inf, np.float32)
    s = np.zeros(X.shape, np.float32)
    for i in range(n):
        ax, ay = verts[i]
        bx, by = verts[(i + 1) % n]
        ex, ey = bx - ax, by - ay
        t = np.clip(((X - ax) * ex + (Y - ay) * ey) / (ex * ex + ey * ey), 0, 1)
        d = (X - ax - ex * t) ** 2 + (Y - ay - ey * t) ** 2
        closer = d < best
        best = np.where(closer, d, best)
        s = np.where(closer, (starts[i] + t * lens[i]) / total, s)
    return s


def render_rim_sweep_sheet():
    sil = crest_sd(X, Y)
    rim = cov(sil - 2) * (1 - cov(sil + 12))
    rim_profile = rim * (0.55 + 0.65 * np.exp(-((sil + 7) / 4) ** 2))
    s = crest_perimeter_param()
    u = 2 * np.minimum(s, 1 - s)  # 0 top vertex .. 1 bottom tip

    rows = math.ceil(RIM_SWEEP_FRAMES / RIM_SWEEP_COLUMNS)
    sheet = Image.new('RGBA', (W * RIM_SWEEP_COLUMNS, H * rows), (0, 0, 0, 0))
    w = RIM_SWEEP_WIDTH
    for k in range(RIM_SWEEP_FRAMES):
        c = -w + (1 + 2 * w) * k / (RIM_SWEEP_FRAMES - 1)
        band = np.exp(-((u - c) / w) ** 2)
        flash = np.exp(-((u - 1) / 0.09) ** 2) * math.exp(-((c - 1) / 0.12) ** 2) * 1.3
        intensity = np.clip(rim_profile * (band + flash), 0, 1)

        bloom = Image.fromarray((intensity * 255).astype(np.uint8), 'L').filter(ImageFilter.GaussianBlur(6 * S))
        total = np.clip(intensity + np.asarray(bloom, np.float32) / 255 * 0.5, 0, 1)
        alpha = total.reshape(H, S, W, S).mean(axis=(1, 3))

        cell = np.zeros((H, W, 4), np.float32)
        cell[..., :3] = RIM_SWEEP_COLOR
        cell[..., 3] = alpha
        sheet.paste(Image.fromarray((cell * 255 + 0.5).astype(np.uint8), 'RGBA'),
                    ((k % RIM_SWEEP_COLUMNS) * W, (k // RIM_SWEEP_COLUMNS) * H))
    return sheet


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--out-dir', default=os.path.join('Assets', 'Art', 'UI'))
    parser.add_argument('--rim-sweep', action='store_true',
                        help='write only the %s.png sprite sheet' % RIM_SWEEP_NAME)
    args = parser.parse_args()
    os.makedirs(args.out_dir, exist_ok=True)
    if args.rim_sweep:
        path = os.path.join(args.out_dir, RIM_SWEEP_NAME + '.png')
        render_rim_sweep_sheet().save(path)
        print('wrote', path)
        return
    for name, fn, accent in ICONS:
        path = os.path.join(args.out_dir, name + '.png')
        render(scaled(fn, GLYPH_SCALE), accent).save(path)
        print('wrote', path)


if __name__ == '__main__':
    main()
