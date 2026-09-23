"""序章の終わり、ドラゴンが島の中心に爪を突き立てたときのエフェクト (HeartShardScatter) を組んで、再生用プレハブを書く。

    python tools/art/heart_shatter_effect.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Story/HeartShardScatter.efkefc   (+ _Source/Story/HeartShardScatter.efkproj)
- Assets/Prefab/Particle/HeartShardScatter.prefab    : scale 8 で一度だけ再生して消える

白い閃光と衝撃の輪のあと、島の底に埋まっていた緑・金・赤の3つの浮遊石 (草原・砂漠・岩石。docs/Story.md) が
地面から抜け出して浮かび上がり、光の尾を引いて三方の空へ飛んでいく。
石は MAGICALxSPIRAL の rock1.efkmodel (差し渡し約 1.8) を借りる。
単位は m (tools/art/magic_fx_lib.py と同じ。プレハブが scale 8 で再生する)。
"""
from __future__ import annotations

import argparse
import math
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.art import magic_fx_textures  # noqa: E402
from tools.art.magic_fx_lib import FIXED, OPAQUE, N, emit_sphere, project  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

NAME = 'HeartShardScatter'
INSTALL_DIR = 'Assets/Art/Effect/Story'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
STONE_MODEL = REPO / 'Assets/Art/Effect/MAGICALxSPIRAL/Model/rock1.efkmodel'
END_FRAME = 190

CORE = (255, 252, 235)
HEART = (190, 150, 255)        # 島の底の光の色 (紫の台座と揃える)
STONES = [                     # (名前, 光の色, 石の色, 水平の向き [deg])
    ('Green', (110, 255, 140), (150, 235, 165), 30.0),
    ('Light', (255, 225, 110), (245, 225, 150), 150.0),
    ('Fire', (255, 110, 70), (240, 140, 110), 270.0),
]
STONE_SIZE = 0.6
RISE_FROM = 1.0                # 中心からの距離。地面の下から斜めに抜け出す
RISE_TO = 1.8
RISE_DEPTH = -1.2
RISE_HEIGHT = 2.6
RISE_START, RISE_FRAMES = 10, 45
FLY_START = RISE_START + RISE_FRAMES
FLY_LIFE = 130
FLY_SPEED = 0.24               # m/frame。上向きに少し足して弧を描かせる
FLY_RISE = 0.1
RISE_SPIN = 1.5                # deg/frame (Y)


def a(rgb, alpha):
    return (*rgb, alpha)


def stone_texture():
    """浮遊石の肌。ほぼ白の濃淡だけで、色はノードの色で付ける"""
    size = 128
    n = magic_fx_textures._fbm(size, size, 91, octaves=5, base=4)
    vein = magic_fx_textures._fbm(size, size, 131, octaves=3, base=3)
    vein = np.exp(-((vein - 0.5) / 0.03) ** 2)            # 細い光の筋
    shade = np.clip(0.55 + (n - 0.5) * 0.6 + vein * 0.45, 0.2, 1.0)
    rgb = np.repeat(shade[..., None], 3, axis=2)
    return magic_fx_textures._to_image(np.ones_like(shade), rgb)


def stone_nodes(name, glow, tint, heading_deg):
    rad = math.radians(heading_deg)
    dx, dz = math.cos(rad), math.sin(rad)
    below = (RISE_FROM * dx, RISE_DEPTH, RISE_FROM * dz)
    above = (RISE_TO * dx, RISE_HEIGHT, RISE_TO * dz)
    vel = (FLY_SPEED * dx, FLY_RISE, FLY_SPEED * dz)
    spin_at_launch = RISE_SPIN * RISE_FRAMES
    model = 'Model/rock1.efkmodel'
    rise = dict(delay=RISE_START, life=RISE_FRAMES, move=(below, above, 0, -20))
    fly = dict(delay=FLY_START, life=FLY_LIFE, at=above, vel=vel)
    return [
        # 地面から抜け出して浮かぶ。ゆっくり回りながら減速して止まる
        N(f'{name}StoneRise', kind='model', model=model, blend=OPAQUE, tex='stone', size=STONE_SIZE,
          rot_rand=(0, 0, 0), spin=(0, RISE_SPIN, 0), color=a(tint, 255), **rise),
        N(f'{name}HaloRise', tex='glow', size=2.6, color=a(glow, 150), fade_in=(20, 0, 0), **rise),
        # 三方へ飛んでいく。回りながら遠ざかる
        N(f'{name}Stone', kind='model', model=model, blend=OPAQUE, tex='stone', size=STONE_SIZE,
          rot_rand=(0, spin_at_launch, 0), spin=(1.2, 4.0, 0.8), color=a(tint, 255), **fly),
        N(f'{name}Halo', tex='glow', size=3.0, color=a(glow, 170), fade_out=(30, 0, -20), **fly),
        N(f'{name}Core', tex='glow_core', size=1.2, color=a(glow, 220), fade_out=(30, 0, -20), **fly),
        # 尾。同じ速さで少しずつ遅れて出るので、飛び立った所から石までの筋になる
        N(f'{name}Tail', tex='glow', delay=FLY_START, count=60, interval=1, life=(20, 30), at=above, vel=vel,
          grow=(1.1, 0.2, 0, 0), color=a(glow, 200), fade_out=(14, 0, -20)),
        # 抜け出す所からこぼれる土と光の粒
        N(f'{name}Dust', tex='spark', delay=RISE_START, count=24, interval=1, life=(30, 50),
          at_rand=((below[0] - 0.4, below[0] + 0.4), 0.0, (below[2] - 0.4, below[2] + 0.4)),
          vel=((-0.02, 0.02), (0.04, 0.1), (-0.02, 0.02)), gravity=(0, -0.004, 0),
          grow=((0.2, 0.35), 0.05, 0, 20), color=a(glow, 230), fade_out=(14, 0, -20)),
    ]


def heart_shard_scatter():
    nodes = [
        # 爪が突き立った瞬間の閃光
        N('Flash', tex='glow_core', life=16, grow=(2.0, 7.0, 20, 0), color=a(CORE, 255), fade_out=(12, 0, -20)),
        N('HeartGlow', tex='glow', life=RISE_START + RISE_FRAMES, grow=(3.0, 5.0, 0, 0), color=a(HEART, 200),
          fade_out=(30, 0, -20)),
        # 地面に沿って広がる衝撃の輪。石が飛び立つときにもう一度
        N('Shockwave', kind='ring', rot=(90, 0, 0), billboard=FIXED, life=36, grow=(0.2, 1.0, 20, -10),
          ring={'outer': (9.0, 0), 'inner': (8.0, 0), 'center_ratio': 0.4, 'outer_color': a(HEART, 0),
                'center_color': a(CORE, 230), 'inner_color': a(HEART, 0)}, fade_out=(28, 0, -20)),
        N('LaunchWave', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, RISE_HEIGHT, 0), delay=FLY_START,
          life=30, grow=(0.2, 1.0, 20, -10),
          ring={'outer': (6.0, 0), 'inner': (5.4, 0), 'center_ratio': 0.4, 'outer_color': a(CORE, 0),
                'center_color': a(CORE, 200), 'inner_color': a(CORE, 0)}, fade_out=(24, 0, -20)),
        # 割れた地面のかけら。飛び散って落ちる
        N('Debris', tex='spark', count=50, interval=0, life=(40, 70), emit=emit_sphere((0.0, 0.8), upper=True),
          vel=(0, (0.08, 0.2), 0), gravity=(0, -0.006, 0), grow=((0.3, 0.5), 0.05, 0, 20), spin=(0, 0, (-6, 6)),
          color=a(HEART, 255), color_spread=(20, 30, 20, 0), fade_out=(16, 0, -20)),
    ]
    for name, glow, tint, heading in STONES:
        nodes += stone_nodes(name, glow, tint, heading)
    return project(nodes, END_FRAME)


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, install: bool):
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    stone_texture().save(build / 'Texture' / 'stone.png')
    (build / 'Model').mkdir(exist_ok=True)
    shutil.copyfile(STONE_MODEL, build / 'Model' / STONE_MODEL.name)
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
