"""緑の浮遊石 (docs/Story.md) のエフェクトを組んで、再生用プレハブを書く。

    python tools/art/green_core_effect.py --build-dir <tmp> [--only <Name> ...] [--install]

- Assets/Art/Effect/Story/<Name>.efkefc   (+ _Source/Story/<Name>.efkproj)
- Assets/Prefab/Particle/<Name>.prefab    : scale 8

GreenCoreAura       置いてある石にかけるループ。脈打つ緑の光・地面に広がる光の輪・立ちのぼる光の粒・淡い光の柱
GreenStoneLiftOff   大顎を倒したあと、石が村の跡から抜け出す瞬間。閃光・光の柱・衝撃の輪・飛び散る岩・土煙
GreenStoneFlight    飛んでいる石に付ける。光の玉と、尾を引く光の粒 (尾はワールドに残る)
Light/FireStoneFlight  同じものの光(黄)・火(赤)の石の色。序章で3つの石が散るときに使う
GreenStoneDock      拠点の島の底に石がはまる瞬間。閃光・衝撃の輪・舞い散る光・落ちる土くれ
LightCoreAura / LightStoneLiftOff  砂漠の光の浮遊石 (黄) の色違い
モデルは Assets/Art/Models/IslandHeart/ (GreenCoreShard.mv1 は高さ 17 = 約 2.1 m、クレーターは半径 18 = 約 2.25 m)。
単位は m (tools/art/magic_fx_lib.py と同じ。プレハブが scale 8 で再生する)。

NOTE: ParticleSystem の Loop は前の再生を消さずに流しきってから次を重ねるので、無限エミッタにすると
      周期ごとに粒が倍になる。どのエミッタも CYCLE フレームぶんだけ出して、自然に終わらせる。
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
from tools.art.magic_fx_lib import FIXED, LIVE_LONG, ON_CREATE, YAXIS, N, emit_circle, emit_sphere, project  # noqa: E402
from tools.art.magic_spell_effects import STONE, flash, rocks, shockwave, smoke, sparks  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, LOOP, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Story'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
CYCLE = 360                    # フレーム。プレハブの playingDuration_secs_ と同じ長さ
END_FRAME = CYCLE + 150        # 最後に出た粒が消えきるまで

GREEN = (110, 255, 140)
PALE = (200, 255, 210)
DEEP = (40, 200, 110)
# 序章で3つの浮遊石が散るときの、光(砂漠)と火(岩石)の石の色。docs/Story.md
LIGHT = (255, 215, 90)
LIGHT_PALE = (255, 245, 205)
LIGHT_DEEP = (215, 150, 40)
FIRE = (255, 110, 60)
FIRE_PALE = (255, 215, 180)
CORE_Y = 1.0                   # 結晶の中ほど


def a(rgb, alpha):
    return (*rgb, alpha)


def every(interval):
    return dict(count=CYCLE // interval, interval=interval)


def core_aura(GREEN, PALE, DEEP):
    return project([
        # 結晶を包む光。大きくなりながら薄れるのを重ねて、ゆっくり脈打たせる
        N('Halo', tex='glow', at=(0, CORE_Y, 0), life=120, grow=(2.0, 3.6, 0, 10), color=a(GREEN, 110),
          fade_in=40, fade_out=(60, 0, -20), **every(60)),
        N('Heart', tex='glow_core', at=(0, CORE_Y, 0), life=90, size=1.4, color=a(PALE, 120),
          fade_in=30, fade_out=(40, 0, -20), **every(45)),
        # 地面の光だまりと、脈ごとに外へ広がる輪
        N('GroundGlow', tex='glow', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.05, 0), life=120, size=5.5,
          color=a(DEEP, 90), fade_in=50, fade_out=(60, 0, -20), **every(60)),
        N('Pulse', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.1, 0), life=80, grow=(0.25, 1.0, 10, -10),
          ring={'outer': (3.0, 0), 'inner': (2.6, 0), 'center_ratio': 0.5, 'outer_color': a(GREEN, 0),
                'center_color': a(PALE, 120), 'inner_color': a(GREEN, 0)},
          fade_out=(50, 0, -20), **every(90)),
        # 地割れのあたりから立ちのぼる光の粒
        N('Motes', tex='spark', life=(100, 160), emit=emit_circle((0.3, 2.3)), at=(0, 0.1, 0),
          vel=((-0.004, 0.004), (0.01, 0.022), (-0.004, 0.004)), grow=((0.14, 0.24), 0.04, 0, 0),
          color=a(PALE, 230), color_spread=(30, 0, 30, 0), fade_in=15, fade_out=(40, 0, -20), **every(3)),
        N('Wisps', tex='glow', life=(120, 180), emit=emit_circle((0.2, 1.4)), at=(0, 0.3, 0),
          vel=((-0.002, 0.002), (0.008, 0.014), (-0.002, 0.002)), grow=((0.5, 0.8), 0.1, 0, 0),
          color=a(GREEN, 90), fade_in=30, fade_out=(60, 0, -20), **every(12)),
        # 空へ抜ける淡い光の柱
        N('Column', tex='beam', billboard=YAXIS, at=(0, 2.6, 0), life=150,
          grow_xyz=((0.6, 5.0, 1.0), (1.0, 6.0, 1.0), 0, 0), color=a(GREEN, 55),
          fade_in=60, fade_out=(70, 0, -20), **every(90)),
    ], END_FRAME)


def green_core_aura():
    return core_aura(GREEN, PALE, DEEP)


def light_core_aura():
    return core_aura(LIGHT, LIGHT_PALE, LIGHT_DEEP)


def stone_lift_off(GREEN, PALE):
    return project([
        flash('Flash', PALE, 9.0, at=(0, 1.2, 0), life=18),
        N('Pillar', tex='beam', billboard=YAXIS, at=(0, 6.0, 0), life=50,
          grow_xyz=((2.5, 12.0, 1.0), (0.6, 14.0, 1.0), 0, 10), color=a(GREEN, 200), fade_out=(35, 0, -20)),
        shockwave('Wave1', PALE, 5.0, at=(0, 0.3, 0), life=30, alpha=230),
        shockwave('Wave2', GREEN, 8.0, at=(0, 0.3, 0), delay=6, life=36),
        N('GroundFlare', tex='glow', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.1, 0), life=70,
          grow=(3.0, 8.0, 20, 0), color=a(GREEN, 170), fade_out=(50, 0, -20)),
        rocks('Chunks', count=28, life=(50, 80), at=(0, 0.3, 0), emit=emit_circle((0.3, 2.0)),
              vel=((-0.03, 0.03), (0.12, 0.26), (-0.03, 0.03)), size=(0.25, 0.6), gravity=-0.009),
        sparks('Sparks', PALE, count=60, speed=0.16, at=(0, 1.0, 0), life=(40, 70), size=(0.14, 0.28),
               gravity=-0.002, upper=True, radius=(0.2, 1.2)),
        smoke('Dust', STONE, count=24, life=(90, 140), delay=2, emit=emit_circle((0.8, 2.6)), at=(0, 0.3, 0),
              vel=((-0.01, 0.01), (0.01, 0.03), (-0.01, 0.01)), grow=(2.0, 6.0), alpha=160),
    ], 150)


def green_stone_lift_off():
    return stone_lift_off(GREEN, PALE)


def light_stone_lift_off():
    return stone_lift_off(LIGHT, LIGHT_PALE)


def stone_flight(glow, pale):
    # NOTE: プレハブは Destroy で LIVE_LONG まで流す。止めるのは石ごと GameObject を消すとき
    return project([
        N('Halo', tex='glow', life=LIVE_LONG, size=4.5, color=a(glow, 150)),
        N('Core', tex='glow_core', life=LIVE_LONG, size=2.2, color=a(pale, 200)),
        N('Star', tex='spark', life=LIVE_LONG, size=3.6, spin=(0, 0, 3), color=a(pale, 150)),
        N('Trail', tex='glow', infinite=True, interval=1, life=40, bind=ON_CREATE, grow=(2.6, 0.3, 0, 10),
          color=a(glow, 150), fade_out=(30, 0, -10)),
        N('Motes', tex='spark', infinite=True, interval=2, life=(50, 80), bind=ON_CREATE,
          at_rand=((-0.8, 0.8), (-0.8, 0.8), (-0.8, 0.8)), vel=((-0.006, 0.006), (-0.006, 0.006), (-0.006, 0.006)),
          grow=((0.18, 0.3), 0.04, 0, 0), color=a(pale, 230), fade_out=(30, 0, -20)),
    ], LIVE_LONG)


def green_stone_flight():
    return stone_flight(GREEN, PALE)


def light_stone_flight():
    return stone_flight(LIGHT, LIGHT_PALE)


def fire_stone_flight():
    return stone_flight(FIRE, FIRE_PALE)


def green_stone_dock():
    return project([
        flash('Flash', PALE, 12.0, life=20),
        N('Bloom', tex='glow', life=90, grow=(4.0, 9.0, 20, 0), color=a(GREEN, 170), fade_out=(70, 0, -20)),
        shockwave('Wave1', PALE, 7.0, life=32, alpha=230),
        shockwave('Wave2', GREEN, 11.0, delay=6, life=40),
        sparks('Sparks', PALE, count=70, speed=0.14, life=(50, 90), size=(0.16, 0.3), gravity=-0.0015,
               radius=(0.3, 1.5)),
        N('Motes', tex='spark', count=40, interval=1, life=(90, 140), emit=emit_sphere((1.0, 4.0)),
          vel=((-0.004, 0.004), (-0.012, -0.004), (-0.004, 0.004)), grow=((0.16, 0.26), 0.04, 0, 0),
          color=a(PALE, 230), fade_in=10, fade_out=(40, 0, -20)),
        rocks('Clods', count=24, life=(60, 100), emit=emit_sphere((0.5, 2.5)),
              vel=((-0.03, 0.03), (-0.04, 0.02), (-0.03, 0.03)), size=(0.2, 0.5), gravity=-0.008),
        smoke('Dust', STONE, count=16, life=(80, 120), emit=emit_sphere((1.0, 3.0)),
              vel=((-0.01, 0.01), (-0.02, -0.005), (-0.01, 0.01)), grow=(2.0, 5.0), alpha=130),
    ], 150)


# 名前 -> (組み立て, 再生モード, 再生秒数)
EFFECTS = {
    'GreenCoreAura': (green_core_aura, LOOP, CYCLE / 60.0),
    'GreenStoneLiftOff': (green_stone_lift_off, DESTROY, 150 / 60.0 + 0.1),
    'GreenStoneFlight': (green_stone_flight, DESTROY, LIVE_LONG / 60.0),
    'LightStoneFlight': (light_stone_flight, DESTROY, LIVE_LONG / 60.0),
    'FireStoneFlight': (fire_stone_flight, DESTROY, LIVE_LONG / 60.0),
    'GreenStoneDock': (green_stone_dock, DESTROY, 150 / 60.0 + 0.1),
    # 砂漠の光の浮遊石 (骸竜の傍)。緑と同じ組み立ての色違い
    'LightCoreAura': (light_core_aura, LOOP, CYCLE / 60.0),
    'LightStoneLiftOff': (light_stone_lift_off, DESTROY, 150 / 60.0 + 0.1),
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


def build_prefab(name: str):
    _, mode, secs = EFFECTS[name]
    prefab = new_prefab(name)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # only version 0 archived it
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'{name}.efkefc.meta'))
    comp.data['playingDuration_secs_'] = Num.of_float(round(secs, 2))
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
        build_effect(build, name, args.install)
        if args.install:
            build_prefab(name)


if __name__ == '__main__':
    main()
