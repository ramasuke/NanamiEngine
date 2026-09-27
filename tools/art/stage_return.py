"""ステージから島へ帰る確認札 (ESC) のスプライトと、実際のゲーム画面に合成したモックを生成する。

    python tools/art/stage_return.py --emit                                   # Assets/Art/UI/StageReturn へ書き出す
    python tools/art/stage_return.py --preview --shot <png> --out-dir <dir>   # 書き出した絵と LAYOUT で完成イメージ
    python tools/art/stage_return.py --shot <screenshot.png> --out-dir <dir>  # 最初の3案

案A「掲示板の貼り紙」に決まった。板・紙・釘・見出しの罫は絵に焼き、文字は TextRenderer で水平に書く。
配置の数値は LAYOUT / layout() にまとめてあり、tools/art/stage_return_prefab.py がそのまま使う。

最初の3案。どれも系統 A (手で触れる物)、背景のゲーム画面は暗くする。
  A: 釘で留めた掲示板の貼り紙。選んでいる行に朱の判子
  B: 蝋封の書付。選択肢は墨書き、選んでいる行に蝋の印
  C: 画面下寄りの小さな鉄札。戦いを隠しすぎない
ホストが帰ると仲間のセッションも切れるので、その断り書き (朱) を載せた版も出す。

Requires Pillow + numpy.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

from tools.art.character_select import (  # noqa: E402
    BODY_FONT, CANDLE, INK, INK_FADE, STAMP_RED, SCREEN_W, SCREEN_H,
    drop_shadow, engrave, font, grid, hint_tag, iron_plate, nail, parchment, paste, paste_tilted,
    stamp_sprite, text, wax_seal, wood_board,
)

TITLE_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'KaiseiDecol-Bold.ttf'
HINT_COLOR = (240, 226, 202, 255)
HOST_NOTE = '仲間との旅も、ここで終わる'
CHOICES = ['島へ帰る', 'まだ残る']
SELECTED = 1  # 誤って押しても帰らないよう、開いたときは「まだ残る」


# ---------------------------------------------------------------- 下ごしらえ
def load_base(shot_path, crop):
    im = Image.open(shot_path).convert('RGBA')
    if crop:
        im = im.crop(crop)
    return im.resize((SCREEN_W, SCREEN_H), Image.LANCZOS)


def dim(base, darken=0.55, blur=4.0, vignette=0.5):
    im = base.convert('RGB').filter(ImageFilter.GaussianBlur(blur))
    arr = np.asarray(im, np.float32) / 255.0
    xx, yy = grid(SCREEN_W, SCREEN_H)
    r = np.hypot((xx - SCREEN_W / 2) / (SCREEN_W / 2), (yy - SCREEN_H / 2) / (SCREEN_H / 2))
    vig = np.clip(r / 1.25, 0, 1) ** 1.7
    arr = arr * (1 - darken) * (1 - vignette * vig)[..., None]
    arr = arr + CANDLE[None, None, :] * 0.05 * (1 - vig)[..., None]
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')


def hint_strip(base, right_x, y, pairs=(('▲▼', '選ぶ'), ('Enter', '決める'), ('Esc', 'やめる'))):
    x = right_x
    for glyph, label in reversed(pairs):
        text(base, (x, y), label, 26, HINT_COLOR, BODY_FONT, anchor='rm', shadow=(2, 2, (0, 0, 0, 210)))
        x -= font(BODY_FONT, 26).getlength(label) + 16
        gw = 46 if len(glyph) == 1 else 68
        paste(base, hint_tag(gw, 42, glyph), x - gw, y - 21)
        x -= gw + 30


def spaced(s):
    return '　'.join(s)


# ---------------------------------------------------------------- 案A 掲示板の貼り紙
def mock_board(base, host):
    im = dim(base)
    bw, bh = 700, 470 if host else 420
    board = wood_board(bw, bh, 311, plank=9999)
    d = ImageDraw.Draw(board)
    d.rectangle([5, 5, bw - 6, bh - 6], outline=(126, 88, 42, 235), width=5)
    paste_tilted(im, board, 960, 520, 0.8)

    pw, ph = 600, 370 if host else 320
    paper = parchment(pw, ph, 322, aged=0.08)
    pd = ImageDraw.Draw(paper)
    text(paper, (pw / 2, 58), spaced('帰還'), 46, INK, TITLE_FONT, anchor='mm')
    pd.line([(56, 98), (pw - 56, 98)], fill=INK_FADE, width=2)
    text(paper, (pw / 2, 138), '浮遊石に乗って、島へ引き返しますか。', 26, INK, BODY_FONT, anchor='mm')
    y0 = 200
    for i, label in enumerate(CHOICES):
        y = y0 + i * 58
        text(paper, (pw / 2, y), label, 34, INK if i == SELECTED else INK_FADE, BODY_FONT, anchor='mm')
        if i == SELECTED:
            pd.line([(pw / 2 - 90, y + 24), (pw / 2 + 90, y + 24)], fill=INK, width=3)
    if host:
        text(paper, (pw / 2, y0 + 58 * 2 + 16), HOST_NOTE, 24, STAMP_RED, BODY_FONT, anchor='mm')
    rot = paper.rotate(-1.6, resample=Image.BICUBIC, expand=True)
    sh = drop_shadow(rot, blur_r=8, offset=(4, 7), alpha=0.55)
    cx, cy = 960, 520
    im.alpha_composite(sh, (int(cx - sh.width / 2), int(cy - sh.height / 2)))
    im.alpha_composite(rot, (int(cx - rot.width / 2), int(cy - rot.height / 2)))
    paste(im, nail(24), cx - 12, cy - ph / 2 + 6)

    stamp = stamp_sprite('残', px=54, angle=-12)
    sel_y = cy - ph / 2 + y0 + SELECTED * 58
    paste(im, stamp, cx + 130, sel_y - stamp.height / 2)
    hint_strip(im, 1860, 1030)
    return im


# ---------------------------------------------------------------- 案B 蝋封の書付
def mock_letter(base, host):
    im = dim(base, darken=0.6)
    pw, ph = 520, 600 if host else 560
    paper = parchment(pw, ph, 341, aged=0.18, ragged=16)
    pd = ImageDraw.Draw(paper)
    text(paper, (pw / 2, 110), spaced('島へ帰る'), 44, INK, TITLE_FONT, anchor='mm')
    for j, line in enumerate(['狩りを切り上げて、', '浮遊石で島へ戻るか。']):
        text(paper, (pw / 2, 190 + j * 40), line, 27, INK, BODY_FONT, anchor='mm')
    pd.line([(80, 262), (pw - 80, 262)], fill=INK_FADE, width=2)
    y0 = 320
    for i, label in enumerate(CHOICES):
        y = y0 + i * 70
        col = INK if i == SELECTED else INK_FADE
        text(paper, (pw / 2 + 20, y), label, 36, col, BODY_FONT, anchor='mm')
    if host:
        text(paper, (pw / 2, y0 + 70 * 2 + 10), HOST_NOTE, 24, STAMP_RED, BODY_FONT, anchor='mm')
    cx, cy = 960, 520
    paste_tilted(im, paper, cx, cy, 1.4)
    seal = wax_seal(84, 9)
    paste(im, drop_shadow(seal, blur_r=6, offset=(3, 5), alpha=0.5), cx - 42 - 18, cy - ph / 2 - 30 - 18)
    paste(im, seal, cx - 42, cy - ph / 2 - 30)
    dot = wax_seal(40, 13)
    sel_y = cy - ph / 2 + y0 + SELECTED * 70
    paste(im, dot, cx - 120, sel_y - 20)
    hint_strip(im, 1860, 1030)
    return im


# ---------------------------------------------------------------- 案C 画面下の小さな鉄札
def mock_plate(base, host):
    im = dim(base, darken=0.35, blur=0.0, vignette=0.35)
    # 下側だけ少し落として札を読ませる
    arr = np.asarray(im, np.float32)
    ramp = np.clip((np.arange(SCREEN_H) - 560) / 360, 0, 1)[:, None, None] * 0.45
    arr[..., :3] *= (1 - ramp)
    im = Image.fromarray(arr.astype(np.uint8), 'RGBA')

    cx, cy = 960, 820
    head = iron_plate(620, 76, seed=51, notch=30)
    engrave(head, (310, 38), '島 へ 帰 る か', 34, TITLE_FONT, anchor='mm')
    paste(im, drop_shadow(head), cx - 310 - 30, cy - 130 - 30)
    paste(im, head, cx - 310, cy - 130)
    for i, label in enumerate(CHOICES):
        x = cx - 150 + i * 300
        sel = i == SELECTED
        pw, ph = (264, 70) if sel else (240, 62)
        plate = iron_plate(pw, ph, seed=60 + i, notch=18)
        engrave(plate, (pw / 2, ph / 2), label, 32 if sel else 28, BODY_FONT, anchor='mm')
        if not sel:
            a = np.asarray(plate, np.float32)
            a[..., :3] *= 0.62
            plate = Image.fromarray(a.astype(np.uint8), 'RGBA')
        else:
            glow = Image.new('RGBA', (pw + 60, ph + 60), (0, 0, 0, 0))
            ImageDraw.Draw(glow).rounded_rectangle([22, 22, pw + 38, ph + 38], 18, outline=(255, 138, 44, 255), width=8)
            paste(im, glow.filter(ImageFilter.GaussianBlur(9)), x - pw / 2 - 30, cy - 20 - 30)
        paste(im, drop_shadow(plate), x - pw / 2 - 30, cy - 20 - 30)
        paste(im, plate, x - pw / 2, cy - 20)
    if host:
        text(im, (cx, cy + 90), HOST_NOTE, 24, (214, 120, 104, 255), BODY_FONT, anchor='mm',
             shadow=(2, 2, (0, 0, 0, 230)))
    hint_strip(im, 1860, 1030)
    return im


# ---------------------------------------------------------------- 案A の書き出し
EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'StageReturn'
BACKDROP_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Backdrop.png'   # 960x540 を 2 倍で敷く
BLACK_MASK_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'BlackMask.png'

NOTICE_CX = 960
PAPER_TOP = 360          # 貼り紙の上端。ホスト版は下へ伸びるだけで、上の文字の位置は変えない
PAPER_W = 600
PAPER_H = {'solo': 320, 'host': 370}
BOARD_PAD = (50, 50)     # 板は紙より左右上下にこれだけ大きい
BOARD_ANGLE = 0.8
PAPER_ANGLE = -1.6
ROW_GAP = 58
UNDERLINE_W = 180
HINT_ITEMS = [('UpDown', '▲▼', '選ぶ'), ('Enter', 'Enter', '決める'), ('Esc', 'Esc', 'やめる')]
HINT_RIGHT, HINT_Y = 1860, 1030
HINT_TAG_W, HINT_TAG_H, HINT_LABEL_PX = 68, 42, 26

LAYOUT = {
    'veil_black': ((SCREEN_W / 2, SCREEN_H / 2), 0.47, 90),   # BlackMask の中心・倍率・濃さ
    'veil': ((SCREEN_W / 2, SCREEN_H / 2), 2.0, 200),         # Backdrop の中心・倍率・濃さ
    'title': ((NOTICE_CX, PAPER_TOP + 58), 46),               # 文字はどれも (中心, px)
    'body': ((NOTICE_CX, PAPER_TOP + 138), 26),
    'rows': [PAPER_TOP + 200, PAPER_TOP + 200 + ROW_GAP],     # 行の中心の y。[帰る, 残る]
    'row_px': 34,
    'row_button': (300, 50),                                  # 行ごとのクリック範囲 (幅, 高さ)
    'underline_dy': 24,
    'stamp_dx': 170,                                          # 行の中心から判子の中心まで
    'host_note': ((NOTICE_CX, PAPER_TOP + 200 + ROW_GAP * 2 + 16), 24),
    'drop_px': 40,
}
BODY_TEXT = '浮遊石に乗って、島へ引き返しますか。'
TITLE_TEXT = spaced('帰還')
STAMP_LABELS = ['帰', '残']


def text_top(center_y, px):
    """TextRenderer の位置は文字の上辺。モックの中心から上辺へ直す (game_over の label_offset と同じ比)"""
    return center_y - px * 0.55


def notice_center(kind):
    return NOTICE_CX, PAPER_TOP + PAPER_H[kind] / 2


def notice_sprite(kind):
    """板・紙・釘・見出しの罫と影を焼いた絵。絵の中心 = notice_center(kind)"""
    ph = PAPER_H[kind]
    bw, bh = PAPER_W + BOARD_PAD[0] * 2, ph + BOARD_PAD[1] * 2
    cw, ch = bw + 120, bh + 120
    canvas = Image.new('RGBA', (cw, ch), (0, 0, 0, 0))
    board = wood_board(bw, bh, 311, plank=9999)
    ImageDraw.Draw(board).rectangle([5, 5, bw - 6, bh - 6], outline=(126, 88, 42, 235), width=5)
    paste_tilted(canvas, board, cw / 2, ch / 2, BOARD_ANGLE)

    paper = parchment(PAPER_W, ph, 322, aged=0.08)
    ImageDraw.Draw(paper).line([(56, 98), (PAPER_W - 56, 98)], fill=INK_FADE, width=2)
    rot = paper.rotate(PAPER_ANGLE, resample=Image.BICUBIC, expand=True)
    sh = drop_shadow(rot, blur_r=8, offset=(4, 7), alpha=0.55)
    canvas.alpha_composite(sh, (int(cw / 2 - sh.width / 2), int(ch / 2 - sh.height / 2)))
    canvas.alpha_composite(rot, (int(cw / 2 - rot.width / 2), int(ch / 2 - rot.height / 2)))
    paste(canvas, nail(24), cw / 2 - 12, ch / 2 - ph / 2 + 6)
    return canvas


def underline_sprite():
    im = Image.new('RGBA', (UNDERLINE_W + 8, 9), (0, 0, 0, 0))
    ImageDraw.Draw(im).line([(4, 4), (UNDERLINE_W + 4, 5)], fill=(*INK, 235), width=3)
    return im


def layout():
    """LAYOUT に、文字幅から決まる操作ヒントの位置を足したもの"""
    L = dict(LAYOUT)
    hints = []
    x = HINT_RIGHT
    f = font(BODY_FONT, HINT_LABEL_PX)
    for key, glyph, label in reversed(HINT_ITEMS):
        label_left = x - f.getlength(label)
        tag_cx = label_left - 16 - HINT_TAG_W / 2
        tag_left = tag_cx - HINT_TAG_W / 2
        hints.append({'key': key, 'glyph': glyph, 'label': label,
                      'tag': (tag_cx, HINT_Y), 'label_pos': ((label_left, HINT_Y), HINT_LABEL_PX),
                      'button': (((tag_left + x) / 2, HINT_Y), (x - tag_left, 52))})
        x = tag_left - 30
    L['hints'] = list(reversed(hints))
    return L


def write_sprite(out_dir, name, image):
    from tools.scene import sprite_meta
    png = out_dir / f'{name}.png'
    image.save(png)
    meta = out_dir / f'{name}.png.meta'
    guid = sprite_meta.read_meta(meta)['guid'] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    print(f'  {name}.png  {image.size[0]}x{image.size[1]}  guid={guid}')


def stamp_name(index):
    return f'StageReturn_Stamp_{"Return" if index == 0 else "Stay"}'


def sprites():
    out = {
        'StageReturn_Notice_Solo': notice_sprite('solo'),
        'StageReturn_Notice_Host': notice_sprite('host'),
        'StageReturn_Underline': underline_sprite(),
    }
    for i, label in enumerate(STAMP_LABELS):
        out[stamp_name(i)] = stamp_sprite(label, px=54, angle=-12)
    for key, glyph, _ in HINT_ITEMS:
        out[f'StageReturn_Hint_{key}'] = hint_tag(HINT_TAG_W, HINT_TAG_H, glyph)
    return out


def emit(out_dir=EMIT_DIR):
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {out_dir}')
    for name, image in sprites().items():
        write_sprite(out_dir, name, image)


def paste_center(base, part, cx, cy):
    base.alpha_composite(part, (int(round(cx - part.width / 2)), int(round(cy - part.height / 2))))


def put_text(base, pos_px, s, color, path=BODY_FONT, anchor='ma'):
    """TextRenderer と同じく (x, 上辺) に置く"""
    (x, cy), px = pos_px
    text(base, (x, text_top(cy, px)), s, px, color if len(color) == 4 else (*color, 255), path, anchor=anchor)


def preview(base, host, selected=SELECTED, sprite_dir=EMIT_DIR):
    """書き出した絵と LAYOUT だけで描く。エンジンは背景をぼかせないので、幕を敷くだけにする"""
    L = layout()

    def load(name):
        return Image.open(sprite_dir / f'{name}.png').convert('RGBA')

    im = base.copy()
    for key, sprite in (('veil_black', BLACK_MASK_SPRITE), ('veil', BACKDROP_SPRITE)):
        (cx, cy), scale, rate = L[key]
        veil = Image.open(sprite).convert('RGBA')
        veil = veil.resize((int(veil.width * scale), int(veil.height * scale)), Image.LANCZOS)
        veil.putalpha(veil.getchannel('A').point(lambda v: v * rate // 255))
        paste_center(im, veil, cx, cy)

    kind = 'host' if host else 'solo'
    paste_center(im, load(f'StageReturn_Notice_{kind.capitalize()}'), *notice_center(kind))
    put_text(im, L['title'], TITLE_TEXT, INK, TITLE_FONT)
    put_text(im, L['body'], BODY_TEXT, INK)
    for i, (y, label) in enumerate(zip(L['rows'], CHOICES)):
        put_text(im, ((NOTICE_CX, y), L['row_px']), label, INK if i == selected else INK_FADE)
    sel_y = L['rows'][selected]
    paste_center(im, load('StageReturn_Underline'), NOTICE_CX, sel_y + L['underline_dy'])
    paste_center(im, load(stamp_name(selected)), NOTICE_CX + L['stamp_dx'], sel_y)
    if host:
        put_text(im, L['host_note'], HOST_NOTE, STAMP_RED)
    for h in L['hints']:
        paste_center(im, load(f'StageReturn_Hint_{h["key"]}'), *h['tag'])
        put_text(im, h['label_pos'], h['label'], HINT_COLOR, anchor='la')
    return im


def contact_sheet(mocks, labels):
    tw, th = 960, 540
    sheet = Image.new('RGB', (tw * 2, th * ((len(mocks) + 1) // 2)), (18, 14, 12))
    for i, (m, label) in enumerate(zip(mocks, labels)):
        t = m.convert('RGB').resize((tw, th), Image.LANCZOS)
        ImageDraw.Draw(t).text((16, 12), label, font=font(BODY_FONT, 30), fill=(255, 240, 210))
        sheet.paste(t, ((i % 2) * tw, (i // 2) * th))
    return sheet


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--emit', action='store_true', help='Assets/Art/UI/StageReturn へスプライトを書き出す')
    ap.add_argument('--preview', action='store_true', help='書き出した絵と LAYOUT で完成イメージを描く')
    ap.add_argument('--shot', default='')
    ap.add_argument('--crop', default='', help='x0,y0,x1,y1 (エディタの窓を外すとき)')
    ap.add_argument('--out-dir', default='')
    args = ap.parse_args()

    if args.emit:
        emit()
    if not args.shot:
        return
    if not args.out_dir:
        raise SystemExit('--out-dir is required with --shot')

    crop = tuple(int(v) for v in args.crop.split(',')) if args.crop else None
    base = load_base(args.shot, crop)
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)

    if args.preview:
        shots, labels = [], []
        for host in (False, True):
            for selected in (1, 0):
                m = preview(base, host, selected)
                m.convert('RGB').save(out / f'stage_return_preview{"_host" if host else ""}_{selected}.png')
                shots.append(m)
                labels.append(CHOICES[selected] + ('（ホスト）' if host else ''))
        contact_sheet(shots, labels).save(out / 'stage_return_preview_sheet.png')
        print(f'wrote -> {out}')
        return

    mocks, labels = [], []
    for key, fn, name in [('A', mock_board, '掲示板の貼り紙'), ('B', mock_letter, '蝋封の書付'), ('C', mock_plate, '下の鉄札')]:
        for host in (False, True):
            m = fn(base, host)
            suffix = '_host' if host else ''
            m.convert('RGB').save(out / f'stage_return_{key}{suffix}.png')
            mocks.append(m)
            labels.append(f'{key}: {name}' + ('（ホスト）' if host else ''))
    contact_sheet(mocks, labels).save(out / 'stage_return_sheet.png')
    print(f'wrote -> {out}')


if __name__ == '__main__':
    main()
