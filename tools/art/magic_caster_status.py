"""Generate the MagicCaster status UI sprites (紫の秘術) from the KnightStatusUI art.

    python tools/art/magic_caster_status.py [--portrait-source RENDER.png] [--out-dir Assets/Art/UI/MagicCasterStatusUI]

KnightStatusUI の画像は読むだけで書き換えない。枠は鋼を少し寒色へ寄せ、棘と矢じりに紫の宝石、輪に刻みを足す。
拳と靴のアイコンの代わりに杖と魔導書を同じメダル形で描く。体力・スタミナのゲージは Knight のものをそのまま使う。
--portrait-source は Blender で MagicCaster.fbx の顔から肩を背景透過で撮った画像。Knight の肖像画に寄せる仕上げ
（色の段差・輪郭の締め・周辺の暗さ）をかけ、人物の切り抜きを輪の内側で切って 240x230 の Portrait.png にする。
一度作った .png.meta は guid を保つので、描き直しても参照は切れない。

Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art.cannon_cooldown_gauge import STEEL, ramp  # noqa: E402
from tools.art.control_guide import Canvas, blur, cov, hexc, sd_circle, sd_rbox  # noqa: E402
from tools.scene import sprite_meta  # noqa: E402

KNIGHT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'KnightStatusUI'
PORTRAIT_SIZE = (240, 230)
ICON_SIZE = (86, 88)
SQ2 = math.sqrt(2)

WHITE = hexc('#ffffff')
BLACK = hexc('#000000')
VIOLET = hexc('#9b6bff')
GEM_RAMP = [(0.0, '#f0e6ff'), (0.35, '#a47bff'), (0.75, '#5a2fc4'), (1.0, '#2a1266')]

# 宝石と刻みの位置（Knight の画像の画素座標）
PORTRAIT_RING_CENTER = (175.0, 159.0)
PORTRAIT_RING_RADIUS = 131.0
PORTRAIT_GEMS = [(300.0, 20.0, 9.0), (342.0, 148.0, 8.0), (332.0, 250.0, 8.0), (44.0, 232.0, 7.0)]
# 肖像は輪の下に描かれる（renderPriority_ -5）。枠の中での肖像の左上と、切り抜く円の半径（輪の帯 104〜146 の中ほど）
PORTRAIT_OFFSET_IN_FRAME = (57.0, 43.0)
PORTRAIT_MASK_RADIUS = 124.0
BAR_GEMS = [(612.0, 27.0, 9.0), (30.0, 66.0, 7.0)]


# ---------------------------------------------------------------- recolor
def to_array(im):
    return np.asarray(im.convert('RGBA'), np.float32) / 255.0


def to_image(arr):
    return Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8), 'RGBA')


def arcane_steel(arr):
    """鋼を少し寒色の紫へ寄せる。赤や黄で描かれた刻印は紫の光に塗り替える"""
    rgb = arr[..., :3]
    lum = rgb @ np.array([0.299, 0.587, 0.114], np.float32)
    cool = np.clip(lum[..., None] * np.array([0.86, 0.82, 1.10], np.float32), 0, 1)
    out = rgb * 0.55 + cool * 0.45
    warm = np.clip((rgb[..., 0] - rgb[..., 2] - 0.06) * 6.0, 0, 1)[..., None]
    glow = np.clip(lum[..., None] * np.array([0.85, 0.62, 1.35], np.float32) + 0.08, 0, 1)
    out = out * (1 - warm) + glow * warm
    return np.concatenate([out, arr[..., 3:]], axis=-1)


def overlay(base, layer):
    """straight alpha の layer を base に重ねる"""
    a = layer[..., 3:]
    out = base.copy()
    out[..., :3] = layer[..., :3] * a + base[..., :3] * base[..., 3:] * (1 - a)
    out[..., 3:] = a + base[..., 3:] * (1 - a)
    out[..., :3] = np.where(out[..., 3:] > 1e-5, out[..., :3] / np.maximum(out[..., 3:], 1e-5), 0)
    return out


def gem_layer(size, gems):
    """紫の宝石（菱形のカット + 光）"""
    w, h = size
    c = Canvas(w, h)
    X, Y = c.X, c.Y
    for gx, gy, r in gems:
        u = ((X - gx) + (Y - gy)) / SQ2
        v = ((X - gx) - (Y - gy)) / SQ2
        a = r / SQ2
        d = sd_rbox(u, v, -a, -a, a, a, r * 0.12)
        c.over(VIOLET, np.clip(blur(cov(d), r * 0.45) * 1.6, 0, 1) * 0.55)
        c.over(BLACK, cov(d - 1.2) * 0.85)
        c.over(ramp(np.clip((np.hypot(X - (gx - r * 0.3), Y - (gy - r * 0.35)) / (r * 1.3)), 0, 1), GEM_RAMP), cov(d))
        c.over(WHITE, cov(sd_circle(X, Y, gx - r * 0.3, gy - r * 0.32, r * 0.2)) * 0.85)
    return to_array(c.resolve())


def ring_runes(size, center, radius):
    """輪の面に細い刻みと紫の筋を入れる"""
    w, h = size
    c = Canvas(w, h)
    X, Y = c.X, c.Y
    cx, cy = center
    rho = np.hypot(X - cx, Y - cy)
    th = np.arctan2(X - cx, -(Y - cy)) % (2 * math.pi)
    n = 36
    seg = (th / (2 * math.pi) * n) % 1.0
    idx = np.floor(th / (2 * math.pi) * n)
    band = (np.abs(rho - radius) < (3.2 + 2.0 * (idx % 3 == 0)))
    tick = cov(np.abs(seg - 0.5) * 2 * math.pi / n * rho - 0.6) * band
    c.over(hexc('#1a1426'), tick * 0.55)
    c.over(VIOLET, np.clip(blur(tick, 1.2) * 1.4, 0, 1) * 0.35)
    return to_array(c.resolve())


def render_portrait_frame():
    src = Image.open(KNIGHT_DIR / 'PortraitFrame.png')
    arr = arcane_steel(to_array(src))
    alpha = arr[..., 3:]
    runes = ring_runes(src.size, PORTRAIT_RING_CENTER, PORTRAIT_RING_RADIUS)
    runes[..., 3:] *= (alpha > 0.5)
    arr = overlay(arr, runes)
    arr = overlay(arr, gem_layer(src.size, PORTRAIT_GEMS))
    return to_image(arr)


def render_status_bar_frame():
    src = Image.open(KNIGHT_DIR / 'StatusBarFrame.png')
    arr = arcane_steel(to_array(src))
    arr = overlay(arr, gem_layer(src.size, BAR_GEMS))
    return to_image(arr)


# ---------------------------------------------------------------- medallion icons
def _medallion():
    w, h = ICON_SIZE
    c = Canvas(w, h)
    X, Y = c.X, c.Y
    cx, cy = w / 2, h / 2
    d = sd_circle(X, Y, cx, cy, 38)
    c.over(BLACK, blur(cov(d), 2.5) * 0.6)
    c.over(ramp((Y - (cy - 38)) / 76, STEEL), cov(d))
    t = np.clip(np.hypot(X - cx, Y - (cy - 6)) / 34, 0, 1)[..., None]
    c.over(hexc('#34303e') * (1 - t) + hexc('#141218') * t, cov(d + 4.5))
    c.over(WHITE, (cov(d) - cov(d + 1.2)) * np.clip((cy - Y) / 38 + 0.2, 0, 1) * 0.45)
    return c, X, Y, cx, cy


def _glyph(c, mask):
    c.over(BLACK, np.clip(blur(mask, 1.4) * 2.2, 0, 1) * 0.75)
    c.over(hexc('#e2e4ea'), mask)


def render_icon_staff():
    c, X, Y, cx, cy = _medallion()
    shaft = cov(np.abs(((X - cx) + (Y - cy)) / SQ2) - 2.6) * cov(np.abs(((X - cx) - (Y - cy)) / SQ2) - 22)
    u = ((X - (cx + 13)) - (Y - (cy - 13))) / SQ2
    v = ((X - (cx + 13)) + (Y - (cy - 13))) / SQ2
    crystal = cov(sd_rbox(u, v, -7.5, -5, 7.5, 5, 1.2))
    claw = cov(np.abs(np.hypot(X - (cx + 12), Y - (cy - 12)) - 9.5) - 1.4) * (((X - cx) - (Y - cy)) < 18)
    _glyph(c, np.clip(shaft + claw, 0, 1))
    c.over(VIOLET, np.clip(blur(crystal, 2.0) * 1.8, 0, 1) * 0.6)
    c.over(ramp(np.clip(np.hypot(X - (cx + 10), Y - (cy - 16)) / 11, 0, 1), GEM_RAMP), crystal)
    return c.resolve()


def render_icon_tome():
    c, X, Y, cx, cy = _medallion()
    cover = cov(sd_rbox(X, Y, cx - 16, cy - 20, cx + 16, cy + 20, 3))
    pages = cov(sd_rbox(X, Y, cx - 13, cy - 17, cx + 17, cy + 22, 2)) * (1 - cover)
    spine = cov(np.abs(X - (cx - 12)) - 1.6) * cov(np.abs(Y - cy) - 19)
    rune = cov(np.abs(np.hypot(X - (cx + 2), Y - cy) - 8) - 1.3) * cover
    _glyph(c, np.clip(cover + pages * 0.8, 0, 1))
    c.over(hexc('#3a2e52'), cover * (1 - cov(sd_rbox(X, Y, cx - 16, cy - 20, cx + 16, cy + 20, 3) + 2.2)))
    c.over(hexc('#8f8a9c'), spine)
    c.over(VIOLET, np.clip(blur(rune, 1.5) * 1.6, 0, 1) * 0.8)
    c.over(lighten(VIOLET, 0.5), rune)
    return c.resolve()


def lighten(col, t):
    return col * (1 - t) + WHITE * t


# ---------------------------------------------------------------- portrait
def render_portrait(source):
    """Blender の撮影画像を Knight の肖像画に寄せる：寄せて切り抜き、色の段差・輪郭の締め・周辺の暗さ。
    alpha は撮影の人物の切り抜きを輪の内側の円で切ったもの"""
    src = Image.open(source).convert('RGBA')
    target_w, target_h = PORTRAIT_SIZE
    scale = max(target_w / src.width, target_h / src.height)
    src = src.resize((round(src.width * scale), round(src.height * scale)), Image.LANCZOS)
    left, top = (src.width - target_w) // 2, (src.height - target_h) // 2
    src = src.crop((left, top, left + target_w, top + target_h))
    im = src.convert('RGB').filter(ImageFilter.ModeFilter(3)).filter(ImageFilter.SMOOTH_MORE)
    im = ImageEnhance.Contrast(im).enhance(1.15)
    im = ImageEnhance.Color(im).enhance(0.9)
    edges = im.filter(ImageFilter.FIND_EDGES).convert('L').point(lambda p: 255 if p > 48 else 0)
    arr = np.asarray(im, np.float32) / 255.0
    arr *= (1.0 - np.asarray(edges, np.float32)[..., None] / 255.0 * 0.35)
    ys, xs = np.mgrid[0:target_h, 0:target_w].astype(np.float32)
    vignette = np.clip(1.0 - (np.hypot((xs - target_w / 2) / target_w, (ys - target_h / 2) / target_h) - 0.25) * 1.4, 0.35, 1.0)
    arr *= vignette[..., None]
    cx = PORTRAIT_RING_CENTER[0] - PORTRAIT_OFFSET_IN_FRAME[0]
    cy = PORTRAIT_RING_CENTER[1] - PORTRAIT_OFFSET_IN_FRAME[1]
    ring_mask = cov(np.hypot(xs - cx, ys - cy) - PORTRAIT_MASK_RADIUS)
    alpha = np.asarray(src.getchannel('A'), np.float32) / 255.0 * ring_mask
    return to_image(np.concatenate([np.clip(arr, 0, 1), alpha[..., None]], axis=-1))


SPRITES = {
    'PortraitFrame': render_portrait_frame,
    'StatusBarFrame': render_status_bar_frame,
    'IconStaff': render_icon_staff,
    'IconTome': render_icon_tome,
}


def write_sprite(out_dir, name, image):
    png = out_dir / f'{name}.png'
    image.save(png)
    meta = out_dir / f'{name}.png.meta'
    guid = sprite_meta.read_meta(meta)['guid'] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    return guid


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/MagicCasterStatusUI')
    ap.add_argument('--portrait-source', help='Blender で撮った MagicCaster の顔〜肩の画像')
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    for name, fn in SPRITES.items():
        image = fn()
        guid = write_sprite(out_dir, name, image)
        print(f'{name:16s} {image.size[0]}x{image.size[1]}  {guid}')

    if args.portrait_source:
        image = render_portrait(args.portrait_source)
        guid = write_sprite(out_dir, 'Portrait', image)
        print(f'{"Portrait":16s} {image.size[0]}x{image.size[1]}  {guid}')


if __name__ == '__main__':
    main()
