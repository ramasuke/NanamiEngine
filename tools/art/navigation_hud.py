"""Mock the "what to do next" navigation (objective text + world marker / edge arrow + NPC surprise mark +
ground trail) on a real gameplay screenshot, in three visual directions.

    python tools/art/navigation_hud.py --shot <screenshot.png> --out-dir <dir> [--resident-only]
    python tools/art/navigation_hud.py --emit     # 字幕の帯・光の玉・蛍を Assets/Art/UI/Navigation/ へ

Writes <dir>/nav_<variant>_<state>.png (1920x1080) and <dir>/nav_sheet.png (all of them side by side).
States: "near" = the target NPC is on screen, "far" = the objective just changed and the target is off
screen to the right. The screen positions below are hand-placed for the MainIsland shot
(player at the bottom centre, the instructor at the top of the stairs).
HUD family B (docs/UIDesign.md): dark teal #06141a, text #fffff7, accent #38d6c4, new/active gold #ffce68, IPA Mincho.
Requires Pillow + numpy.
"""
import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO_ROOT = Path(__file__).resolve().parents[2]
HUD_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'ipam.ttf'
SURPRISE_MARK = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'SurpriseMark.png'

W, H = 1920, 1080
SS = 2

BG = (6, 20, 26)
TEXT = (255, 255, 247)
TEXT_SUB = (210, 228, 223)
TEAL = (56, 214, 196)
GOLD = (255, 206, 104)

# 画面上の位置（MainIsland のショット用）
INSTRUCTOR_HEAD = (1008, 378)
INSTRUCTOR_ICON = (1010, 356)
PLAYER_FEET = (962, 930)

# 教官までの地面の道筋（x, y, 遠近の倍率）
TRAIL_NEAR = [(930, 952, 1.0), (872, 900, 0.92), (836, 836, 0.84), (826, 770, 0.76), (840, 706, 0.68),
              (866, 652, 0.6), (884, 604, 0.52), (902, 560, 0.46), (922, 522, 0.4), (950, 492, 0.35),
              (984, 476, 0.3)]
# 画面右外（転移台）への道筋
TRAIL_FAR = [(1000, 952, 1.0), (1076, 902, 0.92), (1150, 858, 0.84), (1236, 820, 0.76), (1330, 790, 0.7),
             (1430, 766, 0.64), (1540, 748, 0.58), (1650, 736, 0.53), (1760, 728, 0.49), (1870, 722, 0.45),
             (1980, 718, 0.42)]
EDGE_ARROW = (1868, 560)

OBJECTIVE = {
    'near': ('教官に話を聞く', '訓練場の階段の上', 32),
    'far': ('転移台から草原へ渡る', '島の北の転移台', 148),
}


def font(px):
    return ImageFont.truetype(str(HUD_FONT), int(px * SS))


def layer():
    return Image.new('RGBA', (W * SS, H * SS), (0, 0, 0, 0))


def s(v):
    return v * SS


def flatten(base, over, additive=False, blur=0):
    over = over.resize((W, H), Image.LANCZOS)
    if blur:
        over = over.filter(ImageFilter.GaussianBlur(blur))
    if additive:
        b = np.asarray(base, np.float32)
        o = np.asarray(over, np.float32)
        b[..., :3] = np.clip(b[..., :3] + o[..., :3] * (o[..., 3:4] / 255.0), 0, 255)
        return Image.fromarray(b.astype(np.uint8), 'RGBA')
    return Image.alpha_composite(base, over)


def text(d, xy, t, px, color, anchor='la', alpha=255, shadow=True):
    f = font(px)
    x, y = s(xy[0]), s(xy[1])
    if shadow:
        d.text((x + s(1.5), y + s(1.5)), t, font=f, fill=(0, 0, 0, int(alpha * 0.8)), anchor=anchor)
    d.text((x, y), t, font=f, fill=color + (alpha,), anchor=anchor)


def strip(d, right, top, width, height, alpha=0.8, fade_rate=0.45):
    # 右端が濃く、左へ向けて消える帯（操作ガイドの帯を左右反転したもの）
    for i in range(s(width)):
        t = i / s(width)
        a = alpha if t > fade_rate else alpha * (t / fade_rate) ** 1.6
        x = s(right - width) + i
        d.line([(x, s(top)), (x, s(top + height))], fill=BG + (int(255 * a),))


def diamond(d, cx, cy, r, color, width=2.5, fill=None):
    pts = [(s(cx), s(cy - r)), (s(cx + r * 0.72), s(cy)), (s(cx), s(cy + r)), (s(cx - r * 0.72), s(cy))]
    if fill:
        d.polygon(pts, fill=fill)
    d.line(pts + [pts[0]], fill=color + (255,), width=int(s(width)), joint='curve')


def chevron(d, cx, cy, r, angle, color, width=4.0, alpha=255):
    # angle: 0 = 右向き
    ca, sa = math.cos(angle), math.sin(angle)

    def rot(px, py):
        return s(cx + px * ca - py * sa), s(cy + px * sa + py * ca)
    d.line([rot(-r * 0.5, -r), rot(r * 0.5, 0), rot(-r * 0.5, r)], fill=color + (alpha,), width=int(s(width)),
           joint='curve')


def paste_surprise(base, center, height):
    mark = Image.open(SURPRISE_MARK).convert('RGBA')
    w = int(mark.width * height / mark.height)
    mark = mark.resize((w, height), Image.LANCZOS)
    base.alpha_composite(mark, (int(center[0] - w / 2), int(center[1] - height / 2)))
    return base


def cover_icon(base, center):
    # 既存の会話アイコン（盾の山形）を背景の空で塗りつぶして、驚きアイコンに差し替えたように見せる
    x, y = center
    box = (x - 16, y - 20, x + 16, y + 20)
    patch = base.crop((x - 50, y - 20, x - 18, y + 20))
    base.paste(patch, box[:2])
    return base


def trail_points(trail, step=18.0):
    # 遠近の倍率に合わせて間隔を詰めながら、折れ線上に点を並べる
    pts = []
    carry = 0.0
    for (x0, y0, k0), (x1, y1, k1) in zip(trail, trail[1:]):
        seg = math.hypot(x1 - x0, y1 - y0)
        t = carry
        while t < seg:
            u = t / seg
            k = k0 + (k1 - k0) * u
            pts.append((x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, k))
            t += step * k * 2.2
        carry = t - seg
    return pts


# ---------------------------------------------------------------- 案1: 青緑の帯と光の点
def v1_objective(base, state, fresh):
    title, place, dist = OBJECTIVE[state]
    over = layer()
    d = ImageDraw.Draw(over)
    right, top = 1896, 40
    strip(d, right, top, 470, 86)
    accent = GOLD if fresh else TEAL
    d.rectangle([s(right - 3), s(top), s(right), s(top + 86)], fill=accent + (255,))
    text(d, (right - 18, top + 10), 'つ ぎ に す る こ と', 17, accent, anchor='ra')
    text(d, (right - 18, top + 34), title, 30, TEXT, anchor='ra')
    text(d, (right - 18, top + 70), f'{place}　{dist}m', 18, TEXT_SUB, anchor='ra', alpha=220)
    base = flatten(base, over)
    if fresh:
        glow = layer()
        g = ImageDraw.Draw(glow)
        g.rectangle([s(right - 10), s(top - 4), s(right + 4), s(top + 90)], fill=GOLD + (200,))
        base = flatten(base, glow, additive=True, blur=8)
    return base


def v1_trail(base, trail):
    over = layer()
    d = ImageDraw.Draw(over)
    pts = trail_points(trail)
    for i, (x, y, k) in enumerate(pts):
        fade = 1.0 - i / len(pts) * 0.55
        r = 7.0 * k
        d.ellipse([s(x - r * 1.6), s(y - r * 0.7), s(x + r * 1.6), s(y + r * 0.7)], fill=TEAL + (int(230 * fade),))
    base = flatten(base, over)
    return flatten(base, over, additive=True, blur=6)


def v1_marker(base, head, dist):
    cx, cy = head[0], head[1] - 92
    over = layer()
    d = ImageDraw.Draw(over)
    diamond(d, cx, cy, 15, TEAL, fill=BG + (170,))
    diamond(d, cx, cy, 6, TEAL, width=1, fill=TEAL + (255,))
    text(d, (cx, cy + 22), f'{dist}m', 17, TEXT, anchor='ma')
    base = flatten(base, over)
    return flatten(base, over, additive=True, blur=5)


def v1_edge(base, dist):
    x, y = EDGE_ARROW
    over = layer()
    d = ImageDraw.Draw(over)
    d.ellipse([s(x - 34), s(y - 34), s(x + 34), s(y + 34)], fill=BG + (150,))
    chevron(d, x + 6, y, 14, 0, TEAL, width=5)
    chevron(d, x - 8, y, 14, 0, TEAL, width=5, alpha=120)
    text(d, (x - 6, y + 42), f'{dist}m', 17, TEXT, anchor='ma')
    base = flatten(base, over)
    return flatten(base, over, additive=True, blur=5)


def variant1(shot, state):
    base = shot.copy()
    if state == 'near':
        base = v1_trail(base, TRAIL_NEAR)
        base = cover_icon(base, INSTRUCTOR_ICON)
        base = paste_surprise(base, INSTRUCTOR_ICON, 36)
        base = v1_marker(base, INSTRUCTOR_HEAD, OBJECTIVE['near'][2])
        base = v1_objective(base, 'near', fresh=False)
    else:
        base = v1_trail(base, TRAIL_FAR)
        base = v1_edge(base, OBJECTIVE['far'][2])
        base = v1_objective(base, 'far', fresh=True)
    return base


# ---------------------------------------------------------------- 案2: 羅針の環と光の柱
def compass(d, cx, cy, r, needle_angle):
    d.ellipse([s(cx - r), s(cy - r), s(cx + r), s(cy + r)], fill=BG + (205,))
    d.ellipse([s(cx - r), s(cy - r), s(cx + r), s(cy + r)], outline=(160, 190, 184, 255), width=int(s(2)))
    for i in range(16):
        a = i / 16 * math.tau
        r0 = r - (7 if i % 4 == 0 else 4)
        d.line([(s(cx + math.sin(a) * r0), s(cy - math.cos(a) * r0)),
                (s(cx + math.sin(a) * (r - 2)), s(cy - math.cos(a) * (r - 2)))], fill=(160, 190, 184, 200),
               width=int(s(1.5)))
    a = needle_angle
    tip = (cx + math.sin(a) * (r - 10), cy - math.cos(a) * (r - 10))
    tail = (cx - math.sin(a) * (r * 0.45), cy + math.cos(a) * (r * 0.45))
    side = (math.cos(a) * 6, math.sin(a) * 6)
    d.polygon([(s(tip[0]), s(tip[1])), (s(cx + side[0]), s(cy + side[1])), (s(tail[0]), s(tail[1])),
               (s(cx - side[0]), s(cy - side[1]))], fill=GOLD + (255,))
    d.ellipse([s(cx - 3), s(cy - 3), s(cx + 3), s(cy + 3)], fill=BG + (255,))


def v2_objective(base, state, needle, fresh):
    title, place, dist = OBJECTIVE[state]
    over = layer()
    d = ImageDraw.Draw(over)
    cx, cy, r = 1840, 88, 44
    strip(d, cx - 20, cy - 36, 440, 72, alpha=0.72, fade_rate=0.5)
    compass(d, cx, cy, r, needle)
    text(d, (cx - r - 16, cy - 30), title, 30, GOLD if fresh else TEXT, anchor='ra')
    text(d, (cx - r - 16, cy + 10), f'{place}　{dist}m', 18, TEXT_SUB, anchor='ra', alpha=220)
    base = flatten(base, over)
    glow = layer()
    compass(ImageDraw.Draw(glow), cx, cy, r, needle)
    return flatten(base, glow, additive=True, blur=4 if not fresh else 9)


def v2_pillar(base, foot, height, width):
    x, y = foot
    over = layer()
    d = ImageDraw.Draw(over)
    for i in range(int(s(height))):
        t = i / s(height)
        a = (1 - t) ** 1.3 * 0.55
        w = width * (1 - 0.35 * t)
        d.line([(s(x - w / 2), s(y) - i), (s(x + w / 2), s(y) - i)], fill=GOLD + (int(255 * a),))
    base = flatten(base, over, additive=True, blur=6)
    core = layer()
    dc = ImageDraw.Draw(core)
    for i in range(int(s(height * 0.8))):
        t = i / s(height * 0.8)
        dc.line([(s(x - 1.5), s(y) - i), (s(x + 1.5), s(y) - i)], fill=(255, 244, 210, int(255 * (1 - t) * 0.8)))
    base = flatten(base, core, additive=True, blur=1)
    ring = layer()
    dr = ImageDraw.Draw(ring)
    dr.ellipse([s(x - width * 1.1), s(y - width * 0.3), s(x + width * 1.1), s(y + width * 0.3)],
               outline=GOLD + (220,), width=int(s(2)))
    return flatten(base, ring, additive=True, blur=2)


def v2_trail(base, trail):
    # 地面をなでる金色の風の筋
    over = layer()
    d = ImageDraw.Draw(over)
    pts = trail_points(trail, step=2.2)
    for j in range(0, len(pts) - 1):
        if (j // 6) % 3 == 2:
            continue
        (x0, y0, k0), (x1, y1, _) = pts[j], pts[j + 1]
        fade = 1.0 - j / len(pts) * 0.6
        d.line([(s(x0), s(y0)), (s(x1), s(y1))], fill=GOLD + (int(235 * fade),), width=int(s(5.5 * k0)))
    base = flatten(base, over)
    return flatten(base, over, additive=True, blur=7)


def variant2(shot, state):
    base = shot.copy()
    if state == 'near':
        base = v2_trail(base, TRAIL_NEAR)
        base = v2_pillar(base, (1008, 478), 330, 26)
        base = cover_icon(base, INSTRUCTOR_ICON)
        base = paste_surprise(base, INSTRUCTOR_ICON, 36)
        base = v2_objective(base, 'near', needle=0.12, fresh=False)
    else:
        base = v2_trail(base, TRAIL_FAR)
        over = layer()
        d = ImageDraw.Draw(over)
        x, y = EDGE_ARROW
        chevron(d, x, y, 16, 0, GOLD, width=5)
        text(d, (x - 4, y + 30), f'{OBJECTIVE["far"][2]}m', 17, TEXT, anchor='ma')
        base = flatten(base, over)
        base = flatten(base, over, additive=True, blur=6)
        base = v2_objective(base, 'far', needle=1.45, fresh=True)
    return base


# ---------------------------------------------------------------- 案3: 導きの蛍と字幕の目的
def firefly(d, x, y, r, alpha):
    d.ellipse([s(x - r), s(y - r), s(x + r), s(y + r)], fill=(255, 236, 170, alpha))


def v3_fireflies(base, trail, lift=46):
    # 腰の高さを漂う蛍の列。道の上に浮かせ、先へ行くほど疎らに小さく
    rng = np.random.default_rng(3)
    over = layer()
    d = ImageDraw.Draw(over)
    pts = trail_points(trail, step=14)
    for i, (x, y, k) in enumerate(pts):
        jx, jy = rng.normal(0, 7 * k), rng.normal(0, 9 * k)
        a = int(255 * (1.0 - i / len(pts) * 0.5))
        firefly(d, x + jx, y - lift * k + jy, 6.5 * k, a)
    base = flatten(base, over, additive=True, blur=9)
    base = flatten(base, over, additive=True, blur=9)
    return flatten(base, over)


def v3_orb(base, head):
    cx, cy = head[0], head[1] - 40
    over = layer()
    d = ImageDraw.Draw(over)
    firefly(d, cx, cy, 11, 255)
    base = flatten(base, over, additive=True, blur=14)
    over = layer()
    firefly(ImageDraw.Draw(over), cx, cy, 5, 255)
    return flatten(base, over, additive=True, blur=1)


def v3_banner(base, title):
    # 目的が変わった瞬間だけ上中央に出る字幕。数秒で右上の小さな一行へ縮む
    over = layer()
    d = ImageDraw.Draw(over)
    cx, top = W / 2, 150
    for i in range(s(760)):
        t = abs(i / s(760) - 0.5) * 2
        a = 0.78 * (1 - t ** 3)
        x = s(cx - 380) + i
        d.line([(x, s(top)), (x, s(top + 92))], fill=BG + (int(255 * a),))
    d.line([(s(cx - 200), s(top + 2)), (s(cx + 200), s(top + 2))], fill=GOLD + (200,), width=int(s(1.5)))
    text(d, (cx, top + 14), '— 新 た な 目 的 —', 17, GOLD, anchor='ma')
    text(d, (cx, top + 40), title, 36, TEXT, anchor='ma')
    return flatten(base, over)


def v3_corner(base, title, dist):
    over = layer()
    d = ImageDraw.Draw(over)
    right, top = 1896, 44
    strip(d, right, top, 380, 42, alpha=0.7, fade_rate=0.5)
    firefly(d, right - 18, top + 21, 5, 255)
    text(d, (right - 36, top + 9), f'{title}　{dist}m', 24, TEXT, anchor='ra')
    base = flatten(base, over)
    glow = layer()
    firefly(ImageDraw.Draw(glow), right - 18, top + 21, 9, 255)
    return flatten(base, glow, additive=True, blur=6)


def variant3(shot, state):
    base = shot.copy()
    if state == 'near':
        base = v3_fireflies(base, TRAIL_NEAR)
        base = cover_icon(base, INSTRUCTOR_ICON)
        base = paste_surprise(base, INSTRUCTOR_ICON, 36)
        base = v3_orb(base, (INSTRUCTOR_HEAD[0], INSTRUCTOR_HEAD[1] - 26))
        base = v3_corner(base, OBJECTIVE['near'][0], OBJECTIVE['near'][2])
    else:
        base = v3_fireflies(base, TRAIL_FAR)
        over = layer()
        d = ImageDraw.Draw(over)
        x, y = EDGE_ARROW
        for k, a in ((0, 255), (-20, 170), (-40, 90)):
            firefly(d, x + k, y, 7, a)
        base = flatten(base, over, additive=True, blur=10)
        base = flatten(base, over, additive=True, blur=10)
        base = flatten(base, over)
        base = v3_banner(base, OBJECTIVE['far'][0])
    return base


VARIANTS = [
    ('1', '案1 青緑の帯＋光の点', variant1),
    ('2', '案2 羅針盤＋光の柱', variant2),
    ('3', '案3 導きの蛍＋字幕', variant3),
]


# ---------------------------------------------------------------- 案3 の常時表示の差し替え
INK_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'KaiseiDecol-Bold_Ink.ttf'
BODY_FONT = REPO_ROOT / 'Assets' / 'Art' / 'Font' / 'ZenOldMincho-Bold.ttf'
INK = (48, 30, 20)
INK_FADE = (104, 78, 54)
FAR_LABEL = '転移台'
NEAR_LABEL = '教官'


def r1_afterglow(base, title, dist):
    # 字幕の名残: 上中央の字幕がそのまま細く薄くなって残る。左右対称の帯で、操作ガイドの帯とは形が違う
    over = layer()
    d = ImageDraw.Draw(over)
    cx, top, half, h = W / 2, 26, 250, 40
    for i in range(s(half * 2)):
        t = abs(i / s(half * 2) - 0.5) * 2
        a = 0.55 * (1 - t ** 2.2)
        x = s(cx - half) + i
        d.line([(x, s(top)), (x, s(top + h))], fill=BG + (int(255 * a),))
    text(d, (cx, top + 20), f'{title}', 24, TEXT, anchor='mm')
    tw = font(24).getlength(title) / SS
    text(d, (cx + tw / 2 + 12, top + 21), f'{dist}m', 17, TEXT_SUB, anchor='lm', alpha=210)
    firefly(d, cx - tw / 2 - 16, top + 20, 4, 255)
    base = flatten(base, over)
    glow = layer()
    firefly(ImageDraw.Draw(glow), cx - tw / 2 - 16, top + 20, 8, 255)
    return flatten(base, glow, additive=True, blur=6)


def r2_label(base, xy, name, dist, anchor):
    # 蛍の添え書き: 画面の隅には何も置かず、光の玉 / 画面端の蛍の横に行き先と距離だけ添える
    over = layer()
    d = ImageDraw.Draw(over)
    text(d, xy, name, 20, TEXT, anchor=anchor)
    text(d, (xy[0], xy[1] + 22), f'{dist}m', 16, TEXT_SUB, anchor=anchor, alpha=210)
    return flatten(base, over)


def r3_note(base, title, place):
    # 手帳の付箋: 右上に釘で留めた小さな紙片（系統 A の素材）。HUD の青緑とは材質で分ける
    import sys
    sys.path.insert(0, str(REPO_ROOT))
    from tools.art import character_select as cs
    w, h = 330, 96
    paper = cs.parchment(w, h, seed=41, aged=0.25, ragged=9.0)
    cs.text(paper, (w - 26, 22), title, 27, INK, path=INK_FONT, anchor='ra')
    cs.text(paper, (w - 26, 62), place, 17, INK_FADE, path=BODY_FONT, anchor='ra')
    cs.paste(paper, cs.nail(18), w / 2 - 9, 3)
    cs.paste_tilted(base, paper, 1896 - w / 2, 34 + h / 2, -1.6)
    return base


def resident(shot, key, state):
    base = shot.copy()
    title, place, dist = OBJECTIVE[state]
    if state == 'near':
        base = v3_fireflies(base, TRAIL_NEAR)
        base = cover_icon(base, INSTRUCTOR_ICON)
        base = paste_surprise(base, INSTRUCTOR_ICON, 36)
        orb = (INSTRUCTOR_HEAD[0], INSTRUCTOR_HEAD[1] - 26)
        base = v3_orb(base, orb)
    else:
        base = v3_fireflies(base, TRAIL_FAR)
        over = layer()
        d = ImageDraw.Draw(over)
        x, y = EDGE_ARROW
        for k, a in ((0, 255), (-20, 170), (-40, 90)):
            firefly(d, x + k, y, 7, a)
        base = flatten(base, over, additive=True, blur=10)
        base = flatten(base, over, additive=True, blur=10)
        base = flatten(base, over)
    if key == 'r1':
        base = r1_afterglow(base, title, dist)
    elif key == 'r2':
        if state == 'near':
            base = r2_label(base, (1026, INSTRUCTOR_HEAD[1] - 66 - 12), NEAR_LABEL, dist, 'la')
        else:
            base = r2_label(base, (EDGE_ARROW[0] + 6, EDGE_ARROW[1] + 18), FAR_LABEL, dist, 'ra')
    else:
        base = r3_note(base, title, f'{place}　{dist}m')
    return base


RESIDENTS = [
    ('r1', '改1 字幕の名残（上中央に細く残る）'),
    ('r2', '改2 蛍の添え書き（隅には何も置かない）'),
    ('r3', '改3 手帳の付箋（右上に紙片）'),
]


# ---------------------------------------------------------------- 書き出し（採用: 案3 + 改2）
EMIT_DIR = REPO_ROOT / 'Assets' / 'Art' / 'UI' / 'Navigation'
BAND_W, BAND_H = 760, 92
BAND_LINE_HALF = 200


def render_band():
    """上中央の字幕の帯。左右対称に消える暗い帯と、上端の細い金の線（NavigationBanner::bandSprite_）"""
    xs = np.abs(np.linspace(-1.0, 1.0, BAND_W, dtype=np.float32))
    ys = np.arange(BAND_H, dtype=np.float32)
    edge = np.clip(np.minimum(ys, BAND_H - 1 - ys) / 6.0, 0.0, 1.0)
    alpha = 0.78 * (1.0 - xs ** 3)[None, :] * edge[:, None]
    rgb = np.broadcast_to(np.array(BG, np.float32) / 255.0, (BAND_H, BAND_W, 3)).copy()

    line_x = np.abs(np.arange(BAND_W, dtype=np.float32) - BAND_W / 2) / BAND_LINE_HALF
    line_a = np.clip(1.0 - line_x ** 2, 0.0, 1.0) * (200 / 255)
    for y, k in ((2, 1.0), (3, 0.45)):
        a = line_a * k
        rgb[y] = rgb[y] * (1 - a[:, None]) + (np.array(GOLD, np.float32) / 255.0)[None, :] * a[:, None]
        alpha[y] = np.maximum(alpha[y], a)
    out = np.concatenate([rgb, alpha[..., None]], axis=-1)
    return Image.fromarray((np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8), 'RGBA')


def render_glow(size, core_rate, color=(255, 236, 170)):
    """加算で描く光。芯は白に近く、外へなだらかに消える（光の玉・蛍）"""
    r = np.hypot(*np.meshgrid(np.linspace(-1, 1, size), np.linspace(-1, 1, size))).astype(np.float32)
    halo = np.clip(1.0 - r, 0.0, 1.0) ** 2.2
    core = np.clip(1.0 - r / core_rate, 0.0, 1.0) ** 1.5
    alpha = np.clip(halo * 0.75 + core, 0.0, 1.0)
    tint = np.array(color, np.float32) / 255.0
    rgb = tint[None, None, :] * (1 - core[..., None]) + core[..., None]
    out = np.concatenate([rgb, alpha[..., None]], axis=-1)
    return Image.fromarray((np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8), 'RGBA')


def emit():
    import sys
    sys.path.insert(0, str(REPO_ROOT))
    from tools.scene import sprite_meta

    EMIT_DIR.mkdir(parents=True, exist_ok=True)
    sprites = {
        'Navigation_Band': render_band(),
        'Navigation_Orb': render_glow(96, 0.16),
        'Navigation_Firefly': render_glow(32, 0.22),
    }
    for name, image in sprites.items():
        png = EMIT_DIR / f'{name}.png'
        image.save(png)
        meta = EMIT_DIR / f'{name}.png.meta'
        guid = sprite_meta.read_meta(meta)['guid'] if meta.exists() else sprite_meta.mint_guid()
        sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, EMIT_DIR, REPO_ROOT))
        print(f'{name}: {image.width}x{image.height} guid={guid}')


def contact_sheet(images, labels, path):
    cols, rows = 2, len(images) // 2
    tw, th = W // 2, H // 2
    sheet = Image.new('RGB', (cols * tw + 30, rows * (th + 44) + 10), (24, 24, 24))
    d = ImageDraw.Draw(sheet)
    f = ImageFont.truetype(str(HUD_FONT), 26)
    for i, (im, label) in enumerate(zip(images, labels)):
        c, r = i % cols, i // cols
        x, y = 10 + c * (tw + 10), 10 + r * (th + 44)
        d.text((x, y), label, font=f, fill=(240, 240, 240))
        sheet.paste(im.convert('RGB').resize((tw, th), Image.LANCZOS), (x, y + 34))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--shot')
    ap.add_argument('--out-dir')
    ap.add_argument('--emit', action='store_true', help='採用案のスプライトを Assets/Art/UI/Navigation/ に書き出す')
    ap.add_argument('--resident-only', action='store_true', help='案3 の常時表示の差し替え（改1〜3）だけ描く')
    args = ap.parse_args()
    if args.emit:
        emit()
        return
    if not args.shot or not args.out_dir:
        ap.error('--shot と --out-dir が要る（--emit 以外）')
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    shot = Image.open(args.shot).convert('RGBA').resize((W, H))
    images, labels = [], []
    for key, label, fn in ([] if args.resident_only else VARIANTS):
        for state, state_label in (('near', '教官が見えている'), ('far', '目的が変わった直後・目的地は画面外')):
            im = fn(shot, state)
            im.convert('RGB').save(out / f'nav_{key}_{state}.png')
            images.append(im)
            labels.append(f'{label}　／　{state_label}')
    if images:
        contact_sheet(images, labels, out / 'nav_sheet.png')
        print(f'wrote {len(images)} mocks + nav_sheet.png to {out}')

    images, labels = [], []
    for key, label in RESIDENTS:
        for state, state_label in (('near', '教官が見えている'), ('far', '字幕が消えた後・目的地は画面外')):
            im = resident(shot, key, state)
            im.convert('RGB').save(out / f'nav_{key}_{state}.png')
            images.append(im)
            labels.append(f'{label}　／　{state_label}')
    contact_sheet(images, labels, out / 'nav_resident_sheet.png')
    print(f'wrote {len(images)} mocks + nav_resident_sheet.png to {out}')


if __name__ == '__main__':
    main()
