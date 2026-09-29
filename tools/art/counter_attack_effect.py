"""SwordMan のカウンター攻撃 (ジャスト回避からの振り下ろし) のエフェクトを組んで、再生用プレハブを書く。

    python tools/art/counter_attack_effect.py --build-dir <tmp> [--install]

- CounterSlash  : 判定の瞬間にアバターの足元・向きで出す。縦の一閃 + 前方の地面を打つ衝撃 (空振りでも出す)
- CounterImpact : 当たったとき NormalAttackArea の位置に出す。閃光 + 交差する斬撃 + 衝撃波 + 火花
- Assets/Art/Effect/SwordMan/<Name>.efkefc (+ _Source/SwordMan/<Name>.efkproj)
- Assets/Prefab/Particle/SwordMan<Name>Particle.prefab : scale 8 で一度だけ再生して消える

単位は m、+Z がアバターの前方 (tools/art/magic_fx_lib.py の約束事)。NormalAttackArea は足元から 1 m 上・1.6 m 前。
縦の斬撃は後ろのカメラから真横に見えないよう、平面ではなくカメラを向くスプライトで描く。
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
from tools.art.magic_fx_lib import BLEND, FACE, YAXIS, N, emit_circle, emit_sphere, project  # noqa: E402
from tools.art.magic_spell_effects import flash, ground_glow, shockwave, smoke, sparks  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/SwordMan'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'

CORE = (255, 250, 235)
GOLD = (255, 205, 110)
EMBER = (255, 130, 50)
DUST = (150, 128, 100)

GROUND = (0, 0.06, 1.8)      # 振り下ろした刃が地面に届くあたり


def a(rgb, alpha):
    return (*rgb, alpha)


def counter_slash():
    gx, gy, gz = GROUND
    return project([
        # 縦の一閃: 細い芯が上下に伸びる + 外側のにじみ
        N('Blade', tex='streak', billboard=YAXIS, at=(0, 1.3, 1.3), life=10, grow_xyz=((0.5, 0.6, 1), (0.22, 3.4, 1), 30, 0),
          color=a(CORE, 255), fade_out=(8, 0, -20)),
        N('BladeGlow', tex='glow', billboard=YAXIS, at=(0, 1.3, 1.3), life=14, grow_xyz=((0.8, 1.0, 1), (1.3, 4.0, 1), 30, 0),
          color=a(GOLD, 150), fade_out=(12, 0, -20)),
        # 振り下ろしの弧 (カメラに向けた三日月を縦に立てる)
        N('Arc', tex='crescent', billboard=FACE, rot=(0, 0, 90), at=(0, 1.2, 1.2), life=10, grow=(1.6, 3.2, 20, 0),
          color=a(CORE, 235), fade_out=(8, 0, -20)),
        N('ArcTrail', tex='crescent', billboard=FACE, rot=(0, 0, 90), at=(0, 1.2, 1.2), delay=2, life=12,
          grow=(2.0, 3.8, 20, 0), color=a(GOLD, 170), fade_out=(10, 0, -20)),
        # 刃が地面を打つ
        flash('GroundFlash', CORE, 3.2, at=(gx, 0.5, gz), delay=3, life=10, alpha=230),
        ground_glow('GroundBurn', GOLD, 3.6, at=(gx, gy, gz), delay=3, life=26, alpha=190, fade_in=1, fade_out=18),
        shockwave('GroundWave', GOLD, 3.4, at=(gx, gy + 0.02, gz), delay=3, life=22, alpha=220, width=0.16),
        shockwave('GroundWaveOuter', CORE, 5.0, at=(gx, gy + 0.03, gz), delay=5, life=26, alpha=110, width=0.08),
        sparks('GroundSparks', GOLD, count=34, speed=(0.1, 0.22), at=(gx, 0.1, gz), delay=3, life=(18, 32),
               gravity=-0.008, upper=True, size=(0.12, 0.22)),
        N('Chips', tex='rock', blend=BLEND, count=12, delay=3, life=(26, 40), at=(gx, 0.1, gz),
          emit=emit_sphere((0.1, 0.5), upper=True), vel=(0, (0.08, 0.16), 0), gravity=(0, -0.012, 0),
          rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-12, 12)), size=((0.1, 0.22), (0.1, 0.22), 1),
          color=a(DUST, 255), fade_out=(10, 0, -20)),
        smoke('Dust', DUST, count=10, life=(40, 60), delay=4, at=(gx, 0.2, gz), emit=emit_circle((0.3, 1.2)),
              vel=(0, (0.008, 0.02), 0), grow=(1.0, 2.6), alpha=120),
    ], 70)


def counter_impact():
    return project([
        flash('Flash', CORE, 5.0, life=10),
        flash('FlashGold', GOLD, 7.0, delay=1, life=14, alpha=170),
        # 交差する二筋の斬撃
        N('CrossA', tex='crescent', rot=(0, 0, 90), life=12, grow=(2.0, 4.6, 20, 0), color=a(CORE, 250),
          fade_out=(8, 0, -20)),
        N('CrossB', tex='crescent', rot=(0, 0, -35), delay=2, life=12, grow=(1.8, 4.0, 20, 0), color=a(GOLD, 230),
          fade_out=(8, 0, -20)),
        shockwave('AirWave', CORE, 3.2, flat=False, life=16, alpha=220, width=0.14),
        shockwave('AirWaveOuter', GOLD, 5.0, flat=False, delay=3, life=20, alpha=120, width=0.08),
        sparks('Sparks', GOLD, count=60, speed=(0.14, 0.3), life=(16, 30), gravity=-0.006, size=(0.12, 0.24)),
        sparks('HotSparks', CORE, count=20, speed=(0.22, 0.38), life=(10, 18), gravity=0.0, size=(0.16, 0.28)),
        N('Embers', tex='glow', count=24, delay=2, life=(40, 64), emit=emit_sphere((0.2, 0.9)),
          vel=(0, (0.006, 0.02), 0), grow=((0.1, 0.2), 0.03, 0, 0), color=a(EMBER, 230), color_spread=(0, 30, 30, 0),
          fade_out=(20, 0, -20)),
    ], 70)


EFFECTS = {'CounterSlash': counter_slash, 'CounterImpact': counter_impact}


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, name: str, install: bool):
    proj = build / f'{name}.efkproj'
    xmlio.write(proj, EFFECTS[name]())
    run(['tools.effect', 'validate', str(proj)])
    run(['tools.effect', 'compile', str(proj)])
    print(f'compiled {proj.with_suffix(".efkefc")}')
    if install:
        print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{name}.efkefc']))


def build_prefab(name: str, end_frame: int):
    prefab_name = f'SwordMan{name}Particle'
    prefab = new_prefab(prefab_name)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # バージョン 0 だけが書き出していた
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'{name}.efkefc.meta'))
    comp.data['playingDuration_secs_'] = Num.of_float(round(end_frame / 60.0 + 0.1, 2))
    comp.data['playMode_'] = Num.of_int(DESTROY)
    return save(prefab, PREFAB_DIR, prefab_name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true', help='install the effects and write the prefabs')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    for name in EFFECTS:
        build_effect(build, name, args.install)
        if args.install:
            build_prefab(name, 70)


if __name__ == '__main__':
    main()
