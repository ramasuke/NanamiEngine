"""Blender から書き出した建物の FBX を .mv1 に変換して Assets/Art/Models/Settlement へ入れ、当たり判定の箱を
tools/art/data/settlement_colliders.json にまとめる (手順は blib.py の先頭)。

    python tools/art/settlement/install_models.py <work> [Name ...]     # Name を省くと <work>/fbx の全部

変換は tools.model convert (DxLibModelViewer の GUI を動かす) なので、実行中は他の GUI 操作をしない。
テクスチャは FBX にファイル名だけで入っているので、変換前に <work>/tex から FBX の横へ写す。
"""
import json
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
DEST = REPO / 'Assets' / 'Art' / 'Models' / 'Settlement'
COLLIDERS = REPO / 'tools' / 'art' / 'data' / 'settlement_colliders.json'


def run(*args):
    result = subprocess.run([sys.executable, '-m', 'tools.model', *args], cwd=REPO, capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f'tools.model {args[0]} failed:\n{result.stdout}\n{result.stderr}')
    return result.stdout


def main():
    work = Path(sys.argv[1])
    fbx_dir, mv1_dir = work / 'fbx', work / 'mv1'
    mv1_dir.mkdir(parents=True, exist_ok=True)
    names = sys.argv[2:] or sorted(p.stem for p in fbx_dir.glob('*.fbx'))
    for png in (work / 'tex').glob('*.png'):
        shutil.copy2(png, fbx_dir / png.name)

    colliders = json.loads(COLLIDERS.read_text(encoding='utf-8')) if COLLIDERS.exists() else {}
    for name in names:
        fbx, mv1 = fbx_dir / f'{name}.fbx', mv1_dir / f'{name}.mv1'
        run('convert', str(fbx), str(mv1), '--mode', 'mesh', '--with-textures', '--force')
        run('install', str(mv1), '--with-textures', '--source', str(fbx), '--dest', str(DEST / f'{name}.mv1'))
        colliders[name] = json.loads((fbx_dir / f'{name}.colliders.json').read_text(encoding='utf-8'))
        print(f'installed {name}  ({len(colliders[name])} collider box(es))')
    COLLIDERS.write_text(json.dumps(dict(sorted(colliders.items())), indent=1), encoding='utf-8')


if __name__ == '__main__':
    main()
