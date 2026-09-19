"""Generate the control-guide sprites ("slim teal strip" design) used by
Assets/Prefab/UI/ControlGuide/ControlGuideRow.prefab (GamePlay::Ui::ControlGuide / ControlGuideRow), shared by
SwordManControlGuide.prefab and MagicCasterControlGuide.prefab.

    python tools/art/control_guide.py [--out-dir Assets/Art/UI/ControlGuide] [--preview PATH]

Writes one sprite per input glyph (keys are outlined, drawn at full brightness; the rows dim them with
alpha), the row background strip and the additive accent glow used by the "became usable" pulse, plus a
SpriteFile .png.meta for each (an existing .meta keeps its GUID).
Every glyph shares one cap height and one letter size, so rows look the same whichever input they show.
The prefab layout the sprites are sized for is printed at the end (GEOMETRY): the Rows VerticalLayoutGroup
cell and each row part's local position, relative to the row centre.
--preview composites a few guide states and the appear-pulse frames onto a flat background, laid out the
way the prefab places them (rows centred on the guide origin, a row's slot growing with its visibility;
labels use Assets/Art/Font/ipam.ttf like the game).

Glyph letters are rasterised with Segoe UI Bold (Windows font, generation time only).
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
GLYPH_FONT = r'C:/Windows/Fonts/segoeuib.ttf'
LABEL_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'ipam.ttf'

STRIP_W, STRIP_H = 300, 36
STRIP_OPAQUE_RATE = 0.62  # the strip stays at STRIP_ALPHA under the labels, then fades out
STRIP_ALPHA = 0.8
GLOW_W = 64
GLOW_BAR_X = 30
ACCENT_W = 3

KEY_H = 28
KEY_MARGIN = 2
KEY_LETTER_RATE = 0.58
KEY_WORD_RATE = 0.47

# GEOMETRY of the prefab rows (screen px, x relative to the strip's left edge)
CELL_H = 36
CELL_SPACING = 4
GLYPH_COLUMN_X = 18
GLYPH_COLUMN_W = 56
LABEL_X = 92
LABEL_SIZE = 23
LABEL_FONT_SIZE = 60  # ipam.ttf .meta size_
LABEL_SHADOW_OFFSET = 1.5
DIM_ALPHA = 110  # ControlGuide::dimAlpha_
SLIDE_DISTANCE = 8  # ControlGuide::slideDistance_px_
FOCUS_MARK_X = 268  # フォーカス矢印／チェックの中心（帯の左端から）


def hexc(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)], dtype=np.float32)


LINE = hexc('#dff6f2')
KEY_FILL = hexc('#061015')
TEAL = hexc('#38d6c4')
GOLD = hexc('#ffce68')

# Xbox の面ボタン配色。パッド接続時のグリフだけがこの色を使う
PAD_LETTER_COLOR = {'A': hexc('#7cd360'), 'B': hexc('#e9645c'), 'X': hexc('#60a6e9'), 'Y': hexc('#f0c85c')}


# ---------------------------------------------------------------- raster helpers
class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        ys, xs = np.mgrid[0:h * S, 0:w * S].astype(np.float32)
        self.X = (xs + 0.5) / S
        self.Y = (ys + 0.5) / S
        self.img = np.zeros((h * S, w * S, 4), np.float32)

    def over(self, rgb, a):
        a = np.clip(np.asarray(a, np.float32), 0, 1)[..., None]
        self.img[..., :3] = rgb * a + self.img[..., :3] * (1 - a)
        self.img[..., 3:] = a + self.img[..., 3:] * (1 - a)

    def resolve(self):
        c = self.img.reshape(self.h, S, self.w, S, 4).mean(axis=(1, 3))
        a = c[..., 3:]
        rgb = np.where(a > 1e-5, c[..., :3] / np.maximum(a, 1e-5), 0)
        out = np.concatenate([np.clip(rgb, 0, 1), np.clip(a, 0, 1)], axis=-1)
        return Image.fromarray((out * 255 + 0.5).astype(np.uint8), 'RGBA')

    def text_mask(self, text, cx, cy, size):
        im = Image.new('L', (self.w * S, self.h * S), 0)
        f = ImageFont.truetype(GLYPH_FONT, int(size * S))
        ImageDraw.Draw(im).text((cx * S, cy * S), text, font=f, fill=255, anchor='mm')
        return np.asarray(im, np.float32) / 255


def cov(sd):
    return np.clip(0.5 - sd * S, 0, 1).astype(np.float32)


def blur(c, r):
    im = Image.fromarray((np.clip(c, 0, 1) * 255).astype(np.uint8), 'L')
    return np.asarray(im.filter(ImageFilter.GaussianBlur(r * S)), np.float32) / 255


def sd_rbox(x, y, x0, y0, x1, y1, r):
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2 - r, (y1 - y0) / 2 - r
    qx, qy = np.abs(x - cx) - hx, np.abs(y - cy) - hy
    return np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - r


# ---------------------------------------------------------------- glyphs
def key_letter_size(text):
    return KEY_H * (KEY_LETTER_RATE if len(text) == 1 else KEY_WORD_RATE)


def key_width(text):
    font = ImageFont.truetype(GLYPH_FONT, int(key_letter_size(text) * S))
    return max(KEY_H, math.ceil(font.getlength(text) / S + KEY_H * 0.62))


def key(c, x0, y0, text):
    X, Y = c.X, c.Y
    w, h = key_width(text), KEY_H
    d = sd_rbox(X, Y, x0, y0, x0 + w, y0 + h, h * 0.22)
    c.over(KEY_FILL, cov(d) * 0.6)
    c.over(LINE, cov(d) * (1 - cov(d + 1.6)))
    c.over(LINE, cov(d + 1.6) * (1 - cov(d + 2.6)) * (Y > y0 + h - 4) * 0.45)
    c.over(LINE, c.text_mask(text, x0 + w / 2, y0 + h / 2, key_letter_size(text)))


def render_keys(*texts, gap=4):
    widths = [key_width(t) for t in texts]
    c = Canvas(sum(widths) + gap * (len(texts) - 1) + KEY_MARGIN * 2, KEY_H + KEY_MARGIN * 2)
    x = KEY_MARGIN
    for text, w in zip(texts, widths):
        key(c, x, KEY_MARGIN, text)
        x += w + gap
    return c.resolve()


def sd_circle(x, y, cx, cy, r):
    return np.hypot(x - cx, y - cy) - r


def pad_frame(c, cx, cy, r):
    d = sd_circle(c.X, c.Y, cx, cy, r)
    c.over(KEY_FILL, cov(d) * 0.6)
    c.over(LINE, cov(d) * (1 - cov(d + 1.6)))
    return d


def render_pad_button(letter):
    r = KEY_H / 2
    c = Canvas(KEY_H + KEY_MARGIN * 2, KEY_H + KEY_MARGIN * 2)
    cx = cy = KEY_MARGIN + r
    pad_frame(c, cx, cy, r)
    c.over(PAD_LETTER_COLOR[letter], c.text_mask(letter, cx, cy, KEY_H * KEY_LETTER_RATE))
    return c.resolve()


def render_pad_trigger(letter='RT'):
    w, h = round(KEY_H * 0.82), KEY_H
    c = Canvas(w + KEY_MARGIN * 2, h + KEY_MARGIN * 2)
    X, Y = c.X, c.Y
    x0, y0 = KEY_MARGIN, KEY_MARGIN
    d = sd_rbox(X, Y, x0, y0, x0 + w, y0 + h, w * 0.44)
    c.over(KEY_FILL, cov(d) * 0.6)
    c.over(LINE, cov(d) * (1 - cov(d + 1.6)))
    # 引きしろを示す上面のアーチ
    arch = np.abs(sd_circle(X, Y, x0 + w / 2, y0 + h * 0.58, w * 0.42)) - 0.7
    c.over(LINE, cov(arch) * (Y < y0 + h * 0.42) * 0.7)
    c.over(LINE, c.text_mask(letter, x0 + w / 2, y0 + h * 0.72, h * 0.34))
    return c.resolve()


def render_pad_stick(arrows, press=False):
    r = KEY_H / 2
    c = Canvas(KEY_H + KEY_MARGIN * 2, KEY_H + KEY_MARGIN * 2)
    X, Y = c.X, c.Y
    cx = cy = KEY_MARGIN + r
    pad_frame(c, cx, cy, r)

    inner = sd_circle(X, Y, cx, cy, r * 0.46)
    if press:
        c.over(TEAL, cov(inner) * 0.85)
        c.over(hexc('#061015'), cov(sd_circle(X, Y, cx, cy, r * 0.16)))
    else:
        c.over(LINE, cov(inner) * (1 - cov(inner + 1.3)))

    tip, base, wing = r * 0.86, r * 0.6, r * 0.18
    for dx, dy in arrows:
        px, py = -dy, dx
        # 三角形を辺の内側判定で塗る
        ax, ay = cx + dx * tip, cy + dy * tip
        bx, by = cx + dx * base + px * wing, cy + dy * base + py * wing
        ex, ey = cx + dx * base - px * wing, cy + dy * base - py * wing
        inside = np.ones_like(X, np.float32)
        for (sx, sy), (tx, ty), (ox, oy) in (((ax, ay), (bx, by), (ex, ey)),
                                             ((bx, by), (ex, ey), (ax, ay)),
                                             ((ex, ey), (ax, ay), (bx, by))):
            side = (tx - sx) * (Y - sy) - (ty - sy) * (X - sx)
            inside *= (side * np.sign((tx - sx) * (oy - sy) - (ty - sy) * (ox - sx)) >= 0)
        c.over(LINE, inside)
    return c.resolve()


def render_mouse_left():
    w, h = 20, KEY_H
    c = Canvas(w + KEY_MARGIN * 2, h + KEY_MARGIN * 2)
    X, Y = c.X, c.Y
    x0, y0 = KEY_MARGIN, KEY_MARGIN
    d = sd_rbox(X, Y, x0, y0, x0 + w, y0 + h, w * 0.48)
    c.over(KEY_FILL, cov(d) * 0.6)
    c.over(LINE, cov(d) * (1 - cov(d + 1.6)))
    split_y = y0 + h * 0.42
    inner = cov(d + 1.6)
    c.over(LINE, inner * (np.abs(Y - split_y) < 0.65))
    c.over(LINE, inner * (np.abs(X - (x0 + w / 2)) < 0.65) * (Y < split_y))
    c.over(TEAL, cov(d + 3) * (X < x0 + w / 2 - 1.8) * (Y < split_y - 1.8))
    return c.resolve()


# ---------------------------------------------------------------- row parts
def render_strip():
    c = Canvas(STRIP_W, STRIP_H)
    X, Y = c.X, c.Y
    t = np.clip((X / STRIP_W - STRIP_OPAQUE_RATE) / (1 - STRIP_OPAQUE_RATE), 0, 1)
    band = cov(sd_rbox(X, Y, 0, 0, STRIP_W + 40, STRIP_H, 2)) * (X < STRIP_W)
    c.over(hexc('#0b1820'), band * (1 - t) ** 1.6 * STRIP_ALPHA)
    c.over(TEAL, cov(sd_rbox(X, Y, 0, 2, ACCENT_W, STRIP_H - 2, 0.5)))
    return c.resolve()


def render_strip_focus():
    """フォーカス中の行に重ねる帯。通常の帯の上に alpha でクロスフェードさせる"""
    c = Canvas(STRIP_W, STRIP_H)
    X, Y = c.X, c.Y
    t = np.clip((X / STRIP_W - STRIP_OPAQUE_RATE) / (1 - STRIP_OPAQUE_RATE), 0, 1)
    band = cov(sd_rbox(X, Y, 0, 0, STRIP_W + 40, STRIP_H, 2)) * (X < STRIP_W)
    c.over(hexc('#1a2416'), band * (1 - t) ** 1.6 * 0.55)
    c.over(GOLD, band * (1 - t) ** 2.4 * 0.16)
    c.over(GOLD, cov(sd_rbox(X, Y, 0, 2, ACCENT_W, STRIP_H - 2, 0.5)))
    # 上下の細いガイド線。右へ向かって帯と一緒に消える
    edge = (cov(sd_rbox(X, Y, 0, 0.5, STRIP_W, 1.5, 0)) + cov(sd_rbox(X, Y, 0, STRIP_H - 1.5, STRIP_W, STRIP_H - 0.5, 0)))
    c.over(GOLD, np.clip(edge, 0, 1) * (1 - t) ** 1.2 * 0.75)
    return c.resolve()


def render_focus_arrow():
    """フォーカス行の右端で行を指す矢印（加算）"""
    w, h = 16, 22
    c = Canvas(w, h)
    X, Y = c.X, c.Y
    # 左を向いた三角形。x が右へ進むほど許される半分の高さが増える
    half = (X - 2.0) / (w - 4.0) * (h / 2 - 2.0)
    shape = cov(np.abs(Y - h / 2) - half) * (X > 2.0) * (X < w - 2.0)
    halo = blur(shape, 2.2)
    c.over(GOLD, np.clip(halo * 1.5, 0, 1) * 0.5)
    c.over(GOLD, shape)
    return c.resolve()


def render_focus_check():
    """課題クリア時にフォーカス行へ出すチェック"""
    s = 22
    c = Canvas(s, s)
    X, Y = c.X, c.Y
    pts = ((s * 0.18, s * 0.52), (s * 0.40, s * 0.74), (s * 0.82, s * 0.24))
    mark = np.zeros_like(X)
    for (ax, ay), (bx, by) in zip(pts[:-1], pts[1:]):
        vx, vy = bx - ax, by - ay
        length = math.hypot(vx, vy)
        t = np.clip(((X - ax) * vx + (Y - ay) * vy) / (length * length), 0, 1)
        mark = np.maximum(mark, cov(np.hypot(X - (ax + vx * t), Y - (ay + vy * t)) - 1.7))
    c.over(TEAL, mark)
    return c.resolve()


def render_accent_glow():
    c = Canvas(GLOW_W, STRIP_H)
    X, Y = c.X, c.Y
    bar = cov(sd_rbox(X, Y, GLOW_BAR_X, 4, GLOW_BAR_X + ACCENT_W, STRIP_H - 4, 0.5))
    halo = np.clip(blur(bar, 5) * 2.2, 0, 1)
    c.over(TEAL, halo)
    c.over(hexc('#e8fffb'), bar * 0.8)
    return c.resolve()


SPRITES = {
    'ControlGuide_Key_WASD': lambda: render_keys('WASD'),
    'ControlGuide_Key_AD': lambda: render_keys('A', 'D'),
    'ControlGuide_Key_Q': lambda: render_keys('Q'),
    'ControlGuide_Key_E': lambda: render_keys('E'),
    'ControlGuide_Key_Shift': lambda: render_keys('Shift'),
    'ControlGuide_Key_Ctrl': lambda: render_keys('Ctrl'),
    'ControlGuide_Key_Space': lambda: render_keys('Space'),
    'ControlGuide_Mouse_Left': render_mouse_left,
    'ControlGuide_Pad_LStick': lambda: render_pad_stick([(0, -1), (0, 1), (-1, 0), (1, 0)]),
    'ControlGuide_Pad_LStick_LR': lambda: render_pad_stick([(-1, 0), (1, 0)]),
    'ControlGuide_Pad_RStick_Press': lambda: render_pad_stick([], press=True),
    'ControlGuide_Pad_A': lambda: render_pad_button('A'),
    'ControlGuide_Pad_B': lambda: render_pad_button('B'),
    'ControlGuide_Pad_X': lambda: render_pad_button('X'),
    'ControlGuide_Pad_Y': lambda: render_pad_button('Y'),
    'ControlGuide_Pad_RT': render_pad_trigger,
    'ControlGuide_RowStrip': render_strip,
    'ControlGuide_RowStrip_Focus': render_strip_focus,
    'ControlGuide_FocusArrow': render_focus_arrow,
    'ControlGuide_FocusCheck': render_focus_check,
    'ControlGuide_AccentGlow': render_accent_glow,
}

GLYPH_CENTER_X = GLYPH_COLUMN_X + GLYPH_COLUMN_W / 2
GEOMETRY = {
    'Rows VerticalLayoutGroup cellSize_': (STRIP_W, CELL_H),
    'Rows VerticalLayoutGroup spacing_': CELL_SPACING,
    'Rows VerticalLayoutGroup stackUpward_ / centerOnOrigin_': (True, True),
    'Rows localPos': (0, 0),
    'Strip localPos': (0, 0),
    'AccentGlow localPos': (GLOW_W / 2 - GLOW_BAR_X - STRIP_W / 2, 0),
    'Glyph / GlyphFlash localPos': (GLYPH_CENTER_X - STRIP_W / 2, 0),
    'Label localPos': (LABEL_X - STRIP_W / 2, -LABEL_SIZE / 2),
    'LabelShadow localPos': (LABEL_X - STRIP_W / 2 + LABEL_SHADOW_OFFSET, -LABEL_SIZE / 2 + LABEL_SHADOW_OFFSET),
    'Label / LabelShadow localScale': LABEL_SIZE / LABEL_FONT_SIZE,
    'StripFocus localPos': (0, 0),
    'FocusArrow / FocusCheck localPos': (FOCUS_MARK_X - STRIP_W / 2, 0),
}


def write_sprite(out_dir, name, image):
    png = out_dir / f"{name}.png"
    image.save(png)
    meta = out_dir / f"{name}.png.meta"
    guid = sprite_meta.read_meta(meta)["guid"] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    return guid


# ---------------------------------------------------------------- preview (mirrors the prefab rows + ControlGuide::PresentRow)
GLYPH_OF = {'WASD': 'ControlGuide_Key_WASD', 'AD': 'ControlGuide_Key_AD', 'Q': 'ControlGuide_Key_Q',
            'E': 'ControlGuide_Key_E', 'Shift': 'ControlGuide_Key_Shift', 'Ctrl': 'ControlGuide_Key_Ctrl',
            'Space': 'ControlGuide_Key_Space', 'LMB': 'ControlGuide_Mouse_Left'}


def fade(im, alpha):
    a = np.asarray(im, np.float32)
    a[..., 3] *= alpha
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), 'RGBA')


def add_layer(canvas, im, pos, alpha):
    """additive blend (DX_BLENDMODE_ADD) of a straight-alpha sprite"""
    layer = Image.new('RGBA', canvas.size, (0, 0, 0, 0))
    layer.paste(im, pos)
    src = np.asarray(layer, np.float32) / 255
    dst = np.asarray(canvas, np.float32) / 255
    dst[..., :3] = np.clip(dst[..., :3] + src[..., :3] * src[..., 3:] * alpha, 0, 1)
    return Image.fromarray((dst * 255).astype(np.uint8), 'RGBA')


def smoothstep(t):
    t = min(max(t, 0.0), 1.0)
    return t * t * (3 - 2 * t)


def row_centers(rates):
    """VerticalLayoutGroup with stackUpward_ + centerOnOrigin_: y offsets from the origin, one per row (bottom first)"""
    pitch = CELL_H + CELL_SPACING
    along, filled = [], 0.0
    for r in rates:
        along.append(filled + pitch * (r - 1) / 2)
        filled += pitch * r
    centre = (filled - CELL_SPACING - CELL_H) / 2
    return [-(a - centre) for a in along]


def draw_rows(canvas, sprites, rows, left, centre_y):
    """rows: (glyph, label, visibility 0..1, usable 0..1, pulse 0..1) bottom to top.
    A row with visibility > 0 takes smoothstep(visibility) of a slot (the layout group skips disabled rows)."""
    font = ImageFont.truetype(str(LABEL_FONT), LABEL_SIZE)
    rows = [r for r in rows if r[2] > 0]
    offsets = row_centers([smoothstep(r[2]) for r in rows])
    for (glyph, label, vis, usable, pulse), offset in zip(rows, offsets):
        sprite = sprites[GLYPH_OF[glyph]]
        cy = centre_y + offset
        x = left - SLIDE_DISTANCE * (1 - vis) ** 2
        alpha = vis * (DIM_ALPHA / 255 + (1 - DIM_ALPHA / 255) * usable)
        strip = sprites['ControlGuide_RowStrip']
        canvas.alpha_composite(fade(strip, alpha), (int(x), int(cy - strip.height / 2)))
        gx = int(x + GLYPH_CENTER_X - sprite.width / 2)
        gy = int(cy - sprite.height / 2)
        canvas.alpha_composite(fade(sprite, alpha), (gx, gy))
        if pulse > 0:
            glow = sprites['ControlGuide_AccentGlow']
            canvas = add_layer(canvas, glow, (int(x - GLOW_BAR_X), int(cy - glow.height / 2)), pulse * vis * 0.9)
            canvas = add_layer(canvas, sprite, (gx, gy), pulse * vis * 0.45)
        draw = ImageDraw.Draw(canvas)
        a = int(255 * alpha)
        draw.text((x + LABEL_X + LABEL_SHADOW_OFFSET, cy + LABEL_SHADOW_OFFSET), label, font=font,
                  fill=(6, 20, 26, int(a * 0.7)), anchor='lm')
        draw.text((x + LABEL_X, cy), label, font=font, fill=(255, 255, 247, a), anchor='lm')
    return canvas


def render_preview(sprites, path):
    idle = [('WASD', '移動', 1, 1, 0), ('LMB', '攻撃', 1, 1, 0), ('Space', 'ジャンプ', 1, 1, 0),
            ('Ctrl', '回避', 1, 1, 0), ('Q', 'ロックオン', 1, 1, 0), ('E', '会話', 1, 1, 0)]
    run_low = [('WASD', '移動', 1, 1, 0), ('LMB', 'ダッシュ攻撃', 1, 1, 0), ('Space', 'ジャンプ', 1, 0, 0),
               ('Ctrl', '回避', 1, 0, 0), ('Q', 'ロックオン', 1, 1, 0)]
    cannon = [('AD', '旋回', 1, 1, 0), ('LMB', '発射', 1, 1, 0)]
    panels = [('待機', idle), ('ダッシュ中・スタミナ不足', run_low), ('大砲', cannon)]
    # "会話" row appearing: visibility (and its slot) ramps over 0.2s, pulse decays over 0.45s
    for t in (0.05, 0.1, 0.2, 0.35, 0.6):
        vis = min(1.0, t / 0.2)
        pulse = max(0.0, 1 - t / 0.45) if t > 0 else 0
        rows = [r if r[1] != '会話' else ('E', '会話', vis, 1, pulse) for r in idle]
        panels.append((f'会話が使えるようになる {t:.2f}s', rows))
    W, H = 520, 520
    sheet = Image.new('RGB', (W * 4, H * 2), (0, 0, 0))
    cap = ImageFont.truetype(str(LABEL_FONT), 22)
    for i, (title, rows) in enumerate(panels):
        panel = Image.new('RGBA', (W, H), (74, 92, 60, 255))
        grad = np.linspace(0.55, 1.0, H, dtype=np.float32)[:, None, None]
        arr = np.asarray(panel, np.float32)
        arr[..., :3] *= grad
        panel = Image.fromarray(arr.astype(np.uint8), 'RGBA')
        panel = draw_rows(panel, sprites, rows, 16, H / 2)
        ImageDraw.Draw(panel).text((12, 10), title, font=cap, fill=(255, 255, 255, 255))
        sheet.paste(panel.convert('RGB'), ((i % 4) * W, (i // 4) * H))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/ControlGuide')
    ap.add_argument('--preview', help='also write a composite of guide states / pulse frames to this PNG path')
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    sprites = {}
    for name, fn in SPRITES.items():
        sprites[name] = fn()
        guid = write_sprite(out_dir, name, sprites[name])
        print(f"{name:26s} {sprites[name].size[0]}x{sprites[name].size[1]}  {guid}")

    print("GEOMETRY (SwordManControlGuide.prefab / MagicCasterControlGuide.prefab Rows / ControlGuideRow.prefab):")
    for k, v in GEOMETRY.items():
        print(f"  {k} = {v}")

    if args.preview:
        render_preview(sprites, args.preview)
        print(f"preview -> {args.preview}")


if __name__ == '__main__':
    main()
