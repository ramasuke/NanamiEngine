"""草原の野営地の焚き火に付ける、炎と立ちのぼる煙の柱のエフェクトを作って、再生用プレハブを書く。

    python tools/art/campfire_smoke_fx.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Prop/CampfireSmoke.efkefc   (+ _Source/Prop/CampfireSmoke.efkproj)
- Assets/Prefab/Particle/CampfireSmoke.prefab   : scale 8、Loop。grassland_trail.py が焚き火 (Campfire) の上に置く

CampfireSmoke  炉の炎・火の粉・照り返しと、風に少し流されながら約 20 m 立ちのぼる煙。
               スポーン地点から野営地の場所が分かる目印を兼ねる
単位は m (tools/art/magic_fx_lib.py と同じ。at_rand を渡すと at は使われないので、範囲に位置を含める)。
炉 (Campfire.mv1) は石の輪の半径が約 0.7 m。

NOTE: ParticleSystem の Loop は前の再生を消さずに流しきってから次を重ねるので、どのエミッタも CYCLE フレームぶんだけ出して
      自然に終わらせる (tools/art/green_core_effect.py と同じ)。
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
from tools.art.magic_spell_effects import flames, smoke  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import LOOP, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Prop'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
NAME = 'CampfireSmoke'
CYCLE = 240                   # フレーム。プレハブの playingDuration_secs_ と同じ長さ
SMOKE_LIFE = (420, 480)
END_FRAME = CYCLE + SMOKE_LIFE[1] + 20

FIRE_HOT = (255, 225, 140)
FIRE_COOL = (230, 80, 25)
SMOKE = (132, 128, 120)
SMOKE_LOW = (78, 74, 70)


def every(interval):
    return dict(count=CYCLE // interval, interval=interval)


def jitter(r, y0, y1):
    return ((-r, r), (y0, y1), (-r, r))


def campfire_smoke():
    return project([
        # 炉の炎。外側の大きな舌と、芯の明るい舌
        flames('Flame', FIRE_HOT, FIRE_COOL, life=(24, 34), at_rand=jitter(0.3, 0.15, 0.3),
               vel=((-0.002, 0.002), (0.018, 0.028), (-0.002, 0.002)), grow=(0.9, 0.25), **every(3)),
        flames('FlameCore', (255, 245, 200), FIRE_HOT, life=(18, 24), at_rand=jitter(0.15, 0.15, 0.25),
               vel=((-0.001, 0.001), (0.016, 0.022), (-0.001, 0.001)), grow=(0.55, 0.2), **every(4)),
        # 照り返し
        N('Glow', tex='glow', at=(0, 0.6, 0), life=60, grow=(2.4, 2.8, 0, 0), color=(255, 130, 50, 70),
          fade_in=20, fade_out=(30, 0, 0), **every(30)),
        # 火の粉
        N('Embers', tex='spark', life=(70, 110), at_rand=jitter(0.3, 0.4, 0.6),
          vel=((-0.006, 0.006), (0.025, 0.04), (-0.006, 0.006)), size=(0.05, 0.09), color=(255, 170, 80, 255),
          fade_out=(30, 0, 0), **every(8)),
        # 炉のすぐ上の濃い煙と、風に流されながら高く上る煙の柱
        smoke('SmokeLow', SMOKE_LOW, life=(120, 160), at_rand=jitter(0.2, 0.8, 1.0),
              vel=((-0.002, 0.004), (0.03, 0.04), (-0.002, 0.002)), grow=(0.6, 2.4), alpha=150, **every(6)),
        smoke('Smoke', SMOKE, life=SMOKE_LIFE, at_rand=jitter(0.25, 1.2, 1.5),
              vel=((0.004, 0.008), (0.038, 0.048), (-0.002, 0.002)), grow=(1.0, 7.0), alpha=120, **every(5)),
    ], END_FRAME)


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, install: bool):
    proj = build / f'{NAME}.efkproj'
    xmlio.write(proj, campfire_smoke())
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
    comp.data['playingDuration_secs_'] = Num.of_float(round(CYCLE / 60.0, 2))
    comp.data['playMode_'] = Num.of_int(LOOP)
    return save(prefab, PREFAB_DIR, NAME)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true', help='install the effect and write the prefab')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    build_effect(build, args.install)
    if args.install:
        build_prefab()


if __name__ == '__main__':
    main()
