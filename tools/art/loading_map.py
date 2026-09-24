"""ロード画面「紙の航路図」のスプライト書き出しと、配置の確認用プレビュー。

    python tools/art/loading_map.py --emit                     # Assets/Art/UI/Loading/Map へ書き出す
    python tools/art/loading_map.py --preview <out.png>        # 書き出した絵を layout() どおりに並べた 1920x1080

2026-09-22 にキャンバスのデザイン案 D(木の机に広げた羊皮紙の地図、飛行船が航路を進んだ距離が進捗)に決まった。
質感は character_select.py の羊皮紙・木・蝋・釘をそのまま使い、インクは同じ茶色(INK)と朱(STAMP_RED)。
地図の島・地名・方位記号など動かない物は地図の一枚絵に焼き込み、動く物(航路の破線・飛行船・雲・注記)だけを部品にする。
prefab は tools/art/loading_map_prefab.py が layout() を読んで組む。座標はすべて 1920x1080 の画面座標。

Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from character_select import (  # noqa: E402
    SCREEN_W, SCREEN_H, REPO_ROOT, BODY_FONT, BRUSH_FONT, INK, INK_FADE, STAMP_RED,
    fbm, rgba, grid, soften, drop_shadow, parchment, wood_board, nail, wax_seal, font, text, paste,
)

EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Loading' / 'Map'
SS = 2  # インクの線は倍の大きさで描いて縮め、ふちを滑らかにする

# ---------------------------------------------------------------- 配置
PAPER_W, PAPER_H = 1650, 900
PAPER_CENTER = (960, 504)
PAPER_ORIGIN = (PAPER_CENTER[0] - PAPER_W // 2, PAPER_CENTER[1] - PAPER_H // 2)   # (135, 54)
DESK_W, DESK_H = 2600, 1500

BASE_ISLAND = (495, 630)
GRASS_ISLAND = (1395, 372)
# 砂漠地帯 (docs/Story.md 第2章)。もとは岩石地帯の場所で、岩石地帯はやめた
DESERT_ISLAND = (1080, 787)
UNKNOWN_ISLAND = (960, 225)
COMPASS = (1662, 198)

DEPART_ROUTE = [(600, 592), (810, 300), (1140, 645), (1290, 412)]
PROLOGUE_ROUTE = [(165, 918), (300, 960), (402, 858), (474, 750)]
CONTINUE_GRASS_ROUTE = [(1830, 120), (1760, 150), (1600, 250), (1480, 320)]
DEPART_DESERT_ROUTE = [(600, 668), (720, 930), (900, 640), (1000, 760)]
CONTINUE_DESERT_ROUTE = [(1830, 1000), (1600, 960), (1330, 900), (1170, 820)]


def layout():
    """prefab と航路データが読む配置。座標は画面座標、pos は各部品の中心(文字は基準点)"""
    return {
        'desk': {'center': (960, 540)},
        'paper_shadow': {'center': (PAPER_CENTER[0] + 8, PAPER_CENTER[1] + 20)},
        'paper': {'center': PAPER_CENTER},
        'dash_count': 34,
        'kicker': {'pos': (201, 96), 'px': 34, 'color': INK, 'font': 'brush', 'align': 'left', 'text': '出 航'},
        'title': {'pos': (201, 136), 'px': 60, 'color': INK, 'font': 'body', 'align': 'left', 'text': '草原地帯へ'},
        'from_caption': {'pos': (495, 903), 'px': 21, 'color': STAMP_RED, 'font': 'body', 'align': 'center', 'text': '出 発 地'},
        'to_caption': {'pos': (1395, 603), 'px': 21, 'color': STAMP_RED, 'font': 'body', 'align': 'center', 'text': '目 的 地'},
        'dest_circle': {'center': (1395, 372)},
        'stamp': {'center': (1272, 312)},
        'ship': {'center': DEPART_ROUTE[0]},
        'trail_count': 3,
        'cloud_shadows': [(300, 234), (1020, 624), (1600, 444)],
        'front_clouds': [(700, 300, 0), (1500, 760, 1), (2300, 1000, 2)],
        'vignette': {'center': (960, 540)},
        'chip': {'center': (1680, 905)},
        'status': {'pos': (1680, 842), 'px': 36, 'color': INK, 'font': 'brush', 'align': 'center', 'text': '航行中…'},
        'percent': {'pos': (1680, 884), 'px': 66, 'color': INK, 'font': 'body', 'align': 'center', 'text': '0%'},
        'hint_glyph': {'center': (176, 1024)},
        'hint_text': {'pos': (212, 1008), 'px': 24, 'color': (238, 224, 198), 'font': 'body', 'align': 'left'},
    }


def routes():
    """.loadingRoute に書き出す航路。from=None はどこからでも使える"""
    base_caption = (495, 903)
    grass_caption = (1395, 603)
    base_circle = ((495, 637), 1.12)
    grass_circle = ((1395, 372), 1.0)
    desert_caption = (1080, 924)
    desert_circle = ((1080, 787), 1.0)
    return [
        dict(name='DepartToGrassLand', frm='MainIsland', to='GrassLand', pts=DEPART_ROUTE,
             kicker='出 航', title='草原地帯へ', status='航行中…',
             from_caption=('出 発 地', base_caption), to_caption=('目 的 地', grass_caption),
             circle=grass_circle, network=True, cloud=-1.0),
        dict(name='ReturnToMainIsland', frm='GrassLand', to='MainIsland', pts=list(reversed(DEPART_ROUTE)),
             kicker='帰 還', title='拠点へ帰還', status='帰還中…',
             from_caption=('出 発 地', grass_caption), to_caption=('目 的 地', base_caption),
             circle=base_circle, network=False, cloud=1.0),
        dict(name='PrologueToMainIsland', frm=None, to='FirstTouchDownMainIsLand', pts=PROLOGUE_ROUTE,
             kicker='序 章', title='拠点の島へ', status='航行中…',
             from_caption=None, to_caption=('目 的 地', base_caption),
             circle=base_circle, network=False, cloud=-1.0),
        dict(name='ContinueToMainIsland', frm=None, to='MainIsland', pts=PROLOGUE_ROUTE,
             kicker='続 き', title='拠点へ', status='航行中…',
             from_caption=None, to_caption=('目 的 地', base_caption),
             circle=base_circle, network=False, cloud=-1.0),
        dict(name='ContinueToGrassLand', frm=None, to='GrassLand', pts=CONTINUE_GRASS_ROUTE,
             kicker='続 き', title='草原地帯へ', status='航行中…',
             from_caption=None, to_caption=('目 的 地', grass_caption),
             circle=grass_circle, network=True, cloud=1.0),
        dict(name='RetryGrassLand', frm='GrassLand', to='GrassLand', hover=((1395, 330), (140, 52)),
             kicker='再 挑 戦', title='草原地帯へ', status='態勢を立て直し中…',
             from_caption=None, to_caption=None, circle=None, network=True, cloud=-1.0),
        dict(name='DepartToDesert', frm='MainIsland', to='Desert', pts=DEPART_DESERT_ROUTE,
             kicker='出 航', title='砂漠地帯へ', status='航行中…',
             from_caption=('出 発 地', base_caption), to_caption=('目 的 地', desert_caption),
             circle=desert_circle, network=True, cloud=-1.0),
        dict(name='ReturnToMainIslandFromDesert', frm='Desert', to='MainIsland', pts=list(reversed(DEPART_DESERT_ROUTE)),
             kicker='帰 還', title='拠点へ帰還', status='帰還中…',
             from_caption=('出 発 地', desert_caption), to_caption=('目 的 地', base_caption),
             circle=base_circle, network=False, cloud=1.0),
        dict(name='ContinueToDesert', frm=None, to='Desert', pts=CONTINUE_DESERT_ROUTE,
             kicker='続 き', title='砂漠地帯へ', status='航行中…',
             from_caption=None, to_caption=('目 的 地', desert_caption),
             circle=desert_circle, network=True, cloud=1.0),
        dict(name='RetryDesert', frm='Desert', to='Desert', hover=((1080, 745), (140, 52)),
             kicker='再 挑 戦', title='砂漠地帯へ', status='態勢を立て直し中…',
             from_caption=None, to_caption=None, circle=None, network=True, cloud=-1.0),
        dict(name='HoverToTitle', frm=None, to='Title', hover=((495, 560), (150, 58)),
             kicker='休 息', title='タイトルへ', status='帰り支度中…',
             from_caption=None, to_caption=None, circle=None, network=False, cloud=-1.0),
    ]


# ---------------------------------------------------------------- インクの道具
def ink_layer(w, h):
    return Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))


def shrink(layer):
    return layer.resize((layer.width // SS, layer.height // SS), Image.LANCZOS)


def s(v):
    return [c * SS for c in v] if isinstance(v, (tuple, list)) else v * SS


def lw(v):
    """線の太さ。ImageDraw は整数しか受けない"""
    return max(1, int(round(v * SS)))


def ink(alpha=1.0, color=INK):
    return (*color, int(255 * alpha))


def wobble_path(points, seed, amp=1.2):
    """手で引いた線に見えるよう、点を少しだけ揺らす"""
    rng = np.random.default_rng(seed)
    return [(x + rng.normal(0, amp), y + rng.normal(0, amp)) for x, y in points]


def blob(cx, cy, rx, ry, seed, lumps=0.12, count=64):
    """島の輪郭。楕円をノイズで崩す"""
    rng = np.random.default_rng(seed)
    phases = rng.random(3) * math.tau
    pts = []
    for i in range(count):
        a = i / count * math.tau
        k = 1 + lumps * (math.sin(a * 3 + phases[0]) * 0.5 + math.sin(a * 5 + phases[1]) * 0.3
                         + math.sin(a * 7 + phases[2]) * 0.2)
        pts.append((cx + math.cos(a) * rx * k, cy + math.sin(a) * ry * k))
    return pts


def draw_island(d, cx, cy, rx, ry, seed, alpha=1.0):
    outline = blob(cx, cy, rx, ry, seed)
    d.polygon([tuple(s(p)) for p in outline], fill=(104, 78, 54, int(36 * alpha)))
    d.line([tuple(s(p)) for p in outline + outline[:1]], fill=ink(alpha), width=lw(3), joint='curve')
    inner = blob(cx, cy - ry * 0.08, rx * 0.86, ry * 0.8, seed + 1)
    top = [p for p in inner if p[1] < cy]
    d.line([tuple(s(p)) for p in top], fill=ink(0.4 * alpha), width=lw(1))
    # 浮島の下側。ぎざぎざに垂れた岩を引く
    rng = np.random.default_rng(seed + 2)
    xs = np.linspace(cx - rx * 0.78, cx + rx * 0.78, 7)
    under = []
    for i, x in enumerate(xs):
        depth = ry * (0.18 if i % 2 == 0 else 0.5 + rng.random() * 0.3)
        under.append((x, cy + ry * 0.62 + depth))
    under = [(cx - rx * 0.82, cy + ry * 0.55)] + under + [(cx + rx * 0.82, cy + ry * 0.55)]
    d.line([tuple(s(p)) for p in wobble_path(under, seed + 3)], fill=ink(alpha), width=lw(2.4), joint='curve')
    for i in range(1, len(under) - 1, 2):
        x, y = under[i]
        d.line([tuple(s((x - 6, cy + ry * 0.7))), tuple(s((x - 2, y - 10)))], fill=ink(0.45 * alpha), width=lw(1.4))


def draw_towers(d, cx, cy, alpha=1.0):
    for dx, h in ((-27, 30), (-6, 48), (18, 27)):
        x = cx + dx
        d.line([tuple(s((x - 6, cy))), tuple(s((x - 6, cy - h))), tuple(s((x, cy - h - 12))),
                tuple(s((x + 6, cy - h))), tuple(s((x + 6, cy)))], fill=ink(alpha), width=lw(2.4), joint='curve')
    d.line([tuple(s((cx - 42, cy))), tuple(s((cx + 33, cy)))], fill=ink(alpha), width=lw(2.4))


def draw_trees(d, cx, cy, alpha=1.0):
    for dx, dy, r in ((-60, -6, 12), (-27, -21, 13), (12, -6, 12), (48, -24, 13), (78, 0, 10)):
        x, y = cx + dx, cy + dy
        d.ellipse([s(x - r), s(y - r), s(x + r), s(y + r)], fill=(104, 78, 54, int(64 * alpha)),
                  outline=ink(alpha), width=lw(2))
        d.line([tuple(s((x, y + r))), tuple(s((x, y + r + 14)))], fill=ink(alpha), width=lw(2))


def draw_mountains(d, cx, cy, alpha=1.0):
    for dx, w, h in ((-52, 22, 36), (-15, 26, 51), (27, 20, 30)):
        x = cx + dx
        d.line([tuple(s((x - w, cy))), tuple(s((x, cy - h))), tuple(s((x + w, cy)))], fill=ink(alpha),
               width=lw(2.4), joint='curve')


def draw_dunes(d, cx, cy, alpha=1.0):
    """砂丘の稜線を三つと、城塞の塔をひとつ"""
    for dx, dy, w in ((-58, 6, 34), (-8, -10, 40), (44, 8, 30)):
        x, y = cx + dx, cy + dy
        d.arc([s(x - w), s(y - 14), s(x + w), s(y + 14)], 200, 340, fill=ink(alpha), width=lw(2.4))
    tx, ty = cx + 20, cy - 22
    d.rectangle([s(tx - 5), s(ty - 22), s(tx + 5), s(ty)], outline=ink(alpha), width=lw(2))
    d.line([tuple(s((tx - 8, ty - 22))), tuple(s((tx + 8, ty - 22)))], fill=ink(alpha), width=lw(2))


def draw_curl(d, x, y, alpha=0.45, long=True):
    """雲海の渦。小さな弧を二つ繋げる"""
    d.arc([s(x), s(y - 9), s(x + 27), s(y + 9)], 180, 360, fill=ink(alpha, INK_FADE), width=lw(2))
    if long:
        d.arc([s(x + 24), s(y - 6), s(x + 48), s(y + 9)], 180, 350, fill=ink(alpha, INK_FADE), width=lw(2))


def draw_compass(d, cx, cy):
    r = 60
    d.ellipse([s(cx - r), s(cy - r), s(cx + r), s(cy + r)], outline=ink(0.55), width=lw(2))
    d.ellipse([s(cx - 45), s(cy - 45), s(cx + 45), s(cy + 45)], outline=ink(0.3), width=lw(2))
    d.polygon([tuple(s(p)) for p in ((cx, cy - 78), (cx + 10, cy), (cx, cy + 78), (cx - 10, cy))], fill=ink(0.55))
    d.polygon([tuple(s(p)) for p in ((cx - 78, cy), (cx, cy - 9), (cx + 78, cy), (cx, cy + 9))], fill=ink(0.25))


def to_paper(p):
    return (p[0] - PAPER_ORIGIN[0], p[1] - PAPER_ORIGIN[1])


# ---------------------------------------------------------------- 部品
def paper_sprite():
    base = parchment(PAPER_W, PAPER_H, seed=4101, aged=0.18, ragged=16.0)
    lay = ink_layer(PAPER_W, PAPER_H)
    d = ImageDraw.Draw(lay)

    for (x, y, long) in ((255, 375, True), (840, 450, True), (705, 840, True), (1590, 630, True),
                         (1290, 840, True), (1170, 165, True), (345, 780, False), (1500, 210, False)):
        draw_curl(d, *to_paper((x, y)), long=long)

    draw_compass(d, *to_paper(COMPASS))

    bx, by = to_paper(BASE_ISLAND)
    draw_island(d, bx, by - 12, 123, 84, seed=11)
    draw_towers(d, bx, by - 6)
    gx, gy = to_paper(GRASS_ISLAND)
    draw_island(d, gx, gy, 123, 72, seed=23)
    draw_trees(d, gx, gy - 6)
    rx, ry = to_paper(DESERT_ISLAND)
    draw_island(d, rx, ry, 105, 63, seed=37)
    draw_dunes(d, rx, ry + 4)
    ux, uy = to_paper(UNKNOWN_ISLAND)
    for k in range(24):
        if k % 2:
            continue
        a0, a1 = k / 24 * 360, (k + 1) / 24 * 360
        d.arc([s(ux - 81), s(uy - 45), s(ux + 81), s(uy + 45)], a0, a1, fill=ink(0.4), width=lw(2.4))

    # 見出しの下の罫線(見出しの文字そのものは TextRenderer)
    kx, ky = to_paper((201, 213))
    d.line([tuple(s((kx, ky))), tuple(s((kx + 480, ky)))], fill=ink(0.9, INK_FADE), width=lw(3))

    base.alpha_composite(shrink(lay))

    # 地名。場所が変わらないので絵に焼く
    text(base, to_paper((495, 885)), '拠 点', 45, INK, path=BRUSH_FONT, anchor='ms')
    text(base, to_paper((1395, 585)), '草 原 地 帯', 45, INK, path=BRUSH_FONT, anchor='ms')
    text(base, to_paper((1080, 906)), '砂 漠 地 帯', 45, INK, path=BRUSH_FONT, anchor='ms')
    text(base, to_paper((960, 240)), '？', 42, (*INK, 110), path=BRUSH_FONT, anchor='ms')
    text(base, to_paper((960, 318)), '未 踏', 27, (*INK, 110), path=BRUSH_FONT, anchor='ms')
    text(base, to_paper((1662, 105)), '北', 27, INK, path=BRUSH_FONT, anchor='ms')

    # 釘と封蝋で机に留める
    for (x, y) in ((22, 22), (22, PAPER_H - 48), (PAPER_W - 48, PAPER_H - 48)):
        paste(base, nail(26, seed=x + y), x, y)
    seal = wax_seal(96, seed=9)
    paste(base, seal, PAPER_W - 78, -18 + 18)
    return base


def paper_shadow_sprite():
    a = np.zeros((PAPER_H + 120, PAPER_W + 120), np.float32)
    a[60:60 + PAPER_H, 60:60 + PAPER_W] = 1.0
    a = soften(a, 22) * 0.6
    return rgba(np.zeros(a.shape + (3,), np.float32), a)


def desk_sprite():
    board = wood_board(DESK_W, DESK_H, seed=212, plank=210)
    shade = Image.new('RGBA', board.size, (20, 10, 4, 60))
    board.alpha_composite(shade)
    return board


def dash_sprite(color, w, h, alpha):
    lay = ink_layer(w, h)
    ImageDraw.Draw(lay).rounded_rectangle([s(1), s(1), s(w - 1), s(h - 1)], radius=s(h / 2 - 1),
                                          fill=(*color, int(255 * alpha)))
    return shrink(lay)


def dest_circle_sprite():
    w, h = 360, 270
    lay = ink_layer(w, h)
    d = ImageDraw.Draw(lay)
    # 手で丸を付けたように、一周に少し足りない輪を二度なぞる
    d.arc([s(18), s(18), s(w - 18), s(h - 18)], 200, 520, fill=(*STAMP_RED, 225), width=lw(4))
    d.arc([s(24), s(14), s(w - 14), s(h - 24)], 215, 480, fill=(*STAMP_RED, 120), width=lw(2))
    return shrink(lay).rotate(6, resample=Image.BICUBIC)


def ship_sprite():
    w, h = 132, 84
    lay = ink_layer(w, h)
    d = ImageDraw.Draw(lay)
    cx, cy = 70, 30
    line = ink(1.0)
    # 尾翼
    d.polygon([tuple(s(p)) for p in ((cx - 42, cy), (cx - 60, cy - 16), (cx - 55, cy), (cx - 60, cy + 16))],
              fill=(201, 166, 107, 255), outline=line)
    # 気嚢
    d.ellipse([s(cx - 46), s(cy - 22), s(cx + 48), s(cy + 20)], fill=(239, 226, 194, 255), outline=line, width=lw(2.6))
    d.arc([s(cx - 34), s(cy - 34), s(cx + 36), s(cy - 4)], 20, 160, fill=ink(0.55), width=lw(1.4))
    d.arc([s(cx - 34), s(cy - 2), s(cx + 36), s(cy + 26)], 200, 340, fill=ink(0.55), width=lw(1.4))
    d.line([tuple(s((cx - 40, cy))), tuple(s((cx + 44, cy)))], fill=ink(0.55), width=lw(1.4))
    # 吊り索とゴンドラ
    d.line([tuple(s((cx - 14, cy + 18))), tuple(s((cx - 10, cy + 30)))], fill=line, width=lw(2))
    d.line([tuple(s((cx + 18, cy + 18))), tuple(s((cx + 14, cy + 30)))], fill=line, width=lw(2))
    d.polygon([tuple(s(p)) for p in ((cx - 20, cy + 30), (cx + 22, cy + 30), (cx + 16, cy + 42), (cx - 14, cy + 42))],
              fill=(*STAMP_RED, 255), outline=line)
    # プロペラ
    d.line([tuple(s((cx + 22, cy + 36))), tuple(s((cx + 33, cy + 36)))], fill=line, width=lw(2))
    d.ellipse([s(cx + 32), s(cy + 26), s(cx + 37), s(cy + 46)], fill=line)
    return shrink(lay)


def soft_ellipse(w, h, color, alpha, blur):
    xx, yy = grid(w, h)
    d = ((xx - w / 2) / (w / 2 - blur)) ** 2 + ((yy - h / 2) / (h / 2 - blur)) ** 2
    a = soften(np.clip(1.0 - d, 0, 1), blur / 2.5) * alpha
    return rgba(np.broadcast_to(np.array(color, np.float32) / 255.0, a.shape + (3,)), a)


def trail_sprite():
    lay = ink_layer(18, 18)
    ImageDraw.Draw(lay).ellipse([s(2), s(2), s(16), s(16)], outline=ink(0.9), width=lw(1.6))
    return shrink(lay)


def cloud_sprite(w, h, seed, color, alpha, puffs=7):
    """ふくらみを重ねた雲。縁はぼかし、ノイズで濃淡を付ける"""
    rng = np.random.default_rng(seed)
    xx, yy = grid(w, h)
    field = np.zeros((h, w), np.float32)
    for i in range(puffs):
        px = w * (0.18 + 0.64 * i / max(puffs - 1, 1)) + rng.normal(0, w * 0.03)
        py = h * (0.58 - 0.18 * math.sin(i / max(puffs - 1, 1) * math.pi)) + rng.normal(0, h * 0.04)
        r = h * (0.22 + 0.16 * math.sin(i / max(puffs - 1, 1) * math.pi)) * (0.85 + rng.random() * 0.3)
        field = np.maximum(field, np.clip(1 - np.hypot(xx - px, yy - py) / r, 0, 1))
    field = np.clip(field * 2.2, 0, 1)
    field = soften(field, h * 0.06)
    tone = 0.85 + 0.15 * fbm(w, h, seed + 5, octaves=4, base=6)
    # ふくらみが画像の端に届いても四角く切れないよう、縁へ向けて消す
    edge = np.minimum.reduce([xx, w - 1 - xx, yy, h - 1 - yy]) / (h * 0.18)
    a = field * alpha * tone * np.clip(edge, 0, 1)
    rgb = np.broadcast_to(np.array(color, np.float32) / 255.0, a.shape + (3,))
    return rgba(rgb, a)


def vignette_sprite():
    xx, yy = grid(SCREEN_W, SCREEN_H)
    d = np.hypot((xx - SCREEN_W / 2) / (SCREEN_W * 0.62), (yy - SCREEN_H * 0.46) / (SCREEN_H * 0.66))
    a = np.clip((d - 0.62) / 0.5, 0, 1) ** 1.6 * 0.68
    return rgba(np.broadcast_to(np.array([0.08, 0.04, 0.015], np.float32), a.shape + (3,)), a)


def chip_sprite():
    w, h = 330, 300
    chip = parchment(w, h, seed=733, aged=0.35, ragged=10.0)
    canvas = Image.new('RGBA', (w + 40, h + 50), (0, 0, 0, 0))
    canvas.alpha_composite(drop_shadow(chip, blur_r=8, offset=(4, 8), alpha=0.5), (-4, 2))
    canvas.alpha_composite(chip, (20, 30))
    # 上辺をテープで留める
    tape = Image.new('RGBA', (126, 42), (226, 214, 176, 170))
    tape = tape.rotate(-4, resample=Image.BICUBIC, expand=True)
    canvas.alpha_composite(tape, (w // 2 + 20 - tape.width // 2, 10))
    return canvas


def hint_seal_sprite():
    return wax_seal(40, seed=17)


SPRITES = {
    'LoadingMap_Desk': desk_sprite,
    'LoadingMap_PaperShadow': paper_shadow_sprite,
    'LoadingMap_Paper': paper_sprite,
    'LoadingMap_Dash': lambda: dash_sprite(INK, 13, 6, 0.75),
    'LoadingMap_DashPassed': lambda: dash_sprite(STAMP_RED, 16, 7, 1.0),
    'LoadingMap_DestCircle': dest_circle_sprite,
    'LoadingMap_ShipRight': ship_sprite,
    'LoadingMap_ShipLeft': lambda: ship_sprite().transpose(Image.FLIP_LEFT_RIGHT),
    'LoadingMap_ShipShadow': lambda: soft_ellipse(84, 22, (42, 26, 12), 0.34, 7),
    'LoadingMap_TrailPuff': trail_sprite,
    'LoadingMap_CloudShadow': lambda: cloud_sprite(640, 300, 51, (42, 24, 8), 0.16, puffs=5),
    'LoadingMap_Cloud0': lambda: cloud_sprite(760, 280, 61, (251, 246, 234), 0.45),
    'LoadingMap_Cloud1': lambda: cloud_sprite(620, 240, 71, (251, 246, 234), 0.38, puffs=6),
    'LoadingMap_Cloud2': lambda: cloud_sprite(900, 320, 81, (251, 246, 234), 0.42, puffs=8),
    'LoadingMap_Vignette': vignette_sprite,
    'LoadingMap_Chip': chip_sprite,
    'LoadingMap_HintSeal': hint_seal_sprite,
}


def write_sprite(name, image):
    from tools.scene import sprite_meta
    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    png = EMIT_DIR / f'{name}.png'
    image.save(png, optimize=True)
    meta = EMIT_DIR / f'{name}.png.meta'
    guid = sprite_meta.read_meta(meta)['guid'] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, EMIT_DIR, REPO_ROOT))
    print(f'  {name}.png  {image.size[0]}x{image.size[1]}  guid={guid}')


def emit():
    print(f'emit -> {EMIT_DIR}')
    for name, make in SPRITES.items():
        write_sprite(name, make())


def bezier(pts, t):
    u = 1 - t
    return tuple(u ** 3 * pts[0][i] + 3 * u * u * t * pts[1][i] + 3 * u * t * t * pts[2][i] + t ** 3 * pts[3][i]
                 for i in range(2))


def preview(out_path, progress=0.58):
    """書き出した絵を、出発の航路・進捗 progress の状態で並べる(カメラの拡大は掛けない)"""
    def sprite(name):
        return Image.open(EMIT_DIR / f'{name}.png').convert('RGBA')

    def put(base, name, center, angle=0.0):
        im = sprite(name)
        if angle:
            im = im.rotate(-math.degrees(angle), resample=Image.BICUBIC, expand=True)
        base.alpha_composite(im, (int(center[0] - im.width / 2), int(center[1] - im.height / 2)))

    geo = layout()
    base = Image.new('RGBA', (SCREEN_W, SCREEN_H), (0, 0, 0, 255))
    put(base, 'LoadingMap_Desk', geo['desk']['center'])
    put(base, 'LoadingMap_PaperShadow', geo['paper_shadow']['center'])
    put(base, 'LoadingMap_Paper', geo['paper']['center'])
    for (x, y) in geo['cloud_shadows']:
        put(base, 'LoadingMap_CloudShadow', (x, y))

    n = geo['dash_count']
    samples = [bezier(DEPART_ROUTE, i / 400) for i in range(401)]
    lengths = [0.0]
    for a, b in zip(samples, samples[1:]):
        lengths.append(lengths[-1] + math.dist(a, b))

    def at(q):
        target = q * lengths[-1]
        for i in range(1, len(lengths)):
            if lengths[i] >= target:
                a, b = samples[i - 1], samples[i]
                k = (target - lengths[i - 1]) / max(lengths[i] - lengths[i - 1], 1e-6)
                return (a[0] + (b[0] - a[0]) * k, a[1] + (b[1] - a[1]) * k), math.atan2(b[1] - a[1], b[0] - a[0])
        return samples[-1], 0.0

    for i in range(n):
        q = (i + 0.5) / n
        p, ang = at(q)
        put(base, 'LoadingMap_DashPassed' if q <= progress else 'LoadingMap_Dash', p, ang)
    put(base, 'LoadingMap_DestCircle', geo['dest_circle']['center'])

    ship_pos, ship_ang = at(progress)
    put(base, 'LoadingMap_ShipShadow', (ship_pos[0] + 12, ship_pos[1] + 9))
    for k in range(geo['trail_count']):
        tp, _ = at(max(0.0, progress - 0.035 * (k + 1)))
        put(base, 'LoadingMap_TrailPuff', (tp[0], tp[1] - 21))
    put(base, 'LoadingMap_ShipRight', (ship_pos[0], ship_pos[1] - 33), max(-0.24, min(0.24, ship_ang)))

    for spec in (dict(geo['kicker']), dict(geo['title']), dict(geo['from_caption']), dict(geo['to_caption'])):
        anchor = 'la' if spec['align'] == 'left' else 'ma'
        text(base, spec['pos'], spec['text'], spec['px'], spec['color'],
             path=BRUSH_FONT if spec['font'] == 'brush' else BODY_FONT, anchor=anchor)

    put(base, 'LoadingMap_Vignette', geo['vignette']['center'])
    for (x, y, v) in geo['front_clouds']:
        put(base, f'LoadingMap_Cloud{v}', (x, y))
    put(base, 'LoadingMap_Chip', geo['chip']['center'])
    for key, value in (('status', '航行中…'), ('percent', f'{int(progress * 100)}%')):
        spec = geo[key]
        text(base, spec['pos'], value, spec['px'], spec['color'],
             path=BRUSH_FONT if spec['font'] == 'brush' else BODY_FONT, anchor='ma')
    put(base, 'LoadingMap_HintSeal', geo['hint_glyph']['center'])
    hint = geo['hint_text']
    text(base, hint['pos'], '回避の直後は無敵時間が発生する', hint['px'], hint['color'], anchor='la')

    base.convert('RGB').save(out_path)
    print(f'preview -> {out_path}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--emit', action='store_true')
    ap.add_argument('--preview', type=Path)
    args = ap.parse_args()
    if not args.emit and not args.preview:
        ap.error('--emit か --preview を指定してください')
    if args.emit:
        emit()
    if args.preview:
        preview(args.preview)


if __name__ == '__main__':
    main()
