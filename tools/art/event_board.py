"""Generate the event-board (イベント掲示板) UI design mocks composited on a real game screen.

    python tools/art/event_board.py --shot <screenshot.png> --out-dir <dir> [--banner-shot <grassland.png>]

拠点の掲示板を調べたときに出る画面の完成イメージを 1920x1080 で出す。
v1 は「今後のイベント」の一覧だけ(mock_decided)。v2 は木札の見出しで 依頼 / 催し / お知らせ の3頁を切り替える(mock_v2、
配置は V2、部品は v2_sprites)。--emit は v2 で使う部品を書き出す。
素材(羊皮紙・木・釘・蝋・真鍮)は酒場のキャラ選択 (tools/art/character_select.py) と同じものを使い、
汎用のフラットな角丸パネルに寄せない。

エンジンの TextRenderer は文字を回転できないので、文字は必ず水平に置く(傾けるのは紙と飾りだけ)。
行は EventBoardRow.prefab を縦に並べ、右(または左)に選択中の詳細を出す、という
EventBoardUi の構造に収まる案だけを作る。

Requires Pillow + numpy.
"""
import argparse
import datetime
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageOps

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

from tools.art import character_select as cs  # noqa: E402

SCREEN_W, SCREEN_H = cs.SCREEN_W, cs.SCREEN_H
BODY_FONT, BRUSH_FONT = cs.BODY_FONT, cs.BRUSH_FONT
INK, INK_FADE, STAMP_RED = cs.INK, cs.INK_FADE, cs.STAMP_RED
WEEKDAYS = '月火水木金土日'


def when(y, mo, d, h, mi):
    t = datetime.datetime(y, mo, d, h, mi)
    return f'{t.month}/{t.day}({WEEKDAYS[t.weekday()]}) {t.hour}:{t.minute:02d}'


# 同梱のサンプル告知 (Assets/Data/EventNotice) と同じ中身。3件目以降はモックで行数を見せるための仮
ENTRIES = [
    dict(title='草原の群狼 討伐週間', tag='討伐イベント', ongoing=True, status='開催中・残り6日',
         period=f"{when(2026, 9, 18, 12, 0)} 〜 {when(2026, 9, 25, 4, 59)}",
         lines=['草原地帯のハイエナが群れで現れる。', '討伐数に応じて酒場から報酬が出る。'], banner='grass'),
    dict(title='嵐を呼ぶ竜', tag='期間限定クエスト', ongoing=False, status='あと13日で開始',
         period=f"{when(2026, 10, 1, 18, 0)} 〜 {when(2026, 10, 8, 4, 59)}",
         lines=['嵐の夜にだけ姿を見せる竜が島へ飛来する。', '腕に覚えのある者は備えておくこと。'], banner='storm'),
    dict(title='鍛冶屋の大売り出し', tag='街の催し', ongoing=False, status='あと20日で開始',
         period=f"{when(2026, 10, 8, 12, 0)} 〜 {when(2026, 10, 15, 4, 59)}",
         lines=['鍛冶屋の武具が期間中だけ値引きされる。'], banner='tavern'),
    dict(title='星降りの夜市', tag='季節の催し', ongoing=False, status='あと27日で開始',
         period=f"{when(2026, 10, 15, 18, 0)} 〜 {when(2026, 10, 22, 4, 59)}",
         lines=['浮島の広場に夜だけの市が立つ。'], banner='night'),
]


# ---------------------------------------------------------------- バナー(仮)
def banner_art(kind, size, base, grass_shot):
    """本番のバナーはイベントごとに描き起こす。モックでは実画面の切り抜きを色味だけ変えて仮置きする"""
    w, h = size
    if kind == 'grass' and grass_shot is not None:
        src = grass_shot.crop((0, 420, 1374, 1020))
    elif kind == 'tavern':
        src = base.crop((0, 150, 960, 569))
    else:
        src = base.crop((700, 0, 1700, 437))
    img = ImageOps.fit(src.convert('RGB'), (w, h), Image.LANCZOS)
    arr = np.asarray(img, np.float32) / 255.0
    xx, yy = cs.grid(w, h)

    if kind == 'storm':
        lum = arr.mean(-1, keepdims=True)
        arr = lum * 0.55 + arr * 0.15
        arr = arr * np.array([0.72, 0.76, 1.0], np.float32)
        arr = arr * (0.55 + 0.45 * cs.fbm(w, h, 61, octaves=4, base=3))[..., None]
        bolt = Image.new('L', (w, h), 0)
        d = ImageDraw.Draw(bolt)
        x, y = w * 0.72, 0.0
        pts = [(x, y)]
        rng = np.random.default_rng(3)
        while y < h * 0.78:
            y += h * 0.09
            x += rng.uniform(-w * 0.035, w * 0.035)
            pts.append((x, y))
        d.line(pts, fill=255, width=3)
        glow = np.asarray(bolt.filter(ImageFilter.GaussianBlur(9)), np.float32) / 255.0
        core = np.asarray(bolt, np.float32) / 255.0
        arr = arr + np.array([0.75, 0.82, 1.0], np.float32) * (glow * 1.6 + core)[..., None]
        rain = Image.new('L', (w, h), 0)
        rd = ImageDraw.Draw(rain)
        for _ in range(w * h // 1500):
            rx, ry, length = rng.uniform(0, w), rng.uniform(0, h), rng.uniform(14, 34)
            rd.line([(rx, ry), (rx - length * 0.25, ry + length)], fill=int(rng.uniform(50, 130)), width=1)
        rain = np.asarray(rain.filter(ImageFilter.GaussianBlur(0.5)), np.float32) / 255.0
        arr = arr + 0.40 * rain[..., None]
    elif kind == 'night':
        lum = arr.mean(-1, keepdims=True)
        arr = lum * np.array([0.30, 0.26, 0.55], np.float32)
        stars = (np.random.default_rng(9).random((h, w)) > 0.9975).astype(np.float32)
        arr = arr + np.asarray(Image.fromarray((stars * 255).astype(np.uint8), 'L')
                               .filter(ImageFilter.GaussianBlur(0.8)), np.float32)[..., None] / 255.0 * 2.2
    elif kind == 'tavern':
        arr = arr * np.array([1.08, 0.94, 0.74], np.float32)
    else:
        arr = arr * np.array([1.06, 1.0, 0.86], np.float32)

    vignette = 1 - 0.55 * np.clip(np.hypot((xx - w / 2) / (w / 2), (yy - h / 2) / (h / 2)) - 0.55, 0, 1)
    arr = arr * vignette[..., None]
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')


def framed(art, border, seed, brass=False):
    """絵を木(または真鍮)の額に入れる。内側に落ち影を入れて奥まって見せる"""
    w, h = art.width + border * 2, art.height + border * 2
    if brass:
        xx, yy = cs.grid(w, h)
        d = cs.rect_outside(xx, yy, w, h, 1.0)
        inner = -cs.rect_outside(xx, yy, w, h, border)
        mask = cs.soften(np.clip(0.5 - d, 0, 1), 0.6)
        b = cs.bevel(np.clip(inner, 0, 1) * mask, 2.5)
        patina = 0.8 + 0.4 * cs.fbm(w, h, seed, octaves=5, base=9)
        rgb = cs.BRASS[None, None, :] * ((1.1 + 1.6 * b) * patina)[..., None]
        frame = cs.rgba(rgb, mask)
    else:
        frame = cs.wood_board(w, h, seed, plank=9999)
        ImageDraw.Draw(frame).rectangle([3, 3, w - 4, h - 4], outline=(40, 26, 14, 255), width=3)
    frame.alpha_composite(art, (border, border))
    shade = np.zeros((h, w), np.float32)
    shade[border:border + 10, border:w - border] = np.linspace(0.55, 0, 10)[:, None]
    shade[border:h - border, border:border + 8] = np.maximum(
        shade[border:h - border, border:border + 8], np.linspace(0.45, 0, 8)[None, :])
    frame.alpha_composite(cs.rgba(np.zeros((h, w, 3), np.float32), shade))
    return frame


# ---------------------------------------------------------------- 共通の部品
def stamp(label, px=34, angle=-9, color=STAMP_RED):
    return cs.stamp_sprite(label, px=px, angle=angle, color=color)


def hints(base, right_x, y):
    x = right_x
    for glyph, label in reversed([('▲▼', '選ぶ'), ('B', '閉じる')]):
        cs.text(base, (x, y), label, 26, (240, 226, 202, 255), BODY_FONT, anchor='rm',
                shadow=(2, 2, (0, 0, 0, 210)))
        x -= cs.font(BODY_FONT, 26).getlength(label) + 16
        gw = 68 if len(glyph) > 1 else 46
        cs.paste(base, cs.hint_tag(gw, 42, glyph), x - gw, y - 21)
        x -= gw + 30


def arrow_mark(up, size=30, color=(222, 196, 150)):
    lay = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    pts = [(size / 2, 4), (size - 4, size - 6), (4, size - 6)] if up else \
          [(4, 6), (size - 4, 6), (size / 2, size - 4)]
    d.polygon(pts, fill=(*color, 235))
    return lay


def place(base, part, cx, cy):
    cs.paste(base, part, cx - part.width / 2, cy - part.height / 2)


# ---------------------------------------------------------------- 案A 掲示板の貼り紙
A_SLIP = (560, 150)


def slip_a(e, selected, seed):
    w, h = A_SLIP
    p = cs.parchment(w, h, seed, aged=0.04 if selected else 0.20, ragged=10.0)
    cs.text(p, (34, 22), e['title'], 38, INK, BRUSH_FONT)
    cs.text(p, (36, 76), e['tag'], 18, INK_FADE, BODY_FONT)
    cs.text(p, (36, 124), e['status'], 21, STAMP_RED if e['ongoing'] else INK, BODY_FONT, anchor='ls')
    if e['ongoing']:
        place(p, stamp('開 催 中', px=30), w - 92, h - 42)
    return p


def poster_a(e, art_fn):
    w, h = 720, 820
    p = cs.parchment(w, h, 410, aged=0.16, ragged=9.0)
    cs.paste(p, framed(art_fn(e['banner'], (612, 272)), 14, 420), 40, 36)
    cs.text(p, (w / 2, 388), e['title'], 52, INK, BRUSH_FONT, anchor='mm')
    cs.text(p, (w / 2, 438), e['tag'], 21, INK_FADE, BODY_FONT, anchor='mm')
    d = ImageDraw.Draw(p)
    d.line([(64, 470), (w - 64, 470)], fill=INK_FADE, width=2)
    cs.text(p, (70, 510), '期 間', 20, INK_FADE, BODY_FONT, anchor='lm')
    cs.text(p, (150, 510), e['period'], 24, INK, BODY_FONT, anchor='lm')
    cs.text(p, (70, 552), '状 況', 20, INK_FADE, BODY_FONT, anchor='lm')
    cs.text(p, (150, 552), e['status'], 24, STAMP_RED if e['ongoing'] else INK, BODY_FONT, anchor='lm')
    d.line([(64, 588), (w - 64, 588)], fill=(160, 136, 104), width=1)
    for i, line in enumerate(e['lines']):
        cs.text(p, (70, 632 + i * 44), line, 25, INK, BODY_FONT, anchor='lm')
    if e['ongoing']:
        place(p, stamp('開 催 中', px=40, angle=-10), w - 140, h - 86)
    return p


def mock_a(base, art_fn, selected=0):
    im = cs.tavern_grade(base, 0.64, [(400, 590, 660, 1.0), (1400, 570, 620, 0.85)])
    cs.paste(im, cs.hanging_rope(300, 80), 810, 8)
    place(im, cs.tavern_sign(600, 106, '催 し の お 知 ら せ'), 960, 108)

    board = cs.wood_board(660, 800, 131)
    cs.paste(im, cs.drop_shadow(board), 70 - 30, 190 - 30)
    cs.paste(im, board, 70, 190)
    for i, e in enumerate(ENTRIES):
        sel = i == selected
        slip = slip_a(e, sel, 500 + i * 13)
        if sel:
            slip = slip.resize((int(slip.width * 1.06), int(slip.height * 1.06)), Image.LANCZOS)
        cy = 318 + i * 168
        cs.paste(im, cs.drop_shadow(slip, 8, (4, 6), 0.5), 400 - slip.width / 2 - 24, cy - slip.height / 2 - 24)
        place(im, slip, 400, cy)
        cs.paste(im, cs.nail(20), 400 - slip.width * 0.40, cy - slip.height / 2 + 6)
        cs.paste(im, cs.nail(20), 400 + slip.width * 0.36, cy - slip.height / 2 + 4)
        if sel:
            place(im, cs.wax_seal(64, 505), 400 - slip.width / 2 - 14, cy)
    place(im, arrow_mark(False), 400, 952)

    poster = poster_a(ENTRIES[selected], art_fn)
    cs.paste_tilted(im, poster, 1400, 572, 0.0)
    cs.paste(im, cs.nail(24), 1400 - 12, 572 - 410 + 4)
    hints(im, 1856, 1040)
    return im


# ---------------------------------------------------------------- 案B お触れ書きの巻物
B_ROW = (540, 136)


def rod(w, h=34, seed=151):
    xx, yy = cs.grid(w, h)
    body = np.clip(1 - np.abs(yy - h / 2) / (h / 2), 0, 1)
    cap = np.clip(np.minimum(xx, w - 1 - xx) / 10, 0, 1)
    mask = cs.soften((body > 0).astype(np.float32) * cap, 0.6)
    tone = (0.35 + 0.9 * np.sqrt(body)) * (0.8 + 0.35 * cs.fbm(w, h, seed, octaves=4, base=20))
    rgb = np.array([0.30, 0.18, 0.10], np.float32)[None, None, :] * tone[..., None] * 1.6
    return cs.rgba(rgb, mask)


def seal_dot(size, lit, seed):
    s = cs.wax_seal(size, seed)
    if lit:
        return s
    arr = np.asarray(s, np.float32) / 255.0
    lum = arr[..., :3].mean(-1, keepdims=True)
    arr[..., :3] = lum * np.array([0.9, 0.85, 0.8], np.float32) * 1.3
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA')


def brush_swath(w, h, seed=171):
    xx, yy = cs.grid(w, h)
    n = cs.fbm(w, h, seed, octaves=5, base=6)
    edge = np.abs(yy - h / 2) / (h / 2) + (n - 0.5) * 0.7
    fade = np.clip(xx / (w * 0.08), 0, 1) * np.clip((w - xx) / (w * 0.22), 0, 1)
    a = np.clip(1 - edge, 0, 1) ** 0.6 * fade * (0.55 + 0.45 * cs.fbm(w, h, seed + 1, octaves=6, base=30))
    rgb = np.array([0.62, 0.14, 0.10], np.float32)[None, None, :] * np.ones((h, w, 1), np.float32)
    return cs.rgba(rgb, a * 0.38)


def row_b(e, selected, seed):
    w, h = B_ROW
    p = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    if selected:
        cs.paste(p, brush_swath(w - 20, h - 28), 10, 14)
    place(p, seal_dot(58, e['ongoing'], seed), 44, h / 2)
    cs.text(p, (92, 30), e['title'], 34, INK, BRUSH_FONT)
    cs.text(p, (94, 84), e['tag'] + '   ' + e['status'], 20, STAMP_RED if e['ongoing'] else INK_FADE, BODY_FONT)
    d = ImageDraw.Draw(p)
    for x in range(30, w - 30, 14):
        d.line([(x, h - 4), (x + 6, h - 4)], fill=(150, 124, 92, 200), width=2)
    return p


def mock_b(base, art_fn, selected=1):
    im = cs.tavern_grade(base, 0.66, [(380, 560, 640, 1.0), (1330, 560, 700, 0.8)])

    scroll = cs.parchment(600, 820, 610, aged=0.22, ragged=6.0, curl=False)
    cs.paste(im, cs.drop_shadow(scroll), 80 - 30, 155 - 30)
    cs.paste(im, scroll, 80, 155)
    place(im, rod(680), 380, 150)
    place(im, rod(680, seed=152), 380, 978)
    cs.text(im, (380, 222), 'お 触 れ 書 き', 50, (*INK, 255), BRUSH_FONT, anchor='mm')
    d = ImageDraw.Draw(im)
    d.line([(170, 262), (590, 262)], fill=(*INK_FADE, 255), width=2)
    for i, e in enumerate(ENTRIES):
        place(im, row_b(e, i == selected, 620 + i * 7), 380, 350 + i * 150)
    place(im, arrow_mark(False, color=(96, 70, 44)), 380, 940)

    e = ENTRIES[selected]
    pic = framed(art_fn(e['banner'], (780, 330)), 20, 700, brass=True)
    cs.paste(im, cs.drop_shadow(pic), 1330 - pic.width / 2 - 30, 318 - pic.height / 2 - 30)
    place(im, pic, 1330, 318)
    plate = cs.iron_plate(620, 78)
    place(im, plate, 1330, 540)
    cs.engrave(im, (1330, 540), e['title'], 40, BRUSH_FONT, anchor='mm')

    card = cs.parchment(760, 360, 720, aged=0.12, ragged=8.0)
    cs.text(card, (48, 54), e['tag'], 22, INK_FADE, BODY_FONT, anchor='lm')
    cs.text(card, (760 - 48, 54), e['status'], 26, STAMP_RED if e['ongoing'] else INK, BODY_FONT, anchor='rm')
    ImageDraw.Draw(card).line([(44, 88), (716, 88)], fill=INK_FADE, width=2)
    cs.text(card, (48, 128), '期 間', 20, INK_FADE, BODY_FONT, anchor='lm')
    cs.text(card, (130, 128), e['period'], 25, INK, BODY_FONT, anchor='lm')
    for i, line in enumerate(e['lines']):
        cs.text(card, (48, 196 + i * 46), line, 26, INK, BODY_FONT, anchor='lm')
    cs.paste(im, cs.drop_shadow(card, 8, (4, 6), 0.5), 1330 - 380 - 24, 800 - 180 - 24)
    place(im, card, 1330, 800)
    hints(im, 1856, 1040)
    return im


# ---------------------------------------------------------------- 案C 依頼板(バナー主役)
C_TICKET = (500, 120)


def ticket_c(e, selected, seed):
    w, h = C_TICKET
    p = cs.parchment(w, h, seed, aged=0.02 if selected else 0.24, ragged=8.0)
    cs.text(p, (30, 20), e['title'], 32, INK, BRUSH_FONT)
    cs.text(p, (32, h - 24), e['status'], 19, STAMP_RED if e['ongoing'] else INK_FADE, BODY_FONT, anchor='ls')
    if e['ongoing']:
        place(p, stamp('開 催 中', px=26), w - 84, h - 36)
    return p


def mock_c(base, art_fn, selected=0):
    im = cs.tavern_grade(base, 0.62, [(640, 560, 820, 1.0), (1520, 560, 520, 0.8)])
    e = ENTRIES[selected]

    poster = cs.parchment(1060, 880, 810, aged=0.14, ragged=9.0)
    cs.paste(poster, framed(art_fn(e['banner'], (964, 400)), 12, 820), 36, 34)
    cs.text(poster, (60, 530), e['title'], 66, INK, BRUSH_FONT, anchor='lm')
    cs.text(poster, (64, 594), e['tag'], 22, INK_FADE, BODY_FONT, anchor='lm')
    cs.text(poster, (1000, 594), e['status'], 28, STAMP_RED if e['ongoing'] else INK, BODY_FONT, anchor='rm')
    d = ImageDraw.Draw(poster)
    d.line([(56, 628), (1004, 628)], fill=INK_FADE, width=2)
    cs.text(poster, (60, 672), '期 間   ' + e['period'], 25, INK, BODY_FONT, anchor='lm')
    for i, line in enumerate(e['lines']):
        cs.text(poster, (60, 740 + i * 46), line, 27, INK, BODY_FONT, anchor='lm')
    if e['ongoing']:
        place(poster, stamp('開 催 中', px=50, angle=-10), 880, 520)
    cs.paste(im, cs.drop_shadow(poster), 110 - 30, 120 - 30)
    cs.paste(im, poster, 110, 120)
    cs.paste(im, cs.nail(24), 110 + 530 - 12, 128)

    strip = cs.wood_board(560, 820, 171)
    cs.paste(im, cs.drop_shadow(strip), 1240 - 30, 170 - 30)
    cs.paste(im, strip, 1240, 170)
    cs.paste(im, cs.hanging_rope(260, 70), 1390, 40)
    place(im, cs.tavern_sign(440, 92, '依 頼 と 催 し'), 1520, 120)
    for i, en in enumerate(ENTRIES):
        sel = i == selected
        t = ticket_c(en, sel, 850 + i * 11)
        if sel:
            t = t.resize((int(t.width * 1.05), int(t.height * 1.05)), Image.LANCZOS)
        cy = 290 + i * 150
        cs.paste(im, cs.drop_shadow(t, 7, (4, 5), 0.5), 1520 - t.width / 2 - 21, cy - t.height / 2 - 21)
        place(im, t, 1520, cy)
        cs.paste(im, cs.nail(18), 1520 - 9, cy - t.height / 2 + 2)
        if sel:
            place(im, cs.wax_seal(58, 853), 1520 - t.width / 2 - 12, cy)
    place(im, arrow_mark(False), 1520, 900)
    hints(im, 1856, 1040)
    return im


# ---------------------------------------------------------------- 決定案: 案C を左右反転(一覧が左、バナーのポスターが右)
# 1920x1080。画像は中心ピボット、文字は TextRenderer の基準(左寄せ=左上 / 右寄せ=右上 / 中央=上辺の中央)。
# event_board_prefab.py がこの値でプレハブを組むので、モック(mock_decided)もこの値だけで描く。
EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'EventBoard'
BANNER_DIR = EMIT_DIR / 'Banner'
HINT_CANCEL_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Cancel.png'
BANNER_SIZE = (964, 400)
TICKET_SIZE = (500, 120)

LAYOUT = {
    'veil_scale': 0.47,            # BlackMask.png (4096x2894) で画面を覆う倍率
    'veil_blend': 130,
    # 左: 札の一覧
    'rope': (400, 75),
    'sign': (400, 120),
    'strip': (400, 580),
    'rows_origin': (400, 290),
    'row_spacing': 150,
    'max_rows': 4,
    'arrow_up': (400, 200),
    'arrow_down': (400, 900),
    # 行 (EventBoardRow.prefab / 札の中心が原点)
    'row_title': ((-220, -40), 32),
    'row_status': ((-218, 19), 19),
    'row_stamp': (166, 24),
    'row_seal': (-262, 0),
    'row_nail': (0, -49),
    'row_selected_scale': 1.05,
    # 右: 選択中の告知
    'poster': (1270, 560),
    'poster_nail': (1270, 140),
    'banner': (1270, 366),
    'title': ((800, 617), 66),
    'tag': ((804, 703), 22),
    'status': ((1740, 700), 28),
    'rule': (1270, 748),
    'period_label': ((800, 780), 25),
    'period': ((890, 780), 25),
    'desc': ((800, 847), 27, 46, 3),   # 左上, px, 行間, 行数
    'stamp': (1620, 640),
    'empty': ((1270, 540), 30),
    # 操作ガイド
    'hints_right': 1856,
    'hints_y': 1040,
    'hint_px': 26,
}
HINT_COLOR = (240, 226, 202)


def hollow_frame(w, h, border, seed):
    frame = cs.wood_board(w, h, seed, plank=9999)
    ImageDraw.Draw(frame).rectangle([3, 3, w - 4, h - 4], outline=(40, 26, 14, 255), width=3)
    arr = np.asarray(frame).copy()
    arr[border:h - border, border:w - border, 3] = 0
    return Image.fromarray(arr, 'RGBA')


def banner_shade(w, h):
    """バナーの上に重ねる額の内側の落ち影と周辺減光"""
    xx, yy = cs.grid(w, h)
    a = np.zeros((h, w), np.float32)
    a[:10, :] = np.linspace(0.55, 0, 10)[:, None]
    a[:, :8] = np.maximum(a[:, :8], np.linspace(0.45, 0, 8)[None, :])
    a = np.maximum(a, 0.5 * np.clip(np.hypot((xx - w / 2) / (w / 2), (yy - h / 2) / (h / 2)) - 0.7, 0, 1))
    return cs.rgba(np.zeros((h, w, 3), np.float32), a)


def rule_line(w=948, h=3):
    return cs.rgba(np.array([c / 255 for c in INK_FADE], np.float32)[None, None, :] * np.ones((h, w, 1), np.float32),
                   np.ones((h, w), np.float32))


def decided_sprites():
    tw, th = TICKET_SIZE
    return {
        'Ticket_Unselected': cs.parchment(tw, th, 850, aged=0.24, ragged=8.0),
        'Ticket_Selected':   cs.parchment(tw, th, 861, aged=0.02, ragged=8.0),
        'OngoingStamp_Small': stamp('開 催 中', px=26),
        'OngoingStamp':      stamp('開 催 中', px=50, angle=-10),
        'WaxSeal':           cs.wax_seal(58, 853),
        'Nail_Small':        cs.nail(18),
        'Nail':              cs.nail(24),
        'Strip':             cs.wood_board(560, 820, 171),
        'Sign':              cs.tavern_sign(440, 92, '依 頼 と 催 し'),
        'SignRope':          cs.hanging_rope(260, 70),
        'Poster':            cs.parchment(1060, 880, 810, aged=0.14, ragged=9.0),
        'BannerFrame':       hollow_frame(BANNER_SIZE[0] + 24, BANNER_SIZE[1] + 24, 12, 820),
        'BannerShade':       banner_shade(*BANNER_SIZE),
        'Rule':              rule_line(),
        'Arrow_Up':          arrow_mark(True),
        'Arrow_Down':        arrow_mark(False),
        'HintTag_UpDown':    cs.hint_tag(68, 42, '▲▼'),
    }


def hint_layout():
    """右端から「閉じる」「B」「選ぶ」「▲▼」の順に詰める。文字は左上、札は中心の座標で返す"""
    L = LAYOUT
    x, y, px = L['hints_right'], L['hints_y'], L['hint_px']
    items = []
    for name, glyph_w, label in [('cancel', 46, '閉じる'), ('move', 68, '選ぶ')]:
        text_w = cs.font(BODY_FONT, px).getlength(label)
        items.append(dict(kind='text', name=name, text=label, pos=(round(x - text_w), y - px // 2)))
        x -= text_w + 16
        items.append(dict(kind='tag', name=name, pos=(round(x - glyph_w / 2), y)))
        x -= glyph_w + 30
    return items


def draw_text(im, pos, px, s, color, path=BODY_FONT, align='left'):
    anchor = {'left': 'la', 'right': 'ra', 'center': 'ma'}[align]
    cs.text(im, pos, s, px, (*color, 255), path, anchor=anchor)


def mock_decided(base, art_fn, selected=0):
    """プレハブと同じ部品・同じ座標で描いた完成イメージ"""
    L = LAYOUT
    sp = decided_sprites()
    arr = np.asarray(base.convert('RGB'), np.float32) * (1 - L['veil_blend'] / 255)
    im = Image.fromarray(arr.astype(np.uint8), 'RGB').convert('RGBA')

    place(im, sp['Strip'], *L['strip'])
    place(im, sp['SignRope'], *L['rope'])
    place(im, sp['Sign'], *L['sign'])
    ox, oy = L['rows_origin']
    for i, e in enumerate(ENTRIES[:L['max_rows']]):
        sel = i == selected
        k = L['row_selected_scale'] if sel else 1.0
        cx, cy = ox, oy + i * L['row_spacing']
        ticket = sp['Ticket_Selected' if sel else 'Ticket_Unselected']
        if sel:
            ticket = ticket.resize((round(ticket.width * k), round(ticket.height * k)), Image.LANCZOS)
        place(im, ticket, cx, cy)
        place(im, sp['Nail_Small'], cx + L['row_nail'][0] * k, cy + L['row_nail'][1] * k)
        (tx, ty), tpx = L['row_title']
        draw_text(im, (cx + tx * k, cy + ty * k), tpx * k, e['title'], INK, BRUSH_FONT)
        (sx, sy), spx = L['row_status']
        draw_text(im, (cx + sx * k, cy + sy * k), spx * k, e['status'], STAMP_RED if e['ongoing'] else INK_FADE)
        if e['ongoing']:
            place(im, sp['OngoingStamp_Small'], cx + L['row_stamp'][0] * k, cy + L['row_stamp'][1] * k)
        if sel:
            place(im, sp['WaxSeal'], cx + L['row_seal'][0] * k, cy + L['row_seal'][1] * k)
    if len(ENTRIES) > L['max_rows']:
        place(im, sp['Arrow_Down'], *L['arrow_down'])

    e = ENTRIES[selected]
    place(im, sp['Poster'], *L['poster'])
    place(im, sp['Nail'], *L['poster_nail'])
    place(im, art_fn(e['banner'], BANNER_SIZE), *L['banner'])
    place(im, sp['BannerShade'], *L['banner'])
    place(im, sp['BannerFrame'], *L['banner'])
    draw_text(im, L['title'][0], L['title'][1], e['title'], INK, BRUSH_FONT)
    draw_text(im, L['tag'][0], L['tag'][1], e['tag'], INK_FADE)
    draw_text(im, L['status'][0], L['status'][1], e['status'], STAMP_RED if e['ongoing'] else INK, align='right')
    place(im, sp['Rule'], *L['rule'])
    draw_text(im, L['period_label'][0], L['period_label'][1], '期 間', INK_FADE)
    draw_text(im, L['period'][0], L['period'][1], e['period'], INK)
    (dx, dy), dpx, gap, _ = L['desc']
    for i, line in enumerate(e['lines']):
        draw_text(im, (dx, dy + i * gap), dpx, line, INK)
    if e['ongoing']:
        place(im, sp['OngoingStamp'], *L['stamp'])

    hint_cancel = Image.open(HINT_CANCEL_SPRITE).convert('RGBA')
    for item in hint_layout():
        if item['kind'] == 'text':
            draw_text(im, item['pos'], L['hint_px'], item['text'], HINT_COLOR)
        else:
            place(im, sp['HintTag_UpDown'] if item['name'] == 'move' else hint_cancel, *item['pos'])
    return im


def write_sprite(out_dir, name, image):
    cs.write_sprite(out_dir, name, image)


# v1 の吊り看板は v2 で木札の見出しに替わったので書き出さない(モックの mock_decided だけが使う)
V1_ONLY_SPRITES = {'Sign', 'SignRope'}


def emit():
    print(f'emit -> {EMIT_DIR}')
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    sprites = {name: image for name, image in decided_sprites().items() if name not in V1_ONLY_SPRITES}
    sprites.update(v2_sprites())
    for name, image in sprites.items():
        write_sprite(EMIT_DIR, name, image)


def emit_banners(art_fn):
    """同梱のサンプル告知の仮バナー。本番のイベントでは描き起こした 964x400 の絵に差し替える"""
    BANNER_DIR.mkdir(parents=True, exist_ok=True)
    print(f'emit banners -> {BANNER_DIR}')
    write_sprite(BANNER_DIR, 'HyenaHuntWeek', art_fn('grass', BANNER_SIZE))
    write_sprite(BANNER_DIR, 'StormDragonRaid', art_fn('storm', BANNER_SIZE))


# ---------------------------------------------------------------- 看板の3Dモデル用テクスチャ
# モデル本体は Blender で組む(Assets/Art/Models/Prop/EventBoard/_Source/EventNoticeBoard.fbx)。
# 貼り紙はUIと同じ羊皮紙・朱の判・蝋の封で描き、木目もUIの板と同じ作り方にする。
MODEL_SOURCE_DIR = REPO_ROOT / 'Assets' / 'Art' / 'Models' / 'Prop' / 'EventBoard' / '_Source'
PAPER_BG = (196, 176, 136)


def opaque(im, bg):
    out = Image.new('RGBA', im.size, (*bg, 255))
    out.alpha_composite(im)
    return out.convert('RGB')


def ink_scribbles(im, box, rows, seed, color=INK, width=5):
    """読めない程度の走り書き。遠目に文字が並んで見えればよい"""
    rng = np.random.default_rng(seed)
    x0, y0, x1, y1 = box
    d = ImageDraw.Draw(im)
    gap = (y1 - y0) / rows
    for r in range(rows):
        y = y0 + gap * (r + 0.5)
        x = x0
        end = x1 - rng.uniform(0, (x1 - x0) * 0.35)
        while x < end:
            seg = rng.uniform(18, 64)
            pts = [(x + t, y + rng.uniform(-3, 3)) for t in np.linspace(0, seg, 5)]
            d.line(pts, fill=(*color, 230), width=width)
            x += seg + rng.uniform(10, 22)


def model_notices_atlas():
    """1024x1024 を4分割: 左上=告知 / 右上=朱の催し告知 / 左下=竜の手配 / 右下=小さな覚え書き"""
    atlas = Image.new('RGB', (1024, 1024))

    q0 = cs.parchment(512, 512, 910, aged=0.10, ragged=0.0, curl=True)
    cs.text(q0, (256, 92), '告 知', 96, (*INK, 255), BRUSH_FONT, anchor='mm')
    ink_scribbles(q0, (64, 170, 448, 420), 6, 911)
    place(q0, stamp('催', px=64, angle=-8), 400, 440)
    atlas.paste(opaque(q0, PAPER_BG), (0, 0))

    xx, yy = cs.grid(512, 512)
    tone = 0.80 + 0.30 * cs.fbm(512, 512, 920, octaves=5, base=8)
    red = cs.rgba(np.array([0.52, 0.14, 0.10], np.float32)[None, None, :] * tone[..., None],
                  np.ones((512, 512), np.float32))
    ImageDraw.Draw(red).rectangle([22, 22, 489, 489], outline=(222, 190, 120, 255), width=6)
    cs.text(red, (256, 236), '催', 300, (238, 214, 160, 255), BRUSH_FONT, anchor='mm')
    ink_scribbles(red, (80, 400, 432, 470), 2, 921, color=(238, 214, 160))
    atlas.paste(red.convert('RGB'), (512, 0))

    q2 = cs.parchment(512, 512, 930, aged=0.35, ragged=0.0, curl=True)
    cs.text(q2, (256, 190), '竜', 250, (*INK, 255), BRUSH_FONT, anchor='mm')
    ink_scribbles(q2, (70, 350, 442, 470), 3, 931)
    atlas.paste(opaque(q2, PAPER_BG), (0, 512))

    q3 = cs.parchment(512, 512, 940, aged=0.55, ragged=0.0, curl=True)
    ink_scribbles(q3, (60, 70, 452, 380), 6, 941, color=INK_FADE)
    cs.paste(q3, cs.wax_seal(110, 942), 360, 370)
    atlas.paste(opaque(q3, PAPER_BG), (512, 512))
    return atlas


def model_wood(dark):
    wood = opaque(cs.wood_board(512, 512, 960, plank=128), (70, 46, 26))
    if dark:
        arr = np.asarray(wood, np.float32) * 0.62
        wood = Image.fromarray(arr.astype(np.uint8), 'RGB')
    return wood


def emit_model_textures():
    MODEL_SOURCE_DIR.mkdir(parents=True, exist_ok=True)
    for name, image in [('EventNoticeBoard_Wood', model_wood(False)),
                        ('EventNoticeBoard_WoodDark', model_wood(True)),
                        ('EventNoticeBoard_Notices', model_notices_atlas())]:
        path = MODEL_SOURCE_DIR / f'{name}.png'
        image.save(path)
        print(f'  {path.relative_to(REPO_ROOT)}  {image.size[0]}x{image.size[1]}')


# ---------------------------------------------------------------- v2: 依頼 / 催し / お知らせ の3頁 (案A 木札の見出し)
# 左の板と右のポスターはそのままに、吊り看板を木札の見出し3枚に替える。頁は prefab ごとに分け、
# 頁の root は画面の原点に置くので、頁の中の座標もそのまま画面座標で書く。
# 依頼書と便箋の見出し・項目名・罫線は紙に焼き、値だけを TextRenderer にする。
V2_KIND_COLOR = {
    'Important': STAMP_RED,
    'Update': (46, 86, 58),
    'Bug': (54, 60, 112),
    'Event': (150, 84, 26),
    'Guide': INK_FADE,
}
V2_KIND_LABEL = {'Important': '重要', 'Update': '更新', 'Bug': '不具合', 'Event': '催し', 'Guide': '案内'}
V2_KINDS = ['Important', 'Update', 'Bug', 'Event', 'Guide']   # AnnouncementKind の順
V2_EVENT_COLOR = V2_KIND_COLOR['Event']
PIP_FILLED_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'Pip_Filled.png'
PIP_EMPTY_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'Pip_Empty.png'
HINT_CONFIRM_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Confirm.png'

POSTER_ORIGIN = (740, 120)   # 1060x880 のポスターの左上。頁の中の右側はこれからの相対で決める
QUEST_PHOTO_SIZE = (400, 240)
QUEST_PHOTO_BORDER = 10
TAB_SIZE = (168, 76)


def on_poster(x, y):
    return POSTER_ORIGIN[0] + x, POSTER_ORIGIN[1] + y


V2 = {
    'strip': (400, 598),
    'strip_size': (560, 800),
    'tab_rope': (400, 73),
    'tabs_root': (222, 124),
    'tab_spacing': 178,
    'tab_labels': ['依 頼', '催 し', 'お知らせ'],
    'tab_hint_lb': (86, 128),
    'tab_hint_rb': (714, 128),
    # 木札 (EventBoardTab.prefab / 板の中心が原点)
    'tab_cords': (0, -46),
    'tab_label': ((0, -17), 32),
    'tab_badge': (76, -30),
    'tab_badge_text': ((0, -11), 22),
    'tab_selected_scale': 1.06,
    'tab_selected_drop': 8,
    'tab_label_selected': (255, 234, 190),
    'tab_label_unselected': (168, 146, 116),
    # 頁の左(3頁共通)
    'rows_origin': (400, 290),
    'row_spacing': 150,
    'max_rows': 4,
    'arrow_up': (400, 212),
    'arrow_down': (400, 908),
    'poster': (1270, 560),
    'poster_nail': (1270, 140),
    'empty': ((1270, 540), 30),
    # 依頼の札 (EventBoardQuestRow.prefab / 札の中心が原点)
    'qrow_title': ((-220, -40), 30),
    'qrow_coin': (112, -22),
    'qrow_reward': ((128, -38), 26),
    'qrow_place': ((-218, 16), 19),
    'qrow_pips': ((-120, 26), 17, 0.5),      # 1つ目の中心, 間隔, 倍率(24px の点を縮める)
    'qrow_event_chip': (-16, 26),
    'qrow_stamp': (166, 26),
    # 依頼書 (ポスターの左上からの相対)
    'q_event_chip': on_poster(83, 96),
    'q_event_text': (on_poster(120, 86), 19),
    'q_photo': on_poster(56 + (QUEST_PHOTO_SIZE[0] + QUEST_PHOTO_BORDER * 2) / 2,
                         120 + (QUEST_PHOTO_SIZE[1] + QUEST_PHOTO_BORDER * 2) / 2),
    'q_title': (on_poster(530, 124), 46),
    'q_client': (on_poster(626, 223), 25),
    'q_place': (on_poster(626, 267), 25),
    'q_pips': (on_poster(638, 324), 24, 0.67),
    'q_state': (on_poster(626, 355), 25),
    'q_goal': (on_poster(156, 449), 30),
    'q_reward': (on_poster(200, 505), 36),
    'q_limit': (on_poster(156, 567), 25),
    'q_desc': (on_poster(62, 647), 25, 42, 3),
    'q_seal': on_poster(932, 762),
    # お知らせの札 (EventBoardNoticeRow.prefab)
    'nrow_chip': (-182, -27),
    'nrow_date': ((224, -36), 19),
    'nrow_title': ((-218, 10), 23),
    'nrow_unread': (236, -50),
    # 便箋 (ポスターの左上からの相対)
    'n_hanko': on_poster(130, 84),
    'n_date': (on_poster(990, 73), 22),
    'n_title': (on_poster(70, 143), 46),
    'n_body': (on_poster(78, 267), 26, 48, 10),
}


def v2_wood_tab(selected):
    w, h = TAB_SIZE
    b = cs.wood_board(w, h, 300 if selected else 307, plank=9999)
    arr = np.asarray(b, np.float32).copy()
    arr[..., :3] = np.clip(arr[..., :3] * (1.28 if selected else 0.62), 0, 255)
    b = Image.fromarray(arr.astype(np.uint8), 'RGBA')
    ImageDraw.Draw(b).rectangle([4, 4, w - 5, h - 5], outline=(214, 170, 92, 245) if selected else (60, 42, 26, 230),
                                width=4 if selected else 3)
    return b


def v2_tab_cords():
    w, h = 104, 20
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    for x in (4, w - 5):
        d.line([(x, 0), (x, h)], fill=(120, 92, 56, 255), width=3)
    return lay


def v2_coin(size, seed=5):
    xx, yy = cs.grid(size, size)
    r = size / 2
    rad = np.hypot(xx - r + 0.5, yy - r + 0.5)
    mask = cs.soften(np.clip(0.5 - (rad - (r - 2)), 0, 1), 0.6)
    b = cs.bevel(mask, 2.0)
    ring = np.clip(1 - np.abs(rad - r * 0.68) / 1.6, 0, 1)
    patina = 0.85 + 0.3 * cs.fbm(size, size, seed, octaves=4, base=6)
    rgb = cs.BRASS[None, None, :] * ((1.25 + 1.4 * b - 0.35 * ring) * patina)[..., None]
    return cs.rgba(rgb, mask)


def v2_chip(label, color, px, width=None, pad=8):
    """枠線だけの小さな札。種類ごとに幅が変わると左端が揃わないので、width で揃えられる"""
    f = cs.font(BODY_FONT, px)
    w = width or int(f.getlength(label)) + pad * 2
    h = px + 12
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    d.rectangle([1, 1, w - 2, h - 2], outline=(*color, 230), width=2)
    d.text((w / 2, h / 2 + 1), label, font=f, fill=(*color, 255), anchor='mm')
    return lay


def v2_hanko(label, color, px=30, width=124):
    """お知らせの種類の角印。朱肉(や墨)で押した四角"""
    h = px + 22
    lay = Image.new('RGBA', (width, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    d.rectangle([2, 2, width - 3, h - 3], outline=(*color, 215), width=4)
    d.text((width / 2, h / 2 + 1), label, font=cs.font(BRUSH_FONT, px), fill=(*color, 225), anchor='mm')
    return lay


def v2_unread_seal(size=40):
    s = cs.wax_seal(size, 77)
    cs.text(s, (size / 2, size / 2 + 1), '新', int(size * 0.5), (250, 226, 196, 255), BRUSH_FONT, anchor='mm')
    return s


def v2_badge(size=40):
    return cs.wax_seal(size, 91)


def v2_seal_ring(size, label, color, filled):
    """受注印の枠。受付中は点線の丸、受けると朱の印に替える"""
    lay = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    if filled:
        d.ellipse([4, 4, size - 5, size - 5], outline=(*color, 220), width=6)
        d.ellipse([16, 16, size - 17, size - 17], outline=(*color, 150), width=2)
        cs.text(lay, (size / 2, size / 2), label, int(size * 0.3), (*color, 255), BRUSH_FONT, anchor='mm')
        return lay.rotate(-12, resample=Image.BICUBIC)
    n = 36
    for i in range(0, n, 2):
        d.arc([4, 4, size - 5, size - 5], i * 360 / n, (i + 1) * 360 / n, fill=(*color, 200), width=3)
    cs.text(lay, (size / 2, size / 2), label, int(size * 0.17), (*color, 255), BODY_FONT, anchor='mm')
    return lay


def v2_quest_poster():
    """依頼書の紙。見出し・項目名・罫線・金貨は焼き、値は TextRenderer が上に書く"""
    w, h = 1060, 880
    p = cs.parchment(w, h, 810, aged=0.14, ragged=9.0)
    d = ImageDraw.Draw(p)
    cs.text(p, (w / 2, 60), '依 頼 書', 36, (*INK, 255), BRUSH_FONT, anchor='mm')
    d.line([(64, 60), (w / 2 - 100, 60)], fill=INK_FADE, width=2)
    d.line([(w / 2 + 100, 60), (w - 64, 60)], fill=INK_FADE, width=2)
    for i, label in enumerate(['依頼主', '場　所', '難　度', '状　況']):
        cs.text(p, (530, 236 + i * 44), label, 21, (*INK_FADE, 255), BODY_FONT, anchor='lm')
    d.line([(56, 420), (w - 56, 420)], fill=INK_FADE, width=2)
    for y, label in [(464, '目 的'), (524, '報 酬'), (580, '期 限')]:
        cs.text(p, (62, y), label, 21, (*INK_FADE, 255), BODY_FONT, anchor='lm')
    place(p, v2_coin(34), 174, 524)
    d.line([(56, 616), (w - 56, 616)], fill=(160, 136, 104), width=1)
    return p


def v2_letter():
    """便箋。罫線と署名は焼き、種類の角印・日時・題名・本文は上に重ねる"""
    w, h = 1060, 880
    ox, oy, lw = 70, 50, 920
    p = cs.parchment(w, h, 812, aged=0.0, ragged=7.0)
    d = ImageDraw.Draw(p)
    d.line([(ox, oy + 170), (ox + lw, oy + 170)], fill=INK_FADE, width=2)
    for y in range(oy + 240, h - 70, 48):
        d.line([(ox, y + 12), (ox + lw, y + 12)], fill=(176, 154, 120), width=1)
    cs.text(p, (ox + lw, h - 50), '— 酒場 運営係', 24, (*INK_FADE, 255), BODY_FONT, anchor='rm')
    return p


def v2_sprites():
    s = {
        'Strip': cs.wood_board(*V2['strip_size'], 171),
        'Tab_Selected': v2_wood_tab(True),
        'Tab_Unselected': v2_wood_tab(False),
        'TabCords': v2_tab_cords(),
        'TabRope': cs.hanging_rope(640, 70),
        'TabBadge': v2_badge(),
        'HintTag_LB': cs.hint_tag(52, 42, 'LB'),
        'HintTag_RB': cs.hint_tag(52, 42, 'RB'),
        'HintTag_LBRB': cs.hint_tag(84, 42, 'LB RB'),
        'Coin_Small': v2_coin(24),
        'EventChip_Small': v2_chip('催し', V2_EVENT_COLOR, 16, pad=6),
        'EventChip': v2_chip('催し', V2_EVENT_COLOR, 19),
        'QuestStamp_Taking': stamp('受 注 中', px=26),
        'QuestStamp_Cleared': stamp('達 成', px=26, color=INK_FADE),
        'QuestStamp_Preparing': stamp('準 備 中', px=24, color=INK_FADE),
        'QuestSeal_Open': v2_seal_ring(150, '受 注 印', INK_FADE, False),
        'QuestSeal_Taking': v2_seal_ring(150, '受 注', STAMP_RED, True),
        'QuestSeal_Cleared': v2_seal_ring(150, '達 成', INK_FADE, True),
        'QuestSeal_Preparing': v2_seal_ring(150, '準 備 中', INK_FADE, False),
        'QuestPoster': v2_quest_poster(),
        'QuestPhotoFrame': hollow_frame(QUEST_PHOTO_SIZE[0] + QUEST_PHOTO_BORDER * 2,
                                        QUEST_PHOTO_SIZE[1] + QUEST_PHOTO_BORDER * 2, QUEST_PHOTO_BORDER, 820),
        'QuestPhotoShade': banner_shade(*QUEST_PHOTO_SIZE),
        'Letter': v2_letter(),
        'UnreadSeal': v2_unread_seal(),
    }
    for kind in V2_KINDS:
        s[f'KindChip_{kind}'] = v2_chip(V2_KIND_LABEL[kind], V2_KIND_COLOR[kind], 18, width=76)
        s[f'KindHanko_{kind}'] = v2_hanko(V2_KIND_LABEL[kind], V2_KIND_COLOR[kind])
    return s


def v2_hint_layout(items):
    """items は右から詰める (札の絵の名前, 札の幅, 文言)。文字は左上、札は中心の座標で返す"""
    L = LAYOUT
    x, y, px = L['hints_right'], L['hints_y'], L['hint_px']
    placed = []
    for sprite, glyph_w, label in reversed(items):
        text_w = cs.font(BODY_FONT, px).getlength(label)
        placed.append(dict(kind='text', text=label, pos=(round(x - text_w), y - px // 2)))
        x -= text_w + 16
        placed.append(dict(kind='tag', sprite=sprite, pos=(round(x - glyph_w / 2), y)))
        x -= glyph_w + 30
    return placed


V2_HINTS_WITH_ACCEPT = [('HintTag_LBRB', 84, '切り替え'), ('HintTag_UpDown', 68, '選ぶ'),
                        ('HintTag_Confirm', 46, '受注する'), ('HintTag_Cancel', 46, '閉じる')]
V2_HINTS_WITHOUT_ACCEPT = [('HintTag_LBRB', 84, '切り替え'), ('HintTag_UpDown', 68, '選ぶ'),
                           ('HintTag_Cancel', 46, '閉じる')]

# 仮データ (モック用。本番は .boardQuest / .announcement)
V2_QUESTS = [
    dict(title='草原のハイエナ退治', place='草原地帯', rank=2, client='酒場の仲介人', goal='ハイエナを 8 頭 討伐する',
         reward='1,200 G', limit='なし', state='open', thumb=REPO_ROOT / 'Assets/Art/UI/StageSelect/thumb_area_grass_wide.png',
         lines=['群れからはぐれたハイエナが街道まで下りてくる。', '荷馬車が襲われる前に数を減らしてほしい。']),
    dict(title='群狼討伐週間 其の一', place='草原地帯', rank=3, client='酒場の仲介人', goal='ハイエナを 20 頭 討伐する',
         reward='2,000 G', limit='9/25(金) 4:59 まで', state='taking', event='草原の群狼 討伐週間',
         thumb=BANNER_DIR / 'HyenaHuntWeek.png', lines=['討伐週間のあいだだけ貼り出される依頼。']),
    dict(title='岩場の暴君', place='岩石地帯', rank=4, client='岩石地帯の見張り', goal='ティラノサウルスを 1 頭 討伐する',
         reward='―', limit='なし', state='preparing', thumb=REPO_ROOT / 'Assets/Art/UI/StageSelect/thumb_area_rocky.png',
         lines=['見張り小屋が二度も踏み潰された。']),
    dict(title='訓練場の案山子', place='拠点', rank=1, client='教官', goal='案山子に 10 回 攻撃を当てる',
         reward='100 G', limit='なし', state='cleared', thumb=None, lines=['まずは体を慣らしておけ。']),
]
V2_NOTICES = [
    dict(title='ver 1.0.1 更新のお知らせ', kind='Update', date='9/18(金)', datetime='9/18(金) 12:00', unread=True,
         lines=['本日、次の内容で更新を行いました。', '', '・拠点に「依頼と催し」の掲示板を設置しました',
                '・草原地帯のハイエナの群れの動きを調整しました', '', '更新データはゲームの起動時に自動で取得されます。']),
    dict(title='9/24 定期メンテナンスのお知らせ', kind='Important', date='9/17(木)', datetime='9/17(木) 18:00',
         unread=True, lines=[]),
    dict(title='「草原の群狼 討伐週間」開催', kind='Event', date='9/16(水)', datetime='9/16(水) 12:00', unread=False,
         lines=[]),
    dict(title='ハイエナが地形にはまる不具合について', kind='Bug', date='9/15(火)', datetime='9/15(火) 20:00', unread=False,
         lines=[]),
]
V2_STATE_TEXT = {'open': '受付中', 'taking': '受注中', 'cleared': '達成済み', 'preparing': '準備中'}
V2_ROW_STAMP = {'taking': 'QuestStamp_Taking', 'cleared': 'QuestStamp_Cleared', 'preparing': 'QuestStamp_Preparing'}
V2_SEAL = {'open': 'QuestSeal_Open', 'taking': 'QuestSeal_Taking', 'cleared': 'QuestSeal_Cleared',
           'preparing': 'QuestSeal_Preparing'}


def _pip_sprites():
    return (Image.open(PIP_FILLED_SPRITE).convert('RGBA'), Image.open(PIP_EMPTY_SPRITE).convert('RGBA'))


def _scaled(im, k):
    return im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.LANCZOS)


def _draw_pips(im, center0, gap, k, rank):
    filled, empty = _pip_sprites()
    for i in range(5):
        place(im, _scaled(filled if i < rank else empty, k), center0[0] + i * gap, center0[1])


def mock_v2(base, art_fn, tab=0, selected=0):
    """v2 のプレハブと同じ部品・同じ座標で描いた完成イメージ"""
    L, V = LAYOUT, V2
    sp = {**decided_sprites(), **v2_sprites()}
    sp['HintTag_Cancel'] = Image.open(HINT_CANCEL_SPRITE).convert('RGBA')
    sp['HintTag_Confirm'] = Image.open(HINT_CONFIRM_SPRITE).convert('RGBA')
    arr = np.asarray(base.convert('RGB'), np.float32) * (1 - L['veil_blend'] / 255)
    im = Image.fromarray(arr.astype(np.uint8), 'RGB').convert('RGBA')

    place(im, sp['Strip'], *V['strip'])
    # 木札の見出し
    tx, ty = V['tabs_root']
    for i, label in enumerate(V['tab_labels']):
        sel = i == tab
        k = V['tab_selected_scale'] if sel else 1.0
        cx, cy = tx + i * V['tab_spacing'], ty + (V['tab_selected_drop'] if sel else 0)
        place(im, _scaled(sp['TabCords'], k), cx + V['tab_cords'][0] * k, cy + V['tab_cords'][1] * k)
    place(im, sp['TabRope'], *V['tab_rope'])
    for i, label in enumerate(V['tab_labels']):
        sel = i == tab
        k = V['tab_selected_scale'] if sel else 1.0
        cx, cy = tx + i * V['tab_spacing'], ty + (V['tab_selected_drop'] if sel else 0)
        place(im, _scaled(sp['Tab_Selected' if sel else 'Tab_Unselected'], k), cx, cy)
        (lx, ly), lpx = V['tab_label']
        color = V['tab_label_selected'] if sel else V['tab_label_unselected']
        draw_text(im, (cx + lx * k, cy + ly * k), round(lpx * k), label, color, BRUSH_FONT, align='center')
        if i == 2:
            unread = sum(n['unread'] for n in V2_NOTICES)
            bx, by = cx + V['tab_badge'][0] * k, cy + V['tab_badge'][1] * k
            place(im, sp['TabBadge'], bx, by)
            (btx, bty), bpx = V['tab_badge_text']
            draw_text(im, (bx + btx, by + bty), bpx, str(unread), (252, 236, 210), align='center')
    place(im, sp['HintTag_LB'], *V['tab_hint_lb'])
    place(im, sp['HintTag_RB'], *V['tab_hint_rb'])

    ox, oy = V['rows_origin']
    rows = V2_QUESTS if tab == 0 else V2_NOTICES if tab == 2 else ENTRIES
    for i, e in enumerate(rows[:V['max_rows']]):
        sel = i == selected
        k = L['row_selected_scale'] if sel else 1.0
        cx, cy = ox, oy + i * V['row_spacing']

        def at(offset):
            return cx + offset[0] * k, cy + offset[1] * k

        place(im, _scaled(sp['Ticket_Selected' if sel else 'Ticket_Unselected'], k), cx, cy)
        place(im, sp['Nail_Small'], *at(L['row_nail']))
        if tab == 0:
            draw_text(im, at(V['qrow_title'][0]), round(V['qrow_title'][1] * k), e['title'], INK, BRUSH_FONT)
            place(im, _scaled(sp['Coin_Small'], k), *at(V['qrow_coin']))
            draw_text(im, at(V['qrow_reward'][0]), round(V['qrow_reward'][1] * k), e['reward'], INK, BRUSH_FONT)
            draw_text(im, at(V['qrow_place'][0]), round(V['qrow_place'][1] * k), e['place'], INK_FADE)
            (px0, py0), gap, pk = V['qrow_pips']
            _draw_pips(im, at((px0, py0)), gap * k, pk * k, e['rank'])
            if e.get('event'):
                place(im, _scaled(sp['EventChip_Small'], k), *at(V['qrow_event_chip']))
            if e['state'] in V2_ROW_STAMP:
                place(im, _scaled(sp[V2_ROW_STAMP[e['state']]], k), *at(V['qrow_stamp']))
        elif tab == 2:
            place(im, _scaled(sp[f"KindChip_{e['kind']}"], k), *at(V['nrow_chip']))
            draw_text(im, at(V['nrow_date'][0]), round(V['nrow_date'][1] * k), e['date'], INK_FADE, align='right')
            draw_text(im, at(V['nrow_title'][0]), round(V['nrow_title'][1] * k), e['title'], INK)
            if e['unread']:
                place(im, sp['UnreadSeal'], *at(V['nrow_unread']))
        else:
            draw_text(im, at(L['row_title'][0]), round(L['row_title'][1] * k), e['title'], INK, BRUSH_FONT)
            draw_text(im, at(L['row_status'][0]), round(L['row_status'][1] * k), e['status'],
                      STAMP_RED if e['ongoing'] else INK_FADE)
            if e['ongoing']:
                place(im, _scaled(sp['OngoingStamp_Small'], k), *at(L['row_stamp']))
        if sel:
            place(im, sp['WaxSeal'], *at(L['row_seal']))
    if len(rows) > V['max_rows']:
        place(im, sp['Arrow_Down'], *V['arrow_down'])

    e = rows[selected]
    if tab == 0:
        place(im, sp['QuestPoster'], *V['poster'])
        if e.get('event'):
            place(im, sp['EventChip'], *V['q_event_chip'])
            draw_text(im, V['q_event_text'][0], V['q_event_text'][1], e['event'], V2_EVENT_COLOR)
        if e['thumb'] is not None:
            photo = ImageOps.fit(Image.open(e['thumb']).convert('RGB'), QUEST_PHOTO_SIZE, Image.LANCZOS).convert('RGBA')
            place(im, photo, *V['q_photo'])
            place(im, sp['QuestPhotoShade'], *V['q_photo'])
            place(im, sp['QuestPhotoFrame'], *V['q_photo'])
        draw_text(im, V['q_title'][0], V['q_title'][1], e['title'], INK, BRUSH_FONT)
        draw_text(im, V['q_client'][0], V['q_client'][1], e['client'], INK)
        draw_text(im, V['q_place'][0], V['q_place'][1], e['place'], INK)
        (qx, qy), gap, pk = V['q_pips']
        _draw_pips(im, (qx, qy), gap, pk, e['rank'])
        draw_text(im, V['q_state'][0], V['q_state'][1], V2_STATE_TEXT[e['state']],
                  STAMP_RED if e['state'] == 'taking' else INK)
        draw_text(im, V['q_goal'][0], V['q_goal'][1], e['goal'], INK)
        draw_text(im, V['q_reward'][0], V['q_reward'][1], e['reward'], INK, BRUSH_FONT)
        draw_text(im, V['q_limit'][0], V['q_limit'][1], e['limit'], INK)
        (dx, dy), dpx, gap, _ = V['q_desc']
        for i, line in enumerate(e['lines']):
            draw_text(im, (dx, dy + i * gap), dpx, line, INK)
        place(im, sp[V2_SEAL[e['state']]], *V['q_seal'])
    elif tab == 2:
        place(im, sp['Letter'], *V['poster'])
        place(im, sp[f"KindHanko_{e['kind']}"], *V['n_hanko'])
        draw_text(im, V['n_date'][0], V['n_date'][1], e['datetime'], INK_FADE, align='right')
        draw_text(im, V['n_title'][0], V['n_title'][1], e['title'], INK, BRUSH_FONT)
        (bx, by), bpx, gap, _ = V['n_body']
        for i, line in enumerate(e['lines']):
            draw_text(im, (bx, by + i * gap), bpx, line, INK)
    else:
        place(im, sp['Poster'], *L['poster'])
        place(im, art_fn(e['banner'], BANNER_SIZE), *L['banner'])
        place(im, sp['BannerShade'], *L['banner'])
        place(im, sp['BannerFrame'], *L['banner'])
        draw_text(im, L['title'][0], L['title'][1], e['title'], INK, BRUSH_FONT)
        draw_text(im, L['tag'][0], L['tag'][1], e['tag'], INK_FADE)
        draw_text(im, L['status'][0], L['status'][1], e['status'], STAMP_RED if e['ongoing'] else INK, align='right')
        place(im, sp['Rule'], *L['rule'])
        draw_text(im, L['period_label'][0], L['period_label'][1], '期 間', INK_FADE)
        draw_text(im, L['period'][0], L['period'][1], e['period'], INK)
        (dx, dy), dpx, gap, _ = L['desc']
        for i, line in enumerate(e['lines']):
            draw_text(im, (dx, dy + i * gap), dpx, line, INK)
        if e['ongoing']:
            place(im, sp['OngoingStamp'], *L['stamp'])
    place(im, sp['Nail'], *V['poster_nail'])

    can_accept = tab == 0 and e['state'] == 'open'
    for item in v2_hint_layout(V2_HINTS_WITH_ACCEPT if can_accept else V2_HINTS_WITHOUT_ACCEPT):
        if item['kind'] == 'text':
            draw_text(im, item['pos'], L['hint_px'], item['text'], HINT_COLOR)
        else:
            place(im, sp[item['sprite']], *item['pos'])
    return im


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot', default='', help='拠点の実画面 (1920x1080 に合わせる)')
    ap.add_argument('--banner-shot', default='', help='仮バナーに使う草原の実画面')
    ap.add_argument('--out-dir', default='')
    ap.add_argument('--emit', action='store_true', help='決定案のスプライトを Assets/Art/UI/EventBoard へ書き出す')
    ap.add_argument('--emit-banners', action='store_true', help='サンプル告知の仮バナーを書き出す (--shot / --banner-shot が要る)')
    ap.add_argument('--emit-model-textures', action='store_true', help='看板の3Dモデル用テクスチャを _Source へ書き出す')
    args = ap.parse_args()

    if args.emit_model_textures:
        emit_model_textures()
        return

    if args.emit:
        emit()
        if not args.emit_banners:
            return

    base = cs.load_base(args.shot)
    grass = cs.load_base(args.banner_shot) if args.banner_shot else None

    def art_fn(kind, size):
        return banner_art(kind, size, base, grass)

    if args.emit_banners:
        emit_banners(art_fn)
        return

    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    plans = [
        ('A_board', '案A  掲示板の貼り紙', mock_a),
        ('B_scroll', '案B  お触れ書きの巻物', mock_b),
        ('C_poster', '案C  依頼板(バナー主役)', mock_c),
        ('D_decided', '決定  案C を左右反転', mock_decided),
        ('E_v2_quest', 'v2  依頼', lambda b, f: mock_v2(b, f, tab=0)),
        ('F_v2_event', 'v2  催し', lambda b, f: mock_v2(b, f, tab=1)),
        ('G_v2_notice', 'v2  お知らせ', lambda b, f: mock_v2(b, f, tab=2)),
    ]
    mocks = []
    for key, label, fn in plans:
        im = fn(base.copy(), art_fn)
        path = out / f'eventboard_{key}.png'
        im.convert('RGB').save(path)
        mocks.append(im)
        print(f'wrote {path}')
    sheet = out / 'eventboard_compare.png'
    cs.contact_sheet(mocks, [label for _, label, _ in plans]).convert('RGB').save(sheet)
    print(f'wrote {sheet}')

if __name__ == '__main__':
    main()
