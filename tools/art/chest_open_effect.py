"""宝箱を開けた時のエフェクト (ChestOpenBurst) を組んで、再生用プレハブを書く。

    python tools/art/chest_open_effect.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Prop/ChestOpenBurst.efkefc   (+ _Source/Prop/ChestOpenBurst.efkproj)
- Assets/Prefab/Particle/ChestOpenBurst.prefab   : scale 8 で一度だけ再生して消える

単位は m (tools/art/magic_fx_lib.py と同じ。プレハブが scale 8 で再生する)。
原点は宝箱の DropPoint (フタの少し上)。箱の口は原点の MOUTH m 下、箱は幅 2 m (TreasureChest.prefab の scale 2)。
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
from tools.art.magic_fx_lib import FIXED, YAXIS, N, emit_circle, emit_sphere, project  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

NAME = 'ChestOpenBurst'
INSTALL_DIR = 'Assets/Art/Effect/Prop'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
END_FRAME = 120
MOUTH = -0.5

CORE = (255, 250, 225)
GOLD = (255, 212, 95)
AMBER = (255, 160, 50)


def a(rgb, alpha):
    return (*rgb, alpha)


def chest_open_burst():
    y = MOUTH
    return project([
        # 開いた瞬間の白い閃光
        N('Flash', tex='glow_core', at=(0, y + 0.3, 0), life=14, grow=(1.1, 3.4, 20, 0), color=a(CORE, 255),
          fade_out=(12, 0, -20)),
        # 箱の中から漏れる光 (上から見える)
        N('InnerGlow', tex='glow', rot=(90, 0, 0), billboard=FIXED, at=(0, y + 0.02, 0), life=80, size=2.6,
          color=a(GOLD, 190), fade_in=4, fade_out=(50, 0, -20)),
        # 立ち上る光の柱 (芯 + 外側のにじみ)
        N('Column', tex='glow', billboard=YAXIS, at=(0, y + 1.6, 0), life=60, size=(0.9, 3.6, 1),
          color=a(CORE, 210), fade_in=4, fade_out=(36, 0, -20)),
        N('ColumnHalo', tex='glow', billboard=YAXIS, at=(0, y + 1.9, 0), life=75, size=(2.4, 4.6, 1),
          color=a(GOLD, 120), fade_in=6, fade_out=(48, 0, -20)),
        # 口の高さで広がる光の輪
        N('Ripple', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, y + 0.05, 0), life=28,
          grow=(0.15, 1.0, 20, -10),
          ring={'outer': (2.2, 0), 'inner': (1.85, 0), 'center_ratio': 0.4, 'outer_color': a(GOLD, 0),
                'center_color': a(GOLD, 220), 'inner_color': a(GOLD, 0)}, fade_out=(22, 0, -20)),
        # 上へ抜ける光の筋
        N('Streaks', tex='streak', billboard=YAXIS, count=16, interval=1, life=(22, 32),
          at_rand=((-0.6, 0.6), y, (-0.4, 0.4)), vel=(0, (0.07, 0.12), 0), size=((0.08, 0.14), (0.9, 1.5), 1),
          color=a(CORE, 230), fade_in=3, fade_out=(16, 0, -20)),
        # 噴き上がって落ちる金の火花
        N('Sparks', tex='spark', count=40, interval=0, life=(40, 60), at=(0, y, 0),
          emit=emit_sphere((0.0, 0.4), upper=True), vel=(0, (0.06, 0.13), 0), gravity=(0, -0.0045, 0),
          grow=((0.16, 0.28), 0.04, 0, 20), spin=(0, 0, (-6, 6)), color=a(GOLD, 255), color_spread=(0, 25, 40, 0),
          fade_out=(14, 0, -20)),
        # ゆっくり昇る光の粒
        N('Motes', tex='glow', count=60, interval=1, life=(50, 85), at=(0, y, 0), emit=emit_circle((0.2, 0.9)),
          vel=(0, (0.015, 0.04), 0), grow=((0.14, 0.26), 0.04, 0, 0), color=a(AMBER, 235),
          color_spread=(0, 40, 40, 0), fade_in=6, fade_out=(28, 0, -20)),
        # まわりで瞬く星
        N('Twinkle', tex='spark', count=16, interval=3, delay=6, life=(18, 30),
          at_rand=((-1.3, 1.3), (y + 0.2, y + 2.6), (-1.0, 1.0)), grow=((0.4, 0.7), 0.05, 0, 20), spin=(0, 0, 4),
          color=a(CORE, 255)),
    ], END_FRAME)


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, install: bool):
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    proj = build / f'{NAME}.efkproj'
    xmlio.write(proj, chest_open_burst())
    run(['tools.effect', 'validate', str(proj)])
    run(['tools.effect', 'compile', str(proj)])
    print(f'compiled {proj.with_suffix(".efkefc")}')
    if install:
        print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{NAME}.efkefc']))


def build_prefab():
    prefab = new_prefab(NAME)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # バージョン 0 だけが書き出していた
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'{NAME}.efkefc.meta'))
    comp.data['playingDuration_secs_'] = Num.of_float(round(END_FRAME / 60.0 + 0.1, 2))
    comp.data['playMode_'] = Num.of_int(DESTROY)
    return save(prefab, PREFAB_DIR, NAME)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true', help='install the effect and write the prefab')
    args = ap.parse_args()
    build_effect(Path(args.build_dir).resolve(), args.install)
    if args.install:
        build_prefab()


if __name__ == '__main__':
    main()
