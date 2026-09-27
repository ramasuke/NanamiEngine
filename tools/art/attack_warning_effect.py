"""敵の攻撃予兆エフェクト (ゼンゼロ風の閃光) を金 / 赤の2色で組んで、再生用プレハブを書く。

    python tools/art/attack_warning_effect.py --build-dir <tmp> [--install]

- Assets/Art/Effect/AttackWarning/AttackWarning{Gold,Red}.efkefc   (+ _Source/AttackWarning/*.efkproj)
- Assets/Prefab/Particle/AttackWarning{Gold,Red}.prefab            : scale 8 で一度だけ再生して消える

金 = 通常攻撃、赤 = 強攻撃。PhysicsAttack / ChargeRush の warningPrefab_ に指定し、敵のボーンに付いて行かせる。
単位は m (tools/art/magic_fx_lib.py と同じ)。原点がボーンの位置で、全部カメラを向く板なので向きは持たない。
光点 -> 縦横の長い光条 + 斜めの短い光条 (四芒星) -> 外へ広がる細い輪、の順で約 0.35 秒。
体や地形に隠れないよう、全ノードの ZTest / ZWrite を切って最前面に描く。
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
from tools.art.magic_fx_lib import N, project  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/AttackWarning'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
END_FRAME = 30

# 名前: (光条の色, 芯の色)。赤は芯も赤寄りにしないと白に負けて赤く見えない
VARIANTS = {
    'AttackWarningGold': ((255, 196, 64), (255, 250, 230)),
    'AttackWarningRed': ((255, 48, 36), (255, 170, 150)),
}


def a(rgb, alpha):
    return (*rgb, alpha)


def W(name, **kw):
    """深度を無視して最前面に描くノード"""
    return N(name, ztest=False, **kw)


def attack_warning(main, core):
    fade = (0, -20)
    return project([
        # 芯の光点: 素早く膨らんで消える
        W('Core', tex='glow_core', life=16, grow=(0.5, 1.4, 20, 0), color=a(core, 255), fade_out=(10, *fade)),
        # まわりのにじみ
        W('Halo', tex='glow', life=22, grow=(1.2, 1.8, 20, 0), color=a(main, 200), fade_in=2, fade_out=(14, *fade)),
        # 四芒星の縦横の光条 (色付きの太い帯 + 白い芯)
        W('RayGlowV', tex='streak', delay=1, life=18, grow_xyz=((0.35, 0.55, 1), (0.18, 2.9, 1), 20, 0),
          color=a(main, 230), fade_out=(14, *fade)),
        W('RayGlowH', tex='streak', delay=1, life=18, rot=(0, 0, 90),
          grow_xyz=((0.35, 0.55, 1), (0.18, 2.9, 1), 20, 0), color=a(main, 230), fade_out=(14, *fade)),
        W('RayV', tex='streak', delay=1, life=18, grow_xyz=((0.16, 0.6, 1), (0.08, 3.2, 1), 20, 0),
          color=a(core, 255), fade_out=(14, *fade)),
        W('RayH', tex='streak', delay=1, life=18, rot=(0, 0, 90),
          grow_xyz=((0.16, 0.6, 1), (0.08, 3.2, 1), 20, 0), color=a(core, 255), fade_out=(14, *fade)),
        # 少し遅れて斜めの短い光条
        W('RayDiagA', tex='streak', delay=3, life=15, rot=(0, 0, 45),
          grow_xyz=((0.1, 0.4, 1), (0.06, 1.6, 1), 20, 0), color=a(main, 200), fade_out=(12, *fade)),
        W('RayDiagB', tex='streak', delay=3, life=15, rot=(0, 0, -45),
          grow_xyz=((0.1, 0.4, 1), (0.06, 1.6, 1), 20, 0), color=a(main, 200), fade_out=(12, *fade)),
        # 外へ広がる細い輪
        W('Ring', kind='ring', delay=2, life=20, grow=(0.2, 1.3, 20, 0),
          ring={'outer': (1.0, 0), 'inner': (0.9, 0), 'center_ratio': 0.5, 'outer_color': a(main, 0),
                'center_color': a(main, 230), 'inner_color': a(main, 0)}, fade_out=(18, *fade)),
    ], END_FRAME)


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, name: str, main, core, install: bool):
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    proj = build / f'{name}.efkproj'
    xmlio.write(proj, attack_warning(main, core))
    run(['tools.effect', 'validate', str(proj)])
    run(['tools.effect', 'compile', str(proj)])
    print(f'compiled {proj.with_suffix(".efkefc")}')
    if install:
        print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{name}.efkefc']))


def build_prefab(name: str):
    prefab = new_prefab(name)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # バージョン 0 だけが書き出していた
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'{name}.efkefc.meta'))
    comp.data['playingDuration_secs_'] = Num.of_float(round(END_FRAME / 60.0 + 0.1, 2))
    comp.data['playMode_'] = Num.of_int(DESTROY)
    return save(prefab, PREFAB_DIR, name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true', help='install the effects and write the prefabs')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    for name, (main_color, core_color) in VARIANTS.items():
        build_effect(build, name, main_color, core_color, args.install)
        # NOTE: 既にあるプレハブは書き直さない (guid や手で変えた値を保つ)
        if args.install and not (PREFAB_DIR / f'{name}.prefab').exists():
            build_prefab(name)


if __name__ == '__main__':
    main()
