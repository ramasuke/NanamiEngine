"""ステージ選択の「部屋の入り方」(相席 / 部屋を作る / 番号で入る) のデザイン案を、ステージ選択画面に合成して出す。

    python tools/art/room_code.py --shot <stage_select_screenshot.png> --out-dir <dir>   # 3案 x 3状態のモック

中継サーバーの非公開部屋 (RelayRoom::Create / Join) を選ぶ UI。置き場所は右の「選択中のステージ」パネルのすぐ下に
敷く同じ地の板 (パネルの中はフレーバー文で埋まる)。出発するボタンはその下へ下げる。ステージ選択は docs/UIDesign.md の系統 A/B のどちらでもない青と金の画面だが、
パネルの中に入れるので、この画面の語彙 (暗い紺の半透明・金のラベル・白い明朝・青い光) に揃える。

案A 三つの札    ★の下に「相席 / 部屋を作る / 番号で入る」の札を横に並べ、←→ で選ぶ。番号は札の下に 6 桁の枠
案B 切り替え行  「部屋  ◀ 相席する ▶」の一行と、その下に説明 (番号で入るときは 6 桁の枠) だけ。一番小さい
案C 出発の確認  出発するを押すとパネルの絵の上に暗い幕を下ろし、三つを縦に並べて選ぶ。番号はその場で入れる

採用は **案B**。--emit で Assets/Art/UI/StageSelect/Room/ に部品を書き出し、room_code_prefab.py が同じ LAYOUT を
読んで StageSelectUI.prefab に組み込む。LAYOUT は画面上の px (ワールド座標) で、prefab 側で親の拡縮を割り戻す。

状態: 相席 (既定) / 部屋を作る / 番号で入る (3 桁まで入れて 4 桁目にカーソル)

Requires Pillow + numpy.
"""
import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))

from character_select import BODY_FONT, font, write_sprite  # noqa: E402

EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'StageSelect' / 'Room'

# 採用案 (案B) の画面上の座標 (= ワールド座標)。room_code_prefab.py が親 (Chrome は縦を 1/1.1 に縮める) の
# 拡縮で割ってローカルに直すので、ここは見た目どおりの px で書く。
# 「選択中のステージ」パネル (y 179-807) の中はフレーバー文 2 行で埋まるので、パネルのすぐ下に同じ地の板を
# 敷いてそこに置き、出発するボタンはその下へ下げる
LAYOUT = {
    'plate': (1550, 887),                # 板の中心 (パネルと同じ幅、y 818-956)
    'label': ((1284, 834), 22),          # 「部 屋」 左寄せ (文字は上端が座標)
    'arrow_left': (1396, 846),           # ◀ の中心
    'arrow_right': (1644, 846),          # ▶ の中心
    'mode': ((1520, 831), 30),           # 相席する / 部屋を作る / 番号で入る (中央寄せ)
    'rule': (1550, 872),                 # 区切り線の中心
    'note': ((1284, 884), 19),           # 説明 (左寄せ)
    'digits': (1300, 904),               # 1 桁目の枠の中心
    'digit_pitch': 38,                   # 枠の間隔
    'digit_group_gap': 14,               # 3 桁目と 4 桁目の間
    'digit_text_px': 30,
    'hint_right': 1830,                  # 操作ヒントの右端
    'hint_y': 930,
    'depart_button_y': 1010,             # 出発するボタンの中心 (板の下)
}
PLATE = (600, 138)
# 着いたあと画面の隅に出す帯 (系統B の HUD: 暗い青緑の地に青緑のアクセント)
HUD_PLATE = (250, 56)
HUD_BG = (6, 20, 26)
HUD_ACCENT = (56, 214, 196)
HUD_LAYOUT = {
    'plate': (1780, 56),          # 帯の中心 (画面右上)
    'label': ((1672, 40), 18),    # 「部 屋 番 号」
    'code': ((1672, 60), 30),     # 482 913
}

DIGIT_BOX = (32, 40)
ARROW = (26, 26)
RULE = (536, 2)

GOLD = (222, 184, 108)
WHITE = (240, 244, 250)
DIM = (140, 152, 168)
BLUE = (40, 168, 250)
NAVY = (14, 26, 38)

# 右の「選択中のステージ」パネル (スクリーンショット上の座標)
PANEL = (1252, 182, 1848, 805)
MODES = ['相席', '部屋を作る', '番号で入る']
MODE_LONG = ['相席する', '部屋を作る', '番号で入る']
MODE_NOTE = [
    '同じステージにいる誰かと一緒に行きます',
    '着いたら部屋の番号が出ます。仲間に伝えてください',
    '仲間から聞いた 6 桁の番号を入れてください',
]
CODE_TYPED = '482'

# ゲーム内の文言と桁数 (StageSelectRoomUi のインスペクタに入る値。room_code_prefab.py が書き込む)
CODE_LENGTH = 6
MODE_HINTS = [
    'LB RB  部屋',
    'LB RB  部屋',
    'LB RB  部屋　▲▼  数字　←→  桁',
]
CODE_INCOMPLETE_HINT = f'番号を {CODE_LENGTH} 桁 入れてください'


def txt(draw, xy, s, px, color, anchor='la', shadow=True):
    f = font(BODY_FONT, px)
    if shadow:
        draw.text((xy[0] + 2, xy[1] + 2), s, font=f, fill=(0, 0, 0, 170), anchor=anchor)
    draw.text(xy, s, font=f, fill=color, anchor=anchor)


def glow_rect(im, box, color=BLUE, radius=6, width=2, blur=6, alpha=200):
    layer = Image.new('RGBA', im.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    d.rounded_rectangle(box, radius, outline=color + (alpha,), width=width + 3)
    layer = layer.filter(ImageFilter.GaussianBlur(blur))
    im.alpha_composite(layer)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle(box, radius, outline=color + (255,), width=width)


def chip(im, box, label, selected):
    d = ImageDraw.Draw(im)
    x0, y0, x1, y1 = box
    # 斜めに削った札 (出発するボタン・ステージ札と同じ六角の語彙)
    cut = (y1 - y0) // 2
    poly = [(x0 + cut, y0), (x1 - cut, y0), (x1, (y0 + y1) // 2), (x1 - cut, y1), (x0 + cut, y1), (x0, (y0 + y1) // 2)]
    layer = Image.new('RGBA', im.size, (0, 0, 0, 0))
    ld = ImageDraw.Draw(layer)
    ld.polygon(poly, fill=(30, 70, 120, 235) if selected else (20, 34, 48, 200))
    if selected:
        g = Image.new('RGBA', im.size, (0, 0, 0, 0))
        ImageDraw.Draw(g).polygon(poly, outline=BLUE + (255,), width=5)
        im.alpha_composite(g.filter(ImageFilter.GaussianBlur(5)))
    im.alpha_composite(layer)
    d.polygon(poly, outline=(BLUE + (255,)) if selected else (70, 90, 110, 255), width=2)
    txt(d, ((x0 + x1) // 2, (y0 + y1) // 2 + 1), label, 24, WHITE if selected else DIM, anchor='mm')


def code_boxes(im, x, y, typed, cursor=True, size=40, gap=8, group_gap=16):
    d = ImageDraw.Draw(im)
    for i in range(6):
        bx = x + i * (size + gap) + (group_gap if i >= 3 else 0)
        box = (bx, y, bx + size, y + size + 6)
        is_cursor = cursor and i == len(typed)
        d.rectangle(box, fill=(10, 20, 30, 230))
        if is_cursor:
            glow_rect(im, box, radius=3)
            txt(d, (bx + size // 2, y - 12), '▲', 13, BLUE, anchor='mm', shadow=False)
            txt(d, (bx + size // 2, y + size + 20), '▼', 13, BLUE, anchor='mm', shadow=False)
        else:
            d.rectangle(box, outline=(80, 100, 124, 255), width=2)
        if i < len(typed):
            txt(d, (bx + size // 2, y + size // 2 + 3), typed[i], 30, WHITE, anchor='mm')
        elif is_cursor:
            txt(d, (bx + size // 2, y + size // 2 + 3), '0', 30, (120, 170, 210), anchor='mm')
    return x + 6 * (size + gap) + group_gap


def hints(im, right_x, y, items):
    d = ImageDraw.Draw(im)
    x = right_x
    for key, label in reversed(items):
        f = font(BODY_FONT, 20)
        lw = d.textlength(label, font=f)
        kw = d.textlength(key, font=font(BODY_FONT, 18)) + 18
        x -= lw
        txt(d, (x, y), label, 20, WHITE, anchor='lm')
        x -= 10 + kw
        d.rounded_rectangle((x, y - 15, x + kw, y + 15), 4, fill=(10, 20, 30, 220), outline=(90, 120, 150, 255), width=1)
        txt(d, (x + kw / 2, y), key, 18, GOLD, anchor='mm', shadow=False)
        x -= 26


def clear_area(im, box):
    """パネルの空き部分の暗い地を塗り直す (スクショのエディタの帯なども消す)"""
    d = ImageDraw.Draw(im)
    d.rectangle(box, fill=NAVY + (255,))
    layer = im.crop(box).filter(ImageFilter.GaussianBlur(2))
    im.paste(layer, box[:2])


def mock_a(base, mode):
    im = base.copy()
    clear_area(im, (1262, 690, 1842, 776))
    d = ImageDraw.Draw(im)
    txt(d, (1286, 704), '行 き 方', 20, GOLD)
    x = 1380
    widths = [112, 160, 160]
    for i, label in enumerate(MODES):
        chip(im, (x, 696, x + widths[i], 736), label, i == mode)
        x += widths[i] + 10
    if mode == 2:
        code_boxes(im, 1290, 752, CODE_TYPED, size=32, gap=6, group_gap=12)
        hints(im, 1840, 890, [('←→', '行き方'), ('▲▼', '数字'), ('0-9', '入力')])
    else:
        txt(d, (1290, 756), MODE_NOTE[mode], 19, DIM)
        hints(im, 1840, 890, [('←→', '行き方'), ('A', '出発する')])
    return im


def mock_b(base, mode):
    im = base.copy()
    clear_area(im, (1262, 690, 1842, 776))
    d = ImageDraw.Draw(im)
    txt(d, (1286, 714), '部 屋', 22, GOLD)
    # フォントに ◀▶ が無いので三角は図形で描く
    d.polygon([(1392, 716), (1408, 706), (1408, 726)], fill=BLUE)
    txt(d, (1520, 718), MODE_LONG[mode], 30, WHITE, anchor='mm')
    d.polygon([(1648, 716), (1632, 706), (1632, 726)], fill=BLUE)
    ImageDraw.Draw(im).line((1286, 750, 1820, 750), fill=(70, 100, 130, 255), width=1)
    if mode == 2:
        code_boxes(im, 1290, 760, CODE_TYPED, size=30, gap=6, group_gap=12)
        hints(im, 1840, 890, [('LB RB', '部屋'), ('▲▼', '数字'), ('0-9', '入力')])
    else:
        txt(d, (1286, 758), MODE_NOTE[mode], 19, DIM)
        hints(im, 1840, 890, [('LB RB', '部屋'), ('A', '出発する')])
    return im


def mock_c(base, mode):
    im = base.copy()
    # 出発するを押したあと: パネルの絵に幕を下ろす
    sheet = Image.new('RGBA', im.size, (0, 0, 0, 0))
    ImageDraw.Draw(sheet).rectangle((1262, 192, 1838, 795), fill=(6, 14, 22, 225))
    im.alpha_composite(sheet)
    d = ImageDraw.Draw(im)
    txt(d, (1550, 236), 'だ れ と 行 く ？', 30, GOLD, anchor='mm')
    d.line((1330, 266, 1770, 266), fill=GOLD + (160,), width=1)
    for i in range(3):
        y = 300 + i * 118
        selected = i == mode
        box = (1300, y, 1800, y + 96)
        layer = Image.new('RGBA', im.size, (0, 0, 0, 0))
        ImageDraw.Draw(layer).rounded_rectangle(box, 6, fill=(30, 70, 120, 220) if selected else (18, 30, 44, 200))
        im.alpha_composite(layer)
        if selected:
            glow_rect(im, box)
        else:
            ImageDraw.Draw(im).rounded_rectangle(box, 6, outline=(60, 80, 100, 255), width=1)
        d = ImageDraw.Draw(im)
        txt(d, (1330, y + 30), MODE_LONG[i], 30, WHITE if selected else DIM, anchor='lm')
        if i == 2 and selected:
            code_boxes(im, 1330, y + 52, CODE_TYPED, size=30, gap=6, group_gap=12, cursor=True)
        else:
            txt(d, (1330, y + 70), MODE_NOTE[i], 18, DIM if not selected else (190, 206, 222), anchor='lm')
    hints(im, 1840, 890, [('▲▼', '選ぶ'), ('A', '出発する'), ('B', 'やめる')] if mode != 2
          else [('←→', '桁'), ('▲▼', '数字'), ('A', '出発する'), ('B', 'やめる')])
    return im


def arrow_sprite(flip):
    w, h = ARROW
    im = Image.new('RGBA', (w * 4, h * 4), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    poly = [(w * 4 - 6, h * 2 - h * 1.4), (w * 4 - 6, h * 2 + h * 1.4), (6, h * 2)]
    d.polygon(poly, fill=BLUE + (255,))
    glow = im.filter(ImageFilter.GaussianBlur(10))
    out = Image.alpha_composite(glow, im)
    if flip:
        out = out.transpose(Image.FLIP_LEFT_RIGHT)
    return out.resize((w, h), Image.LANCZOS)


def digit_sprite(focus):
    w, h = DIGIT_BOX
    im = Image.new('RGBA', (w * 4, h * 4), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    box = (8, 8, w * 4 - 8, h * 4 - 8)
    d.rectangle(box, fill=(10, 20, 30, 235))
    if focus:
        glow = Image.new('RGBA', im.size, (0, 0, 0, 0))
        ImageDraw.Draw(glow).rectangle(box, outline=BLUE + (220,), width=14)
        im.alpha_composite(glow.filter(ImageFilter.GaussianBlur(12)))
        d = ImageDraw.Draw(im)
        d.rectangle(box, outline=BLUE + (255,), width=8)
    else:
        d.rectangle(box, outline=(80, 100, 124, 255), width=6)
    return im.resize((w, h), Image.LANCZOS)


def rule_sprite():
    w, h = RULE
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for x in range(w):
        # 両端を薄くする
        t = min(x, w - 1 - x) / (w * 0.22)
        a = int(190 * min(1.0, t))
        d.line((x, 0, x, h), fill=(70, 100, 130, a))
    return im


def plate_sprite():
    """「選択中のステージ」パネル (panel_stage_detail.png) と同じ地・縁・角の切り欠き"""
    w, h = PLATE
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cut = 8
    poly = [(cut, 0), (w - 1 - cut, 0), (w - 1, cut), (w - 1, h - 1 - cut), (w - 1 - cut, h - 1),
            (cut, h - 1), (0, h - 1 - cut), (0, cut)]
    d.polygon(poly, fill=(8, 14, 22, 165), outline=(120, 160, 205, 110))
    return im


def hud_plate_sprite():
    w, h = HUD_PLATE
    im = Image.new('RGBA', (w * 2, h * 2), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    box = (0, 0, w * 2 - 1, h * 2 - 1)
    d.rectangle(box, fill=HUD_BG + (205,))
    # 左端の細い青緑。操作ガイドの帯と同じ語彙
    d.rectangle((0, 0, 7, h * 2 - 1), fill=HUD_ACCENT + (235,))
    d.line((0, h * 2 - 2, w * 2 - 1, h * 2 - 2), fill=HUD_ACCENT + (90,), width=2)
    return im.resize((w, h), Image.LANCZOS)


def emit():
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {EMIT_DIR}')
    write_sprite(EMIT_DIR, 'Room_Arrow_Left', arrow_sprite(False))
    write_sprite(EMIT_DIR, 'Room_Arrow_Right', arrow_sprite(True))
    write_sprite(EMIT_DIR, 'Room_Digit', digit_sprite(False))
    write_sprite(EMIT_DIR, 'Room_Digit_Focus', digit_sprite(True))
    write_sprite(EMIT_DIR, 'Room_Rule', rule_sprite())
    write_sprite(EMIT_DIR, 'Room_Plate', plate_sprite())
    write_sprite(EMIT_DIR, 'Room_HudPlate', hud_plate_sprite())


def contact_sheet(images, labels, out):
    w, h = 960, 540
    sheet = Image.new('RGB', (w * 3, (h + 40) * len(images) // 3), (20, 20, 24))
    d = ImageDraw.Draw(sheet)
    for i, (im, label) in enumerate(zip(images, labels)):
        x, y = (i % 3) * w, (i // 3) * (h + 40)
        sheet.paste(im.convert('RGB').resize((w, h), Image.LANCZOS), (x, y + 40))
        d.text((x + 12, y + 8), label, font=font(BODY_FONT, 24), fill=(230, 230, 230))
    sheet.save(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot', default='', help='ステージ選択画面のスクリーンショット')
    ap.add_argument('--out-dir', default='')
    ap.add_argument('--emit', action='store_true', help='採用案 (案B) の部品を Assets/Art/UI/StageSelect/Room/ へ')
    args = ap.parse_args()

    if args.emit:
        emit()
        return
    if not args.shot or not args.out_dir:
        raise SystemExit('--shot と --out-dir が要ります (モックを描くとき)')

    base = Image.open(args.shot).convert('RGBA').resize((1920, 1080), Image.LANCZOS)
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)

    images, labels = [], []
    for key, name, fn in (('A', '三つの札', mock_a), ('B', '切り替え行', mock_b), ('C', '出発の確認', mock_c)):
        for mode in range(3):
            im = fn(base, mode)
            im.convert('RGB').save(out / f'room_{key}_{mode}.png')
            images.append(im)
            labels.append(f'案{key} {name} — {MODE_LONG[mode]}')
    contact_sheet(images, labels, out / 'room_contact_sheet.png')
    print(out)


if __name__ == '__main__':
    main()
