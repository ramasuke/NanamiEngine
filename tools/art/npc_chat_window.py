"""NPC 会話ウィンドウのデザイン案を実画面に合成する。

    python tools/art/npc_chat_window.py --shot <screenshot.png> --out-dir <dir>              # 3案のモック
    python tools/art/npc_chat_window.py --emit [--preview --shot <png> --out-dir <dir>]     # 案3 の書き出し
    python tools/art/npc_chat_window_prefab.py                                              # ChattingUiScene を組み直す

案3「黒い板」に決定 (2026-09-24)。--emit は Assets/Art/UI/NpcChat/ に板と名札を書き出し、
--preview は prefab と同じ LAYOUT から描き直して実装とのずれを確かめる。

いまの会話ウィンドウは「半透明の肌色パネル + 紫の発光枠」で、甲板や空が透けて文字が埋もれる。
文字も細く小さく、名前と台詞の区別がない。3案とも次の点を直す:

- 地を不透明(または十分に暗く)して、背景の明暗に文字のコントラストを左右させない。
- 本文を 38〜40px に上げ、行間を広げる。名前は Kaisei Decol で本文と別の札に分ける。
- 台詞がいくつ続くかを小さな印で示す(会話は自動送りなので「次へ」のキー表示は無い)。

--shot には今の会話ウィンドウが写った画面を渡す。旧ウィンドウは周囲からの補間でぼかして消す
(ぼけは残るので、どの案も旧ウィンドウの範囲を覆うか暗くする)。

Requires Pillow + numpy.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from character_select import (  # noqa: E402
    BODY_FONT, INK, INK_FADE, REPO_ROOT, SCREEN_H, SCREEN_W,
    contact_sheet, drop_shadow, font, grid, nail, parchment, paste, paste_tilted, text, wood_board, write_sprite,
)

NAME_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'KaiseiDecol-Bold.ttf'

NPC_NAME = '旅のくノ一'
LINES = ['気をつけなよ。', '空から来るものは、待ってくれない。']
PAGE, PAGES = 1, 3          # いま何番目の台詞か / 台詞の数

OLD_REGION = (326, 722, 1510, 992)   # 旧ウィンドウ(枠の光まで含む)

EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'NpcChat'
NAIL_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'Nail.png'

# 案3 の配置 (1920x1080 の画面 px)。ImageRenderer は中心、TextRenderer は左上 (中央揃えなら上端の中央) が座標。
# 台詞は最長で 4行 x 21字 (2026-09-24 の .npcChat 全件) なので、38px なら 4行でも板に収まる
BOARD_W, BOARD_H = 1150, 216
SLIP_W, SLIP_H = 300, 64       # 名前は 7字まで (32px x 7 = 224) を想定した固定幅
SLIP_ANGLE = -2.0
LAYOUT = {
    'board': (920, 868),
    'body': ((429, 798), 38),
    'slip': (543, 756),
    'name': ((543, 740), 32),
    'nail': (407, 734),
    'pages': ((1451, 932), 22),   # 右揃え
}
BODY_COLOR = (255, 255, 255)        # .npcChat の textColor_ (白) がそのまま使われる。モックの生成りの代わり
NAME_COLOR = INK
PAGE_COLOR = (226, 188, 118)


# ---------------------------------------------------------------- 実画面の下ごしらえ
def remove_old_window(base):
    """旧ウィンドウの範囲を周囲から拡散させて埋める(半透明の逆合成は空が白飛びしたのでやめた)"""
    a = np.asarray(base.convert('RGB'), np.float32)
    m = np.zeros(a.shape[:2], bool)
    m[OLD_REGION[1]:OLD_REGION[3], OLD_REGION[0]:OLD_REGION[2]] = True
    # 粗い解像度で埋めてから細かくすると、少ない反復で中央まで色が届く
    small = Image.fromarray(a.astype(np.uint8)).resize((SCREEN_W // 8, SCREEN_H // 8), Image.BOX)
    ms = np.asarray(Image.fromarray(m.astype(np.uint8) * 255).resize(small.size, Image.NEAREST)) > 0
    cur = diffuse(np.asarray(small, np.float32), ms, 400)
    cur = np.asarray(Image.fromarray(cur.astype(np.uint8)).resize((SCREEN_W, SCREEN_H), Image.BICUBIC), np.float32)
    cur = diffuse(np.where(m[..., None], cur, a), m, 200, keep=a)
    cur = cur + np.random.default_rng(1).normal(0, 5, cur.shape) * m[..., None]
    return Image.fromarray(np.clip(cur, 0, 255).astype(np.uint8), 'RGB').convert('RGBA')


def diffuse(img, m, iters, keep=None):
    keep = img if keep is None else keep
    cur = img.copy()
    for _ in range(iters):
        p = np.pad(cur, ((1, 1), (1, 1), (0, 0)), mode='edge')
        avg = (p[:-2, 1:-1] + p[2:, 1:-1] + p[1:-1, :-2] + p[1:-1, 2:]) / 4
        cur = np.where(m[..., None], avg, keep)
    return cur


def bottom_shade(base, top, strength, warm=False):
    """画面下を縦グラデーションで暗くする。台詞の背後の明暗差を均す"""
    arr = np.asarray(base.convert('RGB'), np.float32) / 255.0
    _, yy = grid(SCREEN_W, SCREEN_H)
    t = np.clip((yy - top) / (SCREEN_H - top), 0, 1) ** 1.3 * strength
    arr = arr * (1 - t[..., None])
    if warm:
        arr = arr * (1 - 0.25 * t[..., None] * np.array([0.0, 0.12, 0.30], np.float32))
    return Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')


def page_pips(base, x, y, color, empty, r=6, gap=22):
    """台詞の数と現在位置。済んだものは塗り、いまのものは大きく、残りは輪"""
    d = ImageDraw.Draw(base)
    for i in range(PAGES):
        cx = x + i * gap
        rr = r + 2 if i == PAGE - 1 else r
        box = [cx - rr, y - rr, cx + rr, y + rr]
        if i < PAGE:
            d.ellipse(box, fill=color)
        else:
            d.ellipse(box, outline=empty, width=2)


def body_lines(base, x, y, color, px, lead, shadow=None):
    for i, line in enumerate(LINES):
        text(base, (x, y + i * lead), line, px, color, BODY_FONT, shadow=shadow)


# ---------------------------------------------------------------- 案1「書付」羊皮紙の帯 + 木の名札
def name_tag(label, px=34, pad_x=30, h=62):
    w = int(font(NAME_FONT, px).getlength(label)) + pad_x * 2
    board = wood_board(w, h, 140, plank=9999)
    ImageDraw.Draw(board).rectangle([3, 3, w - 4, h - 4], outline=(176, 136, 74, 220), width=2)
    text(board, (w / 2, h / 2 + 1), label, px, (250, 228, 186, 255), NAME_FONT, anchor='mm',
         shadow=(2, 2, (0, 0, 0, 200)))
    return board


def mock_note(base):
    im = bottom_shade(base, 560, 0.62, warm=True)
    w, h = 1180, 238
    cx, cy = 918, 866
    paper = parchment(w, h, 410, aged=0.12, ragged=11.0)
    body_lines(paper, 92, 62, (*INK, 255), 40, 62)
    page_pips(paper, w - 110, h - 40, (*INK, 255), (*INK_FADE, 255))
    paste_tilted(im, paper, cx, cy, -0.5)

    tag = name_tag(NPC_NAME)
    paste_tilted(im, tag, cx - w / 2 + 40 + tag.width / 2, cy - h / 2 - 8, 1.6)
    tx = cx - w / 2 + 40
    paste(im, nail(18), tx + 8, cy - h / 2 - 34)
    paste(im, nail(18), tx + tag.width - 26, cy - h / 2 - 30)
    return im


# ---------------------------------------------------------------- 案2「字幕」暗い帯に生成りの文字
def mock_subtitle(base):
    im = bottom_shade(base, 640, 0.86)
    x, y = 470, 812
    text(im, (x, y), NPC_NAME, 34, (232, 194, 122, 255), NAME_FONT, anchor='ls',
         shadow=(2, 2, (0, 0, 0, 220)))
    nw = font(NAME_FONT, 34).getlength(NPC_NAME)
    d = ImageDraw.Draw(im)
    d.line([(x + nw + 22, y - 12), (x + 880, y - 12)], fill=(150, 112, 58, 200), width=2)
    body_lines(im, x, y + 26, (240, 226, 202, 255), 40, 62, shadow=(2, 3, (0, 0, 0, 230)))
    page_pips(im, x + 900, y - 12, (232, 194, 122, 255), (150, 112, 58, 255))
    return im


# ---------------------------------------------------------------- 案3「黒い板」炭色の板に生成り + 紙の名札
def dark_board(w, h):
    board = wood_board(w, h, 520, plank=9999)
    arr = np.asarray(board, np.float32) / 255.0
    arr[..., :3] *= 0.42
    board = Image.fromarray((arr * 255).astype(np.uint8), 'RGBA')
    d = ImageDraw.Draw(board)
    d.rectangle([6, 6, w - 7, h - 7], outline=(126, 92, 46, 235), width=4)
    d.rectangle([14, 14, w - 15, h - 15], outline=(70, 50, 28, 200), width=1)
    return board


def paper_slip(label, px=32):
    w = int(font(NAME_FONT, px).getlength(label)) + 64
    h = 64
    slip = parchment(w, h, 433, aged=0.18, ragged=7.0)
    text(slip, (w / 2, h / 2 + 1), label, px, (*INK, 255), NAME_FONT, anchor='mm')
    return slip


def mock_board(base):
    im = bottom_shade(base, 600, 0.45)
    w, h = 1150, 214
    cx, cy = 918, 872
    board = dark_board(w, h)
    body_lines(board, 84, 58, (240, 226, 202, 255), 40, 62, shadow=(2, 2, (0, 0, 0, 200)))
    page_pips(board, w - 104, h - 36, (226, 188, 118, 255), (126, 92, 46, 255))
    paste_tilted(im, board, cx, cy, 0.0)

    slip = paper_slip(NPC_NAME)
    sx = cx - w / 2 + 48 + slip.width / 2
    paste_tilted(im, slip, sx, cy - h / 2 - 4, -2.0)
    paste(im, nail(18), sx - slip.width / 2 + 8, cy - h / 2 - 28)
    return im


# ---------------------------------------------------------------- 書き出し (案3)
def with_shadow(part, angle=0.0):
    """影ごと 1 枚に焼く。ImageRenderer は中心で置くので、部品が画像の中心に来るようにする"""
    sh = drop_shadow(part)
    pad = (sh.width - part.width) // 2
    out = Image.new('RGBA', sh.size, (0, 0, 0, 0))
    out.alpha_composite(sh)
    out.alpha_composite(part, (pad, pad))
    return out.rotate(angle, resample=Image.BICUBIC, expand=True) if angle else out


def emit_sprites():
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {EMIT_DIR}')
    write_sprite(EMIT_DIR, 'Chat_Board', with_shadow(dark_board(BOARD_W, BOARD_H)))
    write_sprite(EMIT_DIR, 'Chat_NameSlip', with_shadow(parchment(SLIP_W, SLIP_H, 433, aged=0.18, ragged=7.0),
                                                        SLIP_ANGLE))


def preview(base):
    """prefab と同じ LAYOUT で、書き出したスプライトとエンジンの文字の置き方を真似て描く"""
    im = base.copy()

    def put(sprite_path, center):
        sp = Image.open(sprite_path).convert('RGBA')
        paste(im, sp, center[0] - sp.width / 2, center[1] - sp.height / 2)

    put(EMIT_DIR / 'Chat_Board.png', LAYOUT['board'])
    put(EMIT_DIR / 'Chat_NameSlip.png', LAYOUT['slip'])
    put(NAIL_SPRITE, LAYOUT['nail'])
    (x, y), px = LAYOUT['body']
    for i, line in enumerate(LINES):
        text(im, (x, y + i * px), line, px, (*BODY_COLOR, 255), BODY_FONT)
    (x, y), px = LAYOUT['name']
    text(im, (x, y), NPC_NAME, px, (*NAME_COLOR, 255), BODY_FONT, anchor='ma')
    (x, y), px = LAYOUT['pages']
    marks = ''.join('●' if i < PAGE else '○' for i in range(PAGES))
    text(im, (x, y), marks, px, (*PAGE_COLOR, 255), BODY_FONT, anchor='ra')
    return im


# ---------------------------------------------------------------- メイン
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot', default='')
    ap.add_argument('--out-dir', default='')
    ap.add_argument('--emit', action='store_true')
    ap.add_argument('--preview', action='store_true')
    args = ap.parse_args()
    if args.emit:
        emit_sprites()
        if not args.preview:
            return
    if not args.shot or not args.out_dir:
        ap.error('--shot and --out-dir are required for mocks / --preview')
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)

    shot = Image.open(args.shot).convert('RGBA').resize((SCREEN_W, SCREEN_H), Image.LANCZOS)
    clean = remove_old_window(shot)
    if args.preview:
        preview(clean).convert('RGB').save(out / 'chat_preview.png')
        print(f'wrote {out / "chat_preview.png"}')
        return
    mocks = [
        ('0_current', shot, '現状'),
        ('1_note', mock_note(clean), '案1  書付 — 羊皮紙の帯 + 木の名札'),
        ('2_subtitle', mock_subtitle(clean), '案2  字幕 — 暗い帯に生成りの文字'),
        ('3_board', mock_board(clean), '案3  黒い板 — 炭色の板に生成り + 紙の名札'),
    ]
    for name, im, _ in mocks:
        im.convert('RGB').save(out / f'chat_{name}.png')
    contact_sheet([m[1] for m in mocks], [m[2] for m in mocks]).convert('RGB').save(out / 'chat_sheet.png')
    print(f'wrote {len(mocks)} mocks + sheet to {out}')


if __name__ == '__main__':
    main()
