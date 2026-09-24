"""序章でドラゴンが拠点の島を壊すときのエフェクトを組んで、再生用プレハブを書く。

    python tools/art/island_destruction_effects.py --build-dir <tmp> [--only NAME ...] [--install]

- Assets/Art/Effect/Story/<Name>.efkefc   (+ _Source/Story/<Name>.efkproj)
- Assets/Prefab/Particle/<Name>.prefab    : scale 8 で再生する

IslandFireImpact  火球が島に落ちた瞬間。閃光・火柱・衝撃の輪・飛び散る岩・黒煙・焦げ跡
IslandBurning     着弾点で燃え続ける炎と立ちのぼる黒煙 (ループ。GenerateParticle の lifeTime_ で消す)
IslandCrumble     島の底の岩が剥がれて落ちていく
IslandHeartBreak  島の中心に爪が突き立ったとき。光の柱・広がる地割れ・突き出す岩・噴き上がる瓦礫・土煙

単位は m (tools/art/magic_fx_lib.py と同じ)。島は差し渡し 30 m ほど (world 250)。
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.art import magic_fx_textures  # noqa: E402
from tools.art.magic_fx_lib import ADD, BLEND, FIXED, LIVE_LONG, YAXIS, G, N, emit_circle, emit_sphere, project  # noqa: E402
from tools.art.magic_spell_effects import (  # noqa: E402
    BLAST, EARTH, SMOKE_DARK, STONE, a, flames, flash, ground_glow, rocks, shockwave, smoke, sparks,
)
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, LOOP, LOOP_SECS, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Story'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'

HEART = (190, 150, 255)        # 島の心臓の色 (HeartShardScatter と揃える)
HEART_CORE = (255, 250, 240)
SCORCH = (45, 28, 20)
ASH = (95, 88, 82)


def island_fire_impact():
    core, mid, dark = BLAST
    return project([
        flash('Flash', core, 24.0, at=(0, 3, 0), life=16),
        N('Core', tex='glow', at=(0, 4, 0), life=40, grow=(8.0, 20.0, 20, 0), color=a(mid, 235), color_to=a(dark, 0)),
        flames('Fireball', (255, 190, 90), dark, count=40, life=(40, 60), at=(0, 2, 0),
               emit=emit_sphere((1.0, 3.0), upper=True), vel=(0, (0.15, 0.3), 0), grow=((5.0, 7.0), 9.0), alpha=200),
        shockwave('GroundWave', mid, 18.0, at=(0, 0.3, 0), life=34),
        shockwave('AirWave', core, 12.0, flat=False, at=(0, 4, 0), life=22, alpha=170, width=0.2),
        sparks('Embers', core, count=80, speed=0.5, at=(0, 2, 0), life=(40, 70), size=(0.4, 0.7), gravity=-0.012,
               upper=True, radius=(0.5, 2.0)),
        rocks('Debris', count=45, life=(60, 90), at=(0, 1, 0), emit=emit_sphere((0.5, 3.0), upper=True),
              vel=(0, (0.3, 0.5), 0), size=(0.8, 1.8), gravity=-0.015),
        smoke('SmokeColumn', SMOKE_DARK, count=26, life=(140, 190), delay=6, at=(0, 3, 0),
              emit=emit_sphere((1.0, 4.0), upper=True), vel=(0, (0.04, 0.09), 0), grow=(6.0, 16.0), alpha=170),
        ground_glow('Scorch', SCORCH, 20.0, at=(0, 0.2, 0), life=190, delay=4, alpha=200, tex='crack', blend=BLEND,
                    fade_out=60),
        ground_glow('CrackGlow', mid, 16.0, at=(0, 0.25, 0), life=90, alpha=220, tex='crack', fade_out=60),
    ], 200)


def island_burning():
    core, mid, dark = BLAST
    return project([
        N('Flames', tex='flame', infinite=True, interval=2, life=(30, 45), emit=emit_circle((0.5, 5.0)),
          vel=(0, (0.04, 0.08), 0), rot_rand=(0, 0, (-25, 25)), grow=((3.0, 4.5), 1.2, 0, 10),
          color=a((255, 170, 70), 200), color_to=a(dark, 0), ease=(0, 10), fade_in=3),
        smoke('Smoke', SMOKE_DARK, infinite=True, life=(150, 200), interval=3, at=(0, 2, 0),
              emit=emit_circle((0.5, 4.0)), vel=((0.01, 0.03), (0.06, 0.1), 0), grow=(5.0, 18.0), alpha=150),
        N('Embers', tex='spark', infinite=True, interval=2, life=(40, 70), emit=emit_circle((0.5, 5.0)),
          vel=((-0.02, 0.02), (0.05, 0.12), (-0.02, 0.02)), gravity=(0, -0.0008, 0),
          grow=((0.3, 0.5), 0.05, 0, 0), color=a(core, 230), fade_out=(12, 0, -20)),
        ground_glow('Embed', dark, 12.0, at=(0, 0.3, 0), life=LIVE_LONG, alpha=110),
        ground_glow('Scorch', SCORCH, 16.0, at=(0, 0.2, 0), life=LIVE_LONG, alpha=190, tex='crack', blend=BLEND,
                    fade_in=20, fade_out=20),
    ], 120, loop=True)


def island_crumble():
    spread = ((-10.0, 10.0), (-2.0, 2.0), (-10.0, 10.0))
    return project([
        rocks('Chunks', count=40, life=(100, 140), at_rand=spread, vel=((-0.03, 0.03), (-0.05, 0.02), (-0.03, 0.03)),
              size=(1.2, 3.0), gravity=-0.012, rgb=ASH),
        rocks('Pebbles', count=60, life=(80, 120), at_rand=spread, vel=((-0.04, 0.04), (-0.04, 0.03), (-0.04, 0.04)),
              size=(0.4, 0.9), gravity=-0.012),
        smoke('Dust', STONE, count=30, life=(90, 130), at_rand=spread, vel=((-0.01, 0.01), (-0.08, -0.02), (-0.01, 0.01)),
              grow=(4.0, 12.0), alpha=150),
    ], 220)


def _pillar(name, rgb, *, width, height, life, alpha):
    return N(name, tex='glow', billboard=YAXIS, at=(0, height / 2, 0), life=life,
             grow_xyz=((width * 0.4, height * 0.2, 1), (width, height, 1), 20, 0),
             color=a(rgb, alpha), fade_out=(life - 8, 0, -20))


def _spikes(name, *, count, radius, height, width, delay, life, rise=10):
    """HeartBreak 用に magic_spell_effects._spikes を大きくしたもの。地割れから岩が突き出して残る"""
    hot, sand, dust = EARTH
    return G(name, [
        N(name + 'Rise', tex='spike', blend=BLEND, billboard=YAXIS, life=rise,
          move=((0, -0.55, 0), (0, 0.45, 0), 20, 0), color=a(ASH, 255)),
        N(name + 'Stand', tex='spike', blend=BLEND, billboard=YAXIS, delay=rise,
          life=(life[0] - rise, life[1] - rise), at=(0, 0.45, 0), color=a(ASH, 255), color_to=a(dust, 225),
          fade_out=(30, 0, -20)),
    ], count=count, delay=delay, life=life, emit=emit_circle(radius, effects_rotation=False), size=(width, height, 1))


def island_heart_break():
    hot, sand, dust = EARTH
    return project([
        flash('Flash', HEART_CORE, 30.0, at=(0, 3, 0), life=18),
        _pillar('Pillar', HEART, width=7.0, height=60.0, life=50, alpha=220),
        _pillar('PillarCore', HEART_CORE, width=2.5, height=60.0, life=40, alpha=255),
        # 地割れ。暗い割れ目の上に、心臓の光が漏れる割れ目を重ねる
        N('Fissure', tex='crack', blend=BLEND, rot=(90, 0, 0), billboard=FIXED, at=(0, 0.2, 0), life=230,
          grow=(4.0, 24.0, 30, -20), color=a(SCORCH, 235), fade_out=(60, 0, -20)),
        N('FissureGlow', tex='crack', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.25, 0), life=200,
          grow=(4.0, 22.0, 30, -20), color=a(HEART, 255), color_to=a(hot, 160), fade_out=(80, 0, -20)),
        shockwave('Wave1', HEART_CORE, 25.0, at=(0, 0.5, 0), life=36, alpha=230),
        shockwave('Wave2', HEART, 35.0, at=(0, 0.5, 0), delay=8, life=40),
        shockwave('Wave3', sand, 45.0, at=(0, 0.5, 0), delay=16, life=44, alpha=160),
        _spikes('Spikes', count=10, radius=(5.0, 14.0), height=(6.0, 10.0), width=(3.0, 4.5), delay=6,
                life=(200, 220)),
        rocks('Boulders', count=60, life=(70, 110), at=(0, 0.5, 0), emit=emit_circle((1.0, 12.0)),
              vel=(0, (0.3, 0.6), 0), size=(1.0, 2.6), gravity=-0.016, rgb=ASH),
        sparks('HeartSparks', HEART, count=80, speed=0.45, at=(0, 1, 0), life=(40, 70), size=(0.4, 0.8),
               gravity=-0.01, upper=True, radius=(0.5, 3.0)),
        smoke('DustCloud', STONE, count=40, life=(100, 160), delay=4, at_rand=((-18.0, 18.0), (0.5, 2.0), (-18.0, 18.0)),
              vel=((-0.02, 0.02), (0.02, 0.05), (-0.02, 0.02)), grow=(6.0, 16.0), alpha=170),
    ], 240)


EFFECTS = {
    'IslandFireImpact': (island_fire_impact, DESTROY),
    'IslandBurning': (island_burning, LOOP),
    'IslandCrumble': (island_crumble, DESTROY),
    'IslandHeartBreak': (island_heart_break, DESTROY),
}


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, name: str, install: bool):
    proj = build / f'{name}.efkproj'
    xmlio.write(proj, EFFECTS[name][0]())
    run(['tools.effect', 'validate', str(proj)])
    run(['tools.effect', 'compile', str(proj)])
    print(f'compiled {proj.with_suffix(".efkefc")}')
    if install:
        print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{name}.efkefc']))
    return int(xmlio.read(proj).child('EndFrame').text)


def build_prefab(name: str, end_frame: int):
    prefab = new_prefab(name)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # アーカイブしていたのはバージョン 0 だけ
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'{name}.efkefc.meta'))
    mode = EFFECTS[name][1]
    secs = LOOP_SECS if mode == LOOP else round(end_frame / 60.0 + 0.1, 2)
    comp.data['playingDuration_secs_'] = Num.of_float(secs)
    comp.data['playMode_'] = Num.of_int(mode)
    return save(prefab, PREFAB_DIR, name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--only', nargs='*', choices=sorted(EFFECTS))
    ap.add_argument('--install', action='store_true', help='install the effects and write the prefabs')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    for name in args.only or EFFECTS:
        end_frame = build_effect(build, name, args.install)
        if args.install:
            build_prefab(name, end_frame)


if __name__ == '__main__':
    main()
