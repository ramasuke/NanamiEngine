"""狩り場に初めて着いたときの空撮 (StageArrivalMovie) に出す、島の名前と見どころの字幕のデザインモック。

    python tools/art/stage_arrival_caption.py --shot <空撮のスクショ.png> --out-dir <dir>
    python tools/art/stage_arrival_caption.py --emit          # 案C のスプライトを書き出す
    python tools/art/stage_arrival_caption_prefab.py          # 続けてプレハブを組む

空撮は1ショット目に島の名前、続く見どころのショットごとに地名と一言を出す。案ごとに
「島の名前」「見どころ」の2枚を、実際のゲーム画面に合成した 1920x1080 で書き出す。

  案A 墨の帯   : 左下に刷毛で引いた墨の帯。帯の上に生成りの文字。映画の字幕に近い
  案B 羊皮紙の札: 左上に麻紐で吊った羊皮紙の札。インクの文字。キャラ選択や掲示板と同じ手触り
  案C 中央の題字: 島の名前は画面の中央に大きく、上下に手で引いた墨の罫。見どころは下の中央に小さく (2026-09-29 決定)

Pillow と numpy が必要。
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from character_select import (  # noqa: E402
    REPO_ROOT, BODY_FONT, INK, INK_FADE, fbm, grid, soften, rgba, parchment,
    paste_tilted, font,
)

SCREEN_W, SCREEN_H = 1920, 1080
TITLE_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'KaiseiDecol-Bold.ttf'
TITLE_INK_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'KaiseiDecol-Bold_Ink.ttf'
EDGE = (6, 20, 26, 255)
CREAM = (240, 226, 202, 255)
CREAM_FADE = (214, 200, 176, 255)

ISLAND = ('草 原 の 島', '狩人の一族が住む、野生の大きな島')
LANDMARK = ('村 の 跡', '大顎に潰された盆地の村')


def outlined(base, xy, s, px, color, path=TITLE_FONT, anchor='la', stroke=3):
    d = ImageDraw.Draw(base)
    d.text(xy, s, font=font(path, px), fill=color, anchor=anchor, stroke_width=stroke, stroke_fill=EDGE)


def plain(base, xy, s, px, color, path=BODY_FONT, anchor='la', shadow=True):
    d = ImageDraw.Draw(base)
    if shadow:
        d.text((xy[0] + 2, xy[1] + 2), s, font=font(path, px), fill=(0, 0, 0, 210), anchor=anchor)
    d.text(xy, s, font=font(path, px), fill=color, anchor=anchor)


# ---------------------------------------------------------------- 案A 墨の帯
def ink_band(w, h, seed, symmetric=False):
    """刷毛で横に引いた墨。左の打ち込みは濃く、右へかすれて消える。上下の縁はぎざぎざ"""
    xx, yy = grid(w, h)
    n = fbm(w, h, seed, octaves=5, base=5)
    streak = fbm(w, h, seed + 3, octaves=4, base=3)
    # 横に流れる刷毛目: 縦方向にだけ細かい
    bristle = np.asarray(Image.fromarray((fbm(w, h * 6, seed + 9, octaves=3, base=40) * 255).astype(np.uint8))
                         .resize((w, h), Image.BILINEAR), np.float32) / 255.0
    u = xx / w
    # 帯の太さは打ち込みで太く、払いで細く。縁は fbm で揺らす
    half = 0.40 * (1.0 - 0.45 * u) + (n - 0.5) * 0.12
    edge = np.abs(yy / h - 0.5) - half
    across = np.clip(0.5 - edge * 9.0, 0, 1)
    along = np.clip(u / 0.06, 0, 1) * np.clip((1.0 - u) / 0.45 - (streak - 0.5) * 0.6, 0, 1)
    if symmetric:
        # 中央に置く帯は両端とも払う
        half = 0.40 + (n - 0.5) * 0.12
        across = np.clip(0.5 - (np.abs(yy / h - 0.5) - half) * 9.0, 0, 1)
        along = np.clip(np.sin(u * np.pi) * 1.8 - (streak - 0.5) * 0.6, 0, 1)
    a = across * along * (0.70 + 0.30 * bristle)
    a = soften(np.clip(a, 0, 1), 1.4) * 0.88
    rgb = np.array([0.04, 0.06, 0.07], np.float32)[None, None, :] * np.ones((h, w, 1), np.float32)
    return rgba(rgb, a)


def mock_a(base, title, sub, big):
    w, h = (980, 190) if big else (820, 150)
    x, y = 70, SCREEN_H - h - 110
    base.alpha_composite(ink_band(w, h, 41 if big else 43), (x, y))
    px = 76 if big else 58
    outlined(base, (x + 70, y + h * 0.45), title, px, CREAM, anchor='lm')
    plain(base, (x + 74, y + h * 0.45 + px * 0.78), sub, 28 if big else 26, CREAM_FADE, anchor='lm')


# ---------------------------------------------------------------- 案B 羊皮紙の札
def twine(h):
    """縦に垂れる麻紐"""
    w = 10
    xx, yy = grid(w, h)
    d = np.abs(xx - w / 2) - 2.6
    mask = soften(np.clip(0.5 - d, 0, 1), 0.6)
    twist = 0.55 + 0.45 * (np.sin(yy * 0.55 + xx * 0.4) * 0.5 + 0.5)
    rgb = np.array([0.44, 0.33, 0.19], np.float32)[None, None, :] * twist[..., None] * 1.5
    return rgba(rgb, mask)


def mock_b(base, title, sub, big):
    w, h = (560, 200) if big else (470, 170)
    slip = parchment(w, h, 71 if big else 73, aged=0.25)
    tpx = 64 if big else 52
    outlined(slip, (w / 2, h * 0.42), title, tpx, INK + (255,), TITLE_INK_FONT, anchor='mm', stroke=0)
    d = ImageDraw.Draw(slip)
    d.line([(w * 0.18, h * 0.66), (w * 0.82, h * 0.66)], fill=INK_FADE + (200,), width=2)
    plain(slip, (w / 2, h * 0.80), sub, 24 if big else 22, INK_FADE + (255,), anchor='mm', shadow=False)
    cx, cy = 120 + w / 2, 150 + h / 2
    # 画面の上から垂れた2本の麻紐で吊る
    for dx in (-w * 0.32, w * 0.32):
        cord = twine(int(cy - h / 2 + 14))
        base.alpha_composite(cord, (int(cx + dx - cord.width / 2), 0))
    paste_tilted(base, slip, cx, cy, -1.8 if big else 1.4)


# ---------------------------------------------------------------- 案C 中央の題字
def brush_rule(w, seed):
    """手で引いた細い墨の罫。両端がかすれる"""
    h = 14
    xx, yy = grid(w, h)
    n = fbm(w, h, seed, octaves=4, base=6)
    thick = 0.28 + 0.22 * np.sin(np.clip(xx / w, 0, 1) * np.pi)
    a = np.clip(1 - np.abs(yy / h - 0.5 - (n - 0.5) * 0.25) / thick, 0, 1)
    a *= np.clip(np.sin(np.clip(xx / w, 0, 1) * np.pi) * 1.6, 0, 1)
    return rgba(np.full((h, w, 3), 0.94, np.float32), soften(a, 0.6) * 0.9)


def vignette(base, strength):
    xx, yy = grid(SCREEN_W, SCREEN_H)
    r = np.sqrt(((xx / SCREEN_W - 0.5) * 1.6) ** 2 + ((yy / SCREEN_H - 0.5) * 2.0) ** 2)
    a = np.clip((r - 0.2) * strength, 0, 0.55)
    base.alpha_composite(rgba(np.zeros((SCREEN_H, SCREEN_W, 3), np.float32), a))


def mock_c(base, title, sub, big):
    if big:
        vignette(base, 0.8)
        cy = SCREEN_H * 0.42
        rule = brush_rule(620, 91)
        base.alpha_composite(rule, (int(SCREEN_W / 2 - rule.width / 2), int(cy - 78)))
        outlined(base, (SCREEN_W / 2, cy), title, 96, CREAM, anchor='mm', stroke=4)
        base.alpha_composite(rule.transpose(Image.FLIP_LEFT_RIGHT), (int(SCREEN_W / 2 - rule.width / 2), int(cy + 64)))
        plain(base, (SCREEN_W / 2, cy + 112), sub, 30, CREAM_FADE, anchor='mm')
    else:
        cy = SCREEN_H - 170
        band = ink_band(900, 200, 95, symmetric=True)
        base.alpha_composite(band, (int(SCREEN_W / 2 - 450), int(cy - 70)))
        outlined(base, (SCREEN_W / 2, cy), title, 60, CREAM, anchor='mm')
        rule = brush_rule(360, 93)
        base.alpha_composite(rule, (int(SCREEN_W / 2 - rule.width / 2), int(cy + 40)))
        plain(base, (SCREEN_W / 2, cy + 76), sub, 26, CREAM_FADE, anchor='mm')


MOCKS = {'A_InkBand': mock_a, 'B_Parchment': mock_b, 'C_CenterTitle': mock_c}


# ---------------------------------------------------------------- 書き出し (案C に決定)
EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'StageArrival'
# 幕は小さく焼いて拡大する。なだらかな階調なので拡大しても崩れない
VIGNETTE_SCALE = 4
# StageArrivalCaption::vignetteBlendRate_ / bandBlendRate_ と揃える。出しきったときに案C のモックの濃さになるよう焼く
VIGNETTE_BLEND_RATE = 200
BAND_BLEND_RATE = 210
ISLAND_CY = int(SCREEN_H * 0.42)
LANDMARK_CY = SCREEN_H - 170
# 画像は中心の px、文字は (中央の x, 上端の y) と大きさ px。TextRenderer は Transform の位置を上端にして描く。
# 上端は Kaisei Decol / Zen Old の字面の高さから、モックの字の中心 (anchor 'mm') に合うよう取る
LAYOUT = {
    'vignette':       ((SCREEN_W // 2, SCREEN_H // 2), VIGNETTE_SCALE),
    'island_rule_top':    (SCREEN_W // 2, ISLAND_CY - 71),
    'island_title':       ((SCREEN_W // 2, ISLAND_CY - 58), 96),
    'island_rule_bottom': (SCREEN_W // 2, ISLAND_CY + 71),
    'island_subtitle':    ((SCREEN_W // 2, ISLAND_CY + 94), 30),
    'landmark_band':      (SCREEN_W // 2, LANDMARK_CY + 30),
    'landmark_title':     ((SCREEN_W // 2, LANDMARK_CY - 36), 60),
    'landmark_rule':      (SCREEN_W // 2, LANDMARK_CY + 47),
    'landmark_subtitle':  ((SCREEN_W // 2, LANDMARK_CY + 61), 26),
}
TITLE_COLOR = CREAM[:3]
SUBTITLE_COLOR = CREAM_FADE[:3]


def vignette_sprite():
    w, h = SCREEN_W // VIGNETTE_SCALE, SCREEN_H // VIGNETTE_SCALE
    xx, yy = grid(w, h)
    r = np.sqrt(((xx / w - 0.5) * 1.6) ** 2 + ((yy / h - 0.5) * 2.0) ** 2)
    a = np.clip((r - 0.2) * 0.8, 0, 0.55) * 255.0 / VIGNETTE_BLEND_RATE
    return rgba(np.zeros((h, w, 3), np.float32), np.clip(a, 0, 1))


def band_sprite():
    band = ink_band(900, 200, 95, symmetric=True)
    arr = np.asarray(band).astype(np.float32)
    arr[..., 3] = np.clip(arr[..., 3] * 255.0 / BAND_BLEND_RATE, 0, 255)
    return Image.fromarray(arr.astype(np.uint8), 'RGBA')


def emit():
    from character_select import write_sprite
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {EMIT_DIR}')
    write_sprite(EMIT_DIR, 'StageArrival_Vignette', vignette_sprite())
    write_sprite(EMIT_DIR, 'StageArrival_RuleLong', brush_rule(620, 91))
    write_sprite(EMIT_DIR, 'StageArrival_RuleShort', brush_rule(360, 93))
    write_sprite(EMIT_DIR, 'StageArrival_Band', band_sprite())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot')
    ap.add_argument('--out-dir')
    ap.add_argument('--emit', action='store_true', help='案C のスプライトを Assets/Art/UI/StageArrival に書き出す')
    args = ap.parse_args()

    if args.emit:
        emit()
    if not args.shot:
        return
    shot = Image.open(args.shot).convert('RGBA').resize((SCREEN_W, SCREEN_H), Image.LANCZOS)
    # エディタのマウスカーソルが写り込むので左上を空で塗る
    ImageDraw.Draw(shot).rectangle([0, 0, 40, 48], fill=shot.getpixel((60, 60)))
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    for name, fn in MOCKS.items():
        for kind, (title, sub), big in (('1_Island', ISLAND, True), ('2_Landmark', LANDMARK, False)):
            im = shot.copy()
            fn(im, title, sub, big)
            path = out / f'StageArrivalCaption_{name}_{kind}.png'
            im.convert('RGB').save(path)
            print(path)


if __name__ == '__main__':
    main()
