"""設定画面 (タイトル > 設定 / ステージの貼り紙 > 設定) の素材。

    python tools/art/settings_screen.py --emit                                # Assets/Art/UI/Settings/ に書き出す
    python tools/art/settings_screen.py --shot <shot.png> --out <png>         # 書き出した絵を実画面に重ねて確かめる
    python tools/art/settings_prefab.py                                       # 続けてプレハブを組み直す

並びはモンハンライズのオプションに倣う: 左にカテゴリの札、右に半透明の黒漆のパネルと「項目 ─ 値」の一覧、
左下に説明、右下に操作ヒント。ライズの金の細縁は真鍮の線と四隅の金具で表す。
文字は TextRenderer が出すので、絵は板・金具・帯・罫だけにする (位置は下の LAYOUT をプレハブと共有する)。
本文フォント (Zen Old Mincho) に ◀▶ ✓ が無いので、左右の矢印は絵で出す。
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from character_select import (  # noqa: E402
    BODY_FONT, BRASS, hint_tag, text, font, paste, load_base, rgba, grid, soften, bevel, fbm, rect_outside, nail,
    write_sprite,
)

KAISEI = REPO / 'Assets' / 'Art' / 'Font' / 'KaiseiDecol-Bold.ttf'
EMIT_DIR = REPO / 'Assets' / 'Art' / 'UI' / 'Settings'

HEADING = '設　定'
CREAM = (246, 234, 206)
CREAM_FADE = (200, 184, 152)
GOLD = (214, 170, 90)
# ライズのように後ろの景色が透けて見える濃さ
PANEL_ALPHA = 0.76
BACKDROP_BLEND = 100          # BlackMask を重ねる濃さ (0..255)

# モックの中身 (実物は SettingsCatalog.cpp)
PREVIEW_CATEGORIES = ['ゲーム']
PREVIEW_ROWS = [('会話の送り方', '自分で送る', 'ボタンを押すまで、次の言葉を待ちます。')]

# 1920x1080 / 左上原点。文字は (x, 中心 y, px)
TAB_LEFT, TAB_TOP, TAB_W, TAB_H, TAB_PITCH = 96, 196, 440, 60, 74
PANEL = (600, 164, 1848, 700)
HEADER_W, HEADER_H = 300, 58
ROWS_TOP, ROW_H, VISIBLE_ROWS = 290, 60, 6
ROW_LEFT, ROW_RIGHT = 636, 1788
LABEL_X, VALUE_CX, ARROW_DX = 660, 1560, 150
SCROLL_X, SCROLL_THUMB_H = 1812, 60
DESC = (40, 918, 960, 1004)
# キーボードの札。パッドの札は device_hint.py (Tab = LB RB、Cancel = B)
HINTS = [('Tab', 'Q E', '切り替え'), ('UpDown', '▲▼', '選ぶ'), ('LeftRight', '←→', '切り替える'),
         ('Cancel', 'Esc', 'もどる')]
HINT_RIGHT, HINT_Y, HINT_TAG_H, HINT_PX = 1856, 1000, 42, 26


def layout():
    """スプライトの中心と文字の位置。プレハブ (settings_prefab.py) とプレビューが同じものを使う"""
    x0, y0, x1, y1 = PANEL
    L = {
        'panel': ((x0 + x1) / 2, (y0 + y1) / 2),
        'header': ((x0 + x1) / 2, y0),
        'header_text': ((x0 + x1) / 2, y0, 32),
        'category_text': (LABEL_X - 12, 226, 30),
        'category_rule': ((LABEL_X - 12 + ROW_RIGHT) / 2, 256),
        # 札0枚目。n 枚目は y に TAB_PITCH * n を足す
        'tab': (TAB_LEFT + TAB_W / 2, TAB_TOP + TAB_H / 2),
        'tab_text': (TAB_LEFT + 56, TAB_TOP + TAB_H / 2, 30),
        # 行0。n 行目は y に ROW_H * n を足す
        'row_band': ((ROW_LEFT + ROW_RIGHT) / 2, ROWS_TOP + ROW_H / 2),
        'row_rule': ((ROW_LEFT + ROW_RIGHT) / 2, ROWS_TOP + ROW_H),
        'row_label': (LABEL_X, ROWS_TOP + ROW_H / 2, 29),
        'row_value': (VALUE_CX, ROWS_TOP + ROW_H / 2, 29),
        'row_arrow_left': (VALUE_CX - ARROW_DX, ROWS_TOP + ROW_H / 2),
        'row_arrow_right': (VALUE_CX + ARROW_DX, ROWS_TOP + ROW_H / 2),
        'scroll_track': (SCROLL_X, ROWS_TOP + ROW_H * VISIBLE_ROWS / 2),
        'scroll_thumb': (SCROLL_X, ROWS_TOP + SCROLL_THUMB_H / 2),
        'scroll_travel': ROW_H * VISIBLE_ROWS - SCROLL_THUMB_H,
        'desc': ((DESC[0] + DESC[2]) / 2, (DESC[1] + DESC[3]) / 2),
        'desc_text': (DESC[0] + 34, (DESC[1] + DESC[3]) / 2, 27),
    }
    hints = []
    x = HINT_RIGHT
    f = font(BODY_FONT, HINT_PX)
    for key, glyph, label in reversed(HINTS):
        label_left = x - f.getlength(label)
        tag_w = hint_width(glyph)
        tag_cx = label_left - 16 - tag_w / 2
        hints.append({'key': key, 'label': label, 'tag': (tag_cx, HINT_Y), 'text': (label_left, HINT_Y, HINT_PX)})
        x = tag_cx - tag_w / 2 - 30
    L['hints'] = list(reversed(hints))
    return L


def hint_width(glyph):
    if glyph == 'Q E':
        return 84
    return 68 if len(glyph) > 1 else 46


def text_top(center_y, px):
    """TextRenderer の位置は文字の上辺。中心から上辺へ直す (stage_return.text_top と同じ比)"""
    return center_y - px * 0.55


# ---------------------------------------------------------------- 素材
def tab_mask(w, h, point=26):
    """両端を尖らせた札の形 (ライズのカテゴリ札)"""
    xx, yy = grid(w, h)
    side = np.minimum(xx, w - 1 - xx)
    half = (h / 2 - 2) * np.clip(side / point, 0, 1)
    d = np.maximum(np.abs(yy - h / 2) - half, rect_outside(xx, yy, w, h, 1.0))
    return soften(np.clip(0.5 - d, 0, 1), 0.6), d


def lacquer(w, h, seed, alpha=PANEL_ALPHA):
    """黒漆の板。うっすら木目が透ける"""
    xx, yy = grid(w, h)
    grain = np.sin((yy + fbm(w, h, seed, octaves=4, base=3) * 60) * 0.12) * 0.5 + 0.5
    tone = 0.10 + 0.05 * grain + 0.04 * fbm(w, h, seed + 1, octaves=5, base=12)
    rgb = np.array([0.42, 0.30, 0.26], np.float32)[None, None, :] * tone[..., None] * 1.2
    d = rect_outside(xx, yy, w, h, 1.0)
    return rgba(rgb, soften(np.clip(0.5 - d, 0, 1), 0.6) * alpha)


def brass_line(draw, box, width=2, alpha=230):
    draw.rectangle(box, outline=GOLD + (alpha,), width=width)


def corner_fittings(img, size=46, inset=6):
    """四隅の L 字の真鍮金具"""
    w, h = img.size
    xx, yy = grid(size, size)
    t = 9
    m = np.clip(0.5 - np.maximum(np.minimum(xx, yy) - t, np.maximum(xx, yy) - size + 1), 0, 1)
    m = soften(m, 0.6)
    b = bevel(m, 2.0)
    tone = (0.8 + 0.4 * fbm(size, size, 71, octaves=4, base=6)) * (1.0 + 1.4 * b)
    piece = rgba(BRASS[None, None, :] * tone[..., None] * 1.5, m)
    piece.alpha_composite(nail(10), (3, 3))
    for flip_x, flip_y in ((False, False), (True, False), (False, True), (True, True)):
        p = piece
        if flip_x:
            p = p.transpose(Image.FLIP_LEFT_RIGHT)
        if flip_y:
            p = p.transpose(Image.FLIP_TOP_BOTTOM)
        img.alpha_composite(p, (w - size - inset if flip_x else inset, h - size - inset if flip_y else inset))


def panel_sprite():
    x0, y0, x1, y1 = PANEL
    w, h = x1 - x0, y1 - y0
    p = lacquer(w, h, 600)
    d = ImageDraw.Draw(p)
    brass_line(d, [10, 10, w - 11, h - 11], 2)
    brass_line(d, [16, 16, w - 17, h - 17], 1, 120)
    corner_fittings(p)
    return p


def tab_sprite(selected, w=TAB_W, h=TAB_H, seed=650):
    m, d = tab_mask(w, h)
    arr = np.asarray(lacquer(w, h, seed, alpha=1.0), np.float32) / 255
    if selected:
        arr[..., :3] = arr[..., :3] * 0.4 + np.array([0.46, 0.40, 0.33]) * 0.6
    rim = np.clip(1 - np.abs(d + 3.0) / 1.6, 0, 1)
    arr[..., :3] = arr[..., :3] * (1 - rim[..., None]) + (np.array(GOLD) / 255)[None, None, :] * rim[..., None]
    arr[..., 3] = m * (0.92 if selected else 0.70)
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA')


def header_sprite():
    p = tab_sprite(False, HEADER_W, HEADER_H, 630)
    arr = np.asarray(p, np.float32)
    arr[..., 3] = tab_mask(HEADER_W, HEADER_H, point=18)[0] * 255
    return Image.fromarray(arr.astype(np.uint8), 'RGBA')


def band_sprite():
    """選んでいる行の帯。下辺に金の線"""
    w, h = ROW_RIGHT - ROW_LEFT, ROW_H - 4
    im = Image.new('RGBA', (w, h), (150, 120, 80, 120))
    ImageDraw.Draw(im).line([(0, h - 2), (w, h - 2)], fill=GOLD + (220,), width=2)
    return im


def dashed_rule(w=ROW_RIGHT - ROW_LEFT):
    im = Image.new('RGBA', (w, 2), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for x in range(0, w, 14):
        d.line([(x, 0), (x + 7, 0)], fill=GOLD + (70,), width=1)
    return im


def category_rule():
    w = ROW_RIGHT - (LABEL_X - 12)
    im = Image.new('RGBA', (w, 2), GOLD + (180,))
    return im


def arrow_sprite(direction, size=13, color=CREAM_FADE):
    w = h = size * 2 + 4
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    cx, cy, s = w / 2, h / 2, size * direction
    ImageDraw.Draw(im).polygon([(cx + s, cy), (cx - s * 0.7, cy - size), (cx - s * 0.7, cy + size)], fill=color + (255,))
    return im


def scroll_track_sprite():
    h = ROW_H * VISIBLE_ROWS
    im = Image.new('RGBA', (6, h), (0, 0, 0, 0))
    ImageDraw.Draw(im).rectangle([1, 0, 4, h - 1], fill=(20, 14, 10, 160), outline=GOLD + (90,))
    return im


def scroll_thumb_sprite():
    im = Image.new('RGBA', (10, SCROLL_THUMB_H), (0, 0, 0, 0))
    ImageDraw.Draw(im).rectangle([0, 0, 9, SCROLL_THUMB_H - 1], fill=GOLD + (230,))
    return im


def desc_sprite():
    w, h = DESC[2] - DESC[0], DESC[3] - DESC[1]
    p = lacquer(w, h, 640)
    brass_line(ImageDraw.Draw(p), [6, 6, w - 7, h - 7], 2)
    return p


def sprites():
    out = {
        'Settings_Panel': panel_sprite(),
        'Settings_Header': header_sprite(),
        'Settings_Tab': tab_sprite(False),
        'Settings_Tab_Selected': tab_sprite(True),
        'Settings_RowBand': band_sprite(),
        'Settings_RowRule': dashed_rule(),
        'Settings_CategoryRule': category_rule(),
        'Settings_Arrow_Left': arrow_sprite(-1),
        'Settings_Arrow_Right': arrow_sprite(1),
        'Settings_ScrollTrack': scroll_track_sprite(),
        'Settings_ScrollThumb': scroll_thumb_sprite(),
        'Settings_Desc': desc_sprite(),
    }
    for key, glyph, _ in HINTS:
        out[f'Settings_Hint_{key}'] = hint_tag(hint_width(glyph), HINT_TAG_H, glyph)
    return out


def emit(out_dir=EMIT_DIR):
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {out_dir}')
    for name, image in sprites().items():
        write_sprite(out_dir, name, image)


# ---------------------------------------------------------------- 確認用の重ね絵 (プレハブと同じ位置に置く)
def paste_center(base, part, cx, cy):
    base.alpha_composite(part, (int(round(cx - part.width / 2)), int(round(cy - part.height / 2))))


def put_text(base, spec, s, color, path=BODY_FONT, align='la'):
    x, cy, px = spec
    anchor = {'la': 'la', 'ma': 'ma'}[align]
    text(base, (x, text_top(cy, px)), s, px, color + (255,), path, anchor=anchor)


def preview(base, selected_row=0):
    L = layout()
    parts = sprites()
    im = base.convert('RGBA')
    im.alpha_composite(Image.new('RGBA', im.size, (0, 0, 0, BACKDROP_BLEND)))

    for i, name in enumerate(PREVIEW_CATEGORIES):
        sel = i == 0
        cx, cy = L['tab']
        paste_center(im, parts['Settings_Tab_Selected' if sel else 'Settings_Tab'], cx, cy + i * TAB_PITCH)
        x, ty, px = L['tab_text']
        put_text(im, (x, ty + i * TAB_PITCH, px), name, CREAM if sel else CREAM_FADE)

    paste_center(im, parts['Settings_Panel'], *L['panel'])
    paste_center(im, parts['Settings_Header'], *L['header'])
    put_text(im, L['header_text'], HEADING, CREAM, KAISEI, 'ma')
    put_text(im, L['category_text'], PREVIEW_CATEGORIES[0], CREAM)
    paste_center(im, parts['Settings_CategoryRule'], *L['category_rule'])

    for r in range(VISIBLE_ROWS):
        dy = r * ROW_H
        paste_center(im, parts['Settings_RowRule'], L['row_rule'][0], L['row_rule'][1] + dy)
        if r >= len(PREVIEW_ROWS):
            continue
        label, value, _ = PREVIEW_ROWS[r]
        sel = r == selected_row
        if sel:
            paste_center(im, parts['Settings_RowBand'], L['row_band'][0], L['row_band'][1] + dy)
            paste_center(im, parts['Settings_Arrow_Left'], L['row_arrow_left'][0], L['row_arrow_left'][1] + dy)
            paste_center(im, parts['Settings_Arrow_Right'], L['row_arrow_right'][0], L['row_arrow_right'][1] + dy)
        col = CREAM if sel else CREAM_FADE
        x, cy, px = L['row_label']
        put_text(im, (x, cy + dy, px), label, col)
        x, cy, px = L['row_value']
        put_text(im, (x, cy + dy, px), value, col, align='ma')

    paste_center(im, parts['Settings_Desc'], *L['desc'])
    put_text(im, L['desc_text'], PREVIEW_ROWS[selected_row][2], CREAM)

    for h in L['hints']:
        paste_center(im, parts[f'Settings_Hint_{h["key"]}'], *h['tag'])
        put_text(im, h['text'], h['label'], (240, 226, 202))
    return im


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--emit', action='store_true')
    ap.add_argument('--shot')
    ap.add_argument('--out')
    args = ap.parse_args()
    if args.emit:
        emit()
    if args.shot:
        preview(load_base(args.shot)).convert('RGB').save(args.out)
        print(f'wrote {args.out}')


if __name__ == '__main__':
    main()
