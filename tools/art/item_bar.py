"""Generate the item-pouch sprites (screen bottom-right) drawn by GamePlay::Ui::ItemBar / ItemSlot.

    python tools/art/item_bar.py [--out-dir Assets/Art/UI/ItemBar] [--icon-dir Assets/Art/UI/Item]
                                 [--preview PATH] [--shot SCREEN.png]

Writes the slot parts (ItemSlot_*), the count nut, the name plate and the input glyphs into --out-dir, and
one icon per item into --icon-dir, with a SpriteFile .png.meta for each (an existing .meta keeps its GUID, so
regenerating never breaks references).
Design chosen 2026-09-28 from real-screen mocks (案A「鋼のメダル」): the same brushed steel as the KnightStatusUI
portrait frame and its fist / boot medals. The selected item sits in a steel ring with blades on both sides, the
others in small steel medals, the count on a hex nut and the name on an arrow-tipped steel plate like the tip of
the health bar.
Everything is authored at the *selected* slot size; unselected slots are the same sprites drawn at
ItemBar::unselectedScale_ through the slot's Content transform, so only one frame set exists per part.
The layout values ItemBarUI.prefab / ItemSlot.prefab have to agree with are printed at the end (GEOMETRY).
--preview composites the strip the way ItemBar::PresentSlots draws it (selected centred, empty slot dimmed,
the switch pulse, and the dimmed "cannot use" state); with --shot it is drawn onto that 1920x1080 screenshot
at the prefab's root position instead.

Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art.character_select import bevel, fbm, grid, rgba, soften  # noqa: E402
from tools.art.control_guide import (  # noqa: E402
    Canvas, KEY_H, KEY_MARGIN, LINE, cov, hexc, pad_frame, render_keys, render_pad_trigger, sd_rbox,
)
from tools.scene import sprite_meta  # noqa: E402

FONT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'Font'
LABEL_FONT = FONT_DIR / 'ipam.ttf'
NAME_FONT = FONT_DIR / 'KaiseiDecol-Bold.ttf'
NAME_OUTLINE = (6, 20, 26, 255)

# ---- 枠まわり（選択中の枠を等倍とした設計値。非選択は ItemBar 側のスケールで縮む）
DISC = 100.0            # 鋼の環の外径
RIM = 11.0              # 環の太さ
BLADE = 26.0            # 選択中の環の左右に出す刃の長さ
FRAME_PX = 164          # 枠スプライトの一辺（刃が収まる大きさ）
BACKING_PX = 84
GLOW_PX = 150
ICON_PX = 64
NUT_PX = 30
NUT_OFFSET = (32.0, 33.0)    # 枠の中心から個数ナットの中心まで
NAME_BODY_W, NAME_H, NAME_TIP = 236, 42, 34

# ---- 帯の並び（ItemBar の serialize 既定値と合わせること）
SLOT_PITCH = 98.0
UNSELECTED_SCALE = 0.66
DIM_ALPHA = 200
NAME_OFFSET_Y = -88.0
HINT_OFFSET_Y = 84.0
ROOT_POS = (1856, 962)
# ルート（= 一番右の枠の中心）から見た操作ヒントの中心。
# 切替のキー画像はキーボードの「Z X」が幅 64px (パッドは 32px) あるので、それでも「切り替え」に触れない位置
HINT_LAYOUT = {
    'UseLabel': (0.0, HINT_OFFSET_Y),
    'UseGlyph': (-45.0, HINT_OFFSET_Y),
    'CycleLabel': (-129.0, HINT_OFFSET_Y),
    'CycleGlyph': (-211.0, HINT_OFFSET_Y),
}
HINT_TEXT = {'CycleLabel': '切り替え', 'UseLabel': '使う'}
NAME_TEXT_PX, HINT_TEXT_PX, COUNT_TEXT_PX = 26, 21, 19

STEEL = np.array([0.50, 0.53, 0.58], np.float32)
DARK_IN = np.array([0.06, 0.07, 0.09], np.float32)
COUNT_COLOR = (200, 208, 212)
COUNT_SELECTED_COLOR = (255, 206, 104)


def sdf_mask(d, soft=0.7):
    return soften(np.clip(0.5 - d, 0, 1), soft)


def _brushed(w, h, seed, base=18):
    return 0.85 + 0.3 * fbm(w, h, seed, octaves=5, base=base)


# ---------------------------------------------------------------- 枠
def render_backing():
    """環の内側の暗い円板。上から光が落ちて縁ほど暗い"""
    s = BACKING_PX
    xx, yy = grid(s, s)
    c = s / 2
    r = DISC / 2 - RIM + 2
    rad = np.hypot(xx - c + 0.5, yy - c + 0.5)
    mask = sdf_mask(rad - r)
    t = np.clip(np.hypot(xx - c, yy - (c - r * 0.35)) / r, 0, 1)[..., None]
    rgb = np.array([0.20, 0.21, 0.26], np.float32) * (1 - t) + DARK_IN * t
    rgb = rgb * (0.55 + 0.45 * np.clip((r - rad) / 6, 0, 1))[..., None]
    return rgba(rgb, mask * 0.94)


def render_frame(selected):
    """鋼の環。選択中は明るく、左右に騎士の肖像枠と同じ斜めに切った刃を出す"""
    s = FRAME_PX
    xx, yy = grid(s, s)
    c = s / 2
    r = DISC / 2
    bright = 1.0 if selected else 0.8
    rad = np.hypot(xx - c + 0.5, yy - c + 0.5)
    ring_d = np.abs(rad - (r - RIM / 2)) - RIM / 2
    ring = sdf_mask(ring_d)
    brushed = _brushed(s, s, 41 if selected else 42)
    across = np.clip((rad - (r - RIM)) / RIM, 0, 1)
    light = np.clip(((c - xx) + (c - yy) * 1.4) / (s * 0.7) + 0.55, 0.2, 1.2)
    rgb = STEEL[None, None, :] * ((0.45 + 1.1 * np.sin(across * np.pi) * light) * brushed * bright)[..., None]
    alpha = ring
    if selected:
        for sign in (-1, 1):
            u = (xx - (c + sign * (r - 2))) * sign
            half = np.clip(1 - u / BLADE, 0, 1) * 15 * np.clip(u / 4 + 0.5, 0, 1)
            ridge = yy - c + u * 0.35
            blade = sdf_mask(np.maximum(np.abs(ridge) - half, -u)) * (1 - sdf_mask(rad - r))
            face = np.where(ridge < 0, 1.25, 0.7)
            srgb = STEEL[None, None, :] * (face * brushed)[..., None]
            rgb = srgb * blade[..., None] + rgb * (1 - blade[..., None])
            alpha = np.maximum(alpha, blade)
    return rgba(rgb, alpha)


def render_select_glow():
    """切替の瞬間に加算で重ねる、環の縁に走る白い光"""
    s = GLOW_PX
    xx, yy = grid(s, s)
    c = s / 2
    rad = np.hypot(xx - c + 0.5, yy - c + 0.5)
    line = np.clip(1 - np.abs(rad - (DISC / 2 - RIM / 2)) / (RIM / 2), 0, 1)
    halo = soften(line, 6.0)
    a = np.clip(line * 0.55 + halo * 1.2, 0, 1)
    rgb = np.ones((s, s, 3), np.float32) * np.array([0.96, 0.94, 0.88], np.float32)
    return rgba(rgb, a)


def render_count_nut():
    """個数を載せる六角の鋼ナット。選択中かどうかは数字の色（ItemSlot の countSelectedColor_）で見せる"""
    s = NUT_PX
    xx, yy = grid(s, s)
    c = s / 2
    ang = np.arctan2(yy - c, xx - c)
    rad = np.hypot(xx - c, yy - c)
    hexr = (s / 2 - 1) * math.cos(math.pi / 6) / np.cos((ang % (math.pi / 3)) - math.pi / 6)
    d = rad - hexr
    mask = sdf_mask(d)
    inner = sdf_mask(d + 3)
    rim = STEEL[None, None, :] * (1.0 + 1.6 * bevel(mask, 1.4))[..., None]
    rgb = DARK_IN[None, None, :] * inner[..., None] + rim * (1 - inner[..., None])
    return rgba(rgb, mask)


def render_name_plate():
    """体力バーの先端と同じ、右が矢じりに尖った鋼の札。
    左に矢じりと同じ幅の透明を足して、本体の中心 = 画像の中心にする（名前の文字は中央揃えで置く）"""
    w, h = NAME_BODY_W + NAME_TIP * 2, NAME_H
    xx, yy = grid(w, h)
    x0 = NAME_TIP
    point = np.clip((xx - (w - 2 - NAME_TIP)) / NAME_TIP, 0, 1)
    d = np.maximum.reduce([np.abs(yy - h / 2) - (h / 2 - 2) * (1 - point), x0 + 2 - xx, xx - (w - 2)])
    mask = sdf_mask(d)
    inner = sdf_mask(d + 4)
    rim = STEEL[None, None, :] * (_brushed(w, h, 44, base=20) * (1.05 + 1.6 * bevel(mask, 2.0)))[..., None]
    ty = np.clip(yy / h, 0, 1)[..., None]
    in_rgb = np.array([0.13, 0.15, 0.19], np.float32) * (1 - ty) + np.array([0.04, 0.05, 0.07], np.float32) * ty
    rgb = in_rgb * inner[..., None] + rim * (1 - inner[..., None])
    return rgba(rgb, mask * 0.97)


def render_pad_dpad_lr():
    """十字キーの左右。円の中に十字の台座を置き、左右のキーだけ明るくする"""
    r = KEY_H / 2
    c = Canvas(KEY_H + KEY_MARGIN * 2, KEY_H + KEY_MARGIN * 2)
    X, Y = c.X, c.Y
    cx = cy = KEY_MARGIN + r
    pad_frame(c, cx, cy, r)

    arm, thick = r * 0.72, r * 0.24
    cross = np.clip(cov(sd_rbox(X, Y, cx - arm, cy - thick, cx + arm, cy + thick, 1.0))
                    + cov(sd_rbox(X, Y, cx - thick, cy - arm, cx + thick, cy + arm, 1.0)), 0, 1)
    c.over(hexc('#7c8896'), cross * 0.55)

    tip, base, wing = r * 0.74, r * 0.30, r * 0.20
    for direction in (-1, 1):
        ax, ay = cx + direction * tip, cy
        bx, by = cx + direction * base, cy - wing
        ex, ey = cx + direction * base, cy + wing
        inside = np.ones_like(X, np.float32)
        for (sx, sy), (tx, ty), (ox, oy) in (((ax, ay), (bx, by), (ex, ey)),
                                             ((bx, by), (ex, ey), (ax, ay)),
                                             ((ex, ey), (ax, ay), (bx, by))):
            side = (tx - sx) * (Y - sy) - (ty - sy) * (X - sx)
            inside *= (side * np.sign((tx - sx) * (oy - sy) - (ty - sy) * (ox - sx)) >= 0)
        c.over(LINE, inside)
    return c.resolve()


# ---------------------------------------------------------------- アイテムアイコン（写実寄りの塗り）
# 72px で描いて ICON_PX へ縮める
ICON_DRAW = 72


def _shade(mask, base_rgb, seed, grain=0.25, grain_base=10):
    g = 1 - grain / 2 + grain * fbm(mask.shape[1], mask.shape[0], seed, octaves=5, base=grain_base)
    return base_rgb[None, None, :] * (g * (1.0 + 1.4 * bevel(mask, 2.2)))[..., None]


def _flat(rgb):
    return np.ones((ICON_DRAW, ICON_DRAW, 3), np.float32) * np.array(rgb, np.float32)


def _compose(layers):
    s = ICON_DRAW
    out = np.zeros((s, s, 3), np.float32)
    alpha = np.zeros((s, s), np.float32)
    for rgb, m in layers:
        out = rgb * m[..., None] + out * (1 - m[..., None])
        alpha = m + alpha * (1 - m)
    return rgba(out, alpha).resize((ICON_PX, ICON_PX), Image.LANCZOS)


def icon_potion():
    """回復薬: 丸底の硝子瓶に緑の薬、コルク栓"""
    s = ICON_DRAW
    xx, yy = grid(s, s)
    cx, cy = s / 2, s / 2 + 7
    body_d = np.hypot(xx - cx, yy - cy) - 21
    neck_d = np.maximum(np.abs(xx - cx) - 6.5, np.abs(yy - (cy - 25)) - 9)
    glass_d = np.minimum(body_d, neck_d)
    glass = sdf_mask(glass_d)
    liquid = sdf_mask(np.maximum(body_d + 2.5, (cy - 6) - yy))
    cork = sdf_mask(np.maximum(np.abs(xx - cx) - 8, np.abs(yy - (cy - 35)) - 5) - 1)
    lip = sdf_mask(np.maximum(np.abs(xx - cx) - 8.5, np.abs(yy - (cy - 29)) - 1.8) - 0.5)
    rim = np.clip(1 - np.abs(glass_d + 1.5) / 1.6, 0, 1)
    glass_rgb = _flat((0.30, 0.38, 0.40)) + rim[..., None] * 0.5
    t = np.clip(np.hypot(xx - (cx - 5), yy - (cy + 2)) / 22, 0, 1)[..., None]
    liq = np.array([0.36, 0.82, 0.42], np.float32) * (1 - t) + np.array([0.05, 0.28, 0.12], np.float32) * t
    liq = liq * (0.9 + 0.2 * fbm(s, s, 3, octaves=4, base=6))[..., None]
    liq = liq + (np.clip(1 - np.abs(yy - (cy - 6)) / 1.2, 0, 1) * liquid)[..., None] * 0.35
    spec = sdf_mask(np.hypot((xx - (cx - 10)) / 3.2, (yy - (cy - 5)) / 7.5) - 1, 1.0)
    spec2 = sdf_mask(np.hypot(xx - (cx + 11), yy - (cy + 10)) - 1.8, 0.8)
    cork_rgb = _shade(cork, np.array([0.52, 0.36, 0.20], np.float32), 7, grain=0.5, grain_base=8)
    return _compose([(glass_rgb, glass * 0.85), (liq, liquid), (_flat((0.62, 0.66, 0.68)), lip),
                     (cork_rgb, cork), (_flat((1, 1, 1)), spec * 0.75), (_flat((1, 1, 1)), spec2 * 0.6)])


def icon_meat():
    """こんがり肉: 焼き色の付いた骨付き肉"""
    s = ICON_DRAW
    xx, yy = grid(s, s)
    u = (xx - s / 2) * math.cos(0.6) + (yy - s / 2) * math.sin(0.6)
    v = -(xx - s / 2) * math.sin(0.6) + (yy - s / 2) * math.cos(0.6)
    meat_d = np.hypot((u + 6) / 22, v / 16) * 16 - 16 + (fbm(s, s, 11, octaves=3, base=5) - 0.5) * 3
    meat = sdf_mask(meat_d)
    bone_d = np.minimum(np.maximum(np.abs(v) - 3.2, np.abs(u - 16) - 12),
                        np.minimum(np.hypot(u - 28, v - 4) - 4.8, np.hypot(u - 28, v + 4) - 4.8))
    bone = sdf_mask(bone_d) * (1 - meat)
    t = np.clip((meat_d + 16) / 16, 0, 1)[..., None]
    meat_rgb = np.array([0.66, 0.36, 0.18], np.float32) * (1 - t) + np.array([0.28, 0.12, 0.05], np.float32) * t
    meat_rgb = meat_rgb * ((0.7 + 0.6 * fbm(s, s, 12, octaves=5, base=9)) * (1 + 1.2 * bevel(meat, 2.5)))[..., None]
    gloss = sdf_mask(np.hypot((u + 10) / 9, (v + 7) / 3.5) - 1, 1.2) * meat
    bone_rgb = _shade(bone, np.array([0.90, 0.86, 0.76], np.float32), 13, grain=0.15)
    return _compose([(bone_rgb, bone), (meat_rgb, meat), (_flat((1, 0.9, 0.75)), gloss * 0.45)])


def icon_barrel():
    """大タル爆弾: 鉄の箍をはめた樽と火の付いた導火線"""
    s = ICON_DRAW
    xx, yy = grid(s, s)
    cx, cy = s / 2, s / 2 + 5
    bulge = 1 + 0.12 * (1 - ((yy - cy) / 22) ** 2)
    body = sdf_mask(np.maximum(np.abs(xx - cx) - 17 * bulge, np.abs(yy - cy) - 22) - 1)
    across = np.clip((xx - cx) / (17 * bulge), -1, 1)
    staves = 0.8 + 0.2 * np.abs(np.sin((np.arcsin(across) * 3.2) * math.pi))
    round_ = np.sqrt(np.clip(1 - across ** 2, 0, 1)) * 0.7 + 0.35
    light = np.clip(1.15 - (across + 0.3) * 0.35, 0.6, 1.3)
    wood = np.array([0.50, 0.30, 0.15], np.float32) * (0.75 + 0.5 * fbm(s, s, 21, octaves=5, base=4))[..., None]
    wood_rgb = wood * (staves * round_ * light)[..., None]
    hoops = sum(sdf_mask(np.abs(yy - (cy + o)) - 2.3) for o in (-15, 0, 15)) * body
    hoop_rgb = np.array([0.36, 0.38, 0.40], np.float32) * (round_ * light * (0.8 + 0.4 * fbm(s, s, 22)))[..., None] * 1.3
    ft = np.clip((xx - (cx + 2)) / 14, 0, 1)
    fuse_d = np.hypot(xx - (cx + 2 + ft * 12), yy - (cy - 22 - np.sin(ft * 3) * 6)) - 1.6
    fuse = sdf_mask(fuse_d) * (xx > cx) * (xx < cx + 15)
    spark = sdf_mask(np.hypot(xx - (cx + 15), yy - (cy - 26)) - 3.2, 1.6)
    return _compose([(wood_rgb, body), (hoop_rgb, hoops), (_flat((0.78, 0.72, 0.6)), fuse),
                     (_flat((1.0, 0.6, 0.2)), soften(spark, 3.0) * 0.9), (_flat((1.0, 0.95, 0.7)), spark)])


def icon_herb():
    """薬草: 葉脈の通った葉を3枚束ねた株"""
    s = ICON_DRAW
    xx, yy = grid(s, s)
    stem = sdf_mask(np.maximum(np.abs(xx - (s / 2 + (yy - s) * -0.08)) - 1.6, np.abs(yy - (s * 0.78)) - 12))
    layers = [(_flat((0.30, 0.40, 0.16)), stem)]
    for ang, ln, wd, col, seed in ((-42, 20, 8, (0.22, 0.46, 0.18), 31), (40, 19, 8, (0.24, 0.50, 0.20), 32),
                                   (-8, 24, 9.5, (0.34, 0.62, 0.26), 33)):
        a = math.radians(ang - 90)
        lx, ly = s / 2 + math.cos(a) * ln * 0.85, s * 0.62 + math.sin(a) * ln * 0.85
        u = (xx - lx) * math.cos(a) + (yy - ly) * math.sin(a)
        v = -(xx - lx) * math.sin(a) + (yy - ly) * math.cos(a)
        m = sdf_mask((np.hypot(u / ln, v / (wd * (1 - 0.35 * u / ln))) - 1) * wd)
        vein = np.clip(1 - np.abs(v) / 0.9, 0, 1) * 0.35
        layers.append((_shade(m, np.array(col, np.float32), seed, grain=0.35, grain_base=14) + vein[..., None], m))
    return _compose(layers)


SPRITES = {
    'ItemSlot_Backing': render_backing,
    'ItemSlot_Frame': lambda: render_frame(False),
    'ItemSlot_FrameSelected': lambda: render_frame(True),
    'ItemSlot_SelectGlow': render_select_glow,
    'ItemCount_Pill': render_count_nut,
    'ItemName_Plate': render_name_plate,
    'ItemBar_Pad_DPadLR': render_pad_dpad_lr,
    'ItemBar_Pad_LB': lambda: render_pad_trigger('LB'),
    'ItemBar_Key_ZX': lambda: render_keys('Z', 'X'),
    'ItemBar_Key_R': lambda: render_keys('R'),
}

ICONS = {
    'Icon_Potion': icon_potion,
    'Icon_Meat': icon_meat,
    'Icon_Bomb': icon_barrel,
    'Icon_Herb': icon_herb,
}


def text_top(centre_y, px):
    """TextRenderer は文字の上端が y になる（プレビューは縦中央で描いている）ので、中心から px/2 上げる"""
    return math.floor(centre_y - px / 2 + 0.5)


GEOMETRY = {
    'ItemBarUI root localPos (= 一番右の枠の中心)': ROOT_POS,
    'Slots HorizontalLayoutGroup cellSize_': (SLOT_PITCH, 0),
    'Slots HorizontalLayoutGroup spacing_': 0,
    'Slots localPos (ItemBar が実行時に x を上書きする)': (0, 0),
    'NamePlate localPos (x も実行時に上書き)': (0, NAME_OFFSET_Y),
    'NameText localPos (x も実行時に上書き)': (0, text_top(NAME_OFFSET_Y, NAME_TEXT_PX)),
    'ItemBar slotPitch_px_': SLOT_PITCH,
    'ItemBar unselectedScale_': UNSELECTED_SCALE,
    'ItemBar dimAlpha_': DIM_ALPHA,
    'ItemSlot Content localPos / localScale': ((0, 0), 1.0),
    'ItemSlot Backing / Icon / Frame / FrameSelected / SelectGlow localPos': (0, 0),
    'ItemSlot CountPill localPos': NUT_OFFSET,
    'ItemSlot CountText localPos': (NUT_OFFSET[0], text_top(NUT_OFFSET[1], COUNT_TEXT_PX)),
}
GEOMETRY.update({f'Hints {k} localPos': v if k.endswith('Glyph') else (v[0], text_top(v[1], HINT_TEXT_PX))
                 for k, v in HINT_LAYOUT.items()})


def write_sprite(out_dir, name, image):
    png = out_dir / f"{name}.png"
    image.save(png)
    meta = out_dir / f"{name}.png.meta"
    guid = sprite_meta.read_meta(meta)["guid"] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    return guid


# ---------------------------------------------------------------- プレビュー (ItemBar::PresentSlots と同じ処理)
def fade(im, alpha):
    a = np.asarray(im, np.float32).copy()
    a[..., 3] *= alpha
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), 'RGBA')


def add_layer(canvas, im, pos, alpha):
    layer = Image.new('RGBA', canvas.size, (0, 0, 0, 0))
    layer.paste(im, pos)
    src = np.asarray(layer, np.float32) / 255
    dst = np.asarray(canvas, np.float32) / 255
    dst[..., :3] = np.clip(dst[..., :3] + src[..., :3] * src[..., 3:] * alpha, 0, 1)
    return Image.fromarray((dst * 255).astype(np.uint8), 'RGBA')


def paste_centred(canvas, im, centre, alpha, scale=1.0):
    if alpha <= 0.0:
        return
    if scale != 1.0:
        size = (max(1, round(im.width * scale)), max(1, round(im.height * scale)))
        im = im.resize(size, Image.LANCZOS)
    canvas.alpha_composite(fade(im, alpha), (round(centre[0] - im.width / 2), round(centre[1] - im.height / 2)))


def outlined(draw, xy, s, px, fill, alpha):
    stroke = max(1, round(3 * px / 60))
    draw.text(xy, s, font=ImageFont.truetype(str(NAME_FONT), px), anchor='mm', fill=fill + (int(255 * alpha),),
              stroke_width=stroke, stroke_fill=NAME_OUTLINE[:3] + (int(255 * alpha),))


def draw_strip(canvas, sprites, icons, entries, selected, origin, usable_rate, pulse, empty_rate=0.4):
    count = len(entries)
    font_hint = ImageFont.truetype(str(LABEL_FONT), HINT_TEXT_PX)
    centre_slot = count // 2
    dim_rate = DIM_ALPHA / 255.0

    for i in range(count):
        pouch_index = (selected + i - centre_slot) % count
        icon_name, _, num = entries[pouch_index]
        is_selected = (i == centre_slot)
        scale = 1.0 if is_selected else UNSELECTED_SCALE
        alpha = usable_rate * (1.0 if is_selected else dim_rate)
        content = alpha * (1.0 if num > 0 else empty_rate)
        cx = origin[0] - (count - 1 - i) * SLOT_PITCH
        cy = origin[1]

        paste_centred(canvas, sprites['ItemSlot_Backing'], (cx, cy), alpha, scale)
        paste_centred(canvas, icons[icon_name], (cx, cy), content, scale)
        paste_centred(canvas, sprites['ItemSlot_FrameSelected' if is_selected else 'ItemSlot_Frame'],
                      (cx, cy), alpha, scale)
        if is_selected and pulse > 0:
            glow = sprites['ItemSlot_SelectGlow']
            canvas = add_layer(canvas, glow, (round(cx - glow.width / 2), round(cy - glow.height / 2)),
                               pulse * 210 / 255.0)
        px, py = cx + NUT_OFFSET[0] * scale, cy + NUT_OFFSET[1] * scale
        paste_centred(canvas, sprites['ItemCount_Pill'], (px, py), content, scale)
        outlined(ImageDraw.Draw(canvas), (px, py), str(num), round(COUNT_TEXT_PX * scale),
                 COUNT_SELECTED_COLOR if is_selected else COUNT_COLOR, content)

    # 名前プレートは中央の枠の真上
    name_cx = origin[0] - (count - 1 - centre_slot) * SLOT_PITCH
    paste_centred(canvas, sprites['ItemName_Plate'], (name_cx, origin[1] + NAME_OFFSET_Y), usable_rate)
    draw = ImageDraw.Draw(canvas)
    outlined(draw, (name_cx, origin[1] + NAME_OFFSET_Y), entries[selected][1], NAME_TEXT_PX, (255, 255, 255),
             usable_rate)

    for key_name, (ox, oy) in HINT_LAYOUT.items():
        pos = (origin[0] + ox, origin[1] + oy)
        if key_name.endswith('Glyph'):
            sprite = sprites['ItemBar_Key_ZX' if key_name.startswith('Cycle') else 'ItemBar_Key_R']
            paste_centred(canvas, sprite, pos, usable_rate)
        else:
            draw.text(pos, HINT_TEXT[key_name], font=font_hint, anchor='mm',
                      fill=(255, 255, 255, int(255 * usable_rate)))
    return canvas


ENTRIES = [('Icon_Bomb', '大タル爆弾G', 3), ('Icon_Potion', '回復薬グレート', 6), ('Icon_Meat', 'こんがり肉', 0)]


def render_preview(sprites, icons, path, shot=None):
    if shot:
        canvas = Image.open(shot).convert('RGBA').resize((1920, 1080))
        canvas = draw_strip(canvas, sprites, icons, ENTRIES, 1, ROOT_POS, 1.0, 0.0)
        canvas.convert('RGB').save(path)
        return

    panels = [('待機（回復薬を選択）', 1, 1.0, 0.0),
              ('こんがり肉へ切替した瞬間（使い切り）', 2, 1.0, 1.0),
              ('大タル爆弾Gを選択', 0, 1.0, 0.0),
              ('攻撃中（使えない）', 1, 0.6, 0.0)]
    W, H = 560, 300
    sheet = Image.new('RGB', (W * 2, H * 2), (0, 0, 0))
    cap = ImageFont.truetype(str(LABEL_FONT), 20)
    for i, (title, selected, usable, pulse) in enumerate(panels):
        panel = Image.new('RGBA', (W, H), (60, 74, 52, 255))
        grad = np.linspace(0.55, 1.0, H, dtype=np.float32)[:, None, None]
        arr = np.asarray(panel, np.float32).copy()
        arr[..., :3] *= grad
        panel = Image.fromarray(arr.astype(np.uint8), 'RGBA')
        panel = draw_strip(panel, sprites, icons, ENTRIES, selected, (W - 64, H / 2 + 20), usable, pulse)
        ImageDraw.Draw(panel).text((12, 8), title, font=cap, fill=(255, 255, 255, 255))
        sheet.paste(panel.convert('RGB'), ((i % 2) * W, (i // 2) * H))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/ItemBar')
    ap.add_argument('--icon-dir', default='Assets/Art/UI/Item')
    ap.add_argument('--preview', help='also write a composite of pouch states to this PNG path')
    ap.add_argument('--shot', help='with --preview: draw the strip onto this 1920x1080 screenshot instead')
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    icon_dir = Path(args.icon_dir)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir
    if not icon_dir.is_absolute():
        icon_dir = REPO_ROOT / icon_dir
    out_dir.mkdir(parents=True, exist_ok=True)
    icon_dir.mkdir(parents=True, exist_ok=True)

    sprites = {}
    for name, fn in SPRITES.items():
        sprites[name] = fn()
        guid = write_sprite(out_dir, name, sprites[name])
        print(f"{name:28s} {sprites[name].size[0]}x{sprites[name].size[1]}  {guid}")

    icons = {}
    for name, fn in ICONS.items():
        icons[name] = fn()
        guid = write_sprite(icon_dir, name, icons[name])
        print(f"{name:28s} {icons[name].size[0]}x{icons[name].size[1]}  {guid}")

    print("GEOMETRY (ItemBarUI.prefab / ItemSlot.prefab):")
    for k, v in GEOMETRY.items():
        print(f"  {k} = {v}")

    if args.preview:
        render_preview(sprites, icons, args.preview, args.shot)
        print(f"preview -> {args.preview}")


if __name__ == '__main__':
    main()
