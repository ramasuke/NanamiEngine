"""Generate the item-pouch sprites (screen bottom-right) drawn by GamePlay::Ui::ItemBar / ItemSlot.

    python tools/art/item_bar.py [--out-dir Assets/Art/UI/ItemBar] [--icon-dir Assets/Art/UI/Item] [--preview PATH]

Writes the slot parts (ItemSlot_*), the count pill, the name plate and the input glyphs into --out-dir, and
one icon per item into --icon-dir, with a SpriteFile .png.meta for each (an existing .meta keeps its GUID, so
regenerating never breaks references).
Everything is authored at the *selected* slot size; unselected slots are the same sprites drawn at
ItemBar::unselectedScale_ through the slot's Content transform, so only one frame set exists per part.
The palette and the raster helpers come from tools/art/control_guide.py, so the pouch matches the guide.
The layout values ItemBarUI.prefab / ItemSlot.prefab have to agree with are printed at the end (GEOMETRY).
--preview composites the strip the way ItemBar::PresentSlots draws it (selected centred, empty slot dimmed,
the switch pulse, and the dimmed "cannot use" state).

Requires Pillow + numpy.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art.control_guide import (  # noqa: E402
    Canvas, KEY_H, KEY_MARGIN, LINE, TEAL, GOLD, blur, cov, hexc,
    pad_frame, render_keys, render_pad_trigger, sd_circle, sd_rbox,
)
from tools.scene import sprite_meta  # noqa: E402

LABEL_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'ipam.ttf'

# ---- 枠まわり（選択中の枠を等倍とした設計値。非選択は ItemBar 側のスケールで縮む）
SLOT = 70.0             # 枠の内寸（鋼枠の外形）
FRAME_PX = 108          # 枠スプライトの一辺（左右のフィンと外側のティール縁が収まる大きさ）
BACKING_PX = 64
GLOW_PX = 152
ICON_PX = 56
PILL_W, PILL_H = 30, 20
PILL_OFFSET = (20.0, 22.0)   # 枠の中心から個数ピルの中心まで（枠の内側に収まる位置）
NAME_W, NAME_H = 244, 38
NAME_TAIL = 11

# ---- 帯の並び（ItemBar の serialize 既定値と合わせること）
SLOT_PITCH = 76.0
UNSELECTED_SCALE = 0.74
NAME_OFFSET_Y = -72.0
HINT_OFFSET_Y = 62.0
# ルート（= 一番右の枠の中心）から見た操作ヒントの中心
HINT_LAYOUT = {
    'UseLabel': (-24.0, HINT_OFFSET_Y),
    'UseGlyph': (-76.0, HINT_OFFSET_Y),
    'CycleLabel': (-130.0, HINT_OFFSET_Y),
    'CycleGlyph': (-182.0, HINT_OFFSET_Y),
}

STEEL = hexc('#7c8896')
STEEL_LT = hexc('#c0cad4')
INK = hexc('#06101a')
TEAL_DEEP = hexc('#0c2a30')


def _square(px):
    c = Canvas(px, px)
    return c, c.X, c.Y, px / 2.0, px / 2.0


def render_backing():
    """枠の内側の暗板。選択中かどうかは枠の明るさと大きさで見せるので1種類だけ"""
    c, X, Y, cx, cy = _square(BACKING_PX)
    h = SLOT / 2 - 4
    d = sd_rbox(X, Y, cx - h, cy - h, cx + h, cy + h, SLOT * 0.12)
    c.over(INK, cov(d) * 0.86)
    c.over(hexc('#16202a'), cov(d + 2.5) * 0.6)
    # 上から下へのわずかな明暗
    c.over(hexc('#ffffff'), cov(d + 2.5) * np.clip((cy - Y) / SLOT, 0, 1) * 0.05)
    return c.resolve()


def _frame_ring(c, X, Y, cx, cy, color, width_px, alpha=1.0):
    h = SLOT / 2
    d = sd_rbox(X, Y, cx - h, cy - h, cx + h, cy + h, SLOT * 0.15)
    c.over(color, cov(d) * (1 - cov(d + width_px)) * alpha)


def render_frame(selected):
    """鋼の枠。選択中は明るいリム＋外側のティール縁＋左右の三角フィン"""
    c, X, Y, cx, cy = _square(FRAME_PX)
    h = SLOT / 2

    if selected:
        fin = SLOT * 0.18
        for sign in (-1, 1):
            x0 = cx + sign * (h + 3)
            tipx = x0 + sign * fin
            # 三角形（頂点 x が外、底辺が枠側）
            t = np.clip((X - x0) / (tipx - x0), 0, 1) if sign > 0 else np.clip((x0 - X) / (x0 - tipx), 0, 1)
            half = (1 - t) * fin * 0.95
            shape = cov(np.abs(Y - cy) - half) * (t > 0) * (t < 1)
            c.over(STEEL_LT, shape * 0.95)

    _frame_ring(c, X, Y, cx, cy, STEEL_LT if selected else STEEL, 4.2 if selected else 3.2)
    if selected:
        # 外側のティール縁。選ばれている枠だけ光って見えるようにする
        d = sd_rbox(X, Y, cx - h - 4, cy - h - 4, cx + h + 4, cy + h + 4, SLOT * 0.18)
        ring = cov(d) * (1 - cov(d + 2.2))
        c.over(TEAL, np.clip(blur(ring, 2.4) * 1.4, 0, 1) * 0.26)
        c.over(TEAL, ring)
    return c.resolve()


def render_select_glow():
    """切替の瞬間に加算で重ねる光"""
    c, X, Y, cx, cy = _square(GLOW_PX)
    h = SLOT / 2
    d = sd_rbox(X, Y, cx - h, cy - h, cx + h, cy + h, SLOT * 0.15)
    ring = cov(d) * (1 - cov(d + 3.0))
    c.over(TEAL, np.clip(blur(ring, 7.0) * 2.4, 0, 1) * 0.8)
    c.over(hexc('#e8fffb'), ring * 0.75)
    return c.resolve()


def render_count_pill():
    """個数を載せる小さなピル。選択中かどうかは数字の色（ItemSlot の countSelectedColor_）で見せる"""
    c = Canvas(PILL_W, PILL_H)
    X, Y = c.X, c.Y
    d = sd_rbox(X, Y, 1.5, 1.5, PILL_W - 1.5, PILL_H - 1.5, (PILL_H - 3) / 2)
    c.over(INK, cov(d) * 0.95)
    c.over(STEEL, cov(d) * (1 - cov(d + 1.8)))
    return c.resolve()


def render_name_plate():
    """選択中のアイテム名を載せる帯。下向きのしっぽで枠を指す"""
    c = Canvas(NAME_W, NAME_H + NAME_TAIL)
    X, Y = c.X, c.Y
    body = cov(sd_rbox(X, Y, 0, 0, NAME_W, NAME_H, 4))
    tail_half = 8.0 * np.clip(1 - (Y - NAME_H) / NAME_TAIL, 0, 1)
    tail = cov(np.abs(X - NAME_W / 2) - tail_half) * (Y >= NAME_H)
    shape = np.clip(body + tail, 0, 1)
    c.over(hexc('#04121a'), shape * 0.84)
    # 縁取りは本体だけ（しっぽは塗りつぶしのまま）
    edge = cov(sd_rbox(X, Y, 0, 0, NAME_W, NAME_H, 4)) * (1 - cov(sd_rbox(X, Y, 0, 0, NAME_W, NAME_H, 4) + 1.7))
    c.over(TEAL, edge * 0.65)
    c.over(TEAL, cov(sd_rbox(X, Y, 7, 6, 10.5, NAME_H - 6, 0.5)))
    return c.resolve()


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
    c.over(STEEL, cross * 0.55)

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


# ---------------------------------------------------------------- アイテムアイコン
def _icon_canvas():
    c = Canvas(ICON_PX, ICON_PX)
    return c, c.X, c.Y, ICON_PX / 2.0, ICON_PX / 2.0


def _tri(c, pts, color, alpha=1.0):
    X, Y = c.X, c.Y
    inside = np.ones_like(X, np.float32)
    for i in range(3):
        (sx, sy), (tx, ty), (ox, oy) = pts[i], pts[(i + 1) % 3], pts[(i + 2) % 3]
        side = (tx - sx) * (Y - sy) - (ty - sy) * (X - sx)
        inside *= (side * np.sign((tx - sx) * (oy - sy) - (ty - sy) * (ox - sx)) >= 0)
    c.over(color, inside * alpha)


def icon_flask(liquid):
    c, X, Y, cx, cy = _icon_canvas()
    body = sd_circle(X, Y, cx, cy + 6, 15)
    neck = sd_rbox(X, Y, cx - 5, cy - 16, cx + 5, cy, 2)
    cork = sd_rbox(X, Y, cx - 7, cy - 22, cx + 7, cy - 15, 2)
    c.over(hexc('#8d9aa2'), cov(neck) * 0.85)
    c.over(liquid, cov(body))
    c.over(hexc('#ffffff'), cov(sd_circle(X, Y, cx - 5, cy + 2, 4)) * 0.28)
    c.over(STEEL_LT, cov(body) * (1 - cov(body + 1.8)))
    c.over(STEEL_LT, cov(neck) * (1 - cov(neck + 1.4)))
    c.over(hexc('#7a5a36'), cov(cork))
    c.over(hexc('#3e2a16'), cov(cork) * (1 - cov(cork + 1.4)))
    return c.resolve()


def icon_whetstone():
    c, X, Y, cx, cy = _icon_canvas()
    pts = [(cx - 17, cy + 7), (cx - 7, cy - 16), (cx + 17, cy - 6), (cx + 7, cy + 16)]
    for tri in ((pts[0], pts[1], pts[2]), (pts[0], pts[2], pts[3])):
        _tri(c, list(tri), hexc('#848c96'))
    # 稜線
    seg = np.clip(((X - pts[1][0]) * (pts[3][0] - pts[1][0]) + (Y - pts[1][1]) * (pts[3][1] - pts[1][1]))
                  / ((pts[3][0] - pts[1][0]) ** 2 + (pts[3][1] - pts[1][1]) ** 2), 0, 1)
    ridge = np.hypot(X - (pts[1][0] + seg * (pts[3][0] - pts[1][0])), Y - (pts[1][1] + seg * (pts[3][1] - pts[1][1])))
    c.over(STEEL_LT, cov(ridge - 1.1) * 0.8)
    return c.resolve()


def icon_meat():
    c, X, Y, cx, cy = _icon_canvas()
    meat = sd_circle(X, Y, cx - 3, cy + 6, 15)
    c.over(hexc('#8f4a2c'), cov(meat))
    c.over(hexc('#b86c44'), cov(sd_circle(X, Y, cx - 6, cy + 3, 8)) * 0.85)
    c.over(hexc('#54281a'), cov(meat) * (1 - cov(meat + 1.8)))
    bone = sd_rbox(X, Y, cx + 1, cy - 17, cx + 5, cy - 2, 2)
    c.over(hexc('#efeade'), cov(bone))
    c.over(hexc('#efeade'), cov(sd_circle(X, Y, cx + 3, cy - 18, 5)))
    return c.resolve()


def icon_trap():
    c, X, Y, cx, cy = _icon_canvas()
    pit = sd_circle(X, Y, cx, cy, 17) * np.ones_like(X)
    ring = cov(np.abs(np.hypot(X - cx, (Y - cy) * 1.7) - 16) - 1.4)
    c.over(INK, cov(np.hypot(X - cx, (Y - cy) * 1.7) - 16) * 0.9)
    inside = cov(np.hypot(X - cx, (Y - cy) * 1.7) - 15)
    for i in range(5):
        x = cx - 12 + i * 6
        c.over(hexc('#9aa4ae'), cov(np.abs(X - x) - 0.9) * inside * 0.8)
    for i in range(3):
        y = cy - 6 + i * 6
        c.over(hexc('#9aa4ae'), cov(np.abs(Y - y) - 0.9) * inside * 0.8)
    c.over(STEEL_LT, ring)
    del pit
    return c.resolve()


def icon_bomb():
    c, X, Y, cx, cy = _icon_canvas()
    barrel = sd_rbox(X, Y, cx - 13, cy - 8, cx + 13, cy + 17, 4)
    c.over(hexc('#7a5636'), cov(barrel))
    c.over(hexc('#42280f'), cov(barrel) * (1 - cov(barrel + 1.6)))
    for y in (cy - 2, cy + 9):
        c.over(hexc('#aab4be'), cov(np.abs(Y - y) - 1.6) * cov(barrel + 1.0))
    fuse_t = np.clip((X - cx) / 9.0, 0, 1)
    fuse = np.hypot(X - (cx + fuse_t * 9), Y - (cy - 8 - fuse_t * 11)) - 1.5
    c.over(hexc('#d2c8b4'), cov(fuse) * (X >= cx) * (Y <= cy - 8))
    c.over(GOLD, cov(sd_circle(X, Y, cx + 9, cy - 19, 3.6)))
    return c.resolve()


SPRITES = {
    'ItemSlot_Backing': render_backing,
    'ItemSlot_Frame': lambda: render_frame(False),
    'ItemSlot_FrameSelected': lambda: render_frame(True),
    'ItemSlot_SelectGlow': render_select_glow,
    'ItemCount_Pill': render_count_pill,
    'ItemName_Plate': render_name_plate,
    'ItemBar_Pad_DPadLR': render_pad_dpad_lr,
    'ItemBar_Pad_LB': lambda: render_pad_trigger('LB'),
    'ItemBar_Key_ZX': lambda: render_keys('Z', 'X'),
    'ItemBar_Key_R': lambda: render_keys('R'),
}

ICONS = {
    'Icon_Potion': lambda: icon_flask(hexc('#4ac262')),
    'Icon_Whetstone': icon_whetstone,
    'Icon_Meat': icon_meat,
    'Icon_Trap': icon_trap,
    'Icon_Bomb': icon_bomb,
}

GEOMETRY = {
    'ItemBarUI root localPos (= 一番右の枠の中心)': (1860, 972),
    'Slots HorizontalLayoutGroup cellSize_': (SLOT_PITCH, 0),
    'Slots HorizontalLayoutGroup spacing_': 0,
    'Slots localPos (ItemBar が実行時に x を上書きする)': (0, 0),
    'NamePlate / NameText localPos (x も実行時に上書き)': (0, NAME_OFFSET_Y),
    'ItemBar slotPitch_px_': SLOT_PITCH,
    'ItemBar unselectedScale_': UNSELECTED_SCALE,
    'ItemSlot Content localPos / localScale': ((0, 0), 1.0),
    'ItemSlot Backing localPos': (0, 0),
    'ItemSlot Icon localPos': (0, -2),
    'ItemSlot Frame / FrameSelected / SelectGlow localPos': (0, 0),
    'ItemSlot CountPill / CountText localPos': PILL_OFFSET,
}
GEOMETRY.update({f'Hints {k} localPos': v for k, v in HINT_LAYOUT.items()})


def write_sprite(out_dir, name, image):
    png = out_dir / f"{name}.png"
    image.save(png)
    meta = out_dir / f"{name}.png.meta"
    guid = sprite_meta.read_meta(meta)["guid"] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    return guid


# ---------------------------------------------------------------- preview (mirrors ItemBar::PresentSlots)
def fade(im, alpha):
    a = np.asarray(im, np.float32)
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


def draw_strip(canvas, sprites, icons, entries, selected, origin, usable_rate, pulse, dim_alpha=140 / 255.0,
               empty_rate=0.4):
    count = len(entries)
    font_count = ImageFont.truetype(str(LABEL_FONT), 17)
    font_name = ImageFont.truetype(str(LABEL_FONT), 25)
    font_hint = ImageFont.truetype(str(LABEL_FONT), 21)
    centre_slot = count // 2

    for i in range(count):
        pouch_index = (selected + i - centre_slot) % count
        icon_name, _, num = entries[pouch_index]
        is_selected = (i == centre_slot)
        scale = 1.0 if is_selected else UNSELECTED_SCALE
        alpha = usable_rate * (1.0 if is_selected else dim_alpha)
        content = usable_rate * (1.0 if num > 0 else empty_rate) * (1.0 if is_selected else dim_alpha)
        cx = origin[0] - (count - 1 - i) * SLOT_PITCH
        cy = origin[1]

        paste_centred(canvas, sprites['ItemSlot_Backing'], (cx, cy), alpha, scale)
        paste_centred(canvas, icons[icon_name], (cx, cy - 2 * scale), content, scale)
        paste_centred(canvas, sprites['ItemSlot_FrameSelected' if is_selected else 'ItemSlot_Frame'],
                      (cx, cy), alpha, scale)
        if is_selected and pulse > 0:
            glow = sprites['ItemSlot_SelectGlow']
            canvas_size = canvas.size
            glow_scaled = glow.resize((round(glow.width * scale), round(glow.height * scale)), Image.LANCZOS)
            canvas = add_layer(canvas, glow_scaled,
                               (round(cx - glow_scaled.width / 2), round(cy - glow_scaled.height / 2)),
                               pulse * 210 / 255.0)
            del canvas_size
        pill = sprites['ItemCount_Pill']
        px, py = cx + PILL_OFFSET[0] * scale, cy + PILL_OFFSET[1] * scale
        paste_centred(canvas, pill, (px, py), content, scale)
        draw = ImageDraw.Draw(canvas)
        draw.text((px, py - 1), str(num), font=font_count, anchor='mm',
                  fill=((255, 206, 104) if is_selected else (196, 208, 212)) + (int(255 * content),))

    # 名前プレートは中央の枠の真上
    name_cx = origin[0] - (count - 1 - centre_slot) * SLOT_PITCH
    plate = sprites['ItemName_Plate']
    paste_centred(canvas, plate, (name_cx, origin[1] + NAME_OFFSET_Y), usable_rate)
    draw = ImageDraw.Draw(canvas)
    draw.text((name_cx + 6, origin[1] + NAME_OFFSET_Y - NAME_TAIL / 2), entries[selected][1], font=font_name,
              anchor='mm', fill=(228, 240, 242, int(255 * usable_rate)))

    for key_name, (ox, oy) in HINT_LAYOUT.items():
        pos = (origin[0] + ox, origin[1] + oy)
        if key_name.endswith('Glyph'):
            sprite = sprites['ItemBar_Pad_DPadLR' if key_name.startswith('Cycle') else 'ItemBar_Pad_LB']
            paste_centred(canvas, sprite, pos, usable_rate)
        else:
            draw.text(pos, '切替' if key_name.startswith('Cycle') else '使用', font=font_hint, anchor='mm',
                      fill=(206, 216, 220, int(255 * usable_rate)))
    return canvas


def render_preview(sprites, icons, path):
    entries = [('Icon_Potion', '回復薬グレート', 5), ('Icon_Whetstone', '砥石', 9),
               ('Icon_Meat', 'こんがり肉', 3), ('Icon_Trap', '落とし穴', 0),
               ('Icon_Bomb', '大タル爆弾G', 4)]
    panels = [('待機（回復薬を選択）', 0, 1.0, 0.0),
              ('こんがり肉へ切替した瞬間', 2, 1.0, 1.0),
              ('使い切った落とし穴', 3, 1.0, 0.0),
              ('攻撃中（使えない）', 0, 0.6, 0.0)]

    W, H = 520, 260
    sheet = Image.new('RGB', (W * 2, H * 2), (0, 0, 0))
    cap = ImageFont.truetype(str(LABEL_FONT), 20)
    for i, (title, selected, usable, pulse) in enumerate(panels):
        panel = Image.new('RGBA', (W, H), (60, 74, 52, 255))
        grad = np.linspace(0.55, 1.0, H, dtype=np.float32)[:, None, None]
        arr = np.asarray(panel, np.float32)
        arr[..., :3] *= grad
        panel = Image.fromarray(arr.astype(np.uint8), 'RGBA')
        panel = draw_strip(panel, sprites, icons, entries, selected, (W - 32, H / 2 + 10), usable, pulse)
        ImageDraw.Draw(panel).text((12, 8), title, font=cap, fill=(255, 255, 255, 255))
        sheet.paste(panel.convert('RGB'), ((i % 2) * W, (i // 2) * H))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/ItemBar')
    ap.add_argument('--icon-dir', default='Assets/Art/UI/Item')
    ap.add_argument('--preview', help='also write a composite of pouch states to this PNG path')
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
        render_preview(sprites, icons, args.preview)
        print(f"preview -> {args.preview}")


if __name__ == '__main__':
    main()
