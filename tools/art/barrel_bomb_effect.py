"""大樽爆弾の爆発エフェクト (Effekseer .efkproj -> .efkefc) を作り、Assets/Art/Effect/Item に入れる。

    python tools/art/barrel_bomb_effect.py --build-dir DIR [--install]

魔法の ExplosionBlast_Burst と違い、火薬の爆発: 白い閃光 -> 橙の火球と立ち上る炎 -> 樽の木片と鉄片が飛び散り、
黒煙と砂埃が残る。部品とメートル単位は tools/art/magic_spell_effects.py と同じ (プレハブはスケール 8 で再生)。
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art import magic_fx_textures  # noqa: E402
from tools.art.magic_fx_lib import BLEND, N, emit_circle, emit_sphere, project  # noqa: E402
from tools.art.magic_spell_effects import (  # noqa: E402
    a, effect_length_frames, flames, flash, ground_glow, run, shockwave, smoke, sparks,
)
from tools.effect import xmlio  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Item'
NAME = 'BarrelBomb_Explosion'

HOT = (255, 244, 210)
FIRE = (255, 160, 55)
EMBER = (150, 40, 10)
SOOT = (40, 36, 34)
DUST = (150, 125, 95)
WOOD = (125, 82, 45)
IRON = (70, 66, 64)


def debris(name, rgb, *, count, life, speed, size, gravity, tex='shard'):
    return N(name, tex=tex, blend=BLEND, count=count, life=life, at=(0, 0.7, 0),
             emit=emit_sphere((0.2, 0.6), upper=True), vel=(0, speed, 0), gravity=(0, gravity, 0),
             rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-14, 14)), size=(size, size, 1), color=a(rgb, 255),
             fade_out=(12, 0, -20))


def barrel_bomb_explosion():
    return project([
        flash('Flash', HOT, 12.0, at=(0, 0.8, 0), life=10),
        N('Fireball', tex='glow', at=(0, 1.4, 0), life=26, grow=(3.5, 8.0, 30, 0), color=a(FIRE, 245),
          color_to=a(EMBER, 0), ease=(0, 10)),
        flames('Flames', (255, 200, 110), EMBER, count=30, life=(22, 34), at=(0, 0.5, 0),
               emit=emit_sphere((0.3, 1.2), upper=True), vel=(0, (0.12, 0.2), 0), grow=((1.8, 2.6), 3.8),
               alpha=200),
        flames('Column', (255, 190, 90), EMBER, count=10, life=(28, 38), delay=2, at=(0, 1.0, 0),
               emit=emit_circle((0.1, 0.6)), vel=(0, (0.1, 0.16), 0), grow=((2.0, 2.6), 1.2), alpha=170),
        shockwave('GroundWave', (255, 210, 150), 6.5, at=(0, 0.08, 0), life=22, alpha=170, width=0.18),
        shockwave('AirWave', HOT, 4.5, flat=False, at=(0, 1.2, 0), life=14, alpha=130, width=0.15),
        sparks('Sparks', (255, 215, 130), count=50, speed=(0.16, 0.3), at=(0, 0.7, 0), life=(22, 40),
               gravity=-0.009, upper=True, size=(0.14, 0.26)),
        debris('Splinters', WOOD, count=26, life=(40, 60), speed=(0.1, 0.22), size=(0.25, 0.55), gravity=-0.011),
        debris('IronBits', IRON, count=8, life=(40, 56), speed=(0.12, 0.2), size=(0.2, 0.35), gravity=-0.013,
               tex='rock'),
        smoke('Soot', SOOT, count=14, life=(80, 110), delay=5, at=(0, 1.2, 0),
              emit=emit_sphere((0.5, 1.6), upper=True), vel=(0, (0.02, 0.04), 0), grow=(2.4, 6.0), alpha=190),
        smoke('Dust', DUST, count=12, life=(50, 70), delay=2, at=(0, 0.2, 0), emit=emit_circle((0.8, 2.2)),
              vel=(0, (0.004, 0.012), 0), grow=(1.8, 4.2), alpha=150),
        ground_glow('Scorch', (40, 25, 18), 6.5, life=140, delay=4, alpha=190, tex='crack', blend=BLEND,
                    fade_out=70),
    ], 150)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    magic_fx_textures.write_all(build / 'Texture')
    proj = build / f'{NAME}.efkproj'
    tree = barrel_bomb_explosion()
    xmlio.write(proj, tree)
    run(['tools.effect', 'validate', str(proj)])
    run(['tools.effect', 'compile', str(proj)])
    line = f'{NAME} {effect_length_frames(tree)}f'
    if args.install:
        out = run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{NAME}.efkefc'])
        guid = next((ln.split()[-1] for ln in out.splitlines() if 'GUID' in ln), '?')
        line += f'  {guid}'
    print(line)


if __name__ == '__main__':
    main()
