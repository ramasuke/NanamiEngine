"""Generate the shop (merchant stall) UI sprites, and previews composited on a real game screen.

    python tools/art/shop.py --shot <screenshot.png> --out-dir <dir>   # 実画面に合成した完成イメージ
    python tools/art/shop.py --emit                                     # スプライトを Assets/Art/UI/Shop へ
    python tools/art/shop.py --emit-model-textures                      # 露店の3Dモデル用テクスチャを _Source へ

決定案は「品書きの黒板」: 左に木枠の黒板を立て、品物をチョークで書き、選んだ品を丸で囲む。
右には紐で吊るした勘定書き、右下に財布と所持金の札。プレハブ (tools/art/shop_prefab.py) は
LAYOUT の座標でこのスプライトと TextRenderer を並べる。--shot の完成イメージも同じ部品・同じ座標で描くので、
ここで見た配置がそのままゲームに出る。

チョークの文字は TextRenderer が描く。かすれは ChalkGrain (黒板と同じ色の斑点) を文字の上に重ねて出す。
黒板の上に貼る紙片とアイコンは斑点より上に描くので、紙までかすれることはない。

Requires Pillow + numpy.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import character_select as cs  # noqa: E402
from character_select import (  # noqa: E402
    BODY_FONT, BRUSH_FONT, INK, INK_FADE, STAMP_RED, bevel, fbm, grid, parchment, rect_outside, rgba, soften,
    wood_board)

EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Shop'
MODEL_SOURCE_DIR = REPO_ROOT / 'Assets' / 'Art' / 'Models' / 'Prop' / 'MerchantStall' / '_Source'
ITEM_ICON_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Item'
HINT_SPRITES = {
    'HintTag_UpDown': REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'EventBoard' / 'HintTag_UpDown.png',
    'HintTag_Move': REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Move.png',
    'HintTag_Confirm': REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Confirm.png',
    'HintTag_Cancel': REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Cancel.png',
}
BLACK_MASK = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'BlackMask.png'

SCREEN_W, SCREEN_H = 1920, 1080
ICON_PX = 56  # Assets/Art/UI/Item/Icon_*.png の一辺

CHALK = (240, 238, 228)
CHALK_DIM = (200, 208, 196)
CHALK_YELLOW = (250, 226, 150)
CHALK_RED = (214, 120, 104)
HINT_COLOR = (240, 226, 202)
SLATE = np.array([0.12, 0.16, 0.14], np.float32)

# ---------------------------------------------------------------- 寸法と配置
BOARD_SIZE = (640, 800)          # 木枠込みの黒板
BOARD_FRAME = 30
BOARD_SPRITE = (660, 822)        # 粉受けの分だけ横と下へはみ出す
RECEIPT_SIZE = (470, 640)
ROW_PITCH = 150
MAX_ROWS = 4

# 黒板(木枠の左上)を原点にした座標を画面へ
BOARD_ORIGIN = (690, 129)


def on_board(x, y):
    return BOARD_ORIGIN[0] + x, BOARD_ORIGIN[1] + y


RECEIPT_CENTER = (1596, 426)


def on_receipt(x, y):
    """勘定書きの左上からの座標を、勘定書きの中心からの相対へ"""
    return x - RECEIPT_SIZE[0] // 2, y - RECEIPT_SIZE[1] // 2


# 位置はスプライトなら中心、文字なら左上 (右寄せは右上 / 中央寄せは上辺の中央)。
# 行と勘定書きの中身は、それぞれの親からの相対
LAYOUT = {
    'veil': ((SCREEN_W / 2, SCREEN_H / 2), 0.47, 110),     # BlackMask の中心・倍率・濃さ
    'board': on_board(BOARD_SPRITE[0] / 2 - 10, BOARD_SPRITE[1] / 2),
    'grain': on_board(BOARD_SIZE[0] / 2, BOARD_SIZE[1] / 2),
    'title': (on_board(320, 59), 46),
    'title_rule': on_board(320, 129),
    'rows_origin': on_board(320, 226),
    'more_above': on_board(596, 164),
    'more_below': on_board(596, 752),
    'restock': ((-120, -15), 30),
    # 行 (行の中心から)
    'row_button': (600, 140),
    'row_chip': (-184, 0),
    'row_icon': ((-184, 2), 76 / ICON_PX),
    'row_name': ((-120, -38), 40),
    'row_owned': ((-118, 24), 20),
    'row_price': ((250, -17), 38),
    'row_circle': (59, -1),
    'row_arrow': (-234, -4),
    # 勘定書き (中心から)
    'receipt': RECEIPT_CENTER,
    'twine': (RECEIPT_CENTER[0], 66),
    'rc_icon': (on_receipt(86, 166), 84 / ICON_PX),
    'rc_name': (on_receipt(144, 129), 42),
    'rc_owned': (on_receipt(146, 181), 21),
    'rc_desc': (on_receipt(52, 238), 23, 36),                # 1行目の左上・大きさ・行送り
    'rc_desc_lines': 2,
    'rc_unit_price': (on_receipt(418, 377), 26),
    'rc_decrease': on_receipt(320, 446),
    'rc_quantity': (on_receipt(366, 424), 44),
    'rc_increase': on_receipt(412, 446),
    'rc_total': (on_receipt(418, 517), 46),
    'rc_after': (on_receipt(418, 580), 20),
    'rc_refusal': (on_receipt(418, 578), 22),
    'rc_mark_inactive': 70,                                  # 端の矢印の濃さ (BlendImageRenderer)
    'rc_stamp': on_receipt(372, 262),
    # 財布
    'purse': (1438, 872),
    'money_label': ((1500, 801), 22),
    'money_tag': (1612, 880),
    'money_text': ((1612, 863), 34),
    # 操作ヒント (右から詰める)
    'hints_right': 1872,
    'hints_y': 1032,
    'hint_px': 26,
}
HINTS = [('HintTag_UpDown', 68, '選ぶ'), ('HintTag_Move', 68, '個数'),
         ('HintTag_Confirm', 46, '買う'), ('HintTag_Cancel', 46, '閉じる')]


def hint_layout():
    """文字は左上、札は中心の座標で返す (event_board.v2_hint_layout と同じ詰め方)"""
    x, y, px = LAYOUT['hints_right'], LAYOUT['hints_y'], LAYOUT['hint_px']
    placed = []
    for sprite, glyph_w, label in reversed(HINTS):
        text_w = cs.font(BODY_FONT, px).getlength(label)
        placed.append(dict(kind='text', text=label, pos=(round(x - text_w), y - px // 2)))
        x -= text_w + 16
        placed.append(dict(kind='tag', sprite=sprite, pos=(round(x - glyph_w / 2), y)))
        x -= glyph_w + 30
    return placed


# ---------------------------------------------------------------- 素材
def slate(w, h, seed):
    xx, yy = grid(w, h)
    n = fbm(w, h, seed, octaves=5, base=5)
    dust = np.clip(fbm(w, h, seed + 3, octaves=4, base=3) * 2.1 - 1.0, 0, 1)
    swirl = np.clip(np.sin((xx * 0.012 + yy * 0.02) + fbm(w, h, seed + 5, 3, 2) * 9) * 0.5 + 0.5, 0, 1) * dust
    rgb = SLATE[None, None, :] * (0.85 + 0.35 * n)[..., None]
    rgb = rgb + (0.07 * dust + 0.05 * swirl)[..., None]
    return rgba(rgb, np.ones((h, w), np.float32))


def darken(im, k):
    arr = np.asarray(im, np.float32) / 255.0
    arr[..., :3] *= k
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA')


def chalkboard():
    """木枠の黒板と、その下の粉受け。文字は載せない"""
    w, h = BOARD_SIZE
    frame = BOARD_FRAME
    board = darken(wood_board(w, h, 7, plank=9999), 1.15)
    board.alpha_composite(slate(w - frame * 2, h - frame * 2, 8), (frame, frame))
    xx, yy = grid(w, h)
    d = rect_outside(xx, yy, w, h, frame)
    inner = np.clip(1 - (-d) / 16, 0, 1) * (d < 0)
    board.alpha_composite(rgba(np.zeros((h, w, 3), np.float32), inner * 0.55))
    ImageDraw.Draw(board).rectangle([4, 4, w - 5, h - 5], outline=(70, 44, 22, 255), width=3)

    sprite = Image.new('RGBA', BOARD_SPRITE, (0, 0, 0, 0))
    sprite.alpha_composite(board, (10, 0))
    sprite.alpha_composite(wood_board(BOARD_SPRITE[0], 26, 90, plank=9999), (0, h - 4))
    return sprite


def chalk_grain():
    """文字の上に重ねる黒板色の斑点。黒板の上では見えず、チョークの線だけがかすれて見える"""
    w, h = BOARD_SIZE[0] - BOARD_FRAME * 2, BOARD_SIZE[1] - BOARD_FRAME * 2
    rng = np.random.default_rng(11)
    grain = 1 - (0.45 + 0.55 * fbm(w, h, 11, octaves=6, base=70))
    speck = (rng.random((h, w)) < 0.16).astype(np.float32)
    alpha = np.clip(grain * 0.9 + speck * 0.8, 0, 1) * 0.85
    base = slate(w, h, 8)
    arr = np.asarray(base, np.float32) / 255.0
    arr[..., 3] = alpha
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA')


def chalk_stroke(size, draw_fn, seed):
    """draw_fn(ImageDraw) で白く描いた線に、チョークの粒を焼く"""
    w, h = size
    lay = Image.new('RGBA', size, (0, 0, 0, 0))
    draw_fn(ImageDraw.Draw(lay))
    arr = np.asarray(lay, np.float32) / 255.0
    rng = np.random.default_rng(seed)
    grain = (0.45 + 0.55 * fbm(w, h, seed, octaves=6, base=40)) * (rng.random((h, w)) > 0.16)
    arr[..., 3] *= grain * 0.92
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA')


def chalk_rule():
    return chalk_stroke((500, 16), lambda d: d.line([(10, 5), (490, 11)], fill=(*CHALK, 255), width=3), 21)


def chalk_circle():
    w, h = 470, 146
    return chalk_stroke((w, h), lambda d: d.ellipse([6, 6, w - 7, h - 7], outline=(250, 236, 180, 255), width=5), 22)


def chalk_arrow():
    def draw(d):
        d.line([(4, 20), (80, 20)], fill=(250, 236, 180, 255), width=6)
        d.polygon([(96, 20), (74, 2), (74, 38)], fill=(250, 236, 180, 255))
    return chalk_stroke((100, 40), draw, 23)


def chalk_more(pointing_up):
    def draw(d):
        pts = [(4, 18), (16, 2), (28, 18)] if pointing_up else [(4, 2), (16, 18), (28, 2)]
        d.polygon(pts, fill=(*CHALK_DIM, 255))
    return chalk_stroke((32, 20), draw, 24 if pointing_up else 25)


def ink_arrow(pointing_left):
    """勘定書きの個数の左右。Shift-JIS に無い ◀▶ は TextRenderer で書けないので絵にする"""
    w, h = 22, 26
    lay = Image.new('RGBA', (w * 4, h * 4), (0, 0, 0, 0))
    pts = [(80, 8), (8, 52), (80, 96)] if pointing_left else [(8, 8), (80, 52), (8, 96)]
    ImageDraw.Draw(lay).polygon(pts, fill=(*INK, 235))
    return lay.resize((w, h), Image.LANCZOS)


def paper_chip():
    """アイコンを貼る紙片。テープ付き、少し傾ける"""
    size = 104
    p = parchment(size, size, 60, aged=0.15, ragged=6.0)
    tape = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    ImageDraw.Draw(tape).rectangle([size * 0.28, 0, size * 0.72, size * 0.16], fill=(226, 214, 180, 150))
    p.alpha_composite(tape)
    return p.rotate(-3, resample=Image.BICUBIC, expand=True)


def receipt_paper():
    """勘定書き。見出しと罫線は焼き込み、値は TextRenderer が書く"""
    w, h = RECEIPT_SIZE
    p = parchment(w, h, 120, aged=0.18, ragged=10.0)
    d = ImageDraw.Draw(p)
    d.ellipse([w / 2 - 9, 20, w / 2 + 9, 38], fill=(40, 28, 18, 255))
    cs.text(p, (w / 2, 76), 'お 勘 定', 30, (*INK_FADE, 255), BRUSH_FONT, anchor='mm')
    d.line([(46, 104), (w - 46, 104)], fill=INK_FADE, width=2)
    y = 350
    d.line([(46, y), (w - 46, y)], fill=(160, 136, 104), width=1)
    cs.text(p, (52, y + 40), '単 価', 23, (*INK_FADE, 255), BODY_FONT, anchor='lm')
    cs.text(p, (52, y + 96), '個 数', 23, (*INK_FADE, 255), BODY_FONT, anchor='lm')
    d.line([(46, y + 138), (w - 46, y + 138)], fill=INK_FADE, width=3)
    d.line([(46, y + 144), (w - 46, y + 144)], fill=INK_FADE, width=1)
    cs.text(p, (52, y + 190), '合 計', 26, (*INK, 255), BODY_FONT, anchor='lm')
    return p


def twine():
    w, h = 12, 132
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.line([(6, 0), (6, h)], fill=(150, 118, 76, 255), width=3)
    d.line([(7, 0), (7, h)], fill=(88, 64, 36, 150), width=1)
    return im


def leather_purse(w=118, h=128, seed=3):
    xx, yy = grid(w, h)
    body = np.hypot((xx - w / 2) / (w * 0.46), (yy - h * 0.64) / (h * 0.36)) - 1
    neck = rect_outside(xx - w * 0.32, yy - h * 0.18, w * 0.36, h * 0.18, 0)
    frill = np.hypot((xx - w / 2) / (w * 0.30), (yy - h * 0.14) / (h * 0.13)) - 1
    d = np.minimum.reduce([body * 30, neck, frill * 16]) + (fbm(w, h, seed, 3, 5) - 0.5) * 5
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)
    b = bevel(mask, 5.0)
    fold = 0.85 + 0.25 * np.sin(xx * 0.21 + fbm(w, h, seed + 1, 3, 3) * 6)
    tone = (0.75 + 0.45 * fbm(w, h, seed + 2, 5, 9)) * fold * (1.0 + 1.4 * b)
    im = rgba(np.array([0.42, 0.25, 0.12], np.float32)[None, None, :] * tone[..., None], mask)
    d2 = ImageDraw.Draw(im)
    d2.line([(w * 0.26, h * 0.30), (w * 0.74, h * 0.30)], fill=(196, 160, 102, 255), width=4)
    d2.ellipse([w * 0.70, h * 0.26, w * 0.80, h * 0.36], fill=(170, 130, 70, 255))
    return im.rotate(-6, resample=Image.BICUBIC, expand=True)


def money_tag():
    return parchment(230, 64, 150, aged=0.2, ragged=6).rotate(-2, resample=Image.BICUBIC, expand=True)


def paid_stamp():
    """買えたときに勘定書きへ押す朱の角判「毎度」"""
    size = 132
    lay = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    d.rounded_rectangle([10, 10, size - 11, size - 11], radius=10, outline=(*STAMP_RED, 255), width=7)
    cs.text(lay, (size / 2, size / 2 - 22), '毎', 46, (*STAMP_RED, 255), BRUSH_FONT, anchor='mm')
    cs.text(lay, (size / 2, size / 2 + 24), '度', 46, (*STAMP_RED, 255), BRUSH_FONT, anchor='mm')
    arr = np.asarray(lay, np.float32) / 255.0
    rng = np.random.default_rng(31)
    ink = (0.55 + 0.45 * fbm(size, size, 31, octaves=5, base=12)) * (rng.random((size, size)) > 0.08)
    arr[..., 3] *= np.clip(ink * 1.15, 0, 1) * 0.92
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA').rotate(
        -12, resample=Image.BICUBIC, expand=True)


def sprites():
    return {
        'Shop_Board': chalkboard(),
        'Shop_ChalkGrain': chalk_grain(),
        'Shop_ChalkRule': chalk_rule(),
        'Shop_ChalkCircle': chalk_circle(),
        'Shop_ChalkArrow': chalk_arrow(),
        'Shop_ChalkMoreUp': chalk_more(True),
        'Shop_ChalkMoreDown': chalk_more(False),
        'Shop_PaperChip': paper_chip(),
        'Shop_Receipt': receipt_paper(),
        'Shop_Twine': twine(),
        'Shop_Purse': leather_purse(),
        'Shop_MoneyTag': money_tag(),
        'Shop_PaidStamp': paid_stamp(),
        'Shop_QtyLeft': ink_arrow(True),
        'Shop_QtyRight': ink_arrow(False),
    }


def emit():
    print(f'emit -> {EMIT_DIR}')
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    for name, image in sprites().items():
        cs.write_sprite(EMIT_DIR, name, image)


# ---------------------------------------------------------------- 完成イメージ (プレハブと同じ部品・同じ座標)
SAMPLE_ITEMS = [
    dict(name='回復薬グレート', icon='Icon_Potion', price=60, owned=3, max=10,
         desc=['体力を大きく回復する。', '狩りの合間にひと口。']),
    dict(name='こんがり肉', icon='Icon_Meat', price=25, owned=1, max=10,
         desc=['スタミナを回復する。', '焼きたての香ばしい肉。']),
    dict(name='大タル爆弾G', icon='Icon_Bomb', price=120, owned=0, max=5,
         desc=['置いて起爆する大きな樽。']),
]


def place(base, part, center, scale=1.0):
    if scale != 1.0:
        part = part.resize((round(part.width * scale), round(part.height * scale)), Image.LANCZOS)
    cs.paste(base, part, center[0] - part.width / 2, center[1] - part.height / 2)


def fade(im, blend_rate):
    arr = np.asarray(im, np.float32)
    arr[..., 3] *= blend_rate / 255
    return Image.fromarray(arr.astype(np.uint8), 'RGBA')


def draw_text(im, pos, px, s, color, path=BODY_FONT, align='left'):
    anchor = {'left': 'la', 'right': 'ra', 'center': 'ma'}[align]
    cs.text(im, pos, s, px, (*color, 255), path, anchor=anchor)


def add(a, b):
    return a[0] + b[0], a[1] + b[1]


def preview(base, selected=0, quantity=2, wallet=1240, paid=False):
    L = LAYOUT
    sp = sprites()
    (vc, _, veil) = L['veil']
    arr = np.asarray(base.convert('RGB'), np.float32) * (1 - veil / 255)
    im = Image.fromarray(arr.astype(np.uint8), 'RGB').convert('RGBA')

    place(im, sp['Shop_Board'], L['board'])
    title_pos, title_px = L['title']
    draw_text(im, title_pos, title_px, 'よろず屋　本日の品', CHALK, BRUSH_FONT, 'center')
    place(im, sp['Shop_ChalkRule'], L['title_rule'])

    origin = L['rows_origin']
    for i, it in enumerate(SAMPLE_ITEMS):
        row = add(origin, (0, i * ROW_PITCH))
        if i == selected:
            place(im, sp['Shop_ChalkCircle'], add(row, L['row_circle']))
            place(im, sp['Shop_ChalkArrow'], add(row, L['row_arrow']))
        pos, px = L['row_name']
        draw_text(im, add(row, pos), px, it['name'], CHALK, BRUSH_FONT)
        pos, px = L['row_owned']
        draw_text(im, add(row, pos), px, f"手持ち {it['owned']}/{it['max']}", CHALK_DIM)
        pos, px = L['row_price']
        affordable = it['price'] <= wallet
        draw_text(im, add(row, pos), px, f"{it['price']:,} G", CHALK_YELLOW if affordable else CHALK_RED,
                  BRUSH_FONT, 'right')
    if len(SAMPLE_ITEMS) < MAX_ROWS:
        pos, px = L['restock']
        draw_text(im, add(add(origin, (0, len(SAMPLE_ITEMS) * ROW_PITCH)), pos), px, '(入荷待ち)', CHALK_DIM,
                  BRUSH_FONT)
    place(im, sp['Shop_ChalkGrain'], L['grain'])
    for i, it in enumerate(SAMPLE_ITEMS):
        row = add(origin, (0, i * ROW_PITCH))
        place(im, sp['Shop_PaperChip'], add(row, L['row_chip']))
        pos, scale = L['row_icon']
        place(im, Image.open(ITEM_ICON_DIR / f"{it['icon']}.png").convert('RGBA'), add(row, pos), scale)

    rc = L['receipt']
    place(im, sp['Shop_Twine'], L['twine'])
    place(im, sp['Shop_Receipt'], rc)
    it = SAMPLE_ITEMS[selected]
    pos, scale = L['rc_icon']
    place(im, Image.open(ITEM_ICON_DIR / f"{it['icon']}.png").convert('RGBA'), add(rc, pos), scale)
    pos, px = L['rc_name']
    draw_text(im, add(rc, pos), px, it['name'], INK, BRUSH_FONT)
    pos, px = L['rc_owned']
    draw_text(im, add(rc, pos), px, f"手持ち {it['owned']} / {it['max']}", INK_FADE)
    pos, px, step = L['rc_desc']
    for k, line in enumerate(it['desc'][:L['rc_desc_lines']]):
        draw_text(im, add(rc, (pos[0], pos[1] + k * step)), px, line, INK)
    pos, px = L['rc_unit_price']
    draw_text(im, add(rc, pos), px, f"{it['price']:,} G", INK, align='right')
    left = sp['Shop_QtyLeft'] if quantity > 1 else fade(sp['Shop_QtyLeft'], L['rc_mark_inactive'])
    place(im, left, add(rc, L['rc_decrease']))
    pos, px = L['rc_quantity']
    draw_text(im, add(rc, pos), px, str(quantity), INK, BRUSH_FONT, 'center')
    place(im, sp['Shop_QtyRight'], add(rc, L['rc_increase']))
    total = it['price'] * quantity
    pos, px = L['rc_total']
    draw_text(im, add(rc, pos), px, f'{total:,} G', STAMP_RED, BRUSH_FONT, 'right')
    pos, px = L['rc_after']
    draw_text(im, add(rc, pos), px, f'支払い後 {wallet - total:,} G', INK_FADE, align='right')
    if paid:
        place(im, sp['Shop_PaidStamp'], add(rc, L['rc_stamp']))

    place(im, sp['Shop_Purse'], L['purse'])
    pos, px = L['money_label']
    draw_text(im, pos, px, '所持金', HINT_COLOR)
    place(im, sp['Shop_MoneyTag'], L['money_tag'])
    pos, px = L['money_text']
    draw_text(im, pos, px, f'{wallet:,} G', INK, BRUSH_FONT, 'center')

    for item in hint_layout():
        if item['kind'] == 'tag':
            place(im, Image.open(HINT_SPRITES[item['sprite']]).convert('RGBA'), item['pos'])
        else:
            draw_text(im, item['pos'], LAYOUT['hint_px'], item['text'], HINT_COLOR)
    return im


# ---------------------------------------------------------------- 露店の3Dモデル用テクスチャ
def sign_texture():
    """露店の前に立てる A 型看板の黒板面。UIの黒板と同じ見た目に揃える"""
    w, h = 512, 640
    im = slate(w, h, 41)
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    d.text((w / 2, 96), 'よろず屋', font=cs.font(BRUSH_FONT, 92), fill=(*CHALK, 255), anchor='mm')
    d.line([(70, 168), (w - 70, 174)], fill=(*CHALK, 255), width=5)
    for i, (name, price) in enumerate([('回復薬', '60'), ('こんがり肉', '25'), ('大タル爆弾', '120')]):
        y = 250 + i * 110
        d.text((56, y), name, font=cs.font(BRUSH_FONT, 52), fill=(*CHALK, 255), anchor='lm')
        d.text((w - 56, y), f'{price} G', font=cs.font(BRUSH_FONT, 52), fill=(*CHALK_YELLOW, 255), anchor='rm')
    d.ellipse([34, 190, w - 34, 310], outline=(250, 236, 180, 255), width=6)
    arr = np.asarray(lay, np.float32) / 255.0
    rng = np.random.default_rng(42)
    arr[..., 3] *= (0.45 + 0.55 * fbm(w, h, 42, octaves=6, base=70)) * (rng.random((h, w)) > 0.16) * 0.92
    im.alpha_composite(Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA'))
    return im.convert('RGB')


def canopy_texture():
    """布屋根の縞。生成りと臙脂を交互に、布目と色あせを焼く"""
    size = 512
    xx, yy = grid(size, size)
    stripe = ((xx // 64) % 2).astype(np.float32)
    cream = np.array([0.86, 0.80, 0.66], np.float32)
    crimson = np.array([0.52, 0.12, 0.12], np.float32)
    rgb = cream[None, None, :] * (1 - stripe)[..., None] + crimson[None, None, :] * stripe[..., None]
    weave = 0.92 + 0.08 * (np.sin(xx * 1.9) * np.sin(yy * 1.9))
    fade = 0.86 + 0.18 * fbm(size, size, 51, octaves=4, base=3)
    dirt = 1 - 0.25 * np.clip(yy / size - 0.6, 0, 1)
    rgb = rgb * (weave * fade * dirt)[..., None]
    return Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8), 'RGB')


def emit_model_textures():
    MODEL_SOURCE_DIR.mkdir(parents=True, exist_ok=True)
    for name, image in (('MerchantStall_Sign', sign_texture()), ('MerchantStall_Canopy', canopy_texture())):
        path = MODEL_SOURCE_DIR / f'{name}.png'
        image.save(path)
        print(f'wrote {path.relative_to(REPO_ROOT)}  {image.size[0]}x{image.size[1]}')


GEOMETRY = f"""
GEOMETRY (1920x1080 / スプライトは中心、文字は左上。プレハブは tools/art/shop_prefab.py がこの LAYOUT で組む)
  Board        中心 {LAYOUT['board']}  {BOARD_SPRITE[0]}x{BOARD_SPRITE[1]} (黒板 {BOARD_SIZE[0]}x{BOARD_SIZE[1]} の左上 {BOARD_ORIGIN})
  Rows         原点 {LAYOUT['rows_origin']}  行送り {ROW_PITCH}・表示 {MAX_ROWS} 行
  Receipt      中心 {RECEIPT_CENTER}  {RECEIPT_SIZE[0]}x{RECEIPT_SIZE[1]}
  Purse        中心 {LAYOUT['purse']} / MoneyTag 中心 {LAYOUT['money_tag']}
  Hints        右端 x={LAYOUT['hints_right']}, y={LAYOUT['hints_y']}
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot', default='', help='拠点の実画面 (1920x1080 に合わせる)')
    ap.add_argument('--out-dir', default='')
    ap.add_argument('--emit', action='store_true', help='スプライトを Assets/Art/UI/Shop へ書き出す')
    ap.add_argument('--emit-model-textures', action='store_true', help='露店の3Dモデル用テクスチャを _Source へ')
    args = ap.parse_args()

    if args.emit:
        emit()
        print(GEOMETRY)
    if args.emit_model_textures:
        emit_model_textures()
    if args.shot:
        if not args.out_dir:
            raise SystemExit('--shot needs --out-dir')
        out = Path(args.out_dir)
        out.mkdir(parents=True, exist_ok=True)
        base = cs.load_base(args.shot)
        for key, kwargs in (('shop_decided', {}), ('shop_decided_paid', dict(paid=True)),
                            ('shop_decided_meat', dict(selected=1, quantity=1))):
            path = out / f'{key}.png'
            preview(base.copy(), **kwargs).convert('RGB').save(path)
            print(f'wrote {path}')


if __name__ == '__main__':
    main()
