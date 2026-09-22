"""タイトル画面のアセット更新「早馬の荷札」のデザイン案を、タイトル画面に合成して出す。

    python tools/art/asset_update.py --out-dir <dir> [--shot <title_screenshot.png>]   # 3案 x 4状態のモック
    python tools/art/asset_update.py --emit                                           # 案A のスプライトを書き出す
    python tools/art/asset_update.py --preview --out-dir <dir>                        # 案A をゲームと同じ座標で全状態
    python tools/art/asset_update_prefab.py                                           # 案A の prefab を組む

配信アセットの更新 (Packages/AssetUpdater) は揃っているが、画面は仮の MessageBoxW のまま。
更新を「早馬が届けた荷」に見立て、荷札に 荷の数・重さ・送り状(版) を書き、ダウンロードの進みを
蹄の跡で、結果を朱の判子(受領 / 不着)で見せる。

案A 吊り荷札       画面中央に大きな荷札を一枚、釘から麻紐で吊るす。背景は手帳と同じく暗くぼかす
案B 送り状と荷札   板に留めた横長の送り状に荷の中身を品目ごとに書き、左上に荷札を括り付ける
案C 隅の小荷札     タイトル絵はほぼそのまま見せ、右下に小さな荷札を吊るす

状態は AssetUpdateState のうち画面に出る4つ (UpdateAvailable / Downloading / Failed / ReadyToRestart) を並べる。
Checking / UpToDate / NotInstalled / CheckFailed は何も出さない (今と同じくログだけ)。

素材と色は docs/UIDesign.md の系統A (character_select.py の羊皮紙・釘・判子・案内の木札) に揃える。
TextRenderer は文字を回転できないので、傾けるのは紙と飾りだけで、文字は水平に置く。
--shot を省くと Assets/Art/UI/Sample/BackGround.png (タイトルの絵) を 16:9 に切り出して使う。

Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from character_select import (  # noqa: E402
    BODY_FONT, BRUSH_FONT, BRASS, INK, INK_FADE, PARCH, PARCH_OLD, STAMP_RED, bevel, drop_shadow, fbm, font,
    grid, hint_tag, nail, paste, rect_outside, rgba, soften, text, wood_board)
from pause_menu import dim  # noqa: E402

SCREEN_W, SCREEN_H = 1920, 1080
TITLE_ART = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Sample' / 'BackGround.png'

HINT_COLOR = (240, 226, 202)
TWINE = np.array([0.50, 0.38, 0.22], np.float32)
KRAFT = np.array([0.80, 0.68, 0.48], np.float32)   # 荷札は羊皮紙より少し黄みの強い厚紙

# モックに書く値 (UpdateCheckResult / DownloadProgress から出せるものだけ)
SAMPLE = {
    'version': '0.9.3',
    'files': 147,
    'bytes_mb': 312.4,
    'received_mb': 193.6,
    'finished_files': 91,
    'percent': 62,
    'error': '通信が途切れました (タイムアウト)',
    # diff.added / changed の path の拡張子で分けた品目
    'kinds': [('3D モデル', 42, 188.2), ('エフェクト', 18, 61.5), ('音', 9, 27.9), ('絵', 36, 30.1),
              ('データ', 42, 4.7)],
}
STATES = ['confirm', 'download', 'failed', 'done']
STATE_LABELS = {'confirm': '更新の確認', 'download': '受け取り中', 'failed': '失敗', 'done': '完了 → 再起動'}


# ---------------------------------------------------------------- 背景
def load_base(shot_path):
    if shot_path:
        im = Image.open(shot_path).convert('RGBA')
        return im.resize((SCREEN_W, SCREEN_H), Image.LANCZOS) if im.size != (SCREEN_W, SCREEN_H) else im
    art = Image.open(TITLE_ART).convert('RGBA')
    # 絵の周りの黒い額を落として 16:9 に切る
    sx, sy = art.width / 2000, art.height / 1413
    x0, x1 = int(110 * sx), int(1886 * sx)
    w = x1 - x0
    h = int(w * SCREEN_H / SCREEN_W)
    cy = int((118 + 1303) / 2 * sy)
    return art.crop((x0, cy - h // 2, x1, cy + h // 2)).resize((SCREEN_W, SCREEN_H), Image.LANCZOS)


# ---------------------------------------------------------------- 部品
def luggage_tag(w, h, seed, chamfer=None, hole_y=None, aged=0.08):
    """荷札。上の2角を斜めに落とした厚紙に、真鍮の鳩目を打つ"""
    chamfer = chamfer if chamfer is not None else int(w * 0.16)
    hole_y = hole_y if hole_y is not None else int(chamfer * 0.95)
    xx, yy = grid(w, h)
    n = fbm(w, h, seed, octaves=4, base=3)
    d = rect_outside(xx, yy, w, h, 5.0)
    d = np.maximum(d, (chamfer - (xx + yy)) / 1.414)
    d = np.maximum(d, (chamfer - ((w - 1 - xx) + yy)) / 1.414)
    d = d + (n - 0.5) * 5.0
    # 鳩目の穴
    rad = np.hypot(xx - w / 2, yy - hole_y)
    hole_r = max(9.0, w * 0.022)
    d = np.maximum(d, hole_r - rad)
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)

    fiber = 0.86 + 0.18 * fbm(w, h, seed + 7, octaves=6, base=26)
    stain = 1 - 0.26 * np.clip(fbm(w, h, seed + 13, octaves=3, base=2) * 1.9 - 0.8, 0, 1)
    burn = 1 - 0.40 * np.clip((d + 16) / 16, 0, 1)
    curl = 0.92 + 0.14 * np.clip((xx / w + yy / h) / 2 * 1.4, 0, 1)
    tint = KRAFT * (1 - aged) + PARCH_OLD * aged
    rgb = tint[None, None, :] * (fiber * stain * burn * curl)[..., None]

    # 鳩目 (真鍮の環)
    ring = np.clip(1 - np.abs(rad - hole_r * 1.55) / (hole_r * 0.55), 0, 1)
    ring_mask = soften((ring > 0).astype(np.float32) * np.clip(ring * 3, 0, 1), 0.6)
    light = np.clip(((w / 2 - xx) + (hole_y - yy) * 1.3) / (hole_r * 4) + 0.5, 0, 1)
    brass = BRASS[None, None, :] * (0.55 + 1.5 * ring * (0.4 + light))[..., None]
    rgb = rgb * (1 - ring_mask)[..., None] + brass * ring_mask[..., None]
    return rgba(rgb, mask), (w / 2, hole_y)


def invoice(w, h, seed):
    """送り状。羊皮紙より白い帳面の紙に罫を引く"""
    xx, yy = grid(w, h)
    n = fbm(w, h, seed, octaves=4, base=3)
    d = rect_outside(xx, yy, w, h, 6.0) + (n - 0.5) * 7
    mask = soften(np.clip(0.5 - d, 0, 1), 0.7)
    fiber = 0.88 + 0.16 * fbm(w, h, seed + 5, octaves=6, base=24)
    burn = 1 - 0.36 * np.clip((d + 18) / 18, 0, 1)
    tint = PARCH * 1.06
    rgb = tint[None, None, :] * (fiber * burn)[..., None]
    return rgba(rgb, mask)


def twine(base, points, width=5):
    """麻紐。濃い芯の上に明るい撚りを点線で重ねる"""
    d = ImageDraw.Draw(base)
    c_dark = tuple(int(v * 150) for v in TWINE) + (255,)
    c_lit = tuple(int(min(1, v * 1.45) * 255) for v in TWINE) + (220,)
    d.line(points, fill=c_dark, width=width, joint='curve')
    for (x0, y0), (x1, y1) in zip(points, points[1:]):
        seg = math.hypot(x1 - x0, y1 - y0)
        steps = max(1, int(seg / 7))
        for i in range(steps):
            t = (i + 0.3) / steps
            cx, cy = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            d.line([(cx - 1.5, cy - 1.5), (cx + 1.5, cy + 1.5)], fill=c_lit, width=2)


def sag(p0, p1, amount, n=12):
    pts = []
    for i in range(n + 1):
        t = i / n
        x = p0[0] + (p1[0] - p0[0]) * t
        y = p0[1] + (p1[1] - p0[1]) * t + math.sin(t * math.pi) * amount
        pts.append((x, y))
    return pts


def hoofprint(size, filled):
    """蹄鉄の跡。進行方向 (右) に蹄の先を向ける"""
    s = 4
    n = size * s
    big = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    c = n / 2
    rx, ry = n * 0.40, n * 0.44       # 蹄鉄は少し縦長
    thick = n * 0.15
    color = (*INK, 235) if filled else (*INK_FADE, 100)
    # 開いている側を後ろ (左) に向けた U 字。外周と内周を多角形で結ぶ
    span = range(-128, 129, 4)
    outer = [(c + math.cos(math.radians(a)) * rx, c + math.sin(math.radians(a)) * ry) for a in span]
    inner = [(c + math.cos(math.radians(a)) * (rx - thick), c + math.sin(math.radians(a)) * (ry - thick))
             for a in reversed(span)]
    d.polygon(outer + inner, fill=color)
    # 踵の留め (両端を少し太らせる)
    for a in (-128, 128):
        x = c + math.cos(math.radians(a)) * (rx - thick / 2)
        y = c + math.sin(math.radians(a)) * (ry - thick / 2)
        d.rectangle([x - thick * 0.75, y - thick * 0.62, x + thick * 0.35, y + thick * 0.62], fill=color)
    if filled:
        for a in (-100, -62, -24, 24, 62, 100):
            x = c + math.cos(math.radians(a)) * (rx - thick / 2)
            y = c + math.sin(math.radians(a)) * (ry - thick / 2)
            d.ellipse([x - s * 1.5, y - s * 1.5, x + s * 1.5, y + s * 1.5], fill=(*KRAFT_RGB, 255))
    return big.resize((size, size), Image.LANCZOS)


KRAFT_RGB = tuple(int(v * 230) for v in KRAFT)


def hoof_trail(base, x0, x1, y, count, progress01, size=46):
    """左から右へ蹄の跡を並べる。左右の足で上下に少しずらす"""
    lit = int(round(count * progress01))
    step = (x1 - x0 - size) / max(count - 1, 1)
    for i in range(count):
        dy = -7 if i % 2 == 0 else 7
        paste(base, hoofprint(size, i < lit), x0 + i * step, y + dy - size / 2)


def hanko(label, px=58, angle=-9.0, color=STAMP_RED, frame='square', alpha=215):
    """朱の判子。字を縦に積み、枠と一緒にかすれさせる"""
    chars = [ch for ch in label if ch != ' ']
    pad = int(px * 0.34)
    if frame == 'round':
        side = int(px * 2.3)
        w = h = side
    else:
        w = int(px + pad * 2)
        h = int(px * len(chars) * 1.02 + pad * 2)
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    f = font(BRUSH_FONT, px)
    ink = (*color, 255)
    if frame == 'round':
        d.ellipse([4, 4, w - 5, h - 5], outline=ink, width=max(4, px // 11))
        d.ellipse([12, 12, w - 13, h - 13], outline=ink, width=max(2, px // 26))
        size = int(px * 0.72)
        f = font(BRUSH_FONT, size)
        for i, ch in enumerate(chars):
            d.text((w / 2, h / 2 + (i - (len(chars) - 1) / 2) * size * 1.02), ch, font=f, fill=ink, anchor='mm')
    else:
        d.rounded_rectangle([3, 3, w - 4, h - 4], radius=px // 7, outline=ink, width=max(4, px // 10))
        for i, ch in enumerate(chars):
            d.text((w / 2, pad + px * 0.5 + i * px * 1.02), ch, font=f, fill=ink, anchor='mm')
    arr = np.asarray(lay, np.float32) / 255.0
    grain = fbm(w, h, 400 + px, octaves=5, base=18)
    arr[..., 3] *= np.clip((grain - 0.28) * 3.2, 0, 1) * (alpha / 255)
    lay = Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA')
    return lay.rotate(angle, resample=Image.BICUBIC, expand=True)


def paste_center(base, part, cx, cy):
    base.alpha_composite(part, (int(cx - part.width / 2), int(cy - part.height / 2)))


def paste_tilted_shadow(base, part, cx, cy, angle, shadow_alpha=0.62):
    sh = drop_shadow(part, alpha=shadow_alpha).rotate(angle, resample=Image.BICUBIC, expand=True)
    paste_center(base, sh, cx + 4, cy + 6)
    paste_center(base, part.rotate(angle, resample=Image.BICUBIC, expand=True), cx, cy)


def hint_strip(base, right_x, y, items):
    x = right_x
    for glyph, label in reversed(items):
        text(base, (x, y), label, 26, (*HINT_COLOR, 255), BODY_FONT, anchor='rm', shadow=(2, 2, (0, 0, 0, 210)))
        x -= font(BODY_FONT, 26).getlength(label) + 16
        gw = 68 if len(glyph) > 1 else 46
        paste(base, hint_tag(gw, 42, glyph), x - gw, y - 21)
        x -= gw + 30


def rule(base, x0, x1, y, color=INK_FADE, width=2, alpha=200):
    ImageDraw.Draw(base).line([(x0, y), (x1, y)], fill=(*color, alpha), width=width)


def kv_row(base, x0, x1, y, label, value, label_px=28, value_px=40):
    text(base, (x0, y), label, label_px, (*INK_FADE, 255), BODY_FONT, anchor='lm')
    text(base, (x1, y), value, value_px, (*INK, 255), BRUSH_FONT, anchor='rm')


def headline(state):
    return {
        'confirm': '新しい荷が届きました',
        'download': '荷を受け取っています',
        'failed': '荷が届きませんでした',
        'done': '荷を受け取りました',
    }[state]


def hints_for(state):
    return {
        'confirm': [('A', '受け取る'), ('B', 'あとで')],
        'download': [],
        'failed': [('A', 'もう一度'), ('B', 'あとで')],
        'done': [('A', '再起動する')],
    }[state]


def size_line():
    s = SAMPLE
    return f"{s['received_mb']:.1f} / {s['bytes_mb']:.1f} MB    {s['finished_files']} / {s['files']} 件"


# ---------------------------------------------------------------- 案A 吊り荷札 (決定案)
# 2026-09-22 に3案から案A「吊り荷札」に決まった。以下の LAYOUT がそのままプレハブの座標になる
# (tools/art/asset_update_prefab.py)。スプライトは中心、文字は上端 (左寄せは左上 / 中央寄せは上辺の中央 /
# 右寄せは右上)。荷札の傾き・影・麻紐・釘、変わらない「早馬便 荷札」の見出しと早馬の丸印は絵に焼き込み、
# 状態ごとに変わる文字だけを TextRenderer で水平に書く。
EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'AssetUpdate'
HINT_CONFIRM_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Confirm.png'
HINT_CANCEL_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Cancel.png'
BACKDROP_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'PauseMenu' / 'Backdrop.png'   # 960x540 を 2 倍で敷く
BLACK_MASK_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'BlackMask.png'

TAG_W, TAG_H = 600, 780
TAG_CENTER = (960, 590)
TAG_ANGLE = -1.4
TAG_NAIL = (958, 118)
TAG_SEED = 31
HOOF_PX = 46
HOOF_COUNT = 9

LAYOUT = {
    'veil_black': ((SCREEN_W / 2, SCREEN_H / 2), 0.47, 120),   # BlackMask の中心・倍率・濃さ (ぼかせない代わりに沈める)
    'veil': ((SCREEN_W / 2, SCREEN_H / 2), 2.0, 235),          # Backdrop の中心・倍率・濃さ
    'headline': ((960, 407), 50),
    'detail_rows': [530, 614, 698],                             # 行の中心の y
    'detail_label_x': 730, 'detail_label_px': 28,
    'detail_value_x': 1190, 'detail_value_px': 40,
    'detail_rule_dy': 36,
    'note': ((960, 827), 26),
    'hoof_first': (753, 550), 'hoof_step': 51.75, 'hoof_zigzag': 7,
    'percent': ((960, 628), 84),
    'amount': ((960, 747), 26),
    'wait': ((960, 837), 26),
    'warning': ((960, 515), 28),
    'error': ((960, 572), 24), 'error_line_units': 34, 'error_max_lines': 3,  # 半角=1 / 全角=2 で数える
    'stamp': (1000, 805),                                       # 不着 / 版違い
    'stamp_received': (1110, 790),
    'hint_confirm_tag': (1500, 1010), 'hint_confirm_label': ((1532, 997), 26),
    'hint_cancel_tag': (1720, 1010), 'hint_cancel_label': ((1752, 997), 26),
    'hint_confirm_button': ((1580, 1010), (200, 52)),           # クリックできる範囲 (中心と大きさ)
    'hint_cancel_button': ((1790, 1010), (160, 52)),
    'seal_on_tag': (85, 712),                                   # 荷札の左上からの早馬の丸印
    'label_on_tag': (300, 150), 'rule_on_tag': (60, 540, 178),
}
DETAIL_LABELS = ['荷 の 数', '重 さ', '送 り 状']
NOTE_OFFER = '受け取ると自動で再起動します'


def tag_sprite():
    """荷札・影・麻紐・釘をまとめた1枚。戻り値は (絵, 画面での中心)"""
    tag, (hx, hy) = luggage_tag(TAG_W, TAG_H, TAG_SEED)
    lx, ly = LAYOUT['label_on_tag']
    text(tag, (lx, ly), '早 馬 便   荷 札', 26, (*INK_FADE, 255), BODY_FONT, anchor='mm')
    x0, x1, ry = LAYOUT['rule_on_tag']
    rule(tag, x0, x1, ry)
    sx, sy = LAYOUT['seal_on_tag']
    paste_center(tag, hanko('早馬', px=46, angle=-14, frame='round', alpha=170), sx, sy)

    layer = Image.new('RGBA', (SCREEN_W, SCREEN_H), (0, 0, 0, 0))
    cx, cy = TAG_CENTER
    a = math.radians(-TAG_ANGLE)
    ox, oy = hx - TAG_W / 2, hy - TAG_H / 2
    ex = cx + ox * math.cos(a) - oy * math.sin(a)
    ey = cy + ox * math.sin(a) + oy * math.cos(a)
    paste_tilted_shadow(layer, tag, cx, cy, TAG_ANGLE)
    twine(layer, sag((ex - 10, ey), TAG_NAIL, 6) + sag(TAG_NAIL, (ex + 10, ey), 6)[1:])
    paste_center(layer, nail(26), *TAG_NAIL)
    box = layer.getbbox()
    # 中心が整数になるよう偶数幅に揃える
    box = (box[0], box[1], box[2] + (box[2] - box[0]) % 2, box[3] + (box[3] - box[1]) % 2)
    return layer.crop(box), ((box[0] + box[2]) / 2, (box[1] + box[3]) / 2)


def rule_sprite(w=460):
    im = Image.new('RGBA', (w, 3), (0, 0, 0, 0))
    ImageDraw.Draw(im).line([(0, 1), (w - 1, 1)], fill=(*INK_FADE, 110), width=1)
    return im


def stamp_sprites():
    return {
        'AssetUpdate_Stamp_Received': hanko('受領', px=92, angle=-11),
        'AssetUpdate_Stamp_Undelivered': hanko('不着', px=92, angle=-11),
        'AssetUpdate_Stamp_WrongVersion': hanko('版違い', px=80, angle=-11),
    }


def hoof_positions():
    x0, y0 = LAYOUT['hoof_first']
    zig = LAYOUT['hoof_zigzag']
    return [(x0 + i * LAYOUT['hoof_step'], y0 + (-zig if i % 2 == 0 else zig)) for i in range(HOOF_COUNT)]


def emit_sprites():
    from character_select import write_sprite
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {EMIT_DIR}')
    tag, center = tag_sprite()
    write_sprite(EMIT_DIR, 'AssetUpdate_Tag', tag)
    write_sprite(EMIT_DIR, 'AssetUpdate_Rule', rule_sprite())
    write_sprite(EMIT_DIR, 'AssetUpdate_Hoof_Filled', hoofprint(HOOF_PX, True))
    write_sprite(EMIT_DIR, 'AssetUpdate_Hoof_Empty', hoofprint(HOOF_PX, False))
    for name, im in stamp_sprites().items():
        write_sprite(EMIT_DIR, name, im)
    print(f'  AssetUpdate_Tag center = ({center[0]:.1f}, {center[1]:.1f})')


# 状態ごとに出すもの (C++ の AssetUpdateTagUi と同じ分け方)
PREVIEW_TEXT = {
    'confirm': {'headline': '新しい荷が届きました', 'note': NOTE_OFFER, 'hints': ('受け取る', 'あとで')},
    'download': {'headline': '荷を受け取っています', 'hints': None},
    'failed': {'headline': '荷が届きませんでした', 'hints': ('もう一度', 'あとで')},
    'done': {'headline': '荷を受け取りました', 'note': 'ゲームを再起動して荷を開けます', 'hints': ('再起動する', None)},
    'wrong': {'headline': '荷を受け取れません', 'hints': (None, '閉じる')},
}


def wrap_units(s, units, max_lines):
    """半角を1、それ以外を2と数えて折り返す (C++ の AssetUpdateTagUi と同じ規則)。入り切らなければ末尾を … にする"""
    lines, line, used = [], '', 0
    for ch in s:
        w = 1 if ord(ch) < 0x80 else 2
        if used + w > units:
            lines.append(line)
            line, used = '', 0
        line += ch
        used += w
    if line:
        lines.append(line)
    if len(lines) > max_lines:
        lines = lines[:max_lines]
        lines[-1] = lines[-1][:-1] + '…'
    return lines


def put_text(im, pos_px, s, color, path, align='ma'):
    (x, y), px = pos_px
    text(im, (x, y), s, px, (*color, 255), path, anchor=align)


def mock_a(base, state):
    """ゲームと同じ部品・同じ座標で描く (背景のぼかしだけはエンジンに無いので、Backdrop を敷くだけにする)"""
    im = base.copy()
    for key, path in (('veil_black', BLACK_MASK_SPRITE), ('veil', BACKDROP_SPRITE)):
        (vx, vy), vscale, vblend = LAYOUT[key]
        veil = Image.open(path).convert('RGBA')
        veil = veil.resize((int(veil.width * vscale), int(veil.height * vscale)), Image.LANCZOS)
        arr = np.asarray(veil, np.float32).copy()
        arr[..., 3] *= vblend / 255
        paste_center(im, Image.fromarray(arr.astype(np.uint8), 'RGBA'), vx, vy)

    tag, center = tag_sprite()
    paste_center(im, tag, *center)
    spec = PREVIEW_TEXT[state]
    put_text(im, LAYOUT['headline'], spec['headline'], INK, BRUSH_FONT)

    if state in ('confirm', 'done'):
        values = [f"{SAMPLE['files']} 件", f"{SAMPLE['bytes_mb']:.1f} MB", f"{SAMPLE['version']} 版"]
        for y, label, value in zip(LAYOUT['detail_rows'], DETAIL_LABELS, values):
            lp, vp = LAYOUT['detail_label_px'], LAYOUT['detail_value_px']
            put_text(im, ((LAYOUT['detail_label_x'], y - lp // 2), lp), label, INK_FADE, BODY_FONT, 'la')
            put_text(im, ((LAYOUT['detail_value_x'], y - vp // 2), vp), value, INK, BRUSH_FONT, 'ra')
            paste_center(im, rule_sprite(), TAG_CENTER[0], y + LAYOUT['detail_rule_dy'])
        put_text(im, LAYOUT['note'], spec['note'], INK_FADE, BODY_FONT)
    elif state == 'download':
        lit = int(SAMPLE['percent'] / 100 * HOOF_COUNT)
        for i, (x, y) in enumerate(hoof_positions()):
            paste_center(im, hoofprint(HOOF_PX, i < lit), x, y)
        put_text(im, LAYOUT['percent'], f"{SAMPLE['percent']}%", INK, BRUSH_FONT)
        put_text(im, LAYOUT['amount'], size_line(), INK_FADE, BODY_FONT)
        put_text(im, LAYOUT['wait'], 'このまま少しお待ちください', INK_FADE, BODY_FONT)
    elif state in ('failed', 'wrong'):
        error = (SAMPLE['error'] if state == 'failed'
                 else 'ゲーム本体が古いため更新できません (必要 1.1.0 / 今 1.0.0)')
        warning = 'このままでは冒険に出られません' if state == 'failed' else '新しい版のゲームが要ります'
        put_text(im, LAYOUT['warning'], warning, STAMP_RED, BODY_FONT)
        (ex, ey), epx = LAYOUT['error']
        for i, line in enumerate(wrap_units(error, LAYOUT['error_line_units'], LAYOUT['error_max_lines'])):
            put_text(im, ((ex, ey + i * int(epx * 1.45)), epx), line, INK_FADE, BODY_FONT)
        name = 'AssetUpdate_Stamp_Undelivered' if state == 'failed' else 'AssetUpdate_Stamp_WrongVersion'
        paste_center(im, stamp_sprites()[name], *LAYOUT['stamp'])
    if state == 'done':
        paste_center(im, stamp_sprites()['AssetUpdate_Stamp_Received'], *LAYOUT['stamp_received'])

    hints = spec['hints']
    if hints:
        confirm, cancel = hints
        shadow = (2, 2, (0, 0, 0, 210))
        if confirm:
            paste_center(im, Image.open(HINT_CONFIRM_SPRITE).convert('RGBA'), *LAYOUT['hint_confirm_tag'])
            (x, y), px = LAYOUT['hint_confirm_label']
            text(im, (x, y), confirm, px, (*HINT_COLOR, 255), BODY_FONT, anchor='la', shadow=shadow)
        if cancel:
            paste_center(im, Image.open(HINT_CANCEL_SPRITE).convert('RGBA'), *LAYOUT['hint_cancel_tag'])
            (x, y), px = LAYOUT['hint_cancel_label']
            text(im, (x, y), cancel, px, (*HINT_COLOR, 255), BODY_FONT, anchor='la', shadow=shadow)
    return im


# ---------------------------------------------------------------- 案B 送り状と荷札
def mock_b(base, state):
    im = dim(base, darken=0.60, blur=6.0, vignette=0.55, warm=0.30)
    bw, bh = 1120, 700
    bx, by = 960 + 30, 560
    board = wood_board(bw, bh, 57, plank=9999)
    arr = np.asarray(board, np.float32) / 255.0
    arr[..., :3] *= 0.95
    board = Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGBA')
    paste_tilted_shadow(im, board, bx, by, 0.0, shadow_alpha=0.7)
    ImageDraw.Draw(im).rectangle([bx - bw / 2 + 4, by - bh / 2 + 4, bx + bw / 2 - 5, by + bh / 2 - 5],
                                 outline=(70, 44, 22, 255), width=3)

    pw, ph = 900, 580
    px_, py_ = bx + 60, by + 8
    paper = invoice(pw, ph, 71)
    paste_tilted_shadow(im, paper, px_, py_, 0.6, shadow_alpha=0.5)
    for nx, ny in [(-pw / 2 + 34, -ph / 2 + 30), (pw / 2 - 34, -ph / 2 + 26)]:
        paste_center(im, nail(22), px_ + nx, py_ + ny)

    left = px_ - pw / 2 + 70
    right = px_ + pw / 2 - 70
    top = py_ - ph / 2
    text(im, (left, top + 70), '送 り 状', 30, (*INK_FADE, 255), BODY_FONT, anchor='lm')
    text(im, (right, top + 70), f"{SAMPLE['version']} 版", 30, (*INK_FADE, 255), BODY_FONT, anchor='rm')
    rule(im, left, right, top + 100)
    text(im, (left, top + 150), headline(state), 48, (*INK, 255), BRUSH_FONT, anchor='lm')

    if state == 'confirm':
        y = top + 225
        text(im, (left + 10, y), '品 目', 24, (*INK_FADE, 255), BODY_FONT, anchor='lm')
        text(im, (right - 190, y), '数', 24, (*INK_FADE, 255), BODY_FONT, anchor='rm')
        text(im, (right, y), '重 さ', 24, (*INK_FADE, 255), BODY_FONT, anchor='rm')
        y += 44
        for name, count, mb in SAMPLE['kinds']:
            text(im, (left + 10, y), name, 30, (*INK, 255), BODY_FONT, anchor='lm')
            text(im, (right - 190, y), f'{count} 件', 30, (*INK, 255), BODY_FONT, anchor='rm')
            text(im, (right, y), f'{mb:.1f} MB', 30, (*INK, 255), BODY_FONT, anchor='rm')
            rule(im, left, right, y + 24, alpha=70, width=1)
            y += 48
        rule(im, left, right, y - 16, color=INK, width=3, alpha=200)
        text(im, (left + 10, y + 22), '合 計', 30, (*INK, 255), BODY_FONT, anchor='lm')
        text(im, (right - 190, y + 22), f"{SAMPLE['files']} 件", 34, (*INK, 255), BRUSH_FONT, anchor='rm')
        text(im, (right, y + 22), f"{SAMPLE['bytes_mb']:.1f} MB", 34, (*INK, 255), BRUSH_FONT, anchor='rm')
    elif state == 'download':
        # 帳面の罫に沿って墨を引いていく進み具合
        y = top + 290
        bar_x0, bar_x1 = left + 10, right - 10
        rule(im, bar_x0, bar_x1, y - 22, alpha=150, width=2)
        rule(im, bar_x0, bar_x1, y + 22, alpha=150, width=2)
        fill = bar_x0 + (bar_x1 - bar_x0) * SAMPLE['percent'] / 100
        wash = Image.new('RGBA', (int(fill - bar_x0), 34), (0, 0, 0, 0))
        wa = fbm(wash.width, 34, 88, octaves=5, base=12)
        warr = np.zeros((34, wash.width, 4), np.float32)
        warr[..., :3] = np.array(INK, np.float32) / 255
        warr[..., 3] = np.clip(0.62 + 0.4 * wa, 0, 1)
        warr[..., 3][:, -12:] *= np.linspace(1, 0.2, 12)[None, :]
        im.alpha_composite(Image.fromarray((warr * 255).astype(np.uint8), 'RGBA'), (int(bar_x0), int(y - 17)))
        paste(im, hoofprint(48, True), fill - 10, y - 78)
        text(im, (left + 10, y + 90), f"{SAMPLE['percent']}%", 72, (*INK, 255), BRUSH_FONT, anchor='lm')
        text(im, (right, y + 96), size_line(), 28, (*INK_FADE, 255), BODY_FONT, anchor='rm')
        text(im, (left + 10, y + 190), 'このまま少しお待ちください', 26, (*INK_FADE, 255), BODY_FONT, anchor='lm')
    elif state == 'failed':
        text(im, (left + 10, top + 250), SAMPLE['error'], 28, (*INK_FADE, 255), BODY_FONT, anchor='lm')
        text(im, (left + 10, top + 310), 'このままでは冒険に出られません', 30, (*STAMP_RED, 255), BODY_FONT, anchor='lm')
        paste_center(im, hanko('不着', px=100, angle=-10), right - 110, top + 380)
    elif state == 'done':
        text(im, (left + 10, top + 250), f"{SAMPLE['files']} 件  {SAMPLE['bytes_mb']:.1f} MB を受け取りました", 30,
             (*INK, 255), BODY_FONT, anchor='lm')
        text(im, (left + 10, top + 310), 'ゲームを再起動して荷を開けます', 28, (*INK_FADE, 255), BODY_FONT, anchor='lm')
        paste_center(im, hanko('受領', px=100, angle=-10), right - 110, top + 380)

    # 板の左上に荷札を括り付ける
    tw, th = 250, 350
    tag, (hx, hy) = luggage_tag(tw, th, 93, aged=0.12)
    tcx, tcy = bx - bw / 2 + 60, by - bh / 2 + 150
    paste_tilted_shadow(im, tag, tcx, tcy, 8.0)
    knot = (bx - bw / 2 + 190, by - bh / 2 + 30)
    a = math.radians(-8.0)
    ox, oy = hx - tw / 2, hy - th / 2
    ex = tcx + ox * math.cos(a) - oy * math.sin(a)
    ey = tcy + ox * math.sin(a) + oy * math.cos(a)
    twine(im, sag((ex, ey), knot, 4))
    paste_center(im, nail(24), *knot)
    text(im, (tcx, tcy - 10), '早 馬', 44, (*INK, 255), BRUSH_FONT, anchor='mm')
    text(im, (tcx, tcy + 50), '拠点 酒場 宛', 22, (*INK_FADE, 255), BODY_FONT, anchor='mm')

    hint_strip(im, 1856, 1010, hints_for(state))
    return im


# ---------------------------------------------------------------- 案C 隅の小荷札
def mock_c(base, state):
    # タイトル絵は見せたまま、右下だけ落とす
    im = base.copy()
    arr = np.asarray(im.convert('RGB'), np.float32) / 255.0
    xx, yy = grid(SCREEN_W, SCREEN_H)
    corner = np.clip(1 - np.hypot((xx - 1640) / 820, (yy - 640) / 760), 0, 1) ** 0.9
    arr = arr * (1 - 0.72 * corner)[..., None]
    im = Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')

    w, h = 420, 560
    tag, (hx, hy) = luggage_tag(w, h, 47)
    cx, cy = 1600, 640
    angle = 2.2
    a = math.radians(-angle)
    ox, oy = hx - w / 2, hy - h / 2
    ex = cx + ox * math.cos(a) - oy * math.sin(a)
    ey = cy + ox * math.sin(a) + oy * math.cos(a)
    nail_xy = (1612, 262)
    paste_tilted_shadow(im, tag, cx, cy, angle)
    twine(im, sag((ex - 8, ey), nail_xy, 4) + sag(nail_xy, (ex + 8, ey), 4)[1:])
    paste_center(im, nail(24), *nail_xy)

    x0, x1 = cx - w / 2 + 48, cx + w / 2 - 48
    top = cy - h / 2
    text(im, (cx, top + 120), '早 馬 便', 22, (*INK_FADE, 255), BODY_FONT, anchor='mm')
    rule(im, x0, x1, top + 144)
    text(im, (cx, top + 192), headline(state), 36, (*INK, 255), BRUSH_FONT, anchor='mm')

    if state == 'confirm':
        kv_row(im, x0, x1, top + 272, '荷', f"{SAMPLE['files']} 件", 26, 34)
        kv_row(im, x0, x1, top + 332, '重 さ', f"{SAMPLE['bytes_mb']:.1f} MB", 26, 34)
        rule(im, x0, x1, top + 368, alpha=90, width=1)
        text(im, (cx, top + 430), '受け取ると自動で再起動します', 22, (*INK_FADE, 255), BODY_FONT, anchor='mm')
    elif state == 'download':
        hoof_trail(im, x0, x1, top + 275, 6, SAMPLE['percent'] / 100, size=40)
        text(im, (cx, top + 365), f"{SAMPLE['percent']}%", 64, (*INK, 255), BRUSH_FONT, anchor='mm')
        text(im, (cx, top + 440), f"{SAMPLE['received_mb']:.1f} / {SAMPLE['bytes_mb']:.1f} MB", 24,
             (*INK_FADE, 255), BODY_FONT, anchor='mm')
    elif state == 'failed':
        text(im, (cx, top + 262), '通信が途切れました', 24, (*INK_FADE, 255), BODY_FONT, anchor='mm')
        text(im, (cx, top + 306), 'このままでは遊べません', 24, (*STAMP_RED, 255), BODY_FONT, anchor='mm')
        paste_center(im, hanko('不着', px=74, angle=-10), cx, top + 420)
    elif state == 'done':
        text(im, (cx, top + 262), 'ゲームを再起動します', 24, (*INK_FADE, 255), BODY_FONT, anchor='mm')
        paste_center(im, hanko('受領', px=74, angle=-10), cx, top + 395)

    hint_strip(im, 1856, 1010, hints_for(state))
    return im


PLANS = [
    ('A', '案A  吊り荷札', mock_a),
    ('B', '案B  送り状と荷札', mock_b),
    ('C', '案C  隅の小荷札', mock_c),
]


def contact_sheet(rows):
    """案ごとに1行、状態を横に並べる"""
    tw = 640
    th = int(tw * SCREEN_H / SCREEN_W)
    pad, header, col_header = 22, 60, 62
    sheet = Image.new('RGBA', (pad + (tw + pad) * len(STATES), col_header + (th + header) * len(rows) + pad),
                      (20, 15, 11, 255))
    for j, st in enumerate(STATES):
        text(sheet, (pad + j * (tw + pad) + 4, 12), STATE_LABELS[st], 26, (220, 200, 170, 255), BODY_FONT)
    for i, (label, ims) in enumerate(rows):
        y = col_header + i * (th + header)
        text(sheet, (pad + 4, y + 6), label, 34, (250, 214, 150, 255), BRUSH_FONT)
        for j, im in enumerate(ims):
            sheet.alpha_composite(im.convert('RGBA').resize((tw, th), Image.LANCZOS),
                                  (pad + j * (tw + pad), y + header - 6))
    return sheet


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot', default='', help='タイトル画面のスクリーンショット (省略時はタイトルの絵)')
    ap.add_argument('--out-dir', default='')
    ap.add_argument('--emit', action='store_true', help='決定案 (案A) のスプライトを Assets/Art/UI/AssetUpdate へ書き出す')
    ap.add_argument('--preview', action='store_true', help='案A だけを全状態 (版違いを含む) で --out-dir へ描く')
    args = ap.parse_args()

    if args.emit:
        emit_sprites()
    if not args.out_dir:
        if not args.emit:
            ap.error('--out-dir が要る (--emit だけなら省ける)')
        return

    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    base = load_base(args.shot)

    if args.preview:
        for st in list(PREVIEW_TEXT):
            path = out / f'asset_update_A_{st}.png'
            mock_a(base.copy(), st).convert('RGB').save(path)
            print(f'wrote {path}')
        return

    rows = []
    for key, label, fn in PLANS:
        ims = []
        for st in STATES:
            im = fn(base.copy(), st)
            path = out / f'asset_update_{key}_{st}.png'
            im.convert('RGB').save(path)
            ims.append(im)
            print(f'wrote {path}')
        rows.append((label, ims))
    sheet = out / 'asset_update_compare.png'
    contact_sheet(rows).convert('RGB').save(sheet)
    print(f'wrote {sheet}')


if __name__ == '__main__':
    main()
