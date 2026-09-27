"""sky_island_shapes_blender.py の FBX を .mv1 にし、砂の島として Assets に置く。

    blender -b --factory-startup --python tools/art/sky_island_shapes_blender.py -- <work> <work>/tex
    python tools/art/sky_island_models.py <work> [Long Slab ...] [--kind grass|sand]   # 省略時は両方

砂の島は砂の島の空 (DesertScene)、草の島は拠点の島の空 (MainIslandScene) に使う。草原の空は丸い島のまま。

- <work>/tex に SkyTop_Grass.jpg / SkyRock_Grey.jpg (草の島) と SkyTop_Dunes.jpg / SkyRock_Rust.jpg (砂の島) を置いておく
  (もとは isLand/textures の grass02 / mountainRock / sandTop / sandstoneRck)。
- 変換は tools.model convert (DxLibModelViewer を GUI 操作で動かす)。草のテクスチャで書き出した .mv1 の中のテクスチャ名を
  同じ長さの名前に差し替えて砂の島にする。出力は Assets/Art/Models/isLand/Sky/SkyIsland_<形>_{Grass,Sand}.mv1。
- 既にある .meta (GUID) はそのまま残る。新しい .mv1 は自己発光を付けないと黒くなる (他の島と同じ明るさに合わせる)。
"""
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from tools.model import mv1  # noqa: E402

DEST = REPO / 'Assets' / 'Art' / 'Models' / 'isLand' / 'Sky'
SHAPES = ['Long', 'Slab', 'Twin', 'Mesa', 'Shard']
SAND_SWAPS = {b'SkyTop_Grass.jpg\0': b'SkyTop_Dunes.jpg\0', b'SkyRock_Grey.jpg\0': b'SkyRock_Rust.jpg\0'}
EMISSIVE = {
    'grass': ['SkyTop=0.34,0.34,0.34', 'SkyRock=0.45,0.45,0.45'],
    'sand': ['SkyTop=0.36,0.34,0.3', 'SkyRock=0.55,0.5,0.46'],
}


def model_cli(*args):
    subprocess.run([sys.executable, '-m', 'tools.model', *map(str, args)], cwd=REPO, check=True)


def main():
    args = sys.argv[1:]
    kinds = ['grass', 'sand']
    if '--kind' in args:
        i = args.index('--kind')
        kinds = [args[i + 1]]
        del args[i:i + 2]
    work = Path(args[0])
    shapes = args[1:] or SHAPES
    out_dir = work / 'mv1'
    out_dir.mkdir(exist_ok=True)
    DEST.mkdir(parents=True, exist_ok=True)
    for shape in shapes:
        grass = out_dir / f'SkyIsland_{shape}.mv1'
        model_cli('convert', work / 'fbx' / f'SkyIsland_{shape}.fbx', grass, '--mode', 'mesh', '--with-textures', '--force')

        sand = out_dir / f'SkyIsland_{shape}_Sand.mv1'
        if 'sand' in kinds:
            body = mv1.decode(grass.read_bytes())
            for old, new in SAND_SWAPS.items():
                if body.count(old) != 1:
                    raise SystemExit(f'{grass.name}: expected one {old!r}')
                body = body.replace(old, new)
            sand.write_bytes(mv1.encode(body))
            for name in ('SkyTop_Dunes.jpg', 'SkyRock_Rust.jpg'):
                shutil.copy2(work / 'tex' / name, out_dir / name)

        for src, kind in ((grass, 'grass'), (sand, 'sand')):
            if kind not in kinds:
                continue
            dest = DEST / (f'SkyIsland_{shape}_Grass.mv1' if kind == 'grass' else src.name)
            model_cli('install', src, '--dest', dest.relative_to(REPO), '--with-textures')
            args = ['set-emissive', dest]
            for e in EMISSIVE[kind]:
                args += ['--emissive', e]
            model_cli(*args)


if __name__ == '__main__':
    main()
