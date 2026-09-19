"""＠キーで開くメニュー「冒険者の手帳」のスプライト書き出しと、実画面に合成したデザイン案。

    python tools/art/pause_menu.py --emit                                   # Assets/Art/UI/PauseMenu へ書き出す
    python tools/art/pause_menu.py --emit --preview --shot <png> --out-dir <dir>   # 書き出し + 配置の確認
    python tools/art/pause_menu.py --shot <screenshot.png> --out-dir <dir>  # 3案のモック

2026-09-18 に3案(手帳 / 真鍮の輪 / 鉄の見出し)を実画面に合成して比べ、案A「冒険者の手帳」に決まった。
項目は ステータス / 持ち物 / クエスト / 操作方法 / タイトルへ / ゲームへ戻る の6つ。

質感の作り方は character_select.py から流用する(羊皮紙・木・鉄・真鍮・蝋)。
平らな角丸パネルに細い発光枠、という既製品っぽい見た目は避ける。
prefab は tools/art/pause_menu_prefab.py が layout() を読んで組む。

Requires Pillow + numpy.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))

from character_select import (  # noqa: E402
    SCREEN_W, SCREEN_H, REPO_ROOT, BODY_FONT, BRUSH_FONT,
    INK, INK_FADE, STAMP_RED, PARCH, PARCH_OLD, WAX, BRASS, STEEL, CANDLE,
    fbm, rgba, grid, rect_outside, soften, bevel, drop_shadow,
    parchment, wood_board, nail, wax_seal, brass_ring,
    font, text, engrave, ink_pips, paste, paste_tilted,
)

UI_ART = REPO_ROOT / 'Assets' / 'Art' / 'UI'
LEATHER = np.array([0.23, 0.13, 0.09], np.float32)

# ---------------------------------------------------------------- メニューの中身
ENTRIES = [
    ('ステータス', '体の具合と稼ぎを確かめる'),
    ('持ち物', '袋の中身を並べ替える'),
    ('クエスト', '受けた頼みごとを読み返す'),
    ('操作方法', '手綱の握り方を思い出す'),
    ('タイトルへ', '冒険を切り上げる'),
    ('ゲームへ戻る', 'そのまま続ける'),
]
GLYPHS = ['体', '袋', '巻', '手', '終', '戻']
SELECTED = 0

STATS = [('腕っぷし', 4), ('しぶとさ', 4), ('身軽さ', 3)]
ITEMS = [
    ('回復薬グレート', 3, 'Icon_Potion.png'),
    ('こんがり肉', 1, 'Icon_Meat.png'),
    ('大タル爆弾G', 2, 'Icon_Bomb.png'),
]


# ---------------------------------------------------------------- 実画面の下ごしらえ
def donor(im, box, src, flip=False, feather=26, jitter=0.05, seed=1, match=True):
    """箱の中身を、同じ画面の別の場所(主に綺麗な空)で置き換える。
    塗り潰しやぼかしだと平らになるので、元画像の雲や地形をそのまま持ってくる"""
    x0, y0, x1, y1 = box
    w, h = x1 - x0, y1 - y0
    part = im.crop(src).convert('RGB')
    if part.size != (w, h):
        part = part.resize((w, h), Image.LANCZOS)
    if flip:
        part = part.transpose(Image.FLIP_LEFT_RIGHT)
    a = np.asarray(part, np.float32) / 255.0
    if match:
        # 空は下へ行くほど青く沈むので、持ってきたまま貼ると四角い色ムラになる。
        # 箱のすぐ外側の色に合わせて、貼る側の明るさを掛け算で寄せる
        full = np.asarray(im.convert('RGB'), np.float32) / 255.0
        pad, out = 26, []
        if y0 - pad >= 0:
            out.append(full[y0 - pad:y0, x0:x1])
        if y1 + pad <= SCREEN_H:
            out.append(full[y1:y1 + pad, x0:x1])
        if x0 - pad >= 0:
            out.append(full[y0:y1, x0 - pad:x0])
        if x1 + pad <= SCREEN_W:
            out.append(full[y0:y1, x1:x1 + pad])
        want = np.concatenate([o.reshape(-1, 3) for o in out]).mean(0)
        have = a.reshape(-1, 3).mean(0)
        a = a * np.clip(want / np.maximum(have, 1e-3), 0.82, 1.22)[None, None, :]
    if jitter:
        a = a * (1 - jitter / 2 + jitter * fbm(w, h, seed, octaves=4, base=3)[..., None])
    part = Image.fromarray((np.clip(a, 0, 1) * 255).astype(np.uint8), 'RGB')

    f = max(2, min(feather, w // 3, h // 3))
    m = np.ones((h, w), np.float32)
    if y0 > 0:
        m[:f, :] *= np.linspace(0, 1, f)[:, None]
    if y1 < SCREEN_H:
        m[-f:, :] *= np.linspace(1, 0, f)[:, None]
    if x0 > 0:
        m[:, :f] *= np.linspace(0, 1, f)[None, :]
    if x1 < SCREEN_W:
        m[:, -f:] *= np.linspace(1, 0, f)[None, :]
    im.paste(part, (x0, y0), Image.fromarray((m * 255).astype(np.uint8), 'L'))


def wipe_text(im, box, radius=9, threshold=0.10, grow=3.0):
    """細い文字だけを消す。中央値フィルタは細い線を飲み込むので、それと元画像の差が
    大きい所=文字、とみなして、そこだけ差し替える。後ろの地形はぼけずに残る"""
    x0, y0, x1, y1 = box
    src = im.crop(box).convert('RGB')
    med = src.filter(ImageFilter.MedianFilter(radius))
    a = np.asarray(src, np.float32) / 255.0
    b = np.asarray(med, np.float32) / 255.0
    diff = np.abs(a - b).max(-1)
    m = soften(np.clip((diff - threshold) * 14, 0, 1), grow)
    m = np.clip(m * 2.2, 0, 1)
    im.paste(med, (x0, y0), Image.fromarray((m * 255).astype(np.uint8), 'L'))


def band_heal(im, box, pad=7, seed=3):
    """細い横帯(Console/Project のタイトルバー)専用。帯の上下の数行を縦に混ぜる。
    列ごとに色が変わるので、山の上に渡っていても継ぎ目が出にくい"""
    x0, y0, x1, y1 = box
    arr = np.asarray(im.convert('RGB'), np.float32) / 255.0
    w, h = x1 - x0, y1 - y0
    top = arr[y0 - pad:y0, x0:x1].mean(0)
    bot = arr[y1:y1 + pad, x0:x1].mean(0)
    t = np.linspace(0, 1, h, dtype=np.float32)[:, None, None]
    patch = (top[None] * (1 - t) + bot[None] * t)
    patch = patch * (0.94 + 0.12 * fbm(w, h, seed, octaves=5, base=9)[..., None])
    part = Image.fromarray((np.clip(patch, 0, 1) * 255).astype(np.uint8), 'RGB')
    m = np.ones((h, w), np.float32)
    m[:3, :] *= np.linspace(0.2, 1, 3)[:, None]
    m[-3:, :] *= np.linspace(1, 0.2, 3)[:, None]
    if x0 > 0:
        m[:, :20] *= np.linspace(0, 1, 20)[None, :]
    im.paste(part, (x0, y0), Image.fromarray((m * 255).astype(np.uint8), 'L'))


def clean_shot(base):
    """エディタのウィンドウとゲームHUDを消して、素のゲーム画面に近づける。
    メニューを開いている間はHUDを隠す前提なので、HUDも実際に消して見せる。
    消す順番に意味がある(先に消した所を、次の貼り元として使う)"""
    im = base.copy()
    donor(im, (572, 58, 704, 174), (376, 58, 508, 174), feather=22, seed=4)    # 上の矢印ギズモ
    donor(im, (450, 202, 714, 362), (150, 202, 414, 362), feather=26, seed=5)  # 座標軸ギズモ
    donor(im, (1232, 18, 1504, 256), (1516, 54, 1788, 292), feather=24, seed=9)          # GameWindow
    donor(im, (0, 18, 726, 264), (1352, 62, 1908, 250), flip=True, feather=26, seed=6)   # 体力HUD
    donor(im, (760, 16, 1168, 214), (1488, 58, 1896, 256), feather=24, seed=7)           # ボスの紋章
    donor(im, (782, 204, 1012, 308), (296, 204, 526, 308), feather=22, seed=8)           # ボス名(空の上)
    for r in (15, 11, 7):                             # ボス名(山の上)は文字だけ潰す
        wipe_text(im, (992, 200, 1152, 306), radius=r)
    wipe_text(im, (884, 524, 1008, 588), radius=11)   # 敵の小さな体力バー
    donor(im, (0, 0, 1920, 64), (0, 138, 1920, 202), feather=20, seed=10)                # 上端の帯
    band_heal(im, (976, 324, 1920, 356), seed=11)     # Console
    band_heal(im, (1434, 700, 1920, 734), seed=12)    # Project
    donor(im, (0, 484, 344, 600), (344, 484, 688, 600), flip=True, feather=24, seed=13)  # 操作ガイド
    donor(im, (920, 172, 1004, 228), (700, 172, 784, 228), feather=14, seed=14)          # 小さな三角ギズモ
    return smooth_sky(im)


def smooth_sky(im, radius=11, amount=0.75):
    """貼り替えの継ぎ目が空に四角く残るので、空だと判る所だけぼかして馴染ませる。
    雲はもともと柔らかいので、ぼかしても違和感が出ない(山や人物には掛けない)"""
    arr = np.asarray(im.convert('RGB'), np.float32) / 255.0
    sky = (arr[..., 2] > arr[..., 0] * 1.01) & (arr[..., 2] > 0.5)
    yy = np.mgrid[0:SCREEN_H, 0:SCREEN_W][0].astype(np.float32)
    m = soften(sky.astype(np.float32) * np.clip((420 - yy) / 90, 0, 1), 9.0) * amount
    blurred = im.convert('RGB').filter(ImageFilter.GaussianBlur(radius))
    out = im.copy()
    out.paste(blurred, (0, 0), Image.fromarray((np.clip(m, 0, 1) * 255).astype(np.uint8), 'L'))
    return out


# ---------------------------------------------------------------- 背景の落とし方
def dim(base, darken=0.62, blur=6.0, vignette=0.55, warm=0.0):
    im = base.convert('RGB').filter(ImageFilter.GaussianBlur(blur))
    arr = np.asarray(im, np.float32) / 255.0
    xx, yy = grid(SCREEN_W, SCREEN_H)
    r = np.hypot((xx - SCREEN_W / 2) / (SCREEN_W / 2), (yy - SCREEN_H / 2) / (SCREEN_H / 2))
    vig = np.clip(r / 1.25, 0, 1) ** 1.7
    arr = arr * (1 - darken) * (1 - vignette * vig)[..., None]
    if warm:
        arr = arr + CANDLE[None, None, :] * warm * (1 - vig)[..., None] * 0.14
    arr = arr * np.array([1.05, 0.99, 0.92], np.float32)[None, None, :]
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')


# ---------------------------------------------------------------- 共通の部品
def leather(w, h, seed=41, inset=6.0):
    """革表紙。粒の粗い皺と、縁の擦れ"""
    xx, yy = grid(w, h)
    d = rect_outside(xx, yy, w, h, inset)
    mask = soften(np.clip(0.5 - d, 0, 1), 1.0)
    grain = 0.72 + 0.46 * fbm(w, h, seed, octaves=6, base=16)
    crease = 1 - 0.22 * np.clip(fbm(w, h, seed + 3, octaves=3, base=2) * 2.1 - 1.0, 0, 1)
    b = bevel(mask, 5.0)
    wear = 1 + 0.55 * np.clip(1 - np.abs(d + 9) / 9, 0, 1)
    rgb = LEATHER[None, None, :] * (grain * crease * (1 + 1.1 * b) * wear)[..., None] * 1.5
    return rgba(rgb, mask)


def iron_tab(w, h, seed=21, selected=False, notch=18):
    """釘で打った鉄の見出し札。右端だけ斜めに切って、旗のように見せる"""
    xx, yy = grid(w, h)
    d = np.maximum(rect_outside(xx, yy, w, h, 3.0),
                   (xx - (w - 1 - notch)) - (h / 2 - np.abs(yy - h / 2)))
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)
    b = bevel(mask, 3.5)
    hammer = 0.78 + 0.44 * fbm(w, h, seed, octaves=5, base=12)
    tone = STEEL * (1.30 if selected else 1.0)
    rgb = tone[None, None, :] * (hammer * (0.80 + 1.70 * b))[..., None] * 2.1
    rim = np.clip(1 - np.abs(d + 3.0) / 2.6, 0, 1)
    rgb = rgb + BRASS[None, None, :] * (rim * (1.55 if selected else 0.62))[..., None]
    return rgba(rgb, mask)


def medallion(size, glyph, selected, seed=61):
    """鉄の円盤に漢字を彫った記章。選択中は真鍮が効いて大きく見える"""
    xx, yy = grid(size, size)
    r = size / 2
    rad = np.hypot(xx - r + 0.5, yy - r + 0.5)
    d = rad - (r - 3)
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)
    b = bevel(mask, 3.0)
    hammer = 0.76 + 0.48 * fbm(size, size, seed, octaves=5, base=9)
    dish = 1 - 0.34 * np.clip(1 - rad / (r * 0.72), 0, 1)
    tone = STEEL * (1.42 if selected else 1.0)
    rgb = tone[None, None, :] * (hammer * dish * (0.78 + 1.85 * b))[..., None] * 2.1
    im = rgba(rgb, mask)

    ring = brass_ring(size, max(7, size // 11), rivets=10 if selected else 8, seed=seed + 2)
    if not selected:
        a = np.asarray(ring.split()[-1], np.float32) / 255.0 * 0.62
        ring.putalpha(Image.fromarray((a * 255).astype(np.uint8), 'L'))
    im.alpha_composite(ring)
    engrave(im, (size / 2, size / 2 + 2), glyph, int(size * 0.50), BRUSH_FONT, anchor='mm',
            tone=(238, 206, 142) if selected else (198, 192, 182))
    return im


def hint_bar(base, x_right, y, pairs):
    """画面の隅の操作ヒント。キーは羊皮紙の小片、説明はその右に白文字"""
    x = x_right
    d = ImageDraw.Draw(base)
    for i, (key, label) in enumerate(reversed(pairs)):
        lw = d.textlength(label, font=font(BODY_FONT, 25))
        x -= lw
        text(base, (x, y), label, 25, (226, 214, 192), BODY_FONT, anchor='lm',
             shadow=(2, 2, (0, 0, 0, 200)))
        x -= 12
        kw = int(max(40, d.textlength(key, font=font(BODY_FONT, 24)) + 26))
        chip = parchment(kw, 40, 90 + i, aged=0.30, ragged=5.0, curl=False)
        x -= kw
        paste(base, drop_shadow(chip, blur_r=7, offset=(2, 4), alpha=0.55), x - 21, y - 41)
        paste(base, chip, x, y - 20)
        text(base, (x + kw / 2, y + 1), key, 24, INK, BODY_FONT, anchor='mm')
        x -= 32


def money_plate(w=268, h=62, seed=83):
    p = parchment(w, h, seed, aged=0.16, ragged=7.0)
    text(p, (w - 18, h / 2 + 2), '1,240 G', 38, INK, BRUSH_FONT, anchor='rm')
    text(p, (18, h / 2 + 2), '所持金', 23, INK_FADE, BODY_FONT, anchor='lm')
    return p


def portrait_disc(size=196):
    """既存HUDの肖像をそのまま使う。丸く抜いて真鍮の環をかぶせる"""
    src = Image.open(UI_ART / 'KnightStatusUI' / 'Portrait.png').convert('RGBA')
    src = src.resize((size, size), Image.LANCZOS)
    xx, yy = grid(size, size)
    r = size / 2
    mask = soften(np.clip(0.5 - (np.hypot(xx - r, yy - r) - (r - 8)), 0, 1), 1.0)
    a = np.asarray(src.split()[-1], np.float32) / 255.0
    src.putalpha(Image.fromarray((np.clip(a * mask, 0, 1) * 255).astype(np.uint8), 'L'))
    out = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    out.alpha_composite(rgba(np.zeros((size, size, 3), np.float32) + 0.07, mask * 0.9))
    out.alpha_composite(src)
    out.alpha_composite(brass_ring(size, 14, rivets=12, seed=29))
    return out


def item_icon(name, size=76):
    src = Image.open(UI_ART / 'Item' / name).convert('RGBA')
    return src.resize((size, size), Image.LANCZOS)


def ink_bar(page, x, y, w, label, ratio, value, h=22, color=(122, 44, 36)):
    """羊皮紙に描いたゲージ。枠はインクの線、中身は塗り潰し"""
    text(page, (x, y - 6), label, 24, INK_FADE, BODY_FONT, anchor='lb')
    d = ImageDraw.Draw(page)
    d.rectangle([x, y, x + w, y + h], outline=INK, width=2)
    fill_w = int((w - 6) * ratio)
    if fill_w > 0:
        d.rectangle([x + 3, y + 3, x + 3 + fill_w, y + h - 3], fill=color)
    for i in range(1, 5):
        gx = x + int(w * i / 5)
        d.line([(gx, y + 4), (gx, y + h - 4)], fill=(*INK, 120), width=1)
    text(page, (x + w + 14, y + h / 2 + 1), value, 26, INK, BODY_FONT, anchor='lm')


# ================================================================ 案A「冒険者の手帳」
def a_index_page(w, h, seed=101):
    p = parchment(w, h, seed, aged=0.10, ragged=9.0)
    d = ImageDraw.Draw(p)
    text(p, (w / 2, 58), '目 次', 42, INK, BRUSH_FONT, anchor='mm')
    d.line([(48, 96), (w - 48, 96)], fill=INK_FADE, width=2)
    step = min(96, (h - 230) // max(len(ENTRIES) - 1, 1))
    for i, (name, desc) in enumerate(ENTRIES):
        y = 176 + i * step
        if i == SELECTED:
            # 選択中は行の下に一本引く。説明文に掛からないよう説明の下へ
            d.line([(40, y + 56), (w - 40, y + 56)], fill=(*INK, 110), width=3)
            text(p, (46, y), '▶', 27, STAMP_RED, BODY_FONT, anchor='lm')
        text(p, (88, y), name, 36, INK, BODY_FONT, anchor='lm')
        text(p, (90, y + 34), desc, 20, INK_FADE, BODY_FONT, anchor='lm')
        text(p, (w - 48, y + 2), f'{i + 1:>2}', 25, INK_FADE, BODY_FONT, anchor='rm')
    return p


def a_status_page(w, h, seed=113):
    p = parchment(w, h, seed, aged=0.06, ragged=9.0)
    d = ImageDraw.Draw(p)
    text(p, (48, 58), 'ステータス', 44, INK, BRUSH_FONT, anchor='lm')
    d.line([(48, 96), (w - 48, 96)], fill=INK_FADE, width=2)

    paste(p, portrait_disc(176), 50, 118)
    text(p, (254, 152), '剣 士', 48, INK, BRUSH_FONT, anchor='lm')
    text(p, (256, 198), 'SWORDMAN', 21, INK_FADE, BODY_FONT, anchor='lm')
    text(p, (256, 246), '斧一本で前に出る', 25, INK, BODY_FONT, anchor='lm')

    ink_bar(p, 50, 342, 352, '体 力', 1.0, '100 / 100')
    ink_bar(p, 50, 414, 352, 'スタミナ', 0.72, ' 72 / 100', color=(104, 96, 40))

    for i, (label, pips) in enumerate(STATS):
        y = 494 + i * 52
        text(p, (50, y), label, 27, INK, BODY_FONT, anchor='lm')
        ink_pips(d, 200, y, pips, total=5, r=9, gap=29)

    text(p, (396, 494), '討伐      37', 24, INK_FADE, BODY_FONT, anchor='lm')
    text(p, (396, 530), '走破  2:14:08', 24, INK_FADE, BODY_FONT, anchor='lm')
    paste_tilted(p, money_plate(240, 60), 518, 594, -1.8, shadow=False)

    d.line([(48, 648), (w - 48, 648)], fill=(*INK_FADE, 150), width=2)
    text(p, (48, 680), '持ち物', 26, INK_FADE, BODY_FONT, anchor='lm')
    for i, (_, count, icon) in enumerate(ITEMS):
        x = 50 + i * 96
        paste(p, item_icon(icon, 58), x, 700)
        text(p, (x + 60, 752), f'×{count}', 22, INK, BODY_FONT, anchor='rm')
    return p


def mock_book(base):
    im = dim(base, darken=0.58, blur=8.0, vignette=0.64, warm=0.30)
    compose_book(im, a_index_page(PAGE_W, PAGE_H), a_status_page(PAGE_W, PAGE_H))
    text(im, (960, 84), '冒 険 者 の 手 帳', 54, (236, 218, 182), BRUSH_FONT, anchor='mm',
         shadow=(3, 4, (0, 0, 0, 210)))
    hint_bar(im, 1856, 1020, HINTS)
    return im


BOOK_CX, BOOK_CY = 960, 556
PAGE_W, PAGE_H = 636, 782
HINTS = [('↑↓', '選ぶ'), ('Enter', '決定'), ('＠', '閉じる')]
HINT_SPRITES = {'↑↓': 'Hint_UpDown', 'Enter': 'Hint_Enter', '＠': 'Hint_At'}


def compose_book(im, left, right):
    """革表紙・栞紐・見開きの頁・綴じ目を im に重ねる。頁の中身(文字)は呼び出し側で焼いておく"""
    pw, ph = PAGE_W, PAGE_H
    cx, cy = BOOK_CX, BOOK_CY
    bw, bh = pw * 2 + 84 + 88, ph + 84
    cover = leather(bw, bh, seed=43)
    cd = ImageDraw.Draw(cover)
    for inset, wdt, col in [(18, 3, (176, 134, 62, 165)), (28, 2, (150, 112, 52, 120))]:
        cd.rounded_rectangle([inset, inset, bw - 1 - inset, bh - 1 - inset], radius=7,
                             outline=col, width=wdt)
    im.alpha_composite(drop_shadow(cover, blur_r=30, offset=(0, 22), alpha=0.78),
                       (int(cx - bw / 2 - 90), int(cy - bh / 2 - 90)))
    paste(im, cover, cx - bw / 2, cy - bh / 2)
    for ox, oy in [(34, 34), (bw - 66, 34), (34, bh - 66), (bw - 66, bh - 66)]:   # 角金具
        paste(im, brass_ring(56, 9, rivets=4, seed=31 + ox), cx - bw / 2 + ox, cy - bh / 2 + oy)

    # 栞紐。ページの下に敷いて、本の下端からはみ出た分だけが見えるようにする
    rib_h = ph + 150
    rib = np.zeros((rib_h, 22, 3), np.float32) + np.array([0.50, 0.11, 0.10], np.float32)
    rib = rib * (0.62 + 0.62 * fbm(22, rib_h, 5, octaves=4, base=6)[..., None])
    ra = np.ones((rib_h, 22), np.float32)
    ra[-26:] *= np.linspace(1, 0.25, 26)[:, None]        # 先を斜めに切った感じ
    paste(im, rgba(rib, ra), cx + 226, cy - ph / 2 + 30)

    for part, x in [(left, cx - 42 - pw), (right, cx + 42)]:
        paste(im, drop_shadow(part, blur_r=13, offset=(3, 8), alpha=0.52), x - 39, cy - ph / 2 - 39)
        paste(im, part, x, cy - ph / 2)

    # 綴じ目。溝の影と、綴じ糸の代わりの小さな鋲
    gut = np.zeros((ph + 20, 104, 3), np.float32)
    ga = np.clip(1 - np.abs(np.linspace(-1, 1, 104)) ** 1.35, 0, 1) * 0.74
    paste(im, rgba(gut, np.repeat(ga[None, :], ph + 20, 0)), cx - 52, cy - (ph + 20) / 2)
    for k in range(5):
        paste(im, nail(17, seed=k), cx - 8, cy - 296 + k * 148)


# ================================================================ 案B「真鍮の輪」
def mock_ring(base):
    im = dim(base, darken=0.34, blur=2.0, vignette=0.72, warm=0.35)

    cx, cy, R = 960, 552, 286
    # 輪の土台。細い真鍮の環を1本だけ回して、記章を吊っている感じにする
    band = brass_ring(R * 2 + 40, 13, rivets=0, seed=17)
    a = np.asarray(band.split()[-1], np.float32) / 255.0 * 0.55
    band.putalpha(Image.fromarray((a * 255).astype(np.uint8), 'L'))
    paste(im, band, cx - band.width / 2, cy - band.height / 2)

    for i, (name, _) in enumerate(ENTRIES):
        ang = -np.pi / 2 + i * 2 * np.pi / len(ENTRIES)
        sel = (i == SELECTED)
        size = 148 if sel else 108
        mx, my = cx + np.cos(ang) * R, cy + np.sin(ang) * R
        med = medallion(size, GLYPHS[i], sel, seed=61 + i * 3)
        paste(im, drop_shadow(med, blur_r=14, offset=(4, 10), alpha=0.68),
              mx - size / 2 - 42, my - size / 2 - 42)
        paste(im, med, mx - size / 2, my - size / 2)
        if sel:
            paste_tilted(im, wax_seal(58, seed=9), mx + size * 0.38, my + size * 0.36, -12, shadow=False)
        else:
            text(im, (mx, my + size / 2 + 26), name, 23, (206, 192, 170), BODY_FONT, anchor='mm',
                 shadow=(2, 2, (0, 0, 0, 200)))

    # 輪の内側。選択中の名前と一行説明を羊皮紙の小札に
    name, desc = ENTRIES[SELECTED]
    tag = parchment(430, 150, 131, aged=0.14, ragged=10.0)
    text(tag, (215, 56), name, 46, INK, BRUSH_FONT, anchor='mm')
    ImageDraw.Draw(tag).line([(58, 88), (372, 88)], fill=INK_FADE, width=2)
    text(tag, (215, 116), desc, 24, INK, BODY_FONT, anchor='mm')
    paste_tilted(im, tag, cx, cy - 8, -1.6)

    # 上の吊り看板。鎖で吊っているように、真鍮の環を2つ打って上へ伸ばす
    sign = wood_board(392, 92, seed=19, plank=400)
    ImageDraw.Draw(sign).rectangle([10, 10, 381, 81], outline=(150, 108, 46, 150), width=3)
    engrave(sign, (196, 50), 'メ ニ ュ ー', 38, BRUSH_FONT, anchor='mm', tone=(232, 204, 154))
    for rx in (876, 1004):
        ring = brass_ring(40, 8, rivets=0, seed=23)
        paste(im, ring, rx, 84)
        paste(im, rgba(np.zeros((42, 5, 3), np.float32) + np.array([0.40, 0.30, 0.13], np.float32),
                       np.ones((42, 5), np.float32) * 0.9), rx + 18, 44)
    paste_tilted(im, sign, 960, 154, -0.8)

    hint_bar(im, 1856, 1014, [('←→', '回す'), ('Enter', '決定'), ('＠', '閉じる')])
    return im


# ================================================================ 案C「鉄の見出しと大きな紙」
def c_status_sheet(w, h, seed=151):
    p = parchment(w, h, seed, aged=0.08, ragged=12.0)
    d = ImageDraw.Draw(p)
    text(p, (58, 66), 'ステータス', 48, INK, BRUSH_FONT, anchor='lm')
    d.line([(58, 106), (w - 58, 106)], fill=INK_FADE, width=3)

    paste(p, portrait_disc(214), 66, 140)
    text(p, (310, 182), '剣 士', 54, INK, BRUSH_FONT, anchor='lm')
    text(p, (314, 232), 'SWORDMAN', 23, INK_FADE, BODY_FONT, anchor='lm')
    text(p, (314, 282), '斧一本で前に出る', 27, INK, BODY_FONT, anchor='lm')
    paste_tilted(p, money_plate(300, 70), w - 240, 206, -2.0)

    ink_bar(p, 66, 398, 366, '体 力', 1.0, '100 / 100', h=26)
    ink_bar(p, 66, 484, 366, 'スタミナ', 0.72, ' 72 / 100', h=26, color=(104, 96, 40))
    for i, (label, pips) in enumerate(STATS):
        y = 566 + i * 52
        text(p, (66, y), label, 27, INK, BODY_FONT, anchor='lm')
        ink_pips(d, 240, y, pips, total=5, r=9, gap=32)
    text(p, (66, h - 48), '討伐 37      走破 2:14:08', 24, INK_FADE, BODY_FONT, anchor='lm')

    # 右半分。持ち物と受注中のクエストを一望させる
    x = 648
    d.line([(x - 44, 150), (x - 44, h - 110)], fill=(*INK_FADE, 140), width=2)
    text(p, (x, 398), '持ち物', 28, INK_FADE, BODY_FONT, anchor='lm')
    for i, (iname, count, icon) in enumerate(ITEMS):
        ix = x + i * 104
        paste(p, item_icon(icon, 74), ix, 430)
        text(p, (ix + 76, 498), f'×{count}', 23, INK, BODY_FONT, anchor='rm')
    text(p, (x, 584), '受注中', 28, INK_FADE, BODY_FONT, anchor='lm')
    for i, (q, s) in enumerate([('スライムを10体討つ', '7 / 10'), ('失くした指輪を探す', '未着手')]):
        y = 626 + i * 46
        text(p, (x + 14, y), '・' + q, 26, INK, BODY_FONT, anchor='lm')
        text(p, (w - 70, y), s, 24, INK_FADE, BODY_FONT, anchor='rm')
    return p


def mock_plates(base):
    im = dim(base, darken=0.70, blur=9.0, vignette=0.5, warm=0.25)

    # 上下の梁。画面をまたぐ木の棒を渡して、紙と札を打ち付ける土台にする
    for y, hgt in [(82, 64), (978, 56)]:
        beam = wood_board(SCREEN_W, hgt, seed=23 + y, plank=600)
        paste(im, drop_shadow(beam, blur_r=18, offset=(0, 12), alpha=0.66), -54, y - 54)
        paste(im, beam, 0, y)
        for bx in range(120, SCREEN_W, 300):
            paste(im, nail(22, seed=bx), bx, y + hgt / 2 - 11)

    # 左の見出し札
    tw, th = 352, 76
    for i, (name, _) in enumerate(ENTRIES):
        sel = (i == SELECTED)
        y = 208 + i * 88
        x = 62 + (26 if sel else 0)
        tab = iron_tab(tw + (34 if sel else 0), th, seed=21 + i * 5, selected=sel)
        paste(im, drop_shadow(tab, blur_r=12, offset=(6, 9), alpha=0.66), x - 36, y - 36)
        paste(im, tab, x, y)
        engrave(im, (x + 34, y + th / 2 + 1), name, 32, BODY_FONT, anchor='lm',
                tone=(240, 210, 148) if sel else (196, 190, 180))
        paste(im, nail(20, seed=i), x + 12, y + th / 2 - 10)
        if sel:
            paste_tilted(im, wax_seal(56, seed=7), x + tw + 26, y + th / 2, -14, shadow=False)

    sheet = c_status_sheet(1272, 754)
    sx, sy = 556, 196
    paste(im, drop_shadow(sheet, blur_r=20, offset=(8, 14), alpha=0.7), sx - 60, sy - 60)
    paste_tilted(im, sheet, sx + 1272 / 2, sy + 754 / 2, -0.7, shadow=False)
    for nx, ny in [(sx + 26, sy + 20), (sx + 1272 - 44, sy + 20),
                   (sx + 26, sy + 754 - 42), (sx + 1272 - 44, sy + 754 - 42)]:
        paste(im, nail(22, seed=nx), nx, ny)

    plaque = iron_tab(392, 74, seed=44, selected=True, notch=0)
    paste(im, drop_shadow(plaque, blur_r=14, offset=(4, 10), alpha=0.7), 764 - 42, 78 - 42)
    paste(im, plaque, 764, 78)
    engrave(im, (960, 116), 'メ ニ ュ ー', 40, BRUSH_FONT, anchor='mm', tone=(240, 212, 156))
    hint_bar(im, 1856, 1006, [('↑↓', '選ぶ'), ('Enter', '決定'), ('＠', '閉じる')])
    return im


# ================================================================ 決定案(案A)のスプライト書き出し
# 文字・ゲージの中身・持ち物のアイコンはエンジン側(TextRenderer / Slider / ItemData)が描くので、
# 焼くのは革と紙と罫線だけ。座標はすべて 1920x1080 の画面座標で、layout() が prefab の正
# (tools/art/pause_menu_prefab.py がこれを読んで PauseMenu*.prefab を組む)
EMIT_DIR = UI_ART / 'PauseMenu'
PAGE_TOP = BOOK_CY - PAGE_H // 2        # 165
LEFT_PAGE_X = BOOK_CX - 42 - PAGE_W     # 282
RIGHT_PAGE_X = BOOK_CX + 42             # 1002
RULE_Y, DIVIDER_Y = 96, 648
ROW_Y0, ROW_PITCH = 176, 96
BAR_W, BAR_H = 352, 22
MONEY_W, MONEY_H = 220, 58
PORTRAIT_SIZE = 176
HEALTH_INK = (122, 44, 36)
STAMINA_INK = (104, 96, 40)
TITLE_INK = (236, 218, 182)
HINT_INK = (226, 214, 192)


def ink_line(p, xy, color, alpha, width):
    """RGBA の紙に直接半透明で描くと紙ごと透けるので、別の層に描いて重ねる"""
    lay = Image.new('RGBA', p.size, (0, 0, 0, 0))
    ImageDraw.Draw(lay).line(xy, fill=(*color, alpha), width=width)
    p.alpha_composite(lay)


def blank_index_page():
    p = parchment(PAGE_W, PAGE_H, 101, aged=0.10, ragged=9.0)
    ink_line(p, [(48, RULE_Y), (PAGE_W - 48, RULE_Y)], INK_FADE, 255, 2)
    return p


def blank_status_page():
    p = parchment(PAGE_W, PAGE_H, 113, aged=0.06, ragged=9.0)
    ink_line(p, [(48, RULE_Y), (PAGE_W - 48, RULE_Y)], INK_FADE, 255, 2)
    ink_line(p, [(48, DIVIDER_Y), (PAGE_W - 48, DIVIDER_Y)], INK_FADE, 150, 2)
    return p


# 本の影まで収まる左右対称の切り抜き。中心が画面中央 (960, 540) に来るので prefab 側は固定値で置ける
BOOK_CROP = (138, 0, SCREEN_W - 138, SCREEN_H)


def book_sprite():
    """本一冊ぶん(影・革表紙・栞紐・白紙の見開き・綴じ目)。はみ出た栞紐は画面下端で切れる"""
    canvas = Image.new('RGBA', (SCREEN_W, SCREEN_H), (0, 0, 0, 0))
    compose_book(canvas, blank_index_page(), blank_status_page())
    box = canvas.getbbox()
    assert BOOK_CROP[0] <= box[0] and box[2] <= BOOK_CROP[2], f'book shadow {box} leaks out of {BOOK_CROP}'
    return canvas.crop(BOOK_CROP)


def backdrop_sprite(w=960, h=540):
    """画面全体を落とす暗幕。半分の解像度で作り、prefab で 2 倍にする"""
    xx, yy = grid(w, h)
    r = np.hypot((xx - w / 2) / (w / 2), (yy - h / 2) / (h / 2))
    vig = np.clip(r / 1.25, 0, 1) ** 1.7
    rgb = np.zeros((h, w, 3), np.float32) + np.array([0.06, 0.04, 0.03], np.float32)
    return rgba(rgb, 0.60 + 0.30 * vig)


def marker_sprite(w=20, h=24):
    lay = Image.new('RGBA', (w * 4, h * 4), (0, 0, 0, 0))
    ImageDraw.Draw(lay).polygon([(6, 6), (w * 4 - 6, h * 2), (6, h * 4 - 6)], fill=(*STAMP_RED, 255))
    return lay.resize((w, h), Image.LANCZOS)


def underline_sprite(w=556, h=5):
    """選択中の行の下線。筆で引いたように濃淡と太さを揺らす"""
    xx, yy = grid(w, h)
    wobble = fbm(w, 1, 7, octaves=3, base=6)[0] - 0.5
    d = np.abs(yy - (h - 1) / 2 - wobble[None, :] * 1.2) - (1.2 + 0.5 * fbm(w, 1, 9, octaves=3, base=4)[0])[None, :]
    mask = soften(np.clip(0.5 - d, 0, 1), 0.5)
    fade = np.clip(np.minimum(xx, w - 1 - xx) / 18, 0, 1)
    rgb = np.zeros((h, w, 3), np.float32) + np.array(INK, np.float32) / 255.0
    return rgba(rgb, mask * fade * 0.55)


def bar_frame_sprite(w=BAR_W, h=BAR_H):
    """ゲージの枠。Slider の backgroundSprite_ として drawSize_ へ引き伸ばす(目盛りは Slider が描く)"""
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(lay).rectangle([0, 0, w - 1, h - 1], outline=(*INK, 255), width=2)
    return lay


def bar_fill_sprite(color, w=BAR_W, h=BAR_H, seed=3):
    """ゲージの中身。枠の内側 3px を空けて、Slider が値の分だけ左から切り抜く"""
    fill = np.zeros((h, w), np.float32)
    fill[3:h - 3, 3:w - 3] = 1.0
    tone = 0.86 + 0.26 * fbm(w, h, seed, octaves=4, base=10)
    rgb = (np.array(color, np.float32) / 255.0)[None, None, :] * tone[..., None]
    return rgba(rgb, fill)


def hint_chip_sprite(key, seed):
    d = ImageDraw.Draw(Image.new('RGBA', (1, 1)))
    kw = int(max(40, d.textlength(key, font=font(BODY_FONT, 24)) + 26))
    chip = parchment(kw, 40, seed, aged=0.30, ragged=5.0, curl=False)
    text(chip, (kw / 2, 21), key, 24, INK, BODY_FONT, anchor='mm')
    return chip


def layout():
    """prefab に置く位置。TextRenderer は左上(中央揃えは上辺の中央、右揃えは右上)が基準、
    ImageRenderer は画像の中心が基準。px はフォント 60px に対する倍率へ prefab 側で直す"""
    rx, top = RIGHT_PAGE_X, PAGE_TOP
    plate = (rx + 500, top + 536)
    geo = {
        'backdrop': {'center': (960, 540), 'scale': 2.0},
        'title': {'pos': (960, 84 - 27), 'px': 54, 'font': 'brush', 'align': 'center', 'color': TITLE_INK,
                  'text': '冒 険 者 の 手 帳'},
        'index_title': {'pos': (LEFT_PAGE_X + PAGE_W / 2, top + 58 - 21), 'px': 42, 'font': 'brush',
                        'align': 'center', 'color': INK, 'text': '目 次'},
        'rows_root': (LEFT_PAGE_X + 88, top + ROW_Y0),
        'row_pitch': ROW_PITCH,
        'row': {
            'name': {'pos': (0, -18), 'px': 36, 'color': INK, 'align': 'left'},
            'description': {'pos': (2, 34 - 10), 'px': 20, 'color': INK_FADE, 'align': 'left'},
            'number': {'pos': (PAGE_W - 48 - 88, 2 - 12.5), 'px': 25, 'color': INK_FADE, 'align': 'right'},
            'marker': (-29, 0),
            'underline': (-48 + 556 / 2, 56),
        },
        'status_title': {'pos': (rx + 48, top + 58 - 22), 'px': 44, 'font': 'brush', 'align': 'left',
                         'color': INK, 'text': 'ステータス'},
        'portrait': (rx + 50 + PORTRAIT_SIZE / 2, top + 118 + PORTRAIT_SIZE / 2),
        'name': {'pos': (rx + 254, top + 152 - 24), 'px': 48, 'font': 'brush', 'align': 'left', 'color': INK},
        'reading': {'pos': (rx + 256, top + 198 - 10.5), 'px': 21, 'align': 'left', 'color': INK_FADE},
        'tagline': {'pos': (rx + 256, top + 246 - 12.5), 'px': 25, 'align': 'left', 'color': INK},
        'health_label': {'pos': (rx + 50, top + 342 - 30), 'px': 24, 'align': 'left', 'color': INK_FADE,
                         'text': '体 力'},
        'health_bar': {'pos': (rx + 50, top + 342), 'size': (BAR_W, BAR_H)},
        'health_text': {'pos': (rx + 416, top + 353 - 13), 'px': 26, 'align': 'left', 'color': INK},
        'stamina_label': {'pos': (rx + 50, top + 414 - 30), 'px': 24, 'align': 'left', 'color': INK_FADE,
                          'text': 'スタミナ'},
        'stamina_bar': {'pos': (rx + 50, top + 414), 'size': (BAR_W, BAR_H)},
        'stamina_text': {'pos': (rx + 416, top + 425 - 13), 'px': 26, 'align': 'left', 'color': INK},
        'stats': [
            {'label': {'pos': (rx + 50, top + 494 + 52 * i - 13.5), 'px': 27, 'align': 'left', 'color': INK,
                       'text': label},
             'pips': (rx + 200, top + 494 + 52 * i)}
            for i, (label, _) in enumerate(STATS)
        ],
        'pip_gap': 29,
        'pip_scale': 0.8,
        'money_plate': plate,
        'money_label': {'pos': (plate[0] - MONEY_W / 2 + 14, plate[1] - 10.5), 'px': 21, 'align': 'left',
                        'color': INK_FADE, 'text': '所持金'},
        'money_text': {'pos': (plate[0] + MONEY_W / 2 - 14, plate[1] - 17), 'px': 34, 'font': 'brush',
                       'align': 'right', 'color': INK},
        'items_label': {'pos': (rx + 48, top + 680 - 13), 'px': 26, 'align': 'left', 'color': INK_FADE,
                        'text': '持ち物'},
        'items_root': (rx + 50 + 29, top + 700 + 29),
        'item_pitch': 96,
        'item_cell': {
            'icon': (0, 0),
            'count': {'pos': (31, 23 - 11), 'px': 22, 'align': 'right', 'color': INK},
        },
        'hints': [],
    }

    # 右下の操作ヒント。右端から左へ詰める(mock の hint_bar と同じ並べ方)
    d = ImageDraw.Draw(Image.new('RGBA', (1, 1)))
    x, y = 1856, 1020
    for i, (key, label) in enumerate(reversed(HINTS)):
        x -= d.textlength(label, font=font(BODY_FONT, 25))
        label_x = x
        x -= 12
        kw = int(max(40, d.textlength(key, font=font(BODY_FONT, 24)) + 26))
        x -= kw
        geo['hints'].insert(0, {
            'sprite': HINT_SPRITES[key], 'key': key, 'seed': 90 + i,
            'chip': (x + kw / 2, y),
            'label': {'pos': (label_x, y - 12.5), 'px': 25, 'align': 'left', 'color': HINT_INK, 'text': label},
        })
        x -= 32
    return geo


def write_sprite(out_dir, name, image):
    from tools.scene import sprite_meta
    png = out_dir / f'{name}.png'
    image.save(png)
    meta = out_dir / f'{name}.png.meta'
    guid = sprite_meta.read_meta(meta)['guid'] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    print(f'  {name}.png  {image.size[0]}x{image.size[1]}  guid={guid}')


def emit(out_dir=EMIT_DIR):
    """スプライトを書き出す。.meta の guid は既存があれば保つので、何度流しても prefab の参照は切れない"""
    sys.path.insert(0, str(REPO_ROOT))
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {out_dir}')
    write_sprite(out_dir, 'Book', book_sprite())
    write_sprite(out_dir, 'Backdrop', backdrop_sprite())
    write_sprite(out_dir, 'Row_Marker', marker_sprite())
    write_sprite(out_dir, 'Row_Underline', underline_sprite())
    write_sprite(out_dir, 'Portrait_SwordMan', portrait_disc(PORTRAIT_SIZE))
    write_sprite(out_dir, 'Bar_Frame', bar_frame_sprite())
    write_sprite(out_dir, 'Bar_Fill_Health', bar_fill_sprite(HEALTH_INK, seed=3))
    write_sprite(out_dir, 'Bar_Fill_Stamina', bar_fill_sprite(STAMINA_INK, seed=4))
    write_sprite(out_dir, 'MoneyPlate', parchment(MONEY_W, MONEY_H, 83, aged=0.16, ragged=7.0))
    for hint in layout()['hints']:
        write_sprite(out_dir, hint['sprite'], hint_chip_sprite(hint['key'], hint['seed']))


def preview(base, sprite_dir=EMIT_DIR):
    """書き出したスプライトを layout() どおりに置き、文字は TextRenderer と同じ基準で描いて、
    prefab を組む前に位置の食い違いを実画面の上で確かめる"""
    geo = layout()
    im = base.copy()

    def sprite(name):
        return Image.open(sprite_dir / f'{name}.png').convert('RGBA')

    def put(part, center, scale=1.0):
        if scale != 1.0:
            part = part.resize((round(part.width * scale), round(part.height * scale)), Image.LANCZOS)
        paste(im, part, center[0] - part.width / 2, center[1] - part.height / 2)

    def write(spec, s=None, offset=(0, 0)):
        anchor = {'left': 'la', 'center': 'ma', 'right': 'ra'}[spec['align']]
        path = BRUSH_FONT if spec.get('font') == 'brush' else BODY_FONT
        text(im, (spec['pos'][0] + offset[0], spec['pos'][1] + offset[1]), s or spec['text'],
             spec['px'], spec['color'], path, anchor=anchor)

    put(sprite('Backdrop'), geo['backdrop']['center'], geo['backdrop']['scale'])
    put(sprite('Book'), (960, 540))
    write(geo['title'])
    write(geo['index_title'])

    rx, ry = geo['rows_root']
    for i, (name, desc) in enumerate(ENTRIES):
        origin = (rx, ry + i * geo['row_pitch'])
        row = geo['row']
        if i == SELECTED:
            put(sprite('Row_Marker'), (origin[0] + row['marker'][0], origin[1] + row['marker'][1]))
            put(sprite('Row_Underline'), (origin[0] + row['underline'][0], origin[1] + row['underline'][1]))
        write(row['name'], name, origin)
        write(row['description'], desc, origin)
        write(row['number'], str(i + 1), origin)

    write(geo['status_title'])
    put(sprite('Portrait_SwordMan'), geo['portrait'])
    write(geo['name'], '剣士')
    write(geo['reading'], 'SWORDMAN')
    write(geo['tagline'], '斧一本で前に出る')
    for key, ratio, value, fill in [('health', 1.0, '100 / 100', 'Bar_Fill_Health'),
                                    ('stamina', 0.72, '72 / 100', 'Bar_Fill_Stamina')]:
        write(geo[f'{key}_label'])
        bar = geo[f'{key}_bar']
        paste(im, sprite('Bar_Frame'), *bar['pos'])
        f = sprite(fill)
        paste(im, f.crop((0, 0, int(f.width * ratio), f.height)), *bar['pos'])
        write(geo[f'{key}_text'], value)

    pip_on = Image.open(UI_ART / 'CharacterSelect' / 'Pip_Filled.png').convert('RGBA')
    pip_off = Image.open(UI_ART / 'CharacterSelect' / 'Pip_Empty.png').convert('RGBA')
    for stat, (_, filled) in zip(geo['stats'], STATS):
        write(stat['label'])
        for k in range(5):
            put(pip_on if k < filled else pip_off,
                (stat['pips'][0] + k * geo['pip_gap'], stat['pips'][1]), geo['pip_scale'])

    put(sprite('MoneyPlate'), geo['money_plate'])
    write(geo['money_label'])
    write(geo['money_text'], '1,240 G')

    write(geo['items_label'])
    ix, iy = geo['items_root']
    for i, (_, count, icon) in enumerate(ITEMS):
        origin = (ix + i * geo['item_pitch'], iy)
        put(Image.open(UI_ART / 'Item' / icon).convert('RGBA'), origin)
        write(geo['item_cell']['count'], f'×{count}', origin)

    for hint in geo['hints']:
        put(sprite(hint['sprite']), hint['chip'])
        write(hint['label'])
    return im


# ---------------------------------------------------------------- 比較シート
def contact_sheet(mocks, labels):
    cols, scale = 1, 0.5
    tw, th = int(SCREEN_W * scale), int(SCREEN_H * scale)
    pad, head = 26, 46
    sheet = Image.new('RGBA', (tw + pad * 2, (th + head) * len(mocks) + pad * 2), (22, 19, 17, 255))
    for i, (m, lab) in enumerate(zip(mocks, labels)):
        y = pad + i * (th + head)
        text(sheet, (pad + 6, y + 26), lab, 30, (234, 216, 182), BODY_FONT, anchor='lm')
        sheet.alpha_composite(m.convert('RGBA').resize((tw, th), Image.LANCZOS), (pad, y + head))
    return sheet


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot', default='')
    ap.add_argument('--out-dir', default='')
    ap.add_argument('--clean-only', action='store_true')
    ap.add_argument('--emit', action='store_true', help='決定案のスプライトを Assets/Art/UI/PauseMenu へ書き出す')
    ap.add_argument('--preview', action='store_true', help='書き出したスプライトを layout() どおりに実画面へ置いて確かめる')
    args = ap.parse_args()

    if args.emit:
        emit()
        if not args.preview:
            return
    if not args.shot or not args.out_dir:
        ap.error('--shot and --out-dir are required for mocks / --preview')

    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)

    shot = Image.open(args.shot).convert('RGBA')
    if shot.size != (SCREEN_W, SCREEN_H):
        shot = shot.resize((SCREEN_W, SCREEN_H), Image.LANCZOS)
    base = clean_shot(shot)
    if args.preview:
        preview(base).convert('RGB').save(out / 'menu_preview.png')
        print(f"wrote {out / 'menu_preview.png'}")
        return
    base.convert('RGB').save(out / 'menu_00_clean.png')
    print(f"wrote {out / 'menu_00_clean.png'}")
    if args.clean_only:
        return

    plans = [
        ('A_book', '案A  冒険者の手帳（中央の見開き）', mock_book),
        ('B_ring', '案B  真鍮の輪（中央の円環・画面が見える）', mock_ring),
        ('C_plates', '案C  鉄の見出しと大きな紙（全画面・情報量重視）', mock_plates),
    ]
    mocks = []
    for key, label, fn in plans:
        im = fn(base.copy())
        path = out / f'menu_{key}.png'
        im.convert('RGB').save(path)
        mocks.append(im)
        print(f'wrote {path}')

    contact_sheet(mocks, [lab for _, lab, _ in plans]).convert('RGB').save(out / 'menu_compare.png')
    print(f"wrote {out / 'menu_compare.png'}")


if __name__ == '__main__':
    main()
