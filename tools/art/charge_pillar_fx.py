"""突進で倒せる柱 (GamePlay::Prop::ChargeBreakPillar) がぐらつくときのエフェクトと音を作って、再生用プレハブを書く。

    python tools/art/charge_pillar_fx.py --build-dir <tmp> [--install] [--preview <png>]

- Assets/Art/Effect/Prop/PillarTremble.efkefc   (+ _Source/Prop/PillarTremble.efkproj)
- Assets/Prefab/Particle/PillarTremble.prefab   : scale 8。柱の dustPoint_ (折れ目) に出す
- Assets/Audio/Physics/ChargePillar_Tremble.mp3 : 石のこすれる低い音と、こぼれる小石

PillarTremble  折れ目から小石と砂がこぼれ、柱頭からも砂が落ちる
単位は m (tools/art/magic_fx_lib.py と同じ。at_rand を渡すと at は使われないので、範囲に位置を含める)。柱 (RuinPillar*.mv1) は太さ約 1.2 m、折れ目は地面から 1.75 m、
柱頭は折れ目から約 4.2 m 上。
"""
from __future__ import annotations

import argparse
import math
import subprocess
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.art import magic_fx_textures, magic_sfx  # noqa: E402
from tools.art.magic_fx_lib import project  # noqa: E402
from tools.art.magic_spell_effects import STONE, rocks, smoke  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Prop'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
SOUND_DIR = REPO / 'Assets/Audio/Physics'
NAME = 'PillarTremble'
SOUND = 'ChargePillar_Tremble'
END_FRAME = 110

TOP_Y = 4.1          # 折れ目から柱頭まで
RADIUS = 0.6         # 柱の半径
SAND = (200, 184, 156)


def pillar_tremble():
    return project([
        # 折れ目: 外へこぼれて落ちる小石と、垂れる砂
        rocks('CrackPebbles', count=9, life=(26, 36), at_rand=((-RADIUS, RADIUS), (-0.1, 0.15), (-RADIUS, RADIUS)),
              vel=((-0.012, 0.012), (0.0, 0.015), (-0.012, 0.012)), size=(0.07, 0.15), gravity=-0.006),
        smoke('CrackDust', SAND, count=6, life=(40, 60), at_rand=((-RADIUS, RADIUS), (-0.1, 0.1), (-RADIUS, RADIUS)),
              vel=((-0.004, 0.004), (-0.012, -0.006), (-0.004, 0.004)), grow=(0.35, 1.1), alpha=150),
        # 柱頭: 高いところから落ちる砂と小石
        rocks('TopPebbles', count=5, delay=4, life=(40, 48),
              at_rand=((-RADIUS, RADIUS), (TOP_Y - 0.2, TOP_Y), (-RADIUS, RADIUS)),
              vel=((-0.006, 0.006), (0.0, 0.01), (-0.006, 0.006)), size=(0.06, 0.12), gravity=-0.006),
        smoke('TopDust', SAND, count=5, delay=2, life=(50, 70),
              at_rand=((-RADIUS, RADIUS), (TOP_Y - 0.3, TOP_Y), (-RADIUS, RADIUS)),
              vel=((-0.003, 0.003), (-0.03, -0.018), (-0.003, 0.003)), grow=(0.3, 1.0), alpha=120),
        # 根元: 落ちた小石が上げる薄い土ぼこり
        smoke('BaseDust', STONE, count=5, delay=18, life=(40, 60),
              at_rand=((-1.0, 1.0), (-1.7, -1.5), (-1.0, 1.0)),
              vel=((-0.008, 0.008), (0.002, 0.006), (-0.008, 0.008)), grow=(0.5, 1.6), alpha=110),
    ], END_FRAME)


# ---------------------------------------------------------------- 音
def pillar_tremble_sound(rng):
    s = magic_sfx
    d = 1.3
    # 石同士がこすれる低いうなり
    grind_env = s.curve(d, [(0, 0), (0.06, 1), (0.5, 0.7), (d, 0)])
    grind = s.lp(s.brown(d, rng), 180) * grind_env
    creak = s.bp(s.grains(d, rng, s.poisson_times(d, rng, lambda t: 90 * math.exp(-t / 0.5)), 150, 900,
                          (0.01, 0.04)), 120, 900) * grind_env
    # こぼれて地面に当たる小石
    pebbles = s.grains(d, rng, s.poisson_times(d, rng, lambda t: 5 + 40 * math.exp(-((t - 0.45) / 0.25) ** 2)),
                       300, 2200, (0.004, 0.015), (0.2, 0.8))
    mix = s.norm(grind) * 0.7 + s.norm(creak) * 0.45 + s.norm(pebbles) * 0.35
    return s.finish(s.reverb(mix, rng, 0.7, 0.2), -9, fade_in=0.01, fade_out=0.2)


def write_sound():
    magic_sfx.OUT_DIR = SOUND_DIR
    rng = np.random.default_rng(4242)
    stereo = magic_sfx.trim_tail(pillar_tremble_sound(rng))
    path = magic_sfx.write_sound(SOUND, stereo)
    print(f'wrote {path.relative_to(REPO)}  {stereo.shape[1] / magic_sfx.SR:.2f}s')
    return stereo


# ---------------------------------------------------------------- 組み立て
def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, install: bool):
    proj = build / f'{NAME}.efkproj'
    xmlio.write(proj, pillar_tremble())
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
    ap.add_argument('--install', action='store_true', help='install the effect, write the prefab and the sound')
    ap.add_argument('--preview', help='waveform png of the sound')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    build_effect(build, args.install)
    if args.install:
        build_prefab()
        stereo = write_sound()
        if args.preview:
            magic_sfx.render_preview({SOUND: stereo}, args.preview)


if __name__ == '__main__':
    main()
