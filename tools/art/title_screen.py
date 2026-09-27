"""タイトル画面 (TitleScene) の 2D 素材: 題字・選択帯・「ボタンを押してください」の飾り線・周辺減光。

    python tools/art/title_screen.py            # Assets/Art/UI/Title/ に書き出す (.meta の GUID は保つ)
    python tools/art/title_screen.py --preview <shot.png> --out <png>   # 実画面に重ねた確認用

モンハンライズのタイトルのように、右寄せの金属の題字 + 右寄せのメニュー。
題字の後ろには、島の心臓 (緑と金) の光をぼかして敷く。文字は TextRenderer で出す (Kaisei Decol / Zen Old Mincho)。
並びは title_scene.py の LAYOUT と同じ値を使う。
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from character_select import write_sprite  # noqa: E402

OUT_DIR = REPO / 'Assets' / 'Art' / 'UI' / 'Title'
KAISEI = REPO / 'Assets' / 'Art' / 'Font' / 'KaiseiDecol-Bold.ttf'
ZEN = REPO / 'Assets' / 'Art' / 'Font' / 'ZenOldMincho-Bold.ttf'

TITLE = 'ENVIRO HUNTER'
SUBTITLE = '空 の 島 の 狩 人'
GOLD = (214, 170, 90)
TEXT_Y = 190


def _metal_rows(h, y0, y1):
    """上が白金、中ほどが金、下が焦げた真鍮の縦グラデーション。"""
    top, mid, low, bot = (np.array(c, np.float32) for c in
                          ((255, 248, 222), (232, 186, 96), (170, 108, 40), (96, 56, 22)))
    rows = np.zeros((h, 3), np.float32)
    for y in range(h):
        t = float(np.clip((y - y0) / max(y1 - y0, 1), 0, 1))
        if t < 0.45:
            c = top + (mid - top) * (t / 0.45)
        elif t < 0.55:
            c = mid + (low - mid) * ((t - 0.45) / 0.10)  # 真ん中で一度くっきり折れて、金属の稜線に見せる
        else:
            c = low + (bot - low) * ((t - 0.55) / 0.45)
        rows[y] = c
    return rows


def logo(w=1160, h=440):
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))

    # 島の心臓の光 (緑と金) をぼかして背負う。色は一定のまま、濃さだけをぼかす (黒いにじみを出さない)
    for col, cx in (((80, 230, 160), w * 0.38), ((255, 210, 110), w * 0.62)):
        mask = Image.new('L', (w, h), 0)
        md = ImageDraw.Draw(mask)
        for rx, ry, al in ((190, 110, 40), (120, 72, 55), (60, 40, 60)):
            md.ellipse([cx - rx, TEXT_Y - ry, cx + rx, TEXT_Y + ry], fill=al)
        mask = mask.filter(ImageFilter.GaussianBlur(40))
        glow = Image.new('RGBA', (w, h), col + (0,))
        glow.putalpha(mask)
        lay.alpha_composite(glow)

    f = ImageFont.truetype(str(KAISEI), 104)
    mask = Image.new('L', (w, h), 0)
    ImageDraw.Draw(mask).text((w / 2, TEXT_Y), TITLE, font=f, fill=255, anchor='mm')
    m = np.asarray(mask, np.float32) / 255
    ys = np.nonzero(m.max(axis=1) > 0.5)[0]
    metal = np.repeat(_metal_rows(h, ys.min(), ys.max())[:, None, :], w, axis=1)

    # 面取り: 左上から光を当てた浮き彫り
    blur = np.asarray(mask.filter(ImageFilter.GaussianBlur(3)), np.float32) / 255
    gy, gx = np.gradient(blur)
    light = np.clip(-(gx * 0.7 + gy * 0.9) * 6.0, -1, 1)
    metal = np.clip(metal * (1 + 0.45 * light[..., None]) + 60 * np.clip(light, 0, 1)[..., None], 0, 255)

    # 焦げ茶の縁取り + 落ち影
    outline = np.asarray(mask.filter(ImageFilter.MaxFilter(9)), np.float32) / 255
    shadow = np.asarray(mask.filter(ImageFilter.MaxFilter(9)).filter(ImageFilter.GaussianBlur(10)), np.float32) / 255
    shadow = np.roll(np.roll(shadow, 6, axis=0), 4, axis=1)

    out = np.asarray(lay, np.float32).copy()

    def over(rgb, a):
        a = a[..., None]
        out[..., :3] = rgb * a + out[..., :3] * (1 - a)
        out[..., 3:] = 255 * a + out[..., 3:] * (1 - a)

    over(np.zeros((h, w, 3), np.float32), shadow * 0.7)
    over(np.broadcast_to(np.array([34, 20, 10], np.float32), (h, w, 3)), outline)
    over(metal, m)
    lay = Image.fromarray(out.astype(np.uint8), 'RGBA')

    # 副題と両脇の金の罫
    d = ImageDraw.Draw(lay)
    sf = ImageFont.truetype(str(ZEN), 40)
    sw = sf.getlength(SUBTITLE)
    cy = TEXT_Y + 122
    d.text((w / 2 + 2, cy + 2), SUBTITLE, font=sf, fill=(0, 0, 0, 200), anchor='mm')
    d.text((w / 2, cy), SUBTITLE, font=sf, fill=(246, 234, 206, 255), anchor='mm')
    for sgn in (-1, 1):
        x0 = w / 2 + sgn * (sw / 2 + 24)
        x1 = w / 2 + sgn * (sw / 2 + 170)
        d.line([(x0, cy), (x1, cy)], fill=GOLD + (230,), width=2)
        d.polygon([(x0 + sgn * 2, cy), (x0 + sgn * 9, cy - 5), (x0 + sgn * 16, cy), (x0 + sgn * 9, cy + 5)],
                  fill=GOLD + (255,))
    return lay


def select_band(w=560, h=54):
    """選んでいる行の後ろ。右ほど濃い墨を掃いた帯と、下の金の細線 (ライズの選択帯の役)。"""
    x = np.linspace(0, 1, w)[None, :]
    y = np.linspace(-1, 1, h)[:, None]
    rng = np.random.default_rng(3)
    streak = 0.85 + 0.15 * np.repeat(rng.random((h, 1)), w, axis=1)
    a = (x ** 1.5) * np.clip(1 - np.abs(y) ** 6, 0, 1) * streak * 0.78
    a *= np.clip((1 - x) * 30, 0, 1)  # 右端は切れずに消える
    rgb = np.broadcast_to(np.array([30, 20, 12], np.float32), (h, w, 3))
    band = np.dstack([rgb, a * 255]).astype(np.uint8)
    img = Image.fromarray(band, 'RGBA')
    d = ImageDraw.Draw(img)
    line = Image.new('L', (w, 2), 0)
    la = (np.clip((np.linspace(0, 1, w) - 0.3) / 0.25, 0, 1) * np.clip((1 - np.linspace(0, 1, w)) * 20, 0, 1) * 235)
    line = Image.fromarray(np.repeat(la[None, :], 2, axis=0).astype(np.uint8))
    gold = Image.new('RGBA', (w, 2), GOLD + (255,))
    img.paste(gold, (0, h - 3), line)
    d.polygon([(int(w * 0.18), h // 2), (int(w * 0.18) + 8, h // 2 - 8), (int(w * 0.18) + 16, h // 2),
               (int(w * 0.18) + 8, h // 2 + 8)], fill=(230, 190, 100, 255))
    return img


def press_deco(w=760, h=76, gap=440):
    """「ボタンを押してください」の後ろ。白い帆の上でも読めるよう、横長のうっすらした暗がりと、外へ消える金の線"""
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    r = np.sqrt(((xx - w / 2) / (w * 0.36)) ** 2 + ((yy - h / 2) / (h * 0.48)) ** 2)
    a = np.clip(1 - r, 0, 1) ** 1.4 * 0.55
    rgb = np.broadcast_to(np.array([20, 12, 6], np.float32), (h, w, 3))
    img = Image.fromarray(np.dstack([rgb, a * 255]).astype(np.uint8), 'RGBA')
    seg = (w - gap) // 2
    for side in (0, 1):
        ramp = np.linspace(0, 1, seg) if side == 0 else np.linspace(1, 0, seg)
        line = np.zeros((2, seg, 4), np.uint8)
        line[..., 0], line[..., 1], line[..., 2] = GOLD
        line[..., 3] = (ramp ** 0.8 * 230).astype(np.uint8)
        img.alpha_composite(Image.fromarray(line, 'RGBA'), (0 if side == 0 else seg + gap, h // 2 - 1))
    return img


def vignette(w=1920, h=1080):
    """周辺を少し暗く、暖かく落とす (エンジンにポストエフェクトが無いので画像で被せる)。"""
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    r = np.sqrt(((xx - w / 2) / (w / 2)) ** 2 + ((yy - h * 0.48) / (h / 2)) ** 2)
    a = np.clip(r - 0.62, 0, 1) ** 1.5 * 0.85
    # 右側はメニューが読めるよう、もう少し沈める
    a += np.clip((xx / w - 0.55) / 0.45, 0, 1) ** 1.6 * 0.30
    a = np.clip(a, 0, 0.8)
    rgb = np.broadcast_to(np.array([24, 14, 6], np.float32), (h, w, 3))
    return Image.fromarray(np.dstack([rgb, a * 255]).astype(np.uint8), 'RGBA')


def emit():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {OUT_DIR}')
    write_sprite(OUT_DIR, 'Title_Logo', logo())
    write_sprite(OUT_DIR, 'Title_SelectBand', select_band())
    write_sprite(OUT_DIR, 'Title_PressDeco', press_deco())
    write_sprite(OUT_DIR, 'Title_Vignette', vignette())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--preview')
    ap.add_argument('--out')
    args = ap.parse_args()
    if not args.preview:
        emit()
        return
    base = Image.open(args.preview).convert('RGBA').resize((1920, 1080))
    base.alpha_composite(vignette())
    base.alpha_composite(logo(), (760, 110))
    base.alpha_composite(select_band(), (1320, 590))
    base.save(args.out)


if __name__ == '__main__':
    main()
