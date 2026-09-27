"""3D タイトル画面 (飛空艇の船尾甲板) に流す環境エフェクト 2 つとテクスチャを作り、再生用プレハブを書く。

    python tools/art/title_effects.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Title/TitlePetals.efkefc  (+ _Source/Title/TitlePetals.efkproj, Texture/TitlePetal.png)
- Assets/Art/Effect/Title/TitleMotes.efkefc   (+ _Source/Title/TitleMotes.efkproj,  Texture/TitleMote.png)
- Assets/Prefab/Particle/TitlePetalsParticle.prefab / TitleMotesParticle.prefab : scale 1、Loop
  (WindBreezeParticle.prefab を tools.scene copy-prefab で複製して particleFile_ を差し替える)

TitlePetals  風 (+X) に流されながらゆっくり落ちる桜の花びら。回転しながら裏返るので光の当たり方が変わる
TitleMotes   ゆっくり漂って少しずつ昇る金色 (と少しの淡い緑) の光の粒と、ところどころで瞬く光

単位はワールド単位そのまま (人の背丈が約 14)。プレハブはスケール 1 で再生する。
フレームは Effekseer のフレーム (60 fps)。速度はフレームあたり。
カメラが見る範囲はエミッタを中心に約 60 x 30 x 60。

NOTE: どのエミッタも無限生成。ParticleSystem の Loop は playingDuration_secs_ ごとに次の再生を重ねるので、
      プレハブは WindBreeze と同じ 3600 秒にして実質 1 回だけ再生する。
NOTE: 無限生成だけだと画面が埋まるまで寿命ぶん (約 20 秒) かかるので、0 フレーム目に同じ見た目の粒を
      寿命をばらけさせて一度に出し (…Prewarm)、最初から満ちた状態に見せる。
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from tools.art.magic_fx_lib import BLEND, ADD, FACE, FIXED, ON_CREATE, N, project  # noqa: E402
from tools.effect import xmlio  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Title'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
PREFAB_TEMPLATE = PREFAB_DIR / 'WindBreezeParticle.prefab'
PLAYING_SECS = 3600.0
FPS = 60.0


def per_frame(lo, hi):
    """units/s の範囲 -> フレームあたり。"""
    return (round(lo / FPS, 5), round(hi / FPS, 5))


def secs(lo, hi):
    return (int(lo * FPS), int(hi * FPS))


# ---------------------------------------------------------------------------
# テクスチャ

def _grid(n):
    y, x = np.mgrid[0:n, 0:n].astype(np.float32)
    return (x + 0.5) / n * 2 - 1, (y + 0.5) / n * 2 - 1


def _smooth(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def petal_texture(size=128, ss=4):
    """切れ込みのある桜の花びら。根元 (下) が細く、先 (上) が丸くて中央に V の切れ込み。"""
    n = size * ss
    u, v = _grid(n)
    t = (0.95 - v) / 1.85                                  # 0 = 根元、1 = 先端
    knee = 0.62
    w_low = 0.64 * np.sin(np.pi / 2 * np.clip(t / knee, 0, 1)) ** 0.85
    w_top = 0.64 * np.sqrt(np.clip(1 - ((t - knee) / (1 - knee)) ** 2, 0, 1))
    half_w = np.where(t < knee, w_low, w_top)
    inside = (t >= 0) & (t <= 1) & (np.abs(u) < half_w)
    notch = (t > 1 - 0.15 * np.clip(1 - np.abs(u) / 0.13, 0, 1))
    mask = (inside & ~notch).astype(np.float32)

    img = Image.fromarray((mask * 255).astype(np.uint8), 'L').filter(ImageFilter.GaussianBlur(ss * 1.2))
    alpha = np.asarray(img.resize((size, size), Image.LANCZOS)).astype(np.float32) / 255

    su, sv = _grid(size)
    st = (0.95 - sv) / 1.85
    r = np.clip(np.hypot(su / 0.64, (sv - 0.05) / 0.95), 0, 1)
    center = np.array([1.00, 0.965, 0.975])
    edge = np.array([0.965, 0.74, 0.815])
    base = np.array([0.93, 0.62, 0.72])
    k = _smooth(0.15, 1.0, r)[..., None]
    rgb = center * (1 - k) + edge * k
    kb = (1 - _smooth(0.0, 0.22, st))[..., None]            # 根元だけ少し濃く
    rgb = rgb * (1 - kb * 0.6) + base * kb * 0.6
    # 根元から放射状に走るごく淡い筋
    ang = np.arctan2(su, np.maximum(st, 1e-3))
    rgb *= (1 - 0.025 * (0.5 + 0.5 * np.cos(ang * 22)) * _smooth(0.1, 0.6, st))[..., None]
    # 縁ほどわずかに透ける
    alpha = alpha * (0.8 + 0.2 * (1 - _smooth(0.55, 1.0, r)))

    out = np.zeros((size, size, 4), np.uint8)
    out[..., :3] = np.clip(rgb * 255, 0, 255).astype(np.uint8)
    out[..., 3] = np.clip(alpha * 255, 0, 255).astype(np.uint8)
    return Image.fromarray(out, 'RGBA')


def mote_texture(size=64):
    """やわらかい丸い光。白で形はアルファ (色はノードが付ける)。"""
    u, v = _grid(size)
    r = np.hypot(u, v)
    a = 0.75 * np.exp(-(r / 0.38) ** 2) + 0.45 * np.exp(-(r / 0.12) ** 2)
    a = np.clip(a, 0, 1) * (1 - _smooth(0.8, 1.0, r))
    out = np.zeros((size, size, 4), np.uint8)
    out[..., :3] = 255
    out[..., 3] = (a * 255).astype(np.uint8)
    return Image.fromarray(out, 'RGBA')


def write_textures(tex_dir: Path):
    tex_dir.mkdir(parents=True, exist_ok=True)
    petal_texture().save(tex_dir / 'TitlePetal.png')
    mote_texture().save(tex_dir / 'TitleMote.png')


# ---------------------------------------------------------------------------
# エフェクト

# 花びら: 風下 (+X) へ 2〜4 /s、落下 0.6〜1.2 /s、奥行きは少しだけ揺らぐ
PETAL_VEL = (per_frame(2.0, 4.0), per_frame(-1.2, -0.6), per_frame(-0.3, 0.3))
PETAL_LIFE = secs(15, 25)
PETAL_SIZE = (0.25, 0.45)
PETAL_ROT = ((-180, 180), (-180, 180), (-180, 180))
PETAL_SPIN = ((-2.5, 2.5), (-2.5, 2.5), (-1.5, 1.5))    # 度/フレーム
# 風上 (-X) と上寄りの箱から出す。20 秒で約 60 流れ、約 18 落ちる
PETAL_SPAWN = ((-40, 15), (-3, 14), (-30, 30))
# 最初から画面を埋める分は見える範囲全体に置く
PETAL_PREWARM_AT = ((-30, 30), (-12, 14), (-30, 30))
PETAL_PREWARM_LIFE = secs(1, 20)
PALE = (255, 255, 255, 235)
DEEP = (240, 165, 195, 240)


def petals(name, color, spread, *, interval=0, count=1, infinite=False, at_rand, life):
    return N(name, tex='TitlePetal', blend=BLEND, infinite=infinite, count=count, interval=interval,
             life=life, bind=ON_CREATE, bind_rot=ON_CREATE, bind_scale=ON_CREATE,
             at_rand=at_rand, vel=PETAL_VEL, rot_rand=PETAL_ROT, spin=PETAL_SPIN, size=PETAL_SIZE,
             color=color, color_spread=spread, billboard=FIXED, fade_in=60, fade_out=90)


def title_petals():
    # 生きている数の目安: 淡 1200/12 = 100、濃 1200/60 = 20
    return project([
        petals('Petals', PALE, (0, 12, 10, 20), interval=12, infinite=True, at_rand=PETAL_SPAWN, life=PETAL_LIFE),
        petals('PetalsDeep', DEEP, (8, 15, 12, 15), interval=60, infinite=True, at_rand=PETAL_SPAWN,
               life=PETAL_LIFE),
        petals('PetalsPrewarm', PALE, (0, 12, 10, 20), count=85, at_rand=PETAL_PREWARM_AT, life=PETAL_PREWARM_LIFE),
        petals('PetalsDeepPrewarm', DEEP, (8, 15, 12, 15), count=15, at_rand=PETAL_PREWARM_AT,
               life=PETAL_PREWARM_LIFE),
    ], 1800)


# 光の粒: ほぼその場で漂い、ゆっくり昇る (風で少しだけ +X)
MOTE_VEL = (per_frame(-0.12, 0.45), per_frame(0.12, 0.5), per_frame(-0.25, 0.25))
MOTE_ACC = ((-0.00002, 0.00002), (0, 0), (-0.00002, 0.00002))
MOTE_LIFE = secs(6, 10)
MOTE_SPAWN = ((-30, 30), (-13, 10), (-30, 30))
MOTE_PREWARM_LIFE = secs(0.5, 10)
GOLD = (255, 205, 120, 255)
GREEN = (185, 255, 170, 230)


def motes(name, color, spread, *, interval=0, count=1, infinite=False, life):
    return N(name, tex='TitleMote', blend=ADD, infinite=infinite, count=count, interval=interval,
             life=life, bind=ON_CREATE, bind_rot=ON_CREATE, bind_scale=ON_CREATE,
             at_rand=MOTE_SPAWN, vel=MOTE_VEL, acc=MOTE_ACC,
             grow=((0.12, 0.2), (0.07, 0.12)), color=color, color_spread=spread, billboard=FACE,
             fade_in=(120, 0, -20), fade_out=(150, 0, 0))


def title_motes():
    # 生きている数の目安: 金 480/8 = 60、緑 480/40 = 12、瞬き 50/5 = 10
    return project([
        motes('Motes', GOLD, (0, 20, 30, 0), interval=8, infinite=True, life=MOTE_LIFE),
        motes('MotesGreen', GREEN, (15, 0, 20, 0), interval=40, infinite=True, life=MOTE_LIFE),
        motes('MotesPrewarm', GOLD, (0, 20, 30, 0), count=50, life=MOTE_PREWARM_LIFE),
        motes('MotesGreenPrewarm', GREEN, (15, 0, 20, 0), count=10, life=MOTE_PREWARM_LIFE),
        # ところどころで一瞬ふくらんで消える瞬き
        N('Twinkle', tex='TitleMote', blend=ADD, infinite=True, interval=5, life=(30, 70),
          bind=ON_CREATE, bind_rot=ON_CREATE, bind_scale=ON_CREATE, at_rand=MOTE_SPAWN,
          vel=MOTE_VEL, grow=((0.3, 0.45), 0.02, 0, -20), color=(255, 240, 200, 255),
          color_spread=(0, 10, 30, 0), billboard=FACE, fade_in=(12, 0, 0)),
    ], 1200)


EFFECTS = {'TitlePetals': title_petals, 'TitleMotes': title_motes}


# ---------------------------------------------------------------------------
# ビルド

def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, name: str, install: bool):
    proj = build / f'{name}.efkproj'
    xmlio.write(proj, EFFECTS[name]())
    print(run(['tools.effect', 'validate', str(proj)]).strip())
    run(['tools.effect', 'compile', str(proj)])
    print(f'compiled {proj.with_suffix(".efkefc")}')
    if install:
        print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{name}.efkefc']).strip())


def meta_guid(meta: Path) -> str:
    return json.loads(meta.read_text(encoding='utf-8-sig'))['value0']['ptr_wrapper']['data']['guid_']['value_']


def build_prefab(effect: str):
    name = f'{effect}Particle'
    path = PREFAB_DIR / f'{name}.prefab'
    if not path.exists():
        run(['tools.scene', 'copy-prefab', str(PREFAB_TEMPLATE), '--name', name, '--dir', str(PREFAB_DIR)])
    root = json.loads(path.read_text(encoding='utf-8-sig'))
    go_guid = root['guid_']['value_']
    if root.get('name_') != name:
        run(['tools.scene', 'rename-gameobject', str(path), '--guid', go_guid, '--name', name])
    guid = meta_guid(REPO / INSTALL_DIR / f'{effect}.efkefc.meta')
    run(['tools.scene', 'set-component-params', str(path), '--guid', go_guid, '--index', '0',
         '--set', f'particleFile_={guid}', '--set', f'playingDuration_secs_={PLAYING_SECS}',
         '--set', 'playMode_=Loop'])
    print(run(['tools.scene', 'validate', str(path)]).strip())
    print(f'wrote {path}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true', help='install the effects and write the prefabs')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    write_textures(build / 'Texture')
    for name in EFFECTS:
        build_effect(build, name, args.install)
    if args.install:
        for name in EFFECTS:
            build_prefab(name)


if __name__ == '__main__':
    main()
