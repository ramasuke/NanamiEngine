"""ゲームオーバーのスプライト (案A 刻まれた石版) と、実際のゲーム画面に合成したモックを生成する。

    python tools/art/game_over.py --emit                          # Assets/Art/UI/GameOver へ書き出す
    python tools/art/game_over.py --shot <screenshot.png> --out-dir <dir>   # 書き出した絵で完成イメージを合成

方向性: 平らな角丸パネルに細い光る縁、という既製品っぽい見た目を避ける。既存HUDの語彙
(彫り込んだ金属、紋章)に寄せて、墓標のような風化した石版に「力尽きた」を彫り込み、
選択肢は釘で打った鉄札にする。選ばれている札は縁が焼けて熾火のように脈打つ。

石版の傾きと影は絵に焼き込むので、置く位置は絵の中心 = 石版の中心になる。
配置の数値は LAYOUT にまとめてあり、tools/art/game_over_prefab.py がそのまま使う。

Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

SCREEN_W, SCREEN_H = 1920, 1080
SS = 2  # 内部はこの倍率で描いて縮める

EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'GameOver'
BODY_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'ipam.ttf'
SCRATCH_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'onryou.ttf'
LATIN_FONT = Path(r'C:/Windows/Fonts/segoeuib.ttf')
NAIL_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'Nail.png'
HINT_MOVE_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Move.png'
HINT_CONFIRM_SPRITE = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'CharacterSelect' / 'HintTag_Confirm.png'

EMBER = (255, 138, 44)

SLAB_SIZE = (1040, 312)
SLAB_TILT_DEG = -1.2
SLAB_PAD = 44
DIRT_SIZE = (1180, 90)
PLATE_SIZE = (460, 122)
PLATE_PAD = 28

LAYOUT = {
    'slab': (960, 336),
    'dirt': (960, 488),
    'buttons': [(688, 700), (1232, 700)],
    'labels': ['もう一度挑む', 'タイトルへ戻る'],
    'label_px': 44,
    'label_offset': (0, -24),         # 鉄札の中心から TextRenderer の基準点(上辺中央)まで
    'label_shadow_offset': (2, -22),
    'label_color': (188, 196, 204),
    'label_lit_color': (255, 232, 198),
    'plate_event_size': PLATE_SIZE,   # Button の当たり判定
    'hint_y': 880,
    'hint_px': 23,
    'hint_color': (198, 206, 212),
    'hints': [('move', '選ぶ'), ('confirm', '決める')],
    'veil_scale': 0.47,               # BlackMask.png (4096x2894) で画面を覆う倍率
}


# ---------------------------------------------------------------- 下回り
def s(v):
    return int(round(v * SS))


def font(path, px):
    return ImageFont.truetype(str(path), max(1, int(round(px))))


def fbm(w, h, seed, octaves=5, base=4):
    rng = np.random.default_rng(seed)
    out = np.zeros((h, w), np.float32)
    amp, total, res = 1.0, 0.0, base
    for _ in range(octaves):
        n = (rng.random((res + 1, res + 1)) * 255).astype(np.uint8)
        img = Image.fromarray(n).resize((w, h), Image.BICUBIC)
        out += amp * (np.asarray(img, np.float32) / 255.0)
        total += amp
        amp *= 0.5
        res *= 2
    return out / total


def clip_to(img, mask):
    out = img.copy()
    out.putalpha(ImageChops.multiply(img.getchannel('A'), mask))
    return out


def ragged_mask(ww, hh, seed, inset, jitter_x, jitter_y, steps_x=46, steps_y=14):
    """縁を不規則に削った矩形のマスク"""
    rng = np.random.default_rng(seed)
    pts = []
    for i in range(steps_x):
        pts.append((i / steps_x * ww, inset + rng.normal(0, jitter_y)))
    for i in range(steps_y):
        pts.append((ww - inset + rng.normal(0, jitter_x), i / steps_y * hh))
    for i in range(steps_x):
        pts.append((ww - i / steps_x * ww, hh - inset + rng.normal(0, jitter_y * 1.4)))
    for i in range(steps_y):
        pts.append((inset + rng.normal(0, jitter_x), hh - i / steps_y * hh))
    mask = Image.new('L', (ww, hh), 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    return mask


def bevel(img, mask, light, dark, width):
    """上の縁を光らせ、下の縁を落とす"""
    edge = mask.filter(ImageFilter.FIND_EDGES).filter(ImageFilter.GaussianBlur(width))
    hi = Image.new('RGBA', img.size, light[:3] + (0,))
    hi.putalpha(edge.point(lambda v: min(255, int(v * light[3] / 255))))
    img.alpha_composite(hi, (0, -max(1, int(width * 2))))
    lo = Image.new('RGBA', img.size, dark[:3] + (0,))
    lo.putalpha(edge.point(lambda v: min(255, int(v * dark[3] / 255))))
    img.alpha_composite(lo, (0, max(1, int(width * 2))))
    return edge


def carve(base, draw_fn, depth, light=(214, 216, 222, 150), dark=(8, 8, 9, 245)):
    """彫り込み。凹んで見えるように、上/左に影、下/右にハイライトを置く"""
    def make(col):
        layer = Image.new('RGBA', base.size, (0, 0, 0, 0))
        draw_fn(ImageDraw.Draw(layer), col)
        return layer
    o = max(1, int(depth))
    base.alpha_composite(make(light).filter(ImageFilter.GaussianBlur(o * 0.45)), (o, o))
    base.alpha_composite(make(dark).filter(ImageFilter.GaussianBlur(o * 0.35)), (-o, -o))
    base.alpha_composite(make((0, 0, 0, 150)))


def with_shadow(img, pad, blur, offset, alpha):
    """影を焼き込んだ一回り大きい絵にする。pad を上下左右に同じだけ取るので中心はずれない"""
    w, h = img.size
    out = Image.new('RGBA', (w + pad * 2, h + pad * 2), (0, 0, 0, 0))
    shadow = Image.new('RGBA', img.size, (0, 0, 0, 0))
    shadow.putalpha(img.getchannel('A').point(lambda v: int(v * alpha / 255)))
    layer = Image.new('RGBA', out.size, (0, 0, 0, 0))
    layer.alpha_composite(shadow, (pad + offset[0], pad + offset[1]))
    out.alpha_composite(layer.filter(ImageFilter.GaussianBlur(blur)))
    out.alpha_composite(img, (pad, pad))
    return out


def downsample(img):
    return img.resize((img.width // SS, img.height // SS), Image.LANCZOS)


# ---------------------------------------------------------------- 石版
def stone_slab():
    ww, hh = s(SLAB_SIZE[0]), s(SLAB_SIZE[1])
    n = fbm(ww, hh, 11, octaves=6, base=5)
    n2 = fbm(ww, hh, 18, octaves=4, base=18)
    big = fbm(ww, hh, 32, octaves=2, base=2)
    t = np.linspace(0, 1, hh, dtype=np.float32)[:, None]
    grey = (0.10 + 0.20 * n + 0.06 * n2) * (0.72 + 0.55 * big) * (1.22 - 0.55 * t)
    col = np.stack([grey * 1.00, grey * 1.01, grey * 1.06], -1)   # 空の下なので石はわずかに青い
    dirt = np.clip(t - 0.74, 0, None) * 3.0 * (0.4 + 0.7 * n2)
    col = col * (1 - dirt[..., None] * 0.8) + np.array([0.10, 0.085, 0.06], np.float32) * dirt[..., None] * 0.8
    img = Image.fromarray((np.clip(col, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')

    mask = ragged_mask(ww, hh, 11, s(7), s(4), s(4))
    img.putalpha(mask)
    bevel(img, mask, (206, 210, 216, 128), (6, 6, 8, 242), s(1.4))

    rng = np.random.default_rng(11)
    cracks = Image.new('RGBA', img.size, (0, 0, 0, 0))
    cd = ImageDraw.Draw(cracks)
    for _ in range(4):
        x = rng.uniform(ww * 0.15, ww * 0.85)
        y = rng.uniform(0, hh * 0.25)
        path = [(x, y)]
        for _ in range(8):
            x += rng.normal(0, s(14))
            y += rng.uniform(s(12), s(28))
            path.append((x, y))
        cd.line([(p[0] + s(1.2), p[1] + s(1.2)) for p in path], fill=(196, 198, 204, 40), width=max(1, s(1.2)))
        cd.line(path, fill=(10, 10, 12, 150), width=max(1, s(1.4)))
    img.alpha_composite(clip_to(cracks, mask))

    title = font(SCRATCH_FONT, s(150))
    carve(img, lambda d, c: d.text((ww / 2, s(118)), '力尽きた', font=title, fill=c, anchor='mm'), depth=s(4.0))
    latin = font(LATIN_FONT, s(34))
    carve(img, lambda d, c: d.text((ww / 2, s(226)), 'G A M E   O V E R', font=latin, fill=c, anchor='mm'),
          depth=s(2.2), dark=(14, 12, 10, 190))

    img = img.rotate(SLAB_TILT_DEG, resample=Image.BICUBIC, expand=True)
    return downsample(with_shadow(img, s(SLAB_PAD), s(20), (s(6), s(20)), 200))


def slab_dirt():
    """石版の根元の土。着地した瞬間に出す"""
    ww, hh = s(DIRT_SIZE[0]), s(DIRT_SIZE[1])
    img = Image.new('RGBA', (ww, hh), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rng = np.random.default_rng(4)
    cy = hh * 0.34
    for _ in range(340):
        x = rng.uniform(s(30), ww - s(30))
        y = cy + abs(rng.normal(0, s(15)))
        r = rng.uniform(s(2), s(10))
        edge = min(x, ww - x) / (ww * 0.12)
        a = int(np.clip(190 - (y - cy) / (hh * 0.66) * 180, 15, 190) * min(1.0, edge))
        d.ellipse([x - r, y - r * 0.55, x + r, y + r * 0.55], fill=(32, 28, 22, a))
    return downsample(img.filter(ImageFilter.GaussianBlur(s(1.6))))


# ---------------------------------------------------------------- 鉄札
def plate_mask(ww, hh, seed):
    rng = np.random.default_rng(seed + 5)
    pts = []
    for i in range(30):
        pts.append((i / 30 * ww, s(4) + rng.normal(0, s(2.0))))
    for i in range(10):
        pts.append((ww - s(4) + rng.normal(0, s(1.8)), i / 10 * hh))
    for i in range(30):
        pts.append((ww - i / 30 * ww, hh - s(4) + rng.normal(0, s(2.2))))
    for i in range(10):
        pts.append((s(4) + rng.normal(0, s(1.8)), hh - i / 10 * hh))
    mask = Image.new('L', (ww, hh), 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    return mask


def iron_plate(lit, seed=21):
    """叩き出した鉄板。lit は縁が焼けた版（選択中）"""
    ww, hh = s(PLATE_SIZE[0]), s(PLATE_SIZE[1])
    n = fbm(ww, hh, seed, octaves=5, base=6)
    t = np.linspace(0, 1, hh, dtype=np.float32)[:, None, None]
    col = np.array([0.175, 0.19, 0.215], np.float32) * (0.70 + 0.85 * (1 - t) ** 1.7) * (0.78 + 0.48 * n[..., None])
    if lit:
        col = col + np.array([0.045, 0.019, 0.005], np.float32) * (0.30 + 0.90 * (1 - t))
    img = Image.fromarray((np.clip(col, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')

    mask = plate_mask(ww, hh, seed)
    img.putalpha(mask)
    edge = bevel(img, mask, (206, 216, 226, 153), (8, 10, 12, 230), s(1.2))
    if lit:
        glow = Image.new('RGBA', img.size, EMBER + (0,))
        glow.putalpha(edge)
        img.alpha_composite(glow.filter(ImageFilter.GaussianBlur(s(3))))

    nail = Image.open(NAIL_SPRITE).convert('RGBA').resize((s(20), s(20)), Image.LANCZOS)
    for cx, cy in [(s(24), s(22)), (ww - s(24), s(22)), (s(24), hh - s(22)), (ww - s(24), hh - s(22))]:
        img.alpha_composite(nail, (int(cx - s(10)), int(cy - s(10))))

    return downsample(with_shadow(img, s(PLATE_PAD), s(12), (s(4), s(12)), 200))


def plate_ember(seed=21):
    """選択中の札の周りで脈打つ熾火。札の絵と同じ大きさ・中心なので重ねて置くだけでよい。
    札より奥に描くので、内側の光は札に隠れて縁から外へ漏れる分だけが見える"""
    ww, hh = s(PLATE_SIZE[0]), s(PLATE_SIZE[1])
    mask = plate_mask(ww, hh, seed)
    canvas = Image.new('L', (ww + s(PLATE_PAD) * 2, hh + s(PLATE_PAD) * 2), 0)
    canvas.paste(mask, (s(PLATE_PAD), s(PLATE_PAD)))
    glow = canvas.filter(ImageFilter.GaussianBlur(s(10))).point(lambda v: min(255, int(v * 1.6)))
    img = Image.new('RGBA', canvas.size, EMBER + (0,))
    img.putalpha(glow)
    hot = Image.new('RGBA', canvas.size, (255, 196, 120, 0))
    hot.putalpha(canvas.filter(ImageFilter.FIND_EDGES).filter(ImageFilter.GaussianBlur(s(2))).point(lambda v: min(255, v)))
    img.alpha_composite(hot)
    return downsample(img)


# ---------------------------------------------------------------- 書き出し
def sprites():
    return {
        'GameOver_Slab': stone_slab(),
        'GameOver_SlabDirt': slab_dirt(),
        'GameOver_Plate': iron_plate(lit=False),
        'GameOver_PlateLit': iron_plate(lit=True),
        'GameOver_PlateEmber': plate_ember(),
    }


def write_sprite(out_dir, name, image):
    from tools.scene import sprite_meta
    png = out_dir / f'{name}.png'
    image.save(png)
    meta = out_dir / f'{name}.png.meta'
    guid = sprite_meta.read_meta(meta)['guid'] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    print(f'  {name}.png  {image.size[0]}x{image.size[1]}  guid={guid}')


def emit():
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {EMIT_DIR}')
    for name, image in sprites().items():
        write_sprite(EMIT_DIR, name, image)


# ---------------------------------------------------------------- 完成イメージ
def grade(base):
    """死亡時の色調（LowHealthScreenEffect が HP0 で掛ける彩度抜き＋減光）を真似る"""
    a = np.asarray(base.convert('RGB').filter(ImageFilter.GaussianBlur(1.2)), np.float32) / 255.0
    lum = (a * np.array([0.299, 0.587, 0.114], np.float32)).sum(2, keepdims=True)
    a = (a * 0.14 + lum * 0.86) * np.array([0.90, 0.95, 1.06], np.float32) * 0.62
    return Image.fromarray((np.clip(a, 0, 1) * 255).astype(np.uint8), 'RGB').convert('RGBA')


def paste_center(dst, img, center, alpha=255):
    if alpha < 255:
        img = img.copy()
        img.putalpha(img.getchannel('A').point(lambda v: v * alpha // 255))
    dst.alpha_composite(img, (int(center[0] - img.width / 2), int(center[1] - img.height / 2)))


def mock(shot_path, out_dir, veil_alpha=165):
    base = Image.open(shot_path).convert('RGBA')
    if base.size != (SCREEN_W, SCREEN_H):
        base = base.resize((SCREEN_W, SCREEN_H), Image.LANCZOS)
    im = grade(base)
    im.alpha_composite(Image.new('RGBA', im.size, (0, 0, 0, veil_alpha)))

    sp = sprites()
    paste_center(im, sp['GameOver_SlabDirt'], LAYOUT['dirt'])
    paste_center(im, sp['GameOver_Slab'], LAYOUT['slab'])

    d = ImageDraw.Draw(im)
    label_font = font(BODY_FONT, LAYOUT['label_px'])
    for i, (center, label) in enumerate(zip(LAYOUT['buttons'], LAYOUT['labels'])):
        lit = i == 0
        if lit:
            paste_center(im, sp['GameOver_PlateEmber'], center, 200)
        paste_center(im, sp['GameOver_PlateLit' if lit else 'GameOver_Plate'], center)
        tx, ty = center[0] + LAYOUT['label_offset'][0], center[1] + LAYOUT['label_offset'][1]
        sx, sy = center[0] + LAYOUT['label_shadow_offset'][0], center[1] + LAYOUT['label_shadow_offset'][1]
        d.text((sx, sy), label, font=label_font, fill=(6, 8, 10, 190), anchor='ma')
        d.text((tx, ty), label, font=label_font, fill=LAYOUT['label_lit_color' if lit else 'label_color'], anchor='ma')

    for item in hint_layout():
        if item['kind'] == 'tag':
            paste_center(im, Image.open(item['sprite']).convert('RGBA'), item['pos'])
        else:
            d.text(item['pos'], item['text'], font=font(BODY_FONT, LAYOUT['hint_px']), fill=LAYOUT['hint_color'], anchor='la')

    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / 'gameover_A_ingame.png'
    im.convert('RGB').save(path)
    print(f'wrote {path}')


def hint_layout():
    """操作ヒント1行の並び。タグ(中心基準)と文字(左上基準)を中央揃えで並べる"""
    text_font = font(BODY_FONT, LAYOUT['hint_px'])
    sprites_by_kind = {'move': HINT_MOVE_SPRITE, 'confirm': HINT_CONFIRM_SPRITE}
    gap, spacing = 12, 56
    parts = []
    for kind, label in LAYOUT['hints']:
        tag_w = Image.open(sprites_by_kind[kind]).width
        text_w = text_font.getlength(label)
        parts.append((kind, label, tag_w, text_w))
    total = sum(tag_w + gap + text_w for _, _, tag_w, text_w in parts) + spacing * (len(parts) - 1)

    y = LAYOUT['hint_y']
    x = SCREEN_W / 2 - total / 2
    items = []
    for kind, label, tag_w, text_w in parts:
        items.append({'kind': 'tag', 'name': kind, 'sprite': sprites_by_kind[kind], 'pos': (round(x + tag_w / 2), y)})
        x += tag_w + gap
        items.append({'kind': 'text', 'name': kind, 'text': label, 'pos': (round(x), round(y - LAYOUT['hint_px'] / 2 - 1))})
        x += text_w + spacing
    return items


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--emit', action='store_true', help='スプライトを Assets/Art/UI/GameOver へ書き出す')
    ap.add_argument('--shot', default='', help='合成先のゲーム画面(1920x1080)')
    ap.add_argument('--out-dir', default='')
    args = ap.parse_args()

    if args.emit:
        emit()
    if args.shot:
        if not args.out_dir:
            raise SystemExit('--shot には --out-dir も渡す')
        mock(Path(args.shot), Path(args.out_dir))
    if not args.emit and not args.shot:
        ap.print_help()


if __name__ == '__main__':
    main()
