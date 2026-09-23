"""ゲーム内マウスカーソルのスプライト生成

  python tools/art/game_cursor.py --mock --out-dir <dir>   # 3案の比較シートと実寸合成
  python tools/art/game_cursor.py --emit                    # 採用した案A を Assets/Art/UI/Cursor/ に書き出す

1コマ 64x64 (4倍で描いて縮小)。ホットスポットは全案とも左上 (HOT, HOT)。
"""
import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
from character_select import (  # noqa: E402
    BRASS, INK, REPO_ROOT, STEEL, WAX, bevel, fbm, grid, parchment, rgba, soften, text, wood_board, write_sprite)

CELL = 64
SS = 4
S = CELL * SS
HOT = 5
IDLE_FRAMES = 8
PRESS_FRAMES = 4

FEATHER = np.array([0.94, 0.92, 0.86], np.float32)
WOOD = np.array([0.42, 0.26, 0.14], np.float32)
GLINT = np.array([1.0, 0.95, 0.80], np.float32)

EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Cursor'
# NOTE: 光が抜ける 8 コマの前に止め絵を並べて間を作る (ImageAnimationRenderer の cooldown 中は描画されないため)
IDLE_HOLD_FRAMES = 12


# ---------------------------------------------------------------- 共通
def poly_mask(points, scale=1.0, center=(HOT, HOT), angle=0.0):
    """64 単位の多角形を S 解像度のマスクにする。center 周りに縮小・回転"""
    cx, cy = center
    ca, sa = math.cos(math.radians(angle)), math.sin(math.radians(angle))
    pts = []
    for x, y in points:
        dx, dy = (x - cx) * scale, (y - cy) * scale
        pts.append(((cx + dx * ca - dy * sa) * SS, (cy + dx * sa + dy * ca) * SS))
    im = Image.new('L', (S, S), 0)
    ImageDraw.Draw(im).polygon(pts, fill=255)
    return np.asarray(im, np.float32) / 255.0


def ellipse_mask(cx, cy, r):
    im = Image.new('L', (S, S), 0)
    ImageDraw.Draw(im).ellipse(((cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS), fill=255)
    return np.asarray(im, np.float32) / 255.0


def dilate(mask, px):
    im = Image.fromarray((np.clip(mask, 0, 1) * 255).astype(np.uint8), 'L')
    size = int(px * SS) * 2 + 1
    return np.asarray(im.filter(ImageFilter.MaxFilter(size)), np.float32) / 255.0


def metal(mask, base, seed, gloss=0.5):
    """マスクを金属っぽく塗る。上が明るく、ざらつきを少し"""
    xx, yy = grid(S, S)
    n = fbm(S, S, seed, octaves=4, base=6)[..., None]
    b = bevel(mask, 2.0 * SS)[..., None]
    grad = (1.15 - yy / S * 0.45)[..., None]
    col = base * grad * (0.85 + 0.3 * n) + b * gloss
    return col


def compose(layers):
    """[(rgb, alpha)] を下から重ねる"""
    rgb = np.zeros((S, S, 3), np.float32)
    a = np.zeros((S, S), np.float32)
    for c, m in layers:
        m = np.clip(m, 0, 1)
        rgb = c * m[..., None] + rgb * (1 - m[..., None])
        a = m + a * (1 - m)
    out = rgb / np.maximum(a[..., None], 1e-4)
    return rgba(out, a)


def outlined(shape_layers, silhouette, ink_px=1.6):
    ink = np.array(INK, np.float32) / 255.0
    out = dilate(silhouette, ink_px)
    return [(np.broadcast_to(ink, (S, S, 3)), soften(out, 0.6 * SS))] + shape_layers


def finish(im):
    """縮小して、小さな影を付ける"""
    small = im.resize((CELL, CELL), Image.LANCZOS)
    a = np.asarray(small.split()[-1], np.float32) / 255.0
    sh = np.zeros_like(a)
    sh[2:, 2:] = a[:-2, :-2]
    sh = soften(sh, 1.2) * 0.45
    base = rgba(np.zeros(a.shape + (3,), np.float32), sh)
    base.alpha_composite(small)
    return base


def with_spark(im, strength):
    """先端に四方へ伸びる火花を足す"""
    xx, yy = grid(S, S)
    cx, cy = (HOT + 1) * SS, (HOT + 1) * SS
    dx, dy = np.abs(xx - cx) / SS, np.abs(yy - cy) / SS
    rays = np.exp(-dy / 0.9) * np.exp(-dx / (6 * strength + 1)) + np.exp(-dx / 0.9) * np.exp(-dy / (6 * strength + 1))
    core = np.exp(-(dx ** 2 + dy ** 2) / (4 * strength + 0.5))
    a = np.clip((rays * 0.9 + core) * strength, 0, 1)
    spark = rgba(np.broadcast_to(np.array([1.0, 0.93, 0.7], np.float32), (S, S, 3)), a)
    out = im.copy()
    out.alpha_composite(spark)
    return out


# ---------------------------------------------------------------- 案1: 真鍮の矢じり
ARROW = [(HOT, HOT), (HOT, 47), (16, 38), (24, 56), (32, 52), (24, 35.5), (39, 35.5)]


def arrow_frame(kind, k):
    scale = 1.0
    glint_pos = None
    glow = 0.0
    if kind == 'idle':
        # NOTE: 8コマ中 2..6 で光の帯が左上から右下へ抜ける
        if 2 <= k <= 6:
            glint_pos = (k - 2) / 4.0
    else:
        scale = [0.8, 0.86, 0.94, 1.0][k]
        glow = [1.0, 0.8, 0.45, 0.15][k]
    m = soften(poly_mask(ARROW, scale), 0.5 * SS)
    col = metal(m, BRASS, 3, gloss=0.55)
    # 中央の溝 (刻印)
    groove = poly_mask([(HOT + 3, HOT + 9), (HOT + 3, 38), (13, 31), (HOT + 4, 32)], scale)
    col = col * (1 - 0.25 * soften(groove, SS)[..., None])
    layers = [(col, m)]
    if glint_pos is not None:
        xx, yy = grid(S, S)
        d = (xx + yy) / (2 * S) - (glint_pos * 1.1 - 0.05)
        band = np.exp(-(d / 0.045) ** 2) * m
        layers.append((np.broadcast_to(GLINT, (S, S, 3)), band * 0.9))
    if glow > 0:
        red = np.array(STAMP_GLOW, np.float32)
        layers.append((np.broadcast_to(red, (S, S, 3)), np.clip(soften(dilate(groove, 1.5), SS) * glow * 1.2, 0, 1)))
    im = compose(outlined(layers, m, 2.4))
    if glow > 0:
        im = with_spark(im, glow)
    return finish(im)


STAMP_GLOW = (1.0, 0.45, 0.25)


# ---------------------------------------------------------------- 案2: 羽ペン
def quill_frame(kind, k):
    sway = math.sin(k / IDLE_FRAMES * 2 * math.pi) * 5.0 if kind == 'idle' else 0.0
    nib = poly_mask([(HOT, HOT), (HOT + 11, HOT + 5), (HOT + 13, HOT + 13), (HOT + 5, HOT + 11)])
    nib_col = metal(nib, STEEL * 1.8, 5, gloss=0.6)
    slit = poly_mask([(HOT + 1, HOT + 1), (HOT + 9, HOT + 8), (HOT + 8, HOT + 9)])
    # 羽根は付け根 (18,18) を中心に揺らす
    pivot = (HOT + 12, HOT + 12)
    shaft = poly_mask([(HOT + 10, HOT + 12), (HOT + 12, HOT + 10), (58, 57), (57, 58)], 1.0, pivot, sway)
    vane_pts = [(HOT + 16, HOT + 18), (22, 30), (32, 44), (46, 58), (58, 60), (60, 50), (56, 36), (44, 24),
                (30, 18), (HOT + 18, HOT + 16)]
    vane = soften(poly_mask(vane_pts, 1.0, pivot, sway), 0.6 * SS)
    xx, yy = grid(S, S)
    barbs = 0.5 + 0.5 * np.sin((xx - yy) / SS * 1.6)
    vane_col = FEATHER * (0.82 + 0.18 * barbs[..., None]) * (1.1 - (xx + yy)[..., None] / (2 * S) * 0.35)
    shaft_col = np.broadcast_to(WOOD * 1.4, (S, S, 3))
    ink = np.array(INK, np.float32) / 255.0
    layers = [(vane_col, vane), (shaft_col, soften(shaft, 0.5 * SS)), (nib_col, nib),
              (np.broadcast_to(ink, (S, S, 3)), slit * 0.9)]
    if kind == 'press':
        # NOTE: ペン先からインクが一滴落ちて消える
        r = [3.0, 4.0, 3.6, 2.6][k]
        dy = [3, 7, 12, 17][k]
        a = [1, 1, 0.85, 0.5][k]
        rim = ellipse_mask(HOT + 1, HOT + dy, r + 1.2)
        drop = ellipse_mask(HOT + 1, HOT + dy, r)
        hi = ellipse_mask(HOT + 0.2, HOT + dy - r * 0.4, r * 0.35)
        layers.append((np.broadcast_to(np.array([0.94, 0.88, 0.76], np.float32), (S, S, 3)), rim * a))
        layers.append((np.broadcast_to(np.array([0.10, 0.12, 0.28], np.float32), (S, S, 3)), drop * a))
        layers.append((np.broadcast_to(GLINT, (S, S, 3)), hi * a))
    sil = np.maximum.reduce([vane, shaft, nib])
    return finish(compose(outlined(layers, sil, 1.3)))


# ---------------------------------------------------------------- 案3: 羅針盤の針
def needle_frame(kind, k):
    ang = math.sin(k / IDLE_FRAMES * 2 * math.pi) * 4.0 if kind == 'idle' else 0.0
    L, W = 50.0, 7.5
    # 先端 (HOT,HOT) から右下 45° へ伸びる菱形。先端側半分は蝋の赤、後ろ半分は鉄
    ux, uy = 1 / math.sqrt(2), 1 / math.sqrt(2)
    px, py = -uy, ux

    def at(t, w):
        return (HOT + ux * t + px * w, HOT + uy * t + py * w)

    front = poly_mask([at(0, 0), at(L * 0.5, W), at(L * 0.5, -W)], 1.0, (HOT, HOT), ang)
    back = poly_mask([at(L * 0.5, W), at(L, 0), at(L * 0.5, -W)], 1.0, (HOT, HOT), ang)
    ridge = poly_mask([at(0, 0), at(L, 0.8), at(L, 0)], 1.0, (HOT, HOT), ang)
    front, back = soften(front, 0.4 * SS), soften(back, 0.4 * SS)
    c = at(L * 0.5, 0)
    ca, sa = math.cos(math.radians(ang)), math.sin(math.radians(ang))
    cx, cy = HOT + (c[0] - HOT) * ca - (c[1] - HOT) * sa, HOT + (c[0] - HOT) * sa + (c[1] - HOT) * ca
    pin = ellipse_mask(cx, cy, 4.2)
    layers = [(metal(front, WAX * 1.6, 7, 0.45), front), (metal(back, STEEL * 1.9, 9, 0.5), back),
              (np.broadcast_to(GLINT, (S, S, 3)), ridge * 0.35), (metal(pin, BRASS * 1.1, 11, 0.7), pin)]
    sil = np.maximum.reduce([front, back, pin])
    if kind == 'press':
        layers.append((np.broadcast_to(GLINT, (S, S, 3)), ridge * 0.6 * [1.0, 0.75, 0.45, 0.15][k]))
    im = compose(outlined(layers, sil, 1.8))
    if kind == 'press':
        # NOTE: 止まった針に光が走り、先端に火花
        strength = [1.0, 0.75, 0.45, 0.15][k]
        im = with_spark(im, strength)
    return finish(im)


DESIGNS = [
    ('A', '真鍮の矢じり', arrow_frame, '待機: 光が斜めに走る / クリック: 縮んで刻印が赤く光り火花'),
    ('B', '羽ペン', quill_frame, '待機: 羽根がゆっくり揺れる / クリック: インクが一滴落ちる'),
    ('C', '羅針盤の針', needle_frame, '待機: 針が ±4° 振れる / クリック: 針に光が走り先端に火花'),
]


def frames(fn):
    idle = [fn('idle', k) for k in range(IDLE_FRAMES)]
    press = [fn('press', k) for k in range(PRESS_FRAMES)]
    return idle, press


# ---------------------------------------------------------------- モック
def load_title():
    im = Image.open(REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Sample' / 'BackGround.png').convert('RGBA')
    w, h = im.size
    th = int(w * 9 / 16)
    top = (h - th) // 2
    return im.crop((0, top, w, top + th)).resize((1920, 1080), Image.LANCZOS)


def checker(w, h):
    xx, yy = grid(w, h)
    c = ((xx // 8 + yy // 8) % 2)[..., None] * 0.08 + 0.18
    return rgba(np.broadcast_to(c, (h, w, 3)), np.ones((h, w), np.float32))


def mock_sheet(all_frames):
    zoom = 3
    cw = CELL * zoom
    pad = 24
    row_h = cw + 110
    W = pad * 2 + (IDLE_FRAMES + PRESS_FRAMES) * (cw + 8) + 40 + 200
    H = 90 + row_h * len(DESIGNS)
    sheet = wood_board(W, H, 41).convert('RGBA')
    dark = Image.new('RGBA', (W, H), (0, 0, 0, 120))
    sheet.alpha_composite(dark)
    text(sheet, (pad, 22), 'マウスカーソル案 (3倍表示 / 右端は実寸)', 34, (240, 226, 202), shadow=(2, 2, (0, 0, 0)))
    for i, ((key, name, _, desc), (idle, press)) in enumerate(zip(DESIGNS, all_frames)):
        y = 90 + i * row_h
        text(sheet, (pad, y), f'{key}. {name}', 30, (250, 228, 150), shadow=(2, 2, (0, 0, 0)))
        text(sheet, (pad + 300, y + 6), desc, 22, (240, 226, 202), shadow=(2, 2, (0, 0, 0)))
        x = pad
        for j, f in enumerate(idle + press):
            if j == IDLE_FRAMES:
                x += 40
            cell = checker(cw, cw)
            cell.alpha_composite(f.resize((cw, cw), Image.NEAREST))
            sheet.alpha_composite(cell, (x, y + 44))
            tag = f'idle {j}' if j < IDLE_FRAMES else f'click {j - IDLE_FRAMES}'
            text(sheet, (x + 4, y + 46), tag, 16, (220, 220, 220))
            x += cw + 8
        # 実寸: 羊皮紙の上と暗い地の上
        px = x + 20
        paper = parchment(90, 80, 3 + i).convert('RGBA')
        sheet.alpha_composite(paper, (px, y + 44))
        sheet.alpha_composite(idle[0], (px + 18, y + 52))
        sheet.alpha_composite(Image.new('RGBA', (90, 80), (6, 20, 26, 230)), (px + 100, y + 44))
        sheet.alpha_composite(idle[0], (px + 118, y + 52))
    return sheet


def mock_scene(all_frames):
    base = load_title()
    # 画面上に実寸で3案を並べる (メニューボタン付近を指している想定)
    spots = [(700, 640), (960, 640), (1220, 640)]
    for (key, name, _, _), (idle, press), (x, y) in zip(DESIGNS, all_frames, spots):
        base.alpha_composite(idle[3], (x - HOT, y - HOT))
        text(base, (x + 10, y + 80), f'{key}. {name}', 26, (240, 226, 202), shadow=(2, 2, (0, 0, 0)))
        big = idle[3].resize((CELL * 2, CELL * 2), Image.LANCZOS)
        base.alpha_composite(big, (x - HOT * 2 + 10, y - 190))
    return base


# ---------------------------------------------------------------- 書き出し
def sheet(cells):
    out = Image.new('RGBA', (CELL * len(cells), CELL))
    for j, f in enumerate(cells):
        out.alpha_composite(f, (j * CELL, 0))
    return out


def write_sprite_animation(out_dir, name, sheet_name, count):
    """SpriteSheet 形式の .spriteAnimation と .meta を書く。guid は既存を保つ"""
    from tools.scene import sprite_meta
    (out_dir / f'{name}.spriteAnimation').write_bytes(b'')
    meta = out_dir / f'{name}.spriteAnimation.meta'
    guid = sprite_meta.mint_guid()
    if meta.exists():
        guid = json.loads(meta.read_text(encoding='utf-8'))['value0']['ptr_wrapper']['data']['guid_']['value_']
    sprite_guid = sprite_meta.read_meta(out_dir / f'{sheet_name}.png.meta')['guid']
    # NOTE: 既存の .spriteAnimation.meta と同じく、フォルダは \ 区切りでファイル名の前だけ /
    rel_dir = out_dir.resolve().relative_to(REPO_ROOT.resolve())
    content_path = '\\'.join(rel_dir.parts) + f'/{name}.spriteAnimation'
    data = {'value0': {
        'polymorphic_id': 2147483649,
        'polymorphic_name': 'NanamiEngine::Module::Asset::SpriteAnimationFile',
        'ptr_wrapper': {'id': 2147483649, 'data': {
            'cereal_class_version': 0,
            'value0': {'cereal_class_version': 0},
            'sourceType_': 1,
            'sprites_': 0,
            'sprite_': {'cereal_class_version': 0, 'value0': {
                'polymorphic_id': 1073741824,
                'ptr_wrapper': {'id': 2147483650, 'data': {
                    'cereal_class_version': 0,
                    'value0': {'cereal_class_version': 0, 'value_': sprite_guid}}}}},
            'splitCount_': count,
            'splitXCount_': count,
            'splitYCount_': 1,
            'splitSizeX_': CELL,
            'splitSizeY_': CELL,
            'contentPath_': content_path,
            'guid_': {'value_': guid}}}}}
    meta.write_text(json.dumps(data, indent=4, ensure_ascii=False), encoding='utf-8')
    print(f'  {name}.spriteAnimation  {count} frames  guid={guid}')


def emit(out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {out_dir}')
    still = arrow_frame('idle', 0)
    idle = [still] * IDLE_HOLD_FRAMES + [arrow_frame('idle', k) for k in range(IDLE_FRAMES)]
    press = [arrow_frame('press', k) for k in range(PRESS_FRAMES)]
    write_sprite(out_dir, 'Cursor_Idle', sheet(idle))
    write_sprite(out_dir, 'Cursor_Press', sheet(press))
    write_sprite_animation(out_dir, 'Cursor_Idle', 'Cursor_Idle', len(idle))
    write_sprite_animation(out_dir, 'Cursor_Press', 'Cursor_Press', len(press))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--mock', action='store_true')
    ap.add_argument('--emit', action='store_true')
    ap.add_argument('--out-dir', type=Path)
    args = ap.parse_args()
    if args.emit:
        emit(args.out_dir or EMIT_DIR)
        return
    if args.out_dir is None:
        ap.error('--mock には --out-dir が必要です')
    args.out_dir.mkdir(parents=True, exist_ok=True)
    all_frames = [frames(fn) for _, _, fn, _ in DESIGNS]
    if args.mock:
        mock_sheet(all_frames).save(args.out_dir / 'cursor_options.png')
        mock_scene(all_frames).save(args.out_dir / 'cursor_on_title.png')
        for (key, _, _, _), (idle, press) in zip(DESIGNS, all_frames):
            strip = Image.new('RGBA', (CELL * len(idle + press), CELL))
            for j, f in enumerate(idle + press):
                strip.alpha_composite(f, (j * CELL, 0))
            strip.save(args.out_dir / f'cursor_{key}_frames.png')
        print(f'wrote mocks to {args.out_dir}')


if __name__ == '__main__':
    main()
