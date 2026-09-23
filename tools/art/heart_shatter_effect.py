"""序章の終わり、ドラゴンが島の心臓を砕いたときのエフェクト (HeartShardScatter) を組んで、再生用プレハブを書く。

    python tools/art/heart_shatter_effect.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Story/HeartShardScatter.efkefc   (+ _Source/Story/HeartShardScatter.efkproj)
- Assets/Prefab/Particle/HeartShardScatter.prefab    : scale 8 で一度だけ再生して消える

白い閃光と衝撃の輪のあと、緑・金・赤の3つの光 (草原・砂漠・岩石の核片。docs/Story.md) が
尾を引いて三方の空へ散っていく。単位は m (tools/art/magic_fx_lib.py と同じ。プレハブが scale 8 で再生する)。
"""
from __future__ import annotations

import argparse
import math
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.art import magic_fx_textures  # noqa: E402
from tools.art.magic_fx_lib import FIXED, N, emit_sphere, project  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

NAME = 'HeartShardScatter'
INSTALL_DIR = 'Assets/Art/Effect/Story'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
END_FRAME = 150

CORE = (255, 252, 235)
HEART = (190, 150, 255)        # 砕ける前の浮遊石の色 (紫の台座と揃える)
SHARDS = [                     # (名前, 色, 水平の向き [deg])
    ('Green', (110, 255, 140), 30.0),
    ('Light', (255, 225, 110), 150.0),
    ('Fire', (255, 110, 70), 270.0),
]
SHARD_SPEED = 0.22             # m/frame。上向きに少し足して弧を描かせる
SHARD_RISE = 0.12


def a(rgb, alpha):
    return (*rgb, alpha)


def shard_nodes(name, rgb, heading_deg):
    rad = math.radians(heading_deg)
    vel = (SHARD_SPEED * math.cos(rad), SHARD_RISE, SHARD_SPEED * math.sin(rad))
    return [
        # 核片そのもの
        N(f'{name}Orb', tex='glow_core', delay=8, life=110, vel=vel, size=1.3, color=a(rgb, 255),
          fade_in=4, fade_out=(30, 0, -20)),
        N(f'{name}Halo', tex='glow', delay=8, life=110, vel=vel, size=3.2, color=a(rgb, 150),
          fade_in=4, fade_out=(30, 0, -20)),
        # 尾。同じ速さで少しずつ遅れて出るので、原点から核片までの筋になる
        N(f'{name}Tail', tex='glow', delay=8, count=40, interval=1, life=(20, 30), vel=vel,
          grow=(0.9, 0.2, 0, 0), color=a(rgb, 200), fade_out=(14, 0, -20)),
    ]


def heart_shard_scatter():
    nodes = [
        # 砕ける瞬間の閃光
        N('Flash', tex='glow_core', life=16, grow=(2.0, 7.0, 20, 0), color=a(CORE, 255), fade_out=(12, 0, -20)),
        N('HeartGlow', tex='glow', life=24, grow=(3.0, 5.0, 0, 0), color=a(HEART, 220), fade_out=(18, 0, -20)),
        # 地面に沿って広がる衝撃の輪
        N('Shockwave', kind='ring', rot=(90, 0, 0), billboard=FIXED, life=36, grow=(0.2, 1.0, 20, -10),
          ring={'outer': (9.0, 0), 'inner': (8.0, 0), 'center_ratio': 0.4, 'outer_color': a(HEART, 0),
                'center_color': a(CORE, 230), 'inner_color': a(HEART, 0)}, fade_out=(28, 0, -20)),
        # 砕けた石のかけら。飛び散って落ちる
        N('Debris', tex='spark', count=50, interval=0, life=(40, 70), emit=emit_sphere((0.0, 0.8), upper=True),
          vel=(0, (0.08, 0.2), 0), gravity=(0, -0.006, 0), grow=((0.3, 0.5), 0.05, 0, 20), spin=(0, 0, (-6, 6)),
          color=a(HEART, 255), color_spread=(20, 30, 20, 0), fade_out=(16, 0, -20)),
    ]
    for name, rgb, heading in SHARDS:
        nodes += shard_nodes(name, rgb, heading)
    return project(nodes, END_FRAME)


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, install: bool):
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    proj = build / f'{NAME}.efkproj'
    xmlio.write(proj, heart_shard_scatter())
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
