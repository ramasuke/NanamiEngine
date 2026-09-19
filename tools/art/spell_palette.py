"""Generate the MagicCaster spell palette sprites (bottom-left magic circle) drawn by GamePlay::Ui::SpellPalette / SpellSlot.

    python tools/art/spell_palette.py [--out-dir Assets/Art/UI/SpellPalette] [--icon-dir Assets/Art/UI/Spell] [--preview PATH]

Writes the circle, mana arc, halo, slot parts and the glyphs the item bar / control guide don't already have into
--out-dir, and one icon per spell into --icon-dir, with a SpriteFile .png.meta for each (an existing .meta keeps its
GUID, so regenerating never breaks references). Pad face-button glyphs are reused from Assets/Art/UI/ControlGuide.
The layout values SpellPaletteUI.prefab / SpellSlot.prefab have to agree with come from layout() and are printed at
the end (GEOMETRY); tools/art/spell_palette_prefab.py builds the prefabs from the same layout().
Design chosen 2026-09-18 from real-screen mocks (案D: bottom-left, since the item pouch owns the bottom-right).

Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageOps

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art.control_guide import (  # noqa: E402
    Canvas, S, blur, cov, hexc, render_keys, render_mouse_left, render_pad_trigger, sd_circle, sd_rbox,
)
from tools.art.cannon_cooldown_gauge import STEEL, ramp  # noqa: E402
from tools.scene import sprite_meta  # noqa: E402

TAU = 2 * math.pi
SQ2 = math.sqrt(2)

INK = hexc('#070a0d')
WHITE = hexc('#ffffff')
BLACK = hexc('#000000')
GOLD_LINE = hexc('#dcb86e')
VIOLET = hexc('#9b6bff')
VIOLET_RAMP = [(0.0, '#3e2296'), (0.65, '#8a58ff'), (1.0, '#dccbff')]

# ---- 陣（画面座標、ルート = 陣の中心）
ROOT_POS = (262.0, 866.0)
RING_RADIUS = 92.0            # 枠を置く円
ARC_RADIUS = 138.0            # MP の弧
ARC_START_DEG = 210.0         # 12時から時計回り。下側 60° が切れ目
ARC_SPAN_DEG = 300.0
CIRCLE_PX = int(2 * (ARC_RADIUS + 16))
HALO_PX = 390
TIP_PX = 24

# ---- 枠
SOCKET_H = 34.0               # 中心から菱形の頂点まで（手前のページ）
SOCKET_PAD = 10
SOCKET_PX = int(math.ceil(2 * SOCKET_H + 2 * SOCKET_PAD))
BACK_SCALE = 19.0 / SOCKET_H  # 奥のページ（斜め）の縮尺
ICON_PX = 44
PILL_W, PILL_H = 34, 20
PILL_OFFSET = (SOCKET_H * 0.56, SOCKET_H * 0.56)
GLYPH_DISTANCE = SOCKET_H * 0.98

SPELL_COLORS = {
    'MagicBolt': '#9fd4ff', 'FireBall': '#ff9a3a', 'VioletFlame': '#c98aff', 'ExplosionBlast': '#ff6a3c',
    'QuakeBlast': '#dcae6c', 'Heal': '#7eea98', 'Might': '#ffd462', 'RockWall': '#c9c1b2', 'ThunderTrap': '#fff27a',
    'WindCutter': '#8ff0cf', 'ArcaneRay': '#ff7ad0', 'FrostBreath': '#a8e4ff',
}


# ---------------------------------------------------------------- helpers
def sd_diamond(X, Y, cx, cy, h, r):
    u = ((X - cx) + (Y - cy)) / SQ2
    v = ((X - cx) - (Y - cy)) / SQ2
    a = h / SQ2
    return sd_rbox(u, v, -a, -a, a, a, r)


def tapered(X, Y, ax, ay, bx, by, r0, r1):
    dx, dy = bx - ax, by - ay
    p = np.clip(((X - ax) * dx + (Y - ay) * dy) / (dx * dx + dy * dy), 0, 1)
    return np.hypot(X - (ax + p * dx), Y - (ay + p * dy)) - (r0 + (r1 - r0) * p)


def poly(c, pts):
    im = Image.new('L', (c.w * S, c.h * S), 0)
    ImageDraw.Draw(im).polygon([(x * S, y * S) for x, y in pts], fill=255)
    return np.asarray(im, np.float32) / 255


def lighten(col, t):
    return col * (1 - t) + WHITE * t


# ---------------------------------------------------------------- circle + mana arc
def render_circle():
    """金の細線の二重円 + 刻み + 六芒星。暗い放射状の下地だけで、平板なパネルは敷かない"""
    W = CIRCLE_PX
    c = Canvas(W, W)
    X, Y = c.X, c.Y
    cx = cy = W / 2
    rho = np.hypot(X - cx, Y - cy)
    th = np.arctan2(X - cx, -(Y - cy)) % TAU
    c.over(INK, np.clip(1 - rho / (ARC_RADIUS + 4), 0, 1) ** 0.5 * 0.5)
    for R, w in ((RING_RADIUS + 28, 1.5), (RING_RADIUS + 18, 1.0), (RING_RADIUS - 30, 1.0)):
        c.over(GOLD_LINE, cov(np.abs(rho - R) - w / 2) * 0.9)
    n = 72
    idx = np.floor(th / TAU * n)
    seg = (th / TAU * n) % 1.0
    band = (rho > RING_RADIUS + 20) & (rho < RING_RADIUS + 26 - 2 * (idx % 3 == 0))
    c.over(GOLD_LINE, cov(np.abs(seg - 0.5) * TAU / n * rho - 0.55) * band * 0.9)
    for i in range(6):
        a0, a1 = i * TAU / 6, (i + 2) * TAU / 6
        ax, ay = cx + math.sin(a0) * (RING_RADIUS + 18), cy - math.cos(a0) * (RING_RADIUS + 18)
        bx, by = cx + math.sin(a1) * (RING_RADIUS + 18), cy - math.cos(a1) * (RING_RADIUS + 18)
        c.over(GOLD_LINE, cov(tapered(X, Y, ax, ay, bx, by, 0.5, 0.5)) * 0.4)
    return c.resolve()


def _arc_phase(X, Y, cx, cy):
    th = np.arctan2(X - cx, -(Y - cy)) % TAU
    return (th - math.radians(ARC_START_DEG)) % TAU


def render_mana_track():
    W = CIRCLE_PX
    c = Canvas(W, W)
    X, Y = c.X, c.Y
    cx = cy = W / 2
    rho = np.hypot(X - cx, Y - cy)
    track = cov(np.abs(rho - ARC_RADIUS) - 5.5) * (_arc_phase(X, Y, cx, cy) <= math.radians(ARC_SPAN_DEG))
    c.over(INK, track)
    c.over(ramp((Y - (cy - ARC_RADIUS)) / (2 * ARC_RADIUS), STEEL), track * (1 - cov(np.abs(rho - ARC_RADIUS) - 3.8)))
    return c.resolve()


def render_mana_fill():
    """DrawCircleGauge で削って使う全周の帯。色は弧の始点からの位置で付けておく"""
    W = CIRCLE_PX
    c = Canvas(W, W)
    X, Y = c.X, c.Y
    cx = cy = W / 2
    rho = np.hypot(X - cx, Y - cy)
    phase = _arc_phase(X, Y, cx, cy) / math.radians(ARC_SPAN_DEG)
    c.over(ramp(np.clip(phase, 0, 1), VIOLET_RAMP), cov(np.abs(rho - ARC_RADIUS) - 3.2))
    return c.resolve()


def render_mana_tip():
    c = Canvas(TIP_PX, TIP_PX)
    X, Y = c.X, c.Y
    cc = TIP_PX / 2
    c.over(hexc('#e6dcff'), np.clip(blur(cov(sd_circle(X, Y, cc, cc, 4)), 3) * 1.8, 0, 1) * 0.9)
    return c.resolve()


def render_halo():
    c = Canvas(HALO_PX, HALO_PX)
    cc = HALO_PX / 2
    c.over(VIOLET, np.clip(blur(cov(sd_circle(c.X, c.Y, cc, cc, 150 * 0.72)), 150 * 0.28), 0, 1) * 0.55)
    return c.resolve()


# ---------------------------------------------------------------- slot parts
def _slot_canvas():
    c = Canvas(SOCKET_PX, SOCKET_PX)
    return c, c.X, c.Y, SOCKET_PX / 2, SOCKET_PX / 2


def render_socket():
    """鋼の菱形の窓。中は暗い窓で、色はアイコンが持つ"""
    c, X, Y, cx, cy = _slot_canvas()
    h = SOCKET_H
    outer = sd_diamond(X, Y, cx, cy, h, h * 0.10)
    c.over(BLACK, blur(cov(outer), 3.0) * 0.6)
    c.over(INK, cov(outer))
    rim = cov(outer + 1.2) * (1 - cov(outer + h * 0.17))
    c.over(ramp((Y - (cy - h)) / (2 * h), STEEL), rim)
    c.over(WHITE, (cov(outer + 1.2) - cov(outer + 2.3)) * np.clip((cy - Y) / h + 0.25, 0, 1) * 0.6)
    inner = sd_diamond(X, Y, cx, cy, h * 0.80, h * 0.06)
    t = np.clip(np.hypot(X - cx, Y - (cy - h * 0.05)) / (h * 0.78), 0, 1)[..., None]
    c.over(hexc('#2a1d44') * (1 - t) + INK * t, cov(inner))
    c.over(hexc('#3a4048'), cov(inner) * (1 - cov(inner + 1.5)) * 0.8)
    return c.resolve()


def render_socket_active():
    """LT を押している間だけ重ねる、窓の縁の光"""
    c, X, Y, cx, cy = _slot_canvas()
    inner = sd_diamond(X, Y, cx, cy, SOCKET_H * 0.80, SOCKET_H * 0.06)
    trim = cov(inner) * (1 - cov(inner + 1.6))
    c.over(VIOLET, np.clip(blur(trim, 1.6) * 1.8, 0, 1) * 0.6)
    c.over(lighten(VIOLET, 0.45), trim)
    return c.resolve()


def render_window_shade(alpha):
    """窓の形の暗幕。クールタイム（DrawCircleGauge で削る）と MP 不足の両方で使う"""
    c, X, Y, cx, cy = _slot_canvas()
    c.over(hexc('#05070a'), cov(sd_diamond(X, Y, cx, cy, SOCKET_H * 0.80 - 1, SOCKET_H * 0.06)) * alpha)
    return c.resolve()


def render_cost_pill():
    c = Canvas(PILL_W + 6, PILL_H + 6)
    d = sd_rbox(c.X, c.Y, 3, 3, 3 + PILL_W, 3 + PILL_H, PILL_H / 2)
    c.over(BLACK, blur(cov(d), 1.5) * 0.6)
    c.over(INK, cov(d) * 0.94)
    c.over(hexc('#7654c8'), cov(d) * (1 - cov(d + 1.6)))
    return c.resolve()


# ---------------------------------------------------------------- spell icons (authored on a 56 grid)
def _icon_canvas():
    c = Canvas(ICON_PX, ICON_PX)
    return c, c.X, c.Y, ICON_PX / 56.0


def _icon_glow(c, mask, col, k):
    c.over(col, np.clip(blur(mask, 3.2 * k) * 1.5, 0, 1) * 0.55)


def icon_bolt(col):
    c, X, Y, k = _icon_canvas()
    core = sd_circle(X, Y, 34 * k, 22 * k, 9 * k)
    trail = np.minimum.reduce([tapered(X, Y, 34 * k, 22 * k, 12 * k, 44 * k, 8 * k, 0.5 * k),
                               tapered(X, Y, 36 * k, 26 * k, 20 * k, 48 * k, 3 * k, 0.4 * k),
                               tapered(X, Y, 30 * k, 20 * k, 8 * k, 36 * k, 3 * k, 0.4 * k)])
    mask = np.maximum(cov(core), cov(trail))
    _icon_glow(c, mask, col, k)
    c.over(col, cov(trail) * 0.8)
    c.over(col, cov(core))
    c.over(lighten(col, 0.8), cov(sd_circle(X, Y, 35 * k, 21 * k, 4.5 * k)))
    return c.resolve()


def icon_flame(col):
    c, X, Y, k = _icon_canvas()
    body = np.minimum.reduce([
        sd_circle(X, Y, 26 * k, 35 * k, 13 * k),
        tapered(X, Y, 26 * k, 35 * k, 40 * k, 7 * k, 13 * k, 0.6 * k),
        tapered(X, Y, 22 * k, 31 * k, 13 * k, 12 * k, 7 * k, 0.5 * k),
    ])
    core = np.minimum(sd_circle(X, Y, 27 * k, 38 * k, 7 * k), tapered(X, Y, 27 * k, 38 * k, 35 * k, 19 * k, 7 * k, 0.5 * k))
    _icon_glow(c, cov(body), col, k)
    c.over(col, cov(body))
    c.over(lighten(col, 0.72), cov(core))
    return c.resolve()


def icon_blast(col):
    c, X, Y, k = _icon_canvas()
    cx = cy = 28 * k
    rho = np.hypot(X - cx, Y - cy)
    th = np.arctan2(Y - cy, X - cx) + 0.2
    spike = (1 - np.abs((th / TAU * 9) % 1.0 * 2 - 1)) ** 1.6
    star = rho - (8 + 15 * spike) * k
    _icon_glow(c, cov(star), col, k)
    c.over(col, cov(star))
    c.over(lighten(col, 0.55), cov(rho - 9 * k))
    c.over(lighten(col, 0.9), cov(rho - 4.5 * k))
    return c.resolve()


def icon_quake(col):
    c, X, Y, k = _icon_canvas()
    spikes = [((9, 47), (17, 20), (25, 47)), ((21, 47), (30, 7), (40, 47)), ((35, 47), (44, 24), (51, 47))]
    mask = np.zeros_like(X)
    for tri in spikes:
        mask = np.maximum(mask, poly(c, [(x * k, y * k) for x, y in tri]))
    slab = cov(sd_rbox(X, Y, 5 * k, 45 * k, 51 * k, 51 * k, 1.5 * k))
    _icon_glow(c, np.maximum(mask, slab), col, k)
    c.over(col * 0.62, slab)
    for (lx, ly), (ax, ay), (rx, ry) in spikes:
        c.over(col, poly(c, [(lx * k, ly * k), (ax * k, ay * k), (rx * k, ry * k)]))
        c.over(col * 0.66, poly(c, [(ax * k, ay * k), (rx * k, ry * k), (ax * k, ly * k)]))
    return c.resolve()


def _astroid(X, Y, cx, cy, s):
    return ((np.sqrt(np.abs(X - cx)) + np.sqrt(np.abs(Y - cy))) < math.sqrt(s)).astype(np.float32)


def icon_heal(col):
    c, X, Y, k = _icon_canvas()
    cross = np.minimum(sd_rbox(X, Y, 21 * k, 11 * k, 35 * k, 45 * k, 3 * k), sd_rbox(X, Y, 11 * k, 21 * k, 45 * k, 35 * k, 3 * k))
    inner = np.minimum(sd_rbox(X, Y, 25 * k, 15 * k, 31 * k, 41 * k, 2 * k), sd_rbox(X, Y, 15 * k, 25 * k, 41 * k, 31 * k, 2 * k))
    sparks = np.maximum(_astroid(X, Y, 46 * k, 10 * k, 9 * k), _astroid(X, Y, 10 * k, 46 * k, 6 * k))
    _icon_glow(c, np.maximum(cov(cross), sparks), col, k)
    c.over(col, cov(cross))
    c.over(lighten(col, 0.6), cov(inner))
    c.over(lighten(col, 0.8), sparks)
    return c.resolve()


def icon_might(col):
    c, X, Y, k = _icon_canvas()
    blade = poly(c, [(26 * k, 5 * k), (30 * k, 11 * k), (30 * k, 35 * k), (22 * k, 35 * k), (22 * k, 11 * k)])
    guard = cov(sd_rbox(X, Y, 14 * k, 35 * k, 38 * k, 40 * k, 1.5 * k))
    grip = cov(sd_rbox(X, Y, 24 * k, 40 * k, 28 * k, 49 * k, 1 * k))
    pommel = cov(sd_circle(X, Y, 26 * k, 51 * k, 2.8 * k))
    chev = np.zeros_like(X)
    for oy in (0, 9):
        a = np.minimum(tapered(X, Y, 39 * k, (27 + oy) * k, 45 * k, (20 + oy) * k, 1.6 * k, 1.6 * k),
                       tapered(X, Y, 45 * k, (20 + oy) * k, 51 * k, (27 + oy) * k, 1.6 * k, 1.6 * k))
        chev = np.maximum(chev, cov(a))
    _icon_glow(c, np.maximum.reduce([blade, guard, grip, pommel, chev]), col, k)
    c.over(lighten(hexc('#c8d0d8'), 0.2), blade)
    c.over(hexc('#8a929a'), blade * (X > 26 * k))
    c.over(col, np.maximum(guard, pommel))
    c.over(col * 0.6, grip)
    c.over(col, chev)
    return c.resolve()


def icon_wall(col):
    c, X, Y, k = _icon_canvas()
    rows = [(12, [(7, 19), (21, 33), (35, 49)]), (25, [(7, 13), (15, 27), (29, 41), (43, 49)]),
            (38, [(7, 19), (21, 33), (35, 49)])]
    mask = np.zeros_like(X)
    bricks = []
    for i, (y0, xs) in enumerate(rows):
        for j, (x0, x1) in enumerate(xs):
            m = cov(sd_rbox(X, Y, x0 * k, y0 * k, x1 * k, (y0 + 11) * k, 1.4 * k))
            bricks.append((m, 0.78 + 0.22 * ((i * 3 + j * 5) % 4) / 3))
            mask = np.maximum(mask, m)
    _icon_glow(c, mask, col, k)
    for m, shade in bricks:
        c.over(col * shade, m)
    return c.resolve()


def icon_trap(col):
    c, X, Y, k = _icon_canvas()
    rho = np.hypot(X - 28 * k, (Y - 44 * k) * 2.6)
    ring = cov(np.abs(rho - 21 * k) - 1.6 * k)
    bolt = poly(c, [(31 * k, 3 * k), (20 * k, 26 * k), (27 * k, 26 * k), (22 * k, 45 * k), (38 * k, 19 * k),
                    (31 * k, 19 * k), (37 * k, 3 * k)])
    _icon_glow(c, np.maximum(ring, bolt), col, k)
    c.over(col, cov(rho - 21 * k) * 0.22)
    c.over(col, ring)
    c.over(col, bolt)
    c.over(lighten(col, 0.7), bolt * cov(np.abs(X - 29 * k) - 2.2 * k))
    return c.resolve()


def icon_wind(col):
    c, X, Y, k = _icon_canvas()
    blade = np.maximum(sd_circle(X, Y, 30 * k, 30 * k, 21 * k), -sd_circle(X, Y, 37 * k, 23 * k, 19 * k))
    edge = np.maximum(sd_circle(X, Y, 30 * k, 30 * k, 21 * k), -sd_circle(X, Y, 32 * k, 28 * k, 20 * k))
    lines = np.minimum.reduce([tapered(X, Y, 4 * k, 42 * k, 20 * k, 50 * k, 1.5 * k, 0.4 * k),
                               tapered(X, Y, 3 * k, 33 * k, 13 * k, 41 * k, 1.2 * k, 0.4 * k)])
    _icon_glow(c, np.maximum(cov(blade), cov(lines)), col, k)
    c.over(col, cov(blade))
    c.over(lighten(col, 0.75), cov(edge))
    c.over(col * 0.85, cov(lines))
    return c.resolve()


def icon_ray(col):
    c, X, Y, k = _icon_canvas()
    beam = tapered(X, Y, 13 * k, 43 * k, 51 * k, 7 * k, 3.0 * k, 6.0 * k)
    core = tapered(X, Y, 13 * k, 43 * k, 51 * k, 7 * k, 1.1 * k, 2.3 * k)
    rho = np.hypot(X - 13 * k, Y - 43 * k)
    circle = cov(np.abs(rho - 8.5 * k) - 1.3 * k)
    _icon_glow(c, np.maximum(cov(beam), circle), col, k)
    c.over(col, circle)
    c.over(col, cov(beam))
    c.over(lighten(col, 0.85), cov(core))
    c.over(lighten(col, 0.85), _astroid(X, Y, 13 * k, 43 * k, 5 * k))
    return c.resolve()


def icon_frost(col):
    c, X, Y, k = _icon_canvas()
    cx = cy = 28 * k
    arms = np.zeros_like(X)
    for i in range(6):
        t = math.pi / 2 + i * math.pi / 3
        ex, ey = cx + 21 * k * math.cos(t), cy - 21 * k * math.sin(t)
        arms = np.maximum(arms, cov(tapered(X, Y, cx, cy, ex, ey, 2.3 * k, 1.3 * k)))
        bx, by = cx + 12 * k * math.cos(t), cy - 12 * k * math.sin(t)
        for s in (-1, 1):
            b = t + s * math.pi / 4
            arms = np.maximum(arms, cov(tapered(X, Y, bx, by, bx + 7 * k * math.cos(b), by - 7 * k * math.sin(b),
                                                1.5 * k, 0.9 * k)))
    _icon_glow(c, arms, col, k)
    c.over(col, arms)
    c.over(lighten(col, 0.8), cov(sd_circle(X, Y, cx, cy, 4.2 * k)))
    return c.resolve()


ICONS = {
    'Icon_Spell_MagicBolt': lambda: icon_bolt(hexc(SPELL_COLORS['MagicBolt'])),
    'Icon_Spell_FireBall': lambda: icon_flame(hexc(SPELL_COLORS['FireBall'])),
    'Icon_Spell_VioletFlame': lambda: icon_flame(hexc(SPELL_COLORS['VioletFlame'])),
    'Icon_Spell_ExplosionBlast': lambda: icon_blast(hexc(SPELL_COLORS['ExplosionBlast'])),
    'Icon_Spell_QuakeBlast': lambda: icon_quake(hexc(SPELL_COLORS['QuakeBlast'])),
    'Icon_Spell_Heal': lambda: icon_heal(hexc(SPELL_COLORS['Heal'])),
    'Icon_Spell_Might': lambda: icon_might(hexc(SPELL_COLORS['Might'])),
    'Icon_Spell_RockWall': lambda: icon_wall(hexc(SPELL_COLORS['RockWall'])),
    'Icon_Spell_ThunderTrap': lambda: icon_trap(hexc(SPELL_COLORS['ThunderTrap'])),
    'Icon_Spell_WindCutter': lambda: icon_wind(hexc(SPELL_COLORS['WindCutter'])),
    'Icon_Spell_ArcaneRay': lambda: icon_ray(hexc(SPELL_COLORS['ArcaneRay'])),
    'Icon_Spell_FrostBreath': lambda: icon_frost(hexc(SPELL_COLORS['FrostBreath'])),
}

SPRITES = {
    'SpellPalette_Circle': render_circle,
    'SpellPalette_ManaTrack': render_mana_track,
    'SpellPalette_ManaFill': render_mana_fill,
    'SpellPalette_ManaTip': render_mana_tip,
    'SpellPalette_Halo': render_halo,
    'SpellSlot_Socket': render_socket,
    'SpellSlot_SocketActive': render_socket_active,
    'SpellSlot_CooldownShade': lambda: render_window_shade(0.72),
    'SpellSlot_ManaLackShade': lambda: render_window_shade(0.55),
    'SpellSlot_CostPill': render_cost_pill,
    'SpellPalette_Key_1': lambda: render_keys('1'),
    'SpellPalette_Key_2': lambda: render_keys('2'),
    'SpellPalette_Key_3': lambda: render_keys('3'),
    'SpellPalette_Key_4': lambda: render_keys('4'),
    'SpellPalette_Key_Range': lambda: render_keys('1-4'),
    'SpellPalette_Pad_LT': lambda: render_pad_trigger('LT'),
    'SpellPalette_Pad_RB': lambda: render_pad_trigger('RB'),
    'SpellPalette_Mouse_Right': lambda: ImageOps.mirror(render_mouse_left()),
}


def layout():
    """SpellPaletteUI / SpellSlot の配置。座標はルート（陣の中心）からの相対、文字の pos は TextRenderer の基準点"""
    front = {'Top': (0.0, -RING_RADIUS), 'Right': (RING_RADIUS, 0.0), 'Bottom': (0.0, RING_RADIUS), 'Left': (-RING_RADIUS, 0.0)}
    d = RING_RADIUS / SQ2
    back = {'TopRight': (d, -d), 'BottomRight': (d, d), 'BottomLeft': (-d, d), 'TopLeft': (-d, -d)}
    name_px = 20
    return {
        'root': ROOT_POS,
        'front_anchors': front,
        'back_anchors': back,
        'back_scale': BACK_SCALE,
        'names': {
            'Top':    {'pos': (0.0, -165.0 - name_px), 'align': 'center', 'px': name_px},
            'Right':  {'pos': (162.0, -name_px / 2), 'align': 'left', 'px': name_px},
            'Bottom': {'pos': (0.0, 165.0), 'align': 'center', 'px': name_px},
            'Left':   {'pos': (-162.0, -name_px / 2), 'align': 'right', 'px': name_px},
        },
        'palette_glyph': (0.0, -30.0),
        'mana_label': {'pos': (0.0, -11.0), 'align': 'center', 'px': 14},
        'mana_value': {'pos': (0.0, 5.0), 'align': 'center', 'px': 34},
        'page_glyph': (-144.0, 132.0),
        'page_text': {'pos': (-124.0, 122.0), 'align': 'left', 'px': 20},
        'arc': {'radius': ARC_RADIUS, 'start_deg': ARC_START_DEG, 'span_deg': ARC_SPAN_DEG},
        'slot': {
            'icon': (0.0, -SOCKET_H * 0.07),
            'cost_pill': PILL_OFFSET,
            'cost_text': {'pos': (PILL_OFFSET[0], PILL_OFFSET[1] - 9.0), 'align': 'center', 'px': 17},
            'cooldown_text': {'pos': (0.0, -11.0), 'align': 'center', 'px': 19},
            'glyph_distance': GLYPH_DISTANCE,
        },
    }


def write_sprite(out_dir, name, image):
    png = out_dir / f"{name}.png"
    image.save(png)
    meta = out_dir / f"{name}.png.meta"
    guid = sprite_meta.read_meta(meta)["guid"] if meta.exists() else sprite_meta.mint_guid()
    sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
    return guid


def render_preview(sprites, icons, path):
    """ざっくり1ページ目を開いた状態を合成する（配置の確認用）"""
    W, H = 520, 460
    sheet = Image.new('RGBA', (W, H), (52, 66, 48, 255))
    geo = layout()
    cx, cy = W / 2, H / 2 + 10

    def put(im, pos, scale=1.0):
        if scale != 1.0:
            im = im.resize((max(1, round(im.width * scale)), max(1, round(im.height * scale))), Image.LANCZOS)
        sheet.alpha_composite(im, (round(cx + pos[0] - im.width / 2), round(cy + pos[1] - im.height / 2)))

    put(sprites['SpellPalette_Halo'], (0, 0))
    put(sprites['SpellPalette_Circle'], (0, 0))
    put(sprites['SpellPalette_ManaTrack'], (0, 0))
    put(sprites['SpellPalette_ManaFill'], (0, 0))
    names = list(icons)
    for i, (key, pos) in enumerate(geo['front_anchors'].items()):
        put(sprites['SpellSlot_Socket'], pos)
        put(sprites['SpellSlot_SocketActive'], pos)
        put(icons[names[1 + i]], (pos[0], pos[1] + geo['slot']['icon'][1]))
        put(sprites['SpellSlot_CostPill'], (pos[0] + PILL_OFFSET[0], pos[1] + PILL_OFFSET[1]))
    for i, (key, pos) in enumerate(geo['back_anchors'].items()):
        put(sprites['SpellSlot_Socket'], pos, BACK_SCALE)
        put(icons[names[5 + i % 4]], pos, BACK_SCALE)
    put(sprites['SpellPalette_Pad_LT'], geo['palette_glyph'])
    put(sprites['SpellPalette_Pad_RB'], geo['page_glyph'])
    sheet.convert('RGB').save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/SpellPalette')
    ap.add_argument('--icon-dir', default='Assets/Art/UI/Spell')
    ap.add_argument('--preview', help='also write a rough composite of the opened palette to this PNG path')
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    icon_dir = Path(args.icon_dir)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir
    if not icon_dir.is_absolute():
        icon_dir = REPO_ROOT / icon_dir
    out_dir.mkdir(parents=True, exist_ok=True)
    icon_dir.mkdir(parents=True, exist_ok=True)

    sprites = {}
    for name, fn in SPRITES.items():
        sprites[name] = fn()
        guid = write_sprite(out_dir, name, sprites[name])
        print(f"{name:28s} {sprites[name].size[0]}x{sprites[name].size[1]}  {guid}")

    icons = {}
    for name, fn in ICONS.items():
        icons[name] = fn()
        guid = write_sprite(icon_dir, name, icons[name])
        print(f"{name:28s} {icons[name].size[0]}x{icons[name].size[1]}  {guid}")

    print("GEOMETRY (SpellPaletteUI.prefab / SpellSlot.prefab):")
    for key, value in layout().items():
        print(f"  {key} = {value}")

    if args.preview:
        render_preview(sprites, icons, args.preview)
        print(f"preview -> {args.preview}")


if __name__ == '__main__':
    main()
