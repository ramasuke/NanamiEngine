"""Build the MagicCaster spell particle effects (Effekseer .efkproj -> .efkefc) and install them under Assets/Art/Effect/Magic.

    python tools/art/magic_spell_effects.py --build-dir DIR [--only NAME ...] [--install]

Two families, both authored in metres (prefabs play them at scale 8, see tools/art/magic_fx_lib.py):
* spell effects - projectile travel / impact, area sigils and bursts, buffs, beam, breath;
* Cast_<Motion> - one per cast animation, spawned at the caster's feet when Cast State starts and choreographed to
  that clip: the hand positions/frames below were measured in Blender from the Mixamo "Magic Spell Pack" clips
  (Assets/Art/Animation/MagicCaster/Spell/_Source) and are converted with the playback speed each clip has in
  MagicCasterAnimation.animTree (CAST_SPEED), so frame f of a clip lands at Effekseer frame 60 * (f - 1) / speed.
  Positions are (x, y, z) with +z = the caster's forward and -x = the caster's right, from the caster's feet.

Requires Pillow + numpy (textures) and the Effekseer CUI configured in tools/effect/effect_config.json.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art import magic_fx_textures  # noqa: E402
from tools.art.magic_fx_lib import (  # noqa: E402
    ADD, BLEND, FACE, FIXED, LIVE_LONG, ON_CREATE, YAXIS, G, N, emit_circle, emit_sphere, flipbook, project,
)
from tools.effect import xmlio  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Magic'

# clip playback speeds (MV1 frames per second) in MagicCasterAnimation.animTree
CAST_SPEED = {
    'OneHandThrust': 60, 'OneHandSweep': 40, 'OneHandUppercut': 40, 'OneHandRaise': 36, 'TwoHandRaise': 36,
    'TwoHandSlam': 38, 'TwoHandBurst': 40, 'TwoHandThrow': 40, 'TwoHandSwingPush': 40, 'TwoHandBeam': 36,
    'TwoHandPushHold': 36, 'TwoHandPray': 45,
}


def ef(motion, clip_frame):
    """Effekseer frame at which the cast clip (Blender numbering, first frame = 1) reaches clip_frame."""
    return round(60 * (clip_frame - 1) / CAST_SPEED[motion])


# ---------------------------------------------------------------- palettes (RGB)
BOLT = ((215, 240, 255), (90, 170, 255))
FIRE = ((255, 236, 190), (255, 140, 40), (230, 70, 20))
VIOLET = ((248, 228, 255), (190, 110, 255), (110, 50, 220))
WIND = ((235, 255, 245), (110, 235, 195))
BLAST = ((255, 232, 170), (255, 110, 50), (200, 50, 20))
EARTH = ((255, 205, 130), (205, 160, 95), (150, 120, 90))
STONE = (185, 170, 150)
THUNDER = ((255, 255, 225), (255, 238, 110), (255, 200, 60))
HEAL = ((230, 255, 235), (120, 240, 150))
MIGHT = ((255, 246, 210), (255, 210, 90), (255, 150, 40))
RAY = ((255, 235, 250), (255, 90, 200), (190, 50, 230))
FROST = ((240, 250, 255), (170, 225, 255), (110, 180, 255))
SMOKE_DARK = (60, 50, 45)


def a(rgb, alpha):
    return (*rgb, alpha)


# ================================================================ shared pieces
def flash(name, rgb, size, *, at=(0, 0, 0), delay=0, life=10, alpha=255):
    return N(name, tex='glow_core', at=at, delay=delay, life=life, grow=(size * 0.35, size, 20, 0),
             color=a(rgb, alpha), fade_out=(max(2, life - 2), 0, -20))


def shockwave(name, rgb, radius, *, at=(0, 0, 0), delay=0, life=24, flat=True, alpha=200, width=0.14):
    return N(name, kind='ring', at=at, delay=delay, life=life, rot=(90, 0, 0) if flat else None,
             billboard=FIXED if flat else FACE, grow=(0.15, 1.0, 20, -10),
             ring={'outer': (radius, 0), 'inner': (radius * (1 - width), 0), 'center_ratio': 0.4,
                   'outer_color': a(rgb, 0), 'center_color': a(rgb, alpha), 'inner_color': a(rgb, 0)},
             fade_out=(max(4, life - 6), 0, -20))


def sparks(name, rgb, *, count, speed, at=(0, 0, 0), delay=0, interval=0, life=(18, 28), size=(0.12, 0.22),
           gravity=-0.004, upper=False, radius=(0.0, 0.15), tex='spark', alpha=255):
    lo, hi = speed if isinstance(speed, tuple) else (speed * 0.6, speed)
    return N(name, tex=tex, count=count, delay=delay, interval=interval, life=life, at=at,
             emit=emit_sphere(radius, upper=upper), vel=(0, (lo, hi), 0), acc=(0, -hi * 0.035, 0),
             gravity=(0, gravity, 0), grow=(size, (size[0] * 0.15, size[1] * 0.15), 0, 20), color=a(rgb, alpha),
             fade_out=(10, 0, -20))


def converge(name, rgb, *, at, radius, count, life, delay=0, interval=1, size=(0.08, 0.16), tex='glow', alpha=230):
    """Particles born on a sphere shell around `at` that fly into it."""
    speed = radius / life
    return N(name, tex=tex, count=count, delay=delay, interval=interval, life=life, at=at,
             emit=emit_sphere(radius), vel=(0, -speed, 0), grow=(size, (size[0] * 1.6, size[1] * 1.6), 0, 20),
             color=a(rgb, alpha), fade_in=6, fade_out=(4, 0, -20))


def rune(name, rgb, radius, *, at=(0, 0.04, 0), delay=0, life=60, alpha=220, spin=1.5, fade_in=8, fade_out=16,
         grow=None, vertical=False):
    """Flat (or, vertical=True, forward-facing) magic circle; the parent spins about the one axis the circle faces.
    Easing runs over a particle's whole life, so only short-lived circles should pass `grow`."""
    size = radius / 0.47
    circle = dict(tex='rune_circle', billboard=FIXED, life=life, color=a(rgb, alpha), fade_in=fade_in,
                  fade_out=(fade_out, 0, -20))
    if grow:
        circle['grow'] = (size * grow[0], size * grow[1], 30, -30)
    else:
        circle['size'] = size
    if vertical:
        return G(name, [N(name + 'Circle', **circle)], at=at, delay=delay, life=life, spin=(0, 0, spin))
    return G(name, [N(name + 'Circle', rot=(90, 0, 0), **circle)], at=at, delay=delay, life=life, spin=(0, spin, 0))


def ground_glow(name, rgb, size, *, at=(0, 0.03, 0), delay=0, life=40, alpha=140, grow=None, tex='glow', blend=ADD,
                fade_in=6, fade_out=20):
    return N(name, tex=tex, blend=blend, rot=(90, 0, 0), billboard=FIXED, at=at, delay=delay, life=life,
             size=None if grow else size, grow=grow, color=a(rgb, alpha), fade_in=fade_in,
             fade_out=(fade_out, 0, -20))


def trail_segment(name, p0, p1, f0, f1, children):
    """Invisible emitter moving p0 -> p1 between Effekseer frames f0 and f1; children spawn along its path."""
    return G(name, children, move=(p0, p1), delay=f0, life=max(1, f1 - f0))


def trail_glow(name, rgb, frames, *, size=(0.32, 0.5), life=14, alpha=210, tex='glow', interval=1, spread=0.04,
               drift=(0, 0.004, 0)):
    return N(name, tex=tex, count=max(1, frames // interval + 1), interval=interval, life=life, bind=ON_CREATE,
             at_rand=((-spread, spread), (-spread, spread), (-spread, spread)),
             vel=tuple((d - 0.003, d + 0.003) for d in drift), grow=(size, (size[0] * 0.2, size[1] * 0.2), 0, 10),
             color=a(rgb, alpha), fade_out=(life - 2, 0, -10))


def orb(name, core, glow_rgb, *, life, size=None, at=None, move=None, delay=0, grow=None, alpha=235, flicker=True):
    kw = dict(at=at, move=move, delay=delay, life=life)
    k = 1.35   # hand-sized orbs read too small next to a 2 m caster at the default 1.0
    size = size * k if size else None
    grow = (grow[0] * k, grow[1] * k) if grow else None
    halo_grow = (grow[0] * 2.6, grow[1] * 2.6, 20, 0) if grow else None
    core_grow = (grow[0], grow[1], 20, 0) if grow else None
    return G(name, [
        N(name + 'Halo', tex='glow', life=life, size=None if grow else size * 2.6, grow=halo_grow,
          color=a(glow_rgb, int(alpha * 0.75)), fade_in=4, fade_out=(6, 0, -20)),
        N(name + 'Core', tex='glow_core', life=life, size=None if grow else size, grow=core_grow,
          color=a(core, alpha), fade_in=4, fade_out=(6, 0, -20)),
        *([N(name + 'Star', tex='spark', life=life, size=None if grow else size * 1.8,
             grow=(grow[0] * 1.8, grow[1] * 1.8, 20, 0) if grow else None, spin=(0, 0, 7),
             color=a(core, int(alpha * 0.7)), fade_in=4, fade_out=(6, 0, -20))] if flicker else []),
    ], **kw)


def smoke(name, rgb, *, count=1, life, at=(0, 0, 0), delay=0, interval=0, emit=None, at_rand=None, vel=(0, 0.01, 0),
          grow=(0.8, 2.0), alpha=140, blend=BLEND, bind=None, infinite=False):
    return N(name, tex='smoke', blend=blend, count=count, infinite=infinite, delay=delay, interval=interval, life=life, at=at,
             at_rand=at_rand, emit=emit, vel=vel, bind=bind, rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-1.2, 1.2)),
             grow=(grow[0], grow[1], 20, -10), color=a(rgb, alpha), color_to=a(rgb, 0), fade_in=4,
             fade_out=(12, 0, -10))


def flames(name, rgb_hot, rgb_cool, *, count, life, at=(0, 0, 0), delay=0, interval=0, emit=None, at_rand=None,
           vel=(0, 0.02, 0), grow=(0.8, 0.3), bind=None, alpha=230, billboard=FACE):
    return N(name, tex='flame', count=count, delay=delay, interval=interval, life=life, at=at, at_rand=at_rand,
             emit=emit, vel=vel, bind=bind, billboard=billboard, rot_rand=(0, 0, (-25, 25)) if billboard == FACE else None,
             grow=(grow[0], grow[1], 0, 10), color=a(rgb_hot, alpha), color_to=a(rgb_cool, 0), ease=(0, 10),
             fade_in=3)


def rocks(name, *, count, life, at=(0, 0, 0), delay=0, emit=None, at_rand=None, vel=(0, (0.06, 0.11), 0),
          size=(0.18, 0.4), gravity=-0.007, rgb=STONE):
    return N(name, tex='rock', blend=BLEND, count=count, delay=delay, life=life, at=at, at_rand=at_rand, emit=emit,
             vel=vel, gravity=(0, gravity, 0), rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-8, 8)),
             size=(size, size, 1), color=a(rgb, 255), fade_out=(10, 0, -20))


# ================================================================ projectiles
def _travel(core, mid, dark, *, core_size, trail_size, flame_tex=None, smoke_rgb=None, embers=True):
    nodes = [
        N('Halo', tex='glow', life=LIVE_LONG, size=core_size * 2.4, color=a(mid, 150)),
        N('Core', tex='glow_core', life=LIVE_LONG, size=core_size, color=a(core, 255)),
        N('Star', tex='spark', life=LIVE_LONG, size=core_size * 1.8, spin=(0, 0, 6), color=a(core, 190)),
        N('Trail', tex='glow', infinite=True, interval=1, life=12, bind=ON_CREATE,
          grow=(trail_size, trail_size * 0.15, 0, 10), color=a(mid, 170), fade_out=(10, 0, -10)),
    ]
    if flame_tex:
        nodes.append(N('Flames', tex=flame_tex, infinite=True, interval=1, life=(12, 18), bind=ON_CREATE,
                       at_rand=((-0.18, 0.18), (-0.12, 0.18), (-0.18, 0.18)),
                       vel=((-0.006, 0.006), (0.006, 0.02), (-0.006, 0.006)), rot_rand=(0, 0, (-30, 30)),
                       grow=((core_size * 0.9, core_size * 1.2), core_size * 0.35, 0, 10),
                       color=a(mid, 150), color_to=a(dark, 0), ease=(0, 10)))
    if embers:
        nodes.append(N('Embers', tex='spark', infinite=True, interval=2, life=(18, 30), bind=ON_CREATE,
                       at_rand=((-0.15, 0.15), (-0.15, 0.15), (-0.15, 0.15)),
                       vel=((-0.018, 0.018), (-0.01, 0.02), (-0.018, 0.018)), gravity=(0, -0.0012, 0),
                       grow=((0.12, 0.22), 0.02, 0, 0), color=a(core, 240), fade_out=(10, 0, -20)))
    if smoke_rgb:
        nodes.append(smoke('Smoke', smoke_rgb, infinite=True, life=(26, 36), interval=3, bind=ON_CREATE,
                           at_rand=((-0.1, 0.1), (-0.1, 0.1), (-0.1, 0.1)), vel=(0, (0.006, 0.012), 0),
                           grow=(core_size * 0.9, core_size * 2.4), alpha=90))
    return nodes


def magic_bolt_travel():
    core, glow = BOLT
    return project(_travel(core, glow, glow, core_size=0.7, trail_size=0.8), 120, loop=True)


def magic_bolt_impact():
    core, glow = BOLT
    return project([
        flash('Flash', core, 2.8),
        shockwave('Wave', glow, 1.5, flat=False, life=16, alpha=220, width=0.25),
        sparks('Sparks', core, count=20, speed=0.08, life=(14, 22), gravity=-0.002),
        N('Motes', tex='glow', count=10, life=(24, 36), emit=emit_sphere((0.1, 0.4)), vel=(0, 0.012, 0),
          grow=((0.2, 0.35), 0.05, 0, 0), color=a(glow, 200), fade_out=(16, 0, -20)),
    ], 50)


def fire_ball_travel():
    core, mid, dark = FIRE
    return project(_travel(core, mid, dark, core_size=0.75, trail_size=1.3, flame_tex='flame',
                           smoke_rgb=SMOKE_DARK), 120, loop=True)


def fire_ball_impact():
    core, mid, dark = FIRE
    return project([
        flash('Flash', core, 5.0, life=12),
        N('Fireball', tex='glow', life=30, grow=(1.6, 4.2, 20, 0), color=a(mid, 230), color_to=a(dark, 0)),
        flames('Flames', (255, 190, 90), dark, count=14, life=(20, 30), emit=emit_sphere((0.2, 0.5), upper=True),
               vel=(0, (0.05, 0.09), 0), grow=((1.2, 1.6), 2.4), alpha=190),
        shockwave('GroundWave', mid, 3.0, life=22),
        sparks('Embers', core, count=30, speed=0.11, life=(24, 40), gravity=-0.005, upper=True),
        smoke('Smoke', SMOKE_DARK, count=8, life=(50, 70), delay=6, emit=emit_sphere((0.3, 0.8), upper=True),
              vel=(0, (0.012, 0.02), 0), grow=(1.5, 3.5), alpha=150),
    ], 90)


def violet_flame_travel():
    core, mid, dark = VIOLET
    nodes = _travel(core, mid, dark, core_size=0.7, trail_size=1.2, flame_tex='flame')
    nodes.append(N('Wisps', tex='smoke', blend=ADD, infinite=True, interval=2, life=(20, 28), bind=ON_CREATE,
                   at_rand=((-0.15, 0.15), (-0.15, 0.15), (-0.15, 0.15)), vel=(0, (0.004, 0.012), 0),
                   rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-3, 3)), grow=(0.8, 1.8, 0, 0),
                   color=a(dark, 120), color_to=a(dark, 0)))
    return project(nodes, 120, loop=True)


def violet_flame_impact():
    core, mid, dark = VIOLET
    return project([
        flash('Flash', core, 4.2, life=12),
        flames('Pillar', (215, 150, 255), dark, count=18, life=(26, 38), interval=1, emit=emit_circle((0.1, 0.6)),
               vel=(0, (0.05, 0.09), 0), grow=((1.0, 1.4), 0.5), billboard=YAXIS, alpha=190),
        shockwave('Wave', mid, 2.6, flat=False, life=18, alpha=210, width=0.2),
        shockwave('GroundWave', mid, 2.8, life=24),
        sparks('Sparks', core, count=24, speed=0.1, life=(20, 34), gravity=-0.001),
        N('Wisps', tex='smoke', blend=ADD, count=8, life=(40, 60), emit=emit_sphere((0.2, 0.6), upper=True),
          vel=(0, (0.01, 0.02), 0), rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-3, 3)), grow=(1.0, 2.6, 0, 0),
          color=a(dark, 130), color_to=a(dark, 0)),
    ], 80)


def wind_cutter_travel():
    core, wind = WIND
    return project([
        N('Blade', tex='crescent', rot=(-90, 0, 0), billboard=FIXED, life=LIVE_LONG, size=2.6, color=a(core, 235)),
        N('BladeGlow', tex='crescent', rot=(-90, 0, 0), billboard=FIXED, life=LIVE_LONG, size=3.4,
          color=a(wind, 120)),
        N('Core', tex='glow', life=LIVE_LONG, size=1.2, color=a(wind, 140)),
        N('Wake', tex='glow', infinite=True, interval=1, life=10, bind=ON_CREATE,
          at_rand=((-1.0, 1.0), (-0.05, 0.05), (-0.1, 0.1)), grow=((0.25, 0.45), 0.05, 0, 10),
          color=a(wind, 170), fade_out=(8, 0, -10)),
        N('Motes', tex='spark', infinite=True, interval=2, life=(14, 22), bind=ON_CREATE,
          at_rand=((-1.1, 1.1), (-0.1, 0.1), (-0.1, 0.1)), vel=((-0.01, 0.01), (-0.004, 0.012), (-0.01, 0.01)),
          grow=((0.12, 0.2), 0.02, 0, 0), color=a(core, 230), fade_out=(8, 0, -20)),
    ], 120, loop=True)


def wind_cutter_impact():
    core, wind = WIND
    return project([
        N('SlashA', tex='crescent', life=12, rot=(0, 0, 45), grow=(1.6, 3.2, 20, 0), color=a(core, 240),
          fade_out=(8, 0, -20)),
        N('SlashB', tex='crescent', life=12, delay=2, rot=(0, 0, -135), grow=(1.6, 3.2, 20, 0), color=a(wind, 220),
          fade_out=(8, 0, -20)),
        flash('Flash', core, 2.4),
        shockwave('Wave', wind, 1.8, flat=False, life=16, alpha=200, width=0.2),
        sparks('Gust', core, count=22, speed=0.12, life=(12, 20), gravity=0.0, size=(0.1, 0.18)),
        N('Leaves', tex='glow', count=10, life=(24, 36), emit=emit_sphere((0.2, 0.6)), vel=(0, (0.02, 0.04), 0),
          acc=(0, -0.001, 0), grow=((0.18, 0.3), 0.05, 0, 0), color=a(wind, 200), fade_out=(12, 0, -20)),
    ], 50)


# ================================================================ area / placement
def explosion_blast_sigil():
    core, mid, dark = BLAST
    r = 3.5
    return project([
        rune('Rune', mid, r, life=LIVE_LONG, spin=2.4, alpha=230),
        rune('RuneInner', core, r * 0.5, life=LIVE_LONG, spin=-4.0, alpha=200),
        ground_glow('Glow', mid, r * 2.1, life=LIVE_LONG, alpha=90),
        N('Closing', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.06, 0), infinite=True, interval=18,
          life=18, grow=(1.3, 0.25, 0, 10),
          ring={'outer': (r, 0), 'inner': (r * 0.92, 0), 'outer_color': a(core, 0), 'center_color': a(core, 230),
                'inner_color': a(core, 0)}, fade_in=4),
        N('Embers', tex='spark', infinite=True, interval=1, life=(18, 28), emit=emit_circle((0.3, r)),
          vel=(0, (0.02, 0.045), 0), grow=((0.14, 0.26), 0.03, 0, 0), color=a(core, 230), fade_out=(8, 0, -20)),
    ], 90, loop=True)


def explosion_blast_burst():
    core, mid, dark = BLAST
    return project([
        flash('Flash', core, 10.0, life=14),
        N('Core', tex='glow', at=(0, 1.2, 0), life=32, grow=(3.0, 7.5, 20, 0), color=a(mid, 235), color_to=a(dark, 0)),
        flames('Flames', (255, 180, 80), dark, count=24, life=(26, 38), at=(0, 0.6, 0),
               emit=emit_sphere((0.3, 1.0), upper=True), vel=(0, (0.1, 0.16), 0), grow=((1.6, 2.2), 3.4), alpha=185),
        shockwave('GroundWave', mid, 5.5, life=28, at=(0, 0.08, 0)),
        shockwave('AirWave', core, 3.8, flat=False, at=(0, 1.2, 0), life=18, alpha=180, width=0.2),
        sparks('Embers', core, count=40, speed=0.2, at=(0, 0.6, 0), life=(28, 46), gravity=-0.006, upper=True),
        smoke('Smoke', SMOKE_DARK, count=12, life=(70, 100), delay=8, at=(0, 0.8, 0),
              emit=emit_sphere((0.5, 1.5), upper=True), vel=(0, (0.015, 0.03), 0), grow=(2.0, 5.0), alpha=160),
        ground_glow('Scorch', (60, 30, 20), 6.0, life=110, delay=4, alpha=170, tex='crack', blend=BLEND,
                    fade_out=60),
    ], 130)


def quake_blast_sigil():
    hot, sand, dust = EARTH
    r = 4.25
    return project([
        ground_glow('Crack', hot, r * 2.2, life=LIVE_LONG, alpha=235, tex='crack'),
        rune('Rune', sand, r, life=LIVE_LONG, spin=0.8, alpha=180),
        smoke('Dust', dust, infinite=True, life=(20, 30), interval=2, emit=emit_circle((0.2, r)),
              vel=(0, (0.008, 0.02), 0), grow=(0.6, 1.4), alpha=120),
        N('Pebbles', tex='rock', blend=BLEND, infinite=True, interval=3, life=(20, 26), emit=emit_circle((0.2, r)),
          vel=(0, (0.04, 0.07), 0), gravity=(0, -0.005, 0), rot_rand=(0, 0, (-180, 180)),
          size=((0.1, 0.2), (0.1, 0.2), 1), color=a(STONE, 255)),
    ], 90, loop=True)


def _spikes(name, *, count, radius, height, width, delay, life, rise=7):
    """Each parent instance is one spike (random spot and size); its children push it out of the ground in `rise`
    frames and then keep it standing, since an easing would otherwise stretch over the spike's whole life."""
    hot, sand, dust = EARTH
    return G(name, [
        N(name + 'Rise', tex='spike', blend=BLEND, billboard=YAXIS, life=rise,
          move=((0, -0.55, 0), (0, 0.45, 0), 20, 0), color=a(sand, 255)),
        N(name + 'Stand', tex='spike', blend=BLEND, billboard=YAXIS, delay=rise, life=(life[0] - rise, life[1] - rise), at=(0, 0.45, 0),
          color=a(sand, 255), color_to=a(dust, 225), fade_out=(14, 0, -20)),
    ], count=count, delay=delay, life=life, emit=emit_circle(radius, effects_rotation=False), size=(width, height, 1))


def quake_blast_burst():
    hot, sand, dust = EARTH
    return project([
        flash('Flash', hot, 5.0, at=(0, 0.3, 0), life=12, alpha=200),
        _spikes('Spikes', count=9, radius=(1.4, 3.8), height=(2.6, 3.6), width=(1.1, 1.5), delay=0, life=(70, 85)),
        _spikes('SpikesInner', count=5, radius=(0.2, 1.3), height=(3.2, 4.2), width=(1.2, 1.6), delay=3, life=(66, 80)),
        N('SpikeGlow', tex='glow', count=14, life=(20, 30), emit=emit_circle((0.2, 3.8)), at=(0, 0.2, 0),
          grow=((1.2, 1.8), 0.3, 0, 10), color=a(hot, 190)),
        shockwave('GroundWave', hot, 5.5, life=26, at=(0, 0.08, 0)),
        smoke('DustRing', dust, count=20, life=(40, 60), at=(0, 0.3, 0), emit=emit_circle((0.5, 1.0)),
              vel=((0.08, 0.13), (0.004, 0.012), 0), grow=(1.0, 3.2), alpha=170),
        rocks('Debris', count=22, life=(40, 55), at=(0, 0.3, 0), emit=emit_sphere((0.3, 1.5), upper=True),
              vel=(0, (0.1, 0.16), 0), gravity=-0.008),
        ground_glow('CrackGlow', hot, 9.0, life=60, alpha=200, tex='crack', fade_out=40),
    ], 110)


def rock_wall_rise():
    hot, sand, dust = EARTH
    return project([
        smoke('Dust', STONE, count=26, life=(40, 70), at_rand=((-3.0, 3.0), (0.1, 0.4), (-0.5, 0.5)),
              vel=((-0.01, 0.01), (0.02, 0.04), (-0.03, 0.03)), grow=(1.0, 3.2), alpha=170),
        rocks('Debris', count=18, life=(35, 50), at_rand=((-2.8, 2.8), (0.5, 2.0), (-0.3, 0.3)),
              vel=((-0.02, 0.02), (0.05, 0.1), (-0.04, 0.04)), size=(0.2, 0.42), gravity=-0.006),
        N('CrackLine', tex='glow', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.04, 0), life=40,
          grow_xyz=((2.0, 0.5, 1), (7.0, 0.9, 1), 20, 0), color=a(hot, 180), fade_out=(24, 0, -20)),
        N('Sparks', tex='spark', count=16, life=(16, 26), at_rand=((-3.0, 3.0), (0.05, 0.3), (-0.3, 0.3)),
          vel=((-0.01, 0.01), (0.04, 0.08), (-0.02, 0.02)), gravity=(0, -0.004, 0),
          grow=((0.15, 0.25), 0.03, 0, 0), color=a(hot, 230)),
    ], 90)


def rock_wall_crumble():
    return project([
        smoke('Dust', STONE, count=24, life=(50, 80), at_rand=((-3.0, 3.0), (0.3, 2.6), (-0.6, 0.6)),
              vel=((-0.012, 0.012), (-0.004, 0.012), (-0.02, 0.02)), grow=(1.2, 3.4), alpha=160),
        rocks('Chunks', count=20, life=(35, 50), at_rand=((-2.6, 2.6), (0.8, 3.0), (-0.4, 0.4)),
              vel=((-0.02, 0.02), (0.0, 0.03), (-0.03, 0.03)), size=(0.25, 0.5), gravity=-0.007),
    ], 100)


def thunder_trap_circle():
    core, mid, hot = THUNDER
    r = 2.5
    return project([
        rune('Rune', mid, r, life=LIVE_LONG, spin=1.0, alpha=255, fade_in=12),
        N('Pulse', tex='glow', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.05, 0), infinite=True, interval=50, life=50,
          size=r * 1.9, color=a(mid, 110), fade_in=(25, 0, 0), fade_out=(25, 0, 0)),
        N('Crackle', tex='spark', infinite=True, interval=3, life=(5, 9), emit=emit_circle(r * 0.96),
          at=(0, 0.1, 0), grow=((0.5, 0.9), 0.15, 0, 0), color=a(core, 255)),
        N('Arcs', tex='lightning', infinite=True, interval=7, life=(4, 7), emit=emit_circle((r * 0.5, r * 0.95)),
          at=(0, 0.45, 0), billboard=YAXIS, size=(0.45, 0.9, 1), color=a(core, 240),
          uv_anim=flipbook(4, 128, 512, 2)),
    ], 120, loop=True)


def thunder_trap_strike():
    core, mid, hot = THUNDER
    bolt = dict(tex='lightning', billboard=YAXIS, life=9, color=a(core, 255), uv_anim=flipbook(4, 128, 512, 2),
                fade_out=(4, 0, -20))
    return project([
        N('BoltA', at=(0, 8, 0), size=(2.2, 16, 1), **bolt),
        N('BoltB', at=(0.5, 8, -0.3), size=(1.6, 16, 1), delay=4, **bolt),
        N('BoltC', at=(-0.4, 8, 0.4), size=(1.8, 16, 1), delay=9, **bolt),
        flash('Flash', core, 7.0, at=(0, 1.0, 0), life=12),
        flash('Flash2', core, 5.0, at=(0, 1.0, 0), delay=9, life=10, alpha=200),
        shockwave('GroundWave', mid, 3.5, life=20, at=(0, 0.08, 0)),
        sparks('Sparks', core, count=32, speed=0.15, at=(0, 0.3, 0), life=(16, 28), gravity=-0.006, upper=True),
        N('Static', tex='glow', count=16, delay=4, life=(30, 50), emit=emit_sphere((0.5, 2.5), upper=True),
          at=(0, 0.3, 0), vel=(0, 0.004, 0), grow=((0.15, 0.3), 0.05, 0, 0), color=a(mid, 230), fade_in=4,
          fade_out=(20, 0, -20)),
        ground_glow('Scorch', (40, 30, 20), 4.5, life=90, alpha=150, tex='crack', blend=BLEND, fade_out=50),
    ], 90)


def heal_bloom():
    core, green = HEAL
    return project([
        rune('Rune', green, 2.0, life=120, spin=2.0, alpha=220, fade_in=10, fade_out=40),
        N('Reach', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.06, 0), life=40, grow=(0.15, 1.0, 20, 0),
          ring={'outer': (7.5, 0), 'inner': (7.1, 0), 'outer_color': a(green, 0), 'center_color': a(green, 150),
                'inner_color': a(green, 0)}, fade_out=(20, 0, -20)),
        N('Column', tex='glow', billboard=YAXIS, at=(0, 3.0, 0), life=80, size=(2.4, 6.5, 1), color=a(core, 110),
          fade_in=12, fade_out=(46, 0, -20)),
        N('Motes', tex='glow', count=60, interval=1, life=(50, 80), emit=emit_circle((0.3, 2.0)),
          vel=(0, (0.025, 0.055), 0), grow=((0.22, 0.38), 0.08, 0, 0), color=a(green, 235), fade_in=6,
          fade_out=(24, 0, -20)),
        N('Crosses', tex='cross', count=8, interval=6, life=50, at_rand=((-1.2, 1.2), (0.4, 1.8), (-1.2, 1.2)),
          vel=(0, 0.018, 0), size=(0.5, 0.7), color=a(core, 235), fade_in=8,
          fade_out=(20, 0, -20)),
        N('Twinkle', tex='spark', count=20, interval=3, life=(20, 36), at=(0, 1.1, 0), emit=emit_sphere((0.5, 1.4)),
          grow=((0.3, 0.55), 0.05, 0, 20), spin=(0, 0, 4), color=a(core, 255)),
    ], 150)


def might_surge():
    core, gold, orange = MIGHT
    return project([
        flash('Flash', core, 3.4, at=(0, 1.3, 0), life=12),
        shockwave('GroundWave', gold, 4.0, life=24, at=(0, 0.08, 0)),
        N('Reach', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.06, 0), life=36, grow=(0.15, 1.0, 20, 0),
          ring={'outer': (7.5, 0), 'inner': (7.1, 0), 'outer_color': a(gold, 0), 'center_color': a(gold, 140),
                'inner_color': a(gold, 0)}, fade_out=(18, 0, -20)),
        flames('Aura', core, orange, count=50, interval=1, life=(24, 36), at=(0, 0.3, 0), emit=emit_circle((0.5, 0.8)),
               vel=(0, (0.04, 0.07), 0), grow=((0.9, 1.3), 0.4), billboard=YAXIS, alpha=200),
        N('Chevrons', tex='chevron', count=6, interval=7, life=36, at=(0, 0.3, 0), vel=(0, 0.06, 0), size=1.3,
          color=a(gold, 255), fade_in=4, fade_out=(16, 0, -20)),
        sparks('Sparks', core, count=24, speed=0.1, at=(0, 1.0, 0), life=(20, 32), gravity=-0.003),
        N('Column', tex='glow', billboard=YAXIS, at=(0, 2.4, 0), life=50, size=(2.0, 5.0, 1), color=a(gold, 110),
          fade_in=8, fade_out=(30, 0, -20)),
    ], 120)


# ================================================================ channels
BEAM_LENGTH = 20.0
BEAM_FRAMES = 111      # ChannelSpellEffect duration_secs_ 1.85


def _tube(name, rgb, radius, alpha, life):
    return N(name, kind='ring', billboard=FIXED, at=(0, 0, BEAM_LENGTH / 2), life=life, size=(1.0, 1.0, BEAM_LENGTH),
             ring={'vertices': 24, 'outer': (radius, 0.5), 'inner': (radius, -0.5), 'center_ratio': 0.5,
                   'outer_color': a(rgb, alpha), 'center_color': a(rgb, alpha), 'inner_color': a(rgb, alpha)},
             fade_in=4, fade_out=(14, 0, -20))


def arcane_ray_beam():
    core, pink, purple = RAY
    life = BEAM_FRAMES
    return project([
        _tube('CoreTube', core, 0.1, 255, life),
        _tube('InnerTube', pink, 0.28, 170, life),
        _tube('OuterTube', purple, 0.6, 60, life),
        N('Haze', tex='glow', count=40, life=life, at_rand=(0, 0, (0.5, BEAM_LENGTH)), size=(1.0, 1.6),
          color=a(pink, 100), fade_in=6, fade_out=(14, 0, -20)),
        N('Pulses', tex='glow_core', count=life // 3, interval=3, life=28, vel=(0, 0, 0.7),
          size=(0.6, 0.9), color=a(core, 230), fade_in=2),
        N('Sparks', tex='spark', count=life, interval=1, life=(30, 40), at_rand=((-0.1, 0.1), (-0.1, 0.1), 0),
          vel=((-0.02, 0.02), (-0.02, 0.02), (0.25, 0.4)), grow=((0.15, 0.3), 0.03, 0, 0), color=a(pink, 240),
          fade_out=(10, 0, -20)),
        N('Muzzle', tex='glow_core', life=life, size=1.7, color=a(core, 255), fade_in=3, fade_out=(12, 0, -20)),
        N('MuzzleStar', tex='spark', life=life, size=2.2, spin=(0, 0, 9), color=a(pink, 220), fade_in=3,
          fade_out=(12, 0, -20)),
    ], 150)


def arcane_ray_hit():
    core, pink, purple = RAY
    return project([
        flash('Flash', core, 1.8, life=8),
        shockwave('Wave', pink, 0.9, flat=False, life=12, alpha=200, width=0.3),
        sparks('Sparks', pink, count=10, speed=0.07, life=(10, 18), gravity=0.0),
    ], 30)


BREATH_FRAMES = 84     # ChannelSpellEffect duration_secs_ 1.4


def frost_breath_cone():
    core, ice, deep = FROST
    n = BREATH_FRAMES
    return project([
        smoke('Mist', (205, 232, 255), count=n, interval=1, life=(36, 48), at_rand=((-0.1, 0.1), (-0.1, 0.1), 0),
              vel=((-0.035, 0.035), (-0.03, 0.02), (0.1, 0.15)), grow=(0.4, 2.6), alpha=120),
        N('Glow', tex='glow', count=n, interval=1, life=28, vel=((-0.02, 0.02), (-0.02, 0.015), 0.16),
          grow=(0.3, 1.4, 0, 0), color=a(ice, 90), color_to=a(deep, 0)),
        N('Shards', tex='shard', count=n // 2, interval=2, life=(20, 30), at_rand=((-0.08, 0.08), (-0.08, 0.08), 0),
          vel=((-0.05, 0.05), (-0.04, 0.03), (0.18, 0.26)), gravity=(0, -0.002, 0), rot_rand=(0, 0, (-180, 180)),
          spin=(0, 0, (-10, 10)), grow_xyz=((0.14, 0.28, 1), (0.05, 0.1, 1)), color=a(core, 235)),
        N('Flakes', tex='snowflake', count=n // 3, interval=3, life=(40, 55), at_rand=((-0.1, 0.1), (-0.1, 0.1), 0),
          vel=((-0.05, 0.05), (-0.03, 0.03), (0.08, 0.12)), gravity=(0, -0.0008, 0), spin=(0, 0, (-4, 4)),
          grow=((0.2, 0.35), 0.1, 0, 0), color=a(core, 230), fade_out=(12, 0, -20)),
        N('HandGlow', tex='glow_core', life=n, size=1.1, color=a(ice, 230), fade_in=4, fade_out=(10, 0, -20)),
    ], 140)


def frost_breath_hit():
    core, ice, deep = FROST
    return project([
        flash('Flash', ice, 1.6, life=8),
        N('Shards', tex='shard', count=10, life=(14, 22), emit=emit_sphere((0.05, 0.2)), vel=(0, (0.05, 0.09), 0),
          gravity=(0, -0.004, 0), grow_xyz=((0.14, 0.28, 1), (0.05, 0.1, 1)), color=a(core, 240)),
        smoke('Puff', (215, 238, 255), count=4, life=(24, 32), emit=emit_sphere((0.1, 0.3)), vel=(0, 0.01, 0),
              grow=(0.6, 1.4), alpha=120),
    ], 40)


# ================================================================ cast effects (choreographed to the clips)
def cast_one_hand_thrust():
    m = 'OneHandThrust'
    core, glow = BOLT
    r13, r20, r24, r27 = (-0.67, 1.24, -0.54), (-0.76, 1.47, -0.16), (-0.74, 1.62, 0.48), (-0.28, 1.44, 1.15)
    seg = lambda n, p0, p1, f0, f1: trail_segment(n, p0, p1, ef(m, f0), ef(m, f1), [
        N(n + 'Glow', tex='glow', life=max(1, ef(m, f1) - ef(m, f0)), size=0.35, color=a(glow, 200)),
        trail_glow(n + 'Trail', glow, ef(m, f1) - ef(m, f0), size=(0.18, 0.3), life=10)])
    fire = ef(m, 27)
    return project([
        seg('Wind', r13, r20, 13, 20), seg('Draw', r20, r24, 20, 24), seg('Thrust', r24, r27, 24, 27),
        flash('Flash', core, 1.5, at=r27, delay=fire, life=9),
        shockwave('Ring', glow, 0.6, flat=False, at=r27, delay=fire, life=10, alpha=200, width=0.3),
        N('Sparks', tex='spark', count=10, delay=fire, life=(10, 16), at=r27, emit=emit_sphere((0.0, 0.1)),
          vel=(0, (0.03, 0.05), 0), acc=(0, -0.002, 0), grow=((0.1, 0.18), 0.02, 0, 0), color=a(core, 255)),
    ], fire + 24)


def cast_one_hand_sweep():
    m = 'OneHandSweep'
    core, wind = WIND
    r12, r16, r18, r21 = (-0.47, 1.76, -0.12), (-0.87, 1.31, 0.07), (-0.77, 1.29, 0.72), (-0.10, 1.35, 1.00)
    r24, r28 = (0.42, 1.38, 0.77), (0.65, 1.38, 0.24)

    def seg(n, p0, p1, f0, f1):
        frames = ef(m, f1) - ef(m, f0)
        return trail_segment(n, p0, p1, ef(m, f0), ef(m, f1), [
            trail_glow(n + 'Arc', wind, frames, size=(0.3, 0.45), life=16, alpha=200, drift=(0, 0, 0)),
            trail_glow(n + 'Sparkle', core, frames, size=(0.1, 0.18), life=12, tex='spark', interval=2)])
    fire = ef(m, 21)
    return project([
        G('Gather', [trail_glow('GatherMotes', wind, ef(m, 16) - ef(m, 12), size=(0.12, 0.2), life=12,
                                spread=0.25, drift=(0, 0.01, 0))],
          move=(r12, r16), delay=ef(m, 12), life=ef(m, 16) - ef(m, 12)),
        seg('SweepA', r16, r18, 16, 18), seg('SweepB', r18, r21, 18, 21), seg('SweepC', r21, r24, 21, 24),
        seg('SweepD', r24, r28, 24, 28),
        N('Crescent', tex='crescent', rot=(-90, 0, 0), billboard=FIXED, at=r21, delay=fire, life=10,
          grow=(1.2, 2.2, 20, 0), color=a(core, 230), fade_out=(8, 0, -20)),
        flash('Flash', wind, 1.2, at=r21, delay=fire, life=8, alpha=200),
        N('Swirl', tex='glow', count=14, interval=3, delay=ef(m, 12), life=(24, 34), emit=emit_circle((0.6, 0.9)),
          at=(0, 0.2, 0), vel=((-0.01, 0.01), (0.02, 0.035), 0), grow=((0.1, 0.18), 0.03, 0, 0), color=a(wind, 200),
          fade_out=(10, 0, -20)),
    ], ef(m, 28) + 20)


def cast_one_hand_uppercut():
    m = 'OneHandUppercut'
    hot, sand, dust = EARTH
    r18, r22, r25, r28, r31 = (-0.58, 0.59, -0.39), (-0.79, 0.54, 0.14), (-0.67, 0.83, 0.76), (-0.36, 1.41, 1.08), \
        (-0.29, 2.06, 0.70)

    def seg(n, p0, p1, f0, f1):
        frames = ef(m, f1) - ef(m, f0)
        return trail_segment(n, p0, p1, ef(m, f0), ef(m, f1), [
            N(n + 'Glow', tex='glow', life=max(1, frames), size=0.45, color=a(hot, 200)),
            trail_glow(n + 'Trail', hot, frames, size=(0.2, 0.32), life=14, alpha=200),
            N(n + 'Rocks', tex='rock', blend=BLEND, count=max(1, frames // 2), interval=2, life=(24, 34),
              bind=ON_CREATE, vel=((-0.015, 0.015), (0.03, 0.06), (-0.01, 0.02)), gravity=(0, -0.004, 0),
              rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-8, 8)), size=((0.1, 0.2), (0.1, 0.2), 1),
              color=a(STONE, 255))])
    fire = ef(m, 28)
    scoop = ef(m, 20)
    return project([
        seg('Scoop', r18, r22, 18, 22), seg('RiseA', r22, r25, 22, 25), seg('RiseB', r25, r28, 25, 28),
        seg('RiseC', r28, r31, 28, 31),
        smoke('ScoopDust', dust, count=6, life=(30, 40), at=(-0.7, 0.1, 0.0), delay=scoop,
              emit=emit_circle((0.1, 0.3)), vel=((0.01, 0.03), (0.01, 0.02), 0), grow=(0.5, 1.4), alpha=150),
        ground_glow('ScoopGlow', hot, 1.4, at=(-0.7, 0.04, 0.0), delay=scoop, life=30, alpha=170),
        G('Rift', [
            smoke('RiftDust', dust, count=12, interval=1, life=(30, 40), bind=ON_CREATE, vel=(0, (0.01, 0.025), 0),
                  at_rand=((-0.3, 0.3), 0, (-0.2, 0.2)), grow=(0.6, 1.6), alpha=160),
            trail_glow('RiftGlow', hot, 12, size=(0.5, 0.8), life=20, alpha=150, drift=(0, 0, 0))],
          move=((0, 0.08, 0.8), (0, 0.08, 4.8), 0, 0), delay=fire, life=12),
    ], fire + 60)


def cast_one_hand_raise():
    m = 'OneHandRaise'
    core, mid, hot = THUNDER
    r15, r18, r20, r23 = (-0.74, 0.88, 0.08), (-0.90, 1.46, 0.07), (-0.87, 2.01, 0.04), (-0.61, 2.39, -0.04)
    hand = (-0.56, 2.36, -0.05)

    def seg(n, p0, p1, f0, f1):
        frames = ef(m, f1) - ef(m, f0)
        return trail_segment(n, p0, p1, ef(m, f0), ef(m, f1), [
            trail_glow(n + 'Sparks', core, frames, size=(0.1, 0.2), life=10, tex='spark'),
            trail_glow(n + 'Glow', mid, frames, size=(0.2, 0.3), life=10, alpha=170)])
    fire = ef(m, 23)
    hold = ef(m, 36) - fire
    return project([
        seg('SwingA', r15, r18, 15, 18), seg('SwingB', r18, r20, 18, 20), seg('SwingC', r20, r23, 20, 23),
        N('Bolt', tex='lightning', billboard=YAXIS, at=(hand[0], hand[1] + 3.6, hand[2]), size=(1.3, 7.2, 1),
          delay=fire - 2, life=8, color=a(core, 255), uv_anim=flipbook(4, 128, 512, 2), fade_out=(4, 0, -20)),
        flash('Flash', core, 1.8, at=hand, delay=fire, life=10),
        N('HandGlow', tex='glow', at=hand, delay=fire, life=hold, size=0.7, color=a(mid, 200), fade_out=(8, 0, -20)),
        N('HandArcs', tex='lightning', at=hand, delay=fire, count=hold // 3, interval=3, life=(3, 5),
          rot_rand=(0, 0, (-180, 180)), grow_xyz=((0.3, 0.8, 1), (0.3, 0.8, 1)), color=a(core, 240),
          uv_anim=flipbook(4, 128, 512, 1)),
        N('HandSparks', tex='spark', at=hand, delay=fire, count=hold // 2, interval=2, life=(8, 14),
          emit=emit_sphere((0.05, 0.2)), vel=(0, (0.02, 0.04), 0), gravity=(0, -0.003, 0),
          grow=((0.1, 0.2), 0.02, 0, 0), color=a(core, 255)),
    ], fire + hold + 20)


def cast_two_hand_raise():
    m = 'TwoHandRaise'
    core, gold, orange = MIGHT
    gather = (-0.08, 0.7, 0.5)
    p35, p39, p43 = (-0.09, 0.87, 0.53), (-0.09, 1.83, 0.46), (-0.12, 2.28, 0.11)
    f_gather0, f_gather1, f_rise, fire = ef(m, 17), ef(m, 35), ef(m, 39), ef(m, 43)
    return project([
        converge('Gather', gold, at=gather, radius=1.6, count=f_gather1 - f_gather0, delay=f_gather0, life=24),
        orb('Orb', core, gold, at=gather, delay=ef(m, 21), life=f_gather1 - ef(m, 21), grow=(0.15, 0.6)),
        orb('RisingOrbA', core, gold, move=(p35, p39), delay=f_gather1, life=f_rise - f_gather1, size=0.6),
        orb('RisingOrbB', core, gold, move=(p39, p43), delay=f_rise, life=fire - f_rise + 2, size=0.65),
        ground_glow('Floor', gold, 3.2, delay=f_gather0, life=fire - f_gather0 + 10, alpha=110, fade_in=12),
        rune('FloorRune', gold, 1.3, delay=f_gather0, life=fire - f_gather0 + 10, alpha=170, spin=3.0),
    ], fire + 20)


def cast_two_hand_slam():
    m = 'TwoHandSlam'
    hot, sand, dust = EARTH
    p25, p29, p33, p37, p41 = (0.0, 1.77, -0.58), (-0.17, 2.13, -0.14), (0.0, 2.08, 0.06), (-0.01, 1.47, 0.36), \
        (0.05, 0.30, 0.24)
    slam = ef(m, 41)
    impact = (0.05, 0.05, 0.35)
    return project([
        orb('OrbA', hot, sand, move=(p25, p29), delay=ef(m, 25), life=ef(m, 29) - ef(m, 25), grow=(0.15, 0.4)),
        orb('OrbB', hot, sand, move=(p29, p33), delay=ef(m, 29), life=ef(m, 33) - ef(m, 29), size=0.45),
        orb('OrbC', hot, sand, move=(p33, p37), delay=ef(m, 33), life=ef(m, 37) - ef(m, 33), size=0.45),
        orb('OrbD', hot, sand, move=(p37, p41), delay=ef(m, 37), life=slam - ef(m, 37), size=0.45),
        converge('Gather', hot, at=(0, 2.1, -0.1), radius=1.2, count=10, delay=ef(m, 25), interval=2, life=16),
        flash('Flash', hot, 2.6, at=impact, delay=slam, life=12),
        shockwave('Wave', hot, 3.0, at=(impact[0], 0.08, impact[2]), delay=slam, life=22),
        smoke('DustRing', dust, count=14, life=(36, 50), at=(impact[0], 0.25, impact[2]), delay=slam,
              emit=emit_circle((0.3, 0.6)), vel=((0.06, 0.1), (0.004, 0.01), 0), grow=(0.8, 2.4), alpha=170),
        rocks('Debris', count=12, life=(30, 45), at=impact, delay=slam, emit=emit_sphere((0.1, 0.4), upper=True),
              vel=(0, (0.07, 0.12), 0)),
        ground_glow('Crack', hot, 3.6, at=(impact[0], 0.04, impact[2]), delay=slam, life=60, alpha=210, tex='crack',
                    fade_out=40),
        G('Fissure', [
            smoke('FissureDust', dust, count=18, interval=2, life=(30, 42), bind=ON_CREATE,
                  at_rand=((-0.3, 0.3), 0, (-0.2, 0.2)), vel=(0, (0.012, 0.03), 0), grow=(0.7, 1.8), alpha=160),
            trail_glow('FissureGlow', hot, 36, size=(0.6, 0.9), life=24, alpha=160, drift=(0, 0, 0)),
            N('FissureRocks', tex='rock', blend=BLEND, count=12, interval=3, life=(20, 30), bind=ON_CREATE,
              vel=((-0.02, 0.02), (0.05, 0.08), (-0.01, 0.01)), gravity=(0, -0.006, 0),
              rot_rand=(0, 0, (-180, 180)), size=((0.12, 0.25), (0.12, 0.25), 1), color=a(STONE, 255))],
          move=((0, 0.1, 0.8), (0, 0.1, 11.0), 0, 0), delay=slam, life=36),
    ], slam + 70)


def cast_two_hand_burst():
    m = 'TwoHandBurst'
    core, mid, dark = BLAST
    p25, p45, p52, p54 = (-0.10, 1.35, 0.31), (-0.11, 1.21, 0.54), (-0.02, 1.55, 0.37), (-0.07, 1.67, 0.29)
    f25, f45, f52, fire = ef(m, 25), ef(m, 45), ef(m, 52), ef(m, 54)
    return project([
        converge('Embers', mid, at=(-0.11, 1.25, 0.5), radius=1.3, count=18, delay=f25, interval=2, life=26,
                 tex='spark', size=(0.12, 0.2)),
        orb('Orb', core, mid, move=(p25, p45), delay=f25, life=f45 - f25, grow=(0.12, 0.5)),
        orb('Compress', core, mid, move=(p45, p52), delay=f45, life=fire - f45 + 1, grow=(0.5, 0.32), alpha=255),
        N('HeatRing', kind='ring', billboard=FACE, move=(p25, p45), delay=f25, life=f45 - f25, spin=(0, 0, 5),
          ring={'outer': (0.7, 0), 'inner': (0.6, 0), 'outer_color': a(mid, 0), 'center_color': a(mid, 170),
                'inner_color': a(mid, 0)}, grow=(0.5, 1.0, 20, 0), fade_in=8),
        flash('Burst', core, 3.4, at=p54, delay=fire, life=12),
        shockwave('Wave', mid, 1.6, flat=False, at=p54, delay=fire, life=14, alpha=220, width=0.25),
        sparks('Scatter', core, count=24, speed=0.12, at=p54, delay=fire, life=(16, 26), gravity=-0.003),
        flames('Puffs', core, dark, count=8, life=(16, 24), at=p54, delay=fire, emit=emit_sphere((0.1, 0.3)),
               vel=(0, (0.05, 0.08), 0), grow=(0.8, 1.4)),
    ], fire + 30)


def cast_two_hand_throw():
    m = 'TwoHandThrow'
    core, mid, dark = FIRE
    p21, p29, p32, p36 = (0.15, 1.92, 0.25), (-0.05, 2.01, -0.57), (0.02, 1.87, -0.51), (-0.14, 1.39, 0.68)
    f21, f29, f32, fire = ef(m, 21), ef(m, 29), ef(m, 32), ef(m, 36)

    def licks(n, frames):
        return flames(n, core, dark, count=max(1, frames), interval=1, life=(10, 14), bind=ON_CREATE,
                      at_rand=((-0.12, 0.12), (-0.05, 0.12), (-0.12, 0.12)), vel=(0, (0.015, 0.03), 0),
                      grow=((0.5, 0.7), 0.2))
    return project([
        G('Form', [orb('FormOrb', core, mid, life=f29 - f21, grow=(0.2, 0.7)), licks('FormLicks', f29 - f21)],
          move=(p21, p29), delay=f21, life=f29 - f21),
        G('Hold', [orb('HoldOrb', core, mid, life=f32 - f29, size=0.72), licks('HoldLicks', f32 - f29)],
          move=(p29, p32), delay=f29, life=f32 - f29),
        G('Throw', [orb('ThrowOrb', core, mid, life=fire - f32, size=0.72), licks('ThrowLicks', fire - f32)],
          move=(p32, p36), delay=f32, life=fire - f32),
        flash('Flash', core, 2.0, at=p36, delay=fire, life=9),
        sparks('Embers', core, count=12, speed=0.06, at=p36, delay=fire, life=(14, 22), gravity=-0.004),
    ], fire + 24)


def cast_two_hand_swing_push():
    m = 'TwoHandSwingPush'
    core, mid, dark = VIOLET
    p17, p29, p33, p36, p40 = (-0.55, 1.38, -0.45), (0.0, 1.49, -0.68), (-0.22, 1.26, -0.49), (-0.46, 1.41, 0.0), \
        (-0.28, 1.52, 0.90)
    f17, f29, f33, f36, fire = ef(m, 17), ef(m, 29), ef(m, 33), ef(m, 36), ef(m, 40)

    def seg(n, p0, p1, f0, f1, size):
        frames = f1 - f0
        return trail_segment(n, p0, p1, f0, f1, [
            orb(n + 'Orb', core, mid, life=frames, size=size),
            flames(n + 'Trail', core, dark, count=max(1, frames), interval=1, life=(12, 18), bind=ON_CREATE,
                   at_rand=((-0.1, 0.1), (-0.1, 0.1), (-0.1, 0.1)), vel=(0, (0.01, 0.025), 0), grow=((0.5, 0.8), 0.2))])
    return project([
        G('Gather', [orb('GatherOrb', core, mid, life=f29 - f17, grow=(0.15, 0.55)),
                     converge('GatherWisps', mid, at=(0, 0, 0), radius=0.9, count=(f29 - f17) // 2, interval=2,
                              life=18)],
          move=(p17, p29), delay=f17, life=f29 - f17),
        seg('SwingA', p29, p33, f29, f33, 0.55), seg('SwingB', p33, p36, f33, f36, 0.55),
        seg('SwingC', p36, p40, f36, fire, 0.55),
        flash('Flash', core, 2.2, at=p40, delay=fire, life=10),
        shockwave('Ring', mid, 0.9, flat=False, at=p40, delay=fire, life=12, alpha=200, width=0.3),
    ], fire + 24)


def cast_two_hand_beam():
    m = 'TwoHandBeam'
    core, pink, purple = RAY
    p17, p29, p33 = (-0.30, 1.71, -0.01), (-0.10, 1.66, 0.03), (-0.03, 1.37, 1.09)
    f17, f29, fire, end = ef(m, 17), ef(m, 29), ef(m, 33), ef(m, 101)
    hold = end - fire
    return project([
        orb('Charge', core, pink, move=(p17, p29), delay=f17, life=f29 - f17, grow=(0.15, 0.6)),
        converge('Spiral', pink, at=(-0.2, 1.75, -0.05), radius=1.2, count=(f29 - f17) // 2, delay=f17, interval=2,
                 life=18, tex='spark', size=(0.1, 0.18)),
        orb('Thrust', core, pink, move=(p29, p33), delay=f29, life=fire - f29 + 2, size=0.6),
        rune('FrontRune', pink, 0.75, at=(-0.04, 1.38, 1.35), delay=fire, life=hold, alpha=230, spin=4.0,
             fade_in=6, fade_out=14, vertical=True),
        rune('FarRune', purple, 0.45, at=(-0.04, 1.38, 1.9), delay=fire + 4, life=hold - 4, alpha=200, spin=-6.0,
             fade_in=6, fade_out=14, vertical=True),
    ], end + 20)


def cast_two_hand_push_hold():
    m = 'TwoHandPushHold'
    core, ice, deep = FROST
    p17, p25, p30 = (-0.17, 1.53, 0.25), (-0.11, 1.56, 0.25), (-0.01, 1.22, 1.04)
    f13, f17, f25, fire, end = ef(m, 13), ef(m, 17), ef(m, 25), ef(m, 30), ef(m, 81)
    hold = end - fire
    return project([
        converge('Flakes', core, at=(-0.14, 1.55, 0.25), radius=1.3, count=(f25 - f13) // 2, delay=f13, interval=2,
                 life=20, tex='snowflake', size=(0.14, 0.24)),
        orb('Charge', core, ice, move=(p17, p25), delay=f17, life=f25 - f17, grow=(0.15, 0.5)),
        orb('Push', core, ice, move=(p25, p30), delay=f25, life=fire - f25 + 2, size=0.5),
        rune('FrontRune', ice, 0.7, at=(0.02, 1.2, 1.35), delay=fire, life=hold, alpha=220, spin=-3.0,
             fade_in=6, fade_out=14, vertical=True),
        smoke('ColdMist', (215, 238, 255), count=hold // 3, interval=3, life=(30, 40), at=(0.02, 1.15, 1.2),
              delay=fire, at_rand=((-0.3, 0.3), (-0.2, 0.2), (-0.1, 0.1)), vel=(0, (-0.01, -0.004), 0),
              grow=(0.4, 1.1), alpha=90),
    ], end + 20)


def cast_two_hand_pray():
    m = 'TwoHandPray'
    core, green = HEAL
    touch = (-0.09, 0.03, 0.5)
    p33, p53, p57, p65 = (-0.14, 0.65, 0.48), (-0.15, 1.96, -0.19), (-0.05, 1.79, -0.20), (-0.13, 1.53, 0.72)
    f21, f33, f53, f57, fire, f77 = ef(m, 21), ef(m, 33), ef(m, 53), ef(m, 57), ef(m, 65), ef(m, 77)
    return project([
        ground_glow('Bloom', green, None, at=touch, delay=f21, life=fire - f21, grow=(0.6, 2.8, 20, 0), alpha=150,
                    fade_in=8),
        rune('TouchRune', green, 0.9, at=(touch[0], 0.05, touch[2]), delay=f21, life=fire - f21, alpha=200, spin=2.5),
        N('Sprouts', tex='glow', count=(f33 - f21), interval=1, delay=f21, life=(30, 44), at=touch,
          emit=emit_circle((0.1, 0.9)), vel=(0, (0.02, 0.04), 0), grow=((0.12, 0.2), 0.04, 0, 0), color=a(green, 230),
          fade_out=(12, 0, -20)),
        G('Rise', [trail_glow('RiseMotes', green, f53 - f33, size=(0.18, 0.3), life=30, alpha=200, spread=0.25,
                              drift=(0, 0.006, 0))],
          move=(p33, p53), delay=f33, life=f53 - f33),
        orb('Gather', core, green, move=(p57, p65), delay=f57, life=fire - f57, grow=(0.2, 0.5)),
        orb('Prayer', core, green, at=p65, delay=fire, life=f77 - fire, size=0.55),
        N('PrayerTwinkle', tex='spark', count=(f77 - fire) // 2, interval=2, delay=fire, life=(16, 26), at=p65,
          emit=emit_sphere((0.2, 0.6)), vel=(0, 0.004, 0), grow=((0.15, 0.3), 0.03, 0, 20), color=a(core, 255)),
    ], f77 + 20)


EFFECTS = {
    'MagicBolt_Travel': magic_bolt_travel,
    'MagicBolt_Impact': magic_bolt_impact,
    'FireBall_Travel': fire_ball_travel,
    'FireBall_Impact': fire_ball_impact,
    'VioletFlame_Travel': violet_flame_travel,
    'VioletFlame_Impact': violet_flame_impact,
    'WindCutter_Travel': wind_cutter_travel,
    'WindCutter_Impact': wind_cutter_impact,
    'ExplosionBlast_Sigil': explosion_blast_sigil,
    'ExplosionBlast_Burst': explosion_blast_burst,
    'QuakeBlast_Sigil': quake_blast_sigil,
    'QuakeBlast_Burst': quake_blast_burst,
    'RockWall_Rise': rock_wall_rise,
    'RockWall_Crumble': rock_wall_crumble,
    'ThunderTrap_Circle': thunder_trap_circle,
    'ThunderTrap_Strike': thunder_trap_strike,
    'Heal_Bloom': heal_bloom,
    'Might_Surge': might_surge,
    'ArcaneRay_Beam': arcane_ray_beam,
    'ArcaneRay_Hit': arcane_ray_hit,
    'FrostBreath_Cone': frost_breath_cone,
    'FrostBreath_Hit': frost_breath_hit,
    'Cast_OneHandThrust': cast_one_hand_thrust,
    'Cast_OneHandSweep': cast_one_hand_sweep,
    'Cast_OneHandUppercut': cast_one_hand_uppercut,
    'Cast_OneHandRaise': cast_one_hand_raise,
    'Cast_TwoHandRaise': cast_two_hand_raise,
    'Cast_TwoHandSlam': cast_two_hand_slam,
    'Cast_TwoHandBurst': cast_two_hand_burst,
    'Cast_TwoHandThrow': cast_two_hand_throw,
    'Cast_TwoHandSwingPush': cast_two_hand_swing_push,
    'Cast_TwoHandBeam': cast_two_hand_beam,
    'Cast_TwoHandPushHold': cast_two_hand_push_hold,
    'Cast_TwoHandPray': cast_two_hand_pray,
}


def effect_length_frames(proj) -> int:
    return int(proj.child('EndFrame').text)


# ================================================================ build / install
def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO_ROOT, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--install', action='store_true')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    magic_fx_textures.write_all(build / 'Texture')
    names = args.only or list(EFFECTS)
    for name in names:
        proj = build / f'{name}.efkproj'
        tree = EFFECTS[name]()
        xmlio.write(proj, tree)
        run(['tools.effect', 'validate', str(proj)])
        run(['tools.effect', 'compile', str(proj)])
        line = f'{name:28s} {effect_length_frames(tree):4d}f'
        if args.install:
            out = run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                       '--dest', f'{INSTALL_DIR}/{name}.efkefc'])
            guid = next((ln.split()[-1] for ln in out.splitlines() if 'GUID' in ln), '?')
            line += f'  {guid}'
        print(line, flush=True)


if __name__ == '__main__':
    main()
