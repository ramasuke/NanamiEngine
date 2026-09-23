"""草原の村の跡に落ちている緑の浮遊石 (docs/Story.md) にかけるオーラのエフェクト (GreenCoreAura) を組んで、再生用プレハブを書く。

    python tools/art/green_core_effect.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Story/GreenCoreAura.efkefc   (+ _Source/Story/GreenCoreAura.efkproj)
- Assets/Prefab/Particle/GreenCoreAura.prefab    : scale 8 で置きっぱなしにするループ

脈打つ緑の光・地面に広がる光の輪・立ちのぼる光の粒・淡い光の柱。
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
from tools.art.magic_fx_lib import FIXED, YAXIS, N, emit_circle, project  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import LOOP, METRE, save, set_field, set_scale  # noqa: E402

NAME = 'GreenCoreAura'
INSTALL_DIR = 'Assets/Art/Effect/Story'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
CYCLE = 360                    # フレーム。プレハブの playingDuration_secs_ と同じ長さ
END_FRAME = CYCLE + 150        # 最後に出た粒が消えきるまで

GREEN = (110, 255, 140)
PALE = (200, 255, 210)
DEEP = (40, 200, 110)
CORE_Y = 1.0                   # 結晶の中ほど


def a(rgb, alpha):
    return (*rgb, alpha)


def every(interval):
    return dict(count=CYCLE // interval, interval=interval)


def green_core_aura():
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


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, install: bool):
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    proj = build / f'{NAME}.efkproj'
    xmlio.write(proj, green_core_aura())
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
    comp.data.pop('isRoop_', None)     # only version 0 archived it
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'{NAME}.efkefc.meta'))
    comp.data['playingDuration_secs_'] = Num.of_float(CYCLE / 60.0)
    comp.data['playMode_'] = Num.of_int(LOOP)
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
