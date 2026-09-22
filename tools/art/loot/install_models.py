"""assets_loot.py で書き出した薬草・宝箱の FBX を .mv1 に変換して Assets/Art/Models/Prop/Loot へ入れる。

    python tools/art/loot/install_models.py <work> [Name ...]     # Name を省くと <work>/fbx の全部
    python tools/art/loot/install_models.py --emissive-only        # 入れ済みの .mv1 の emissive だけ付け直す

変換は tools.model convert (DxLibModelViewer の GUI を動かす) なので、実行中は他の GUI 操作をしない。
"""
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
DEST = REPO / 'Assets' / 'Art' / 'Models' / 'Prop' / 'Loot'
# 木や岩・村の建物と同じ明るさにする (あちらは diffuse 0.8 / emissive 0.36)
EMISSIVE_RATIO = 0.45
# 薬草の葉は足元の草と見分けが付くよう明るめ
LEAF_EMISSIVE_RATIO = 0.8


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

    for name in names:
        fbx, mv1 = fbx_dir / f'{name}.fbx', mv1_dir / f'{name}.mv1'
        run('convert', str(fbx), str(mv1), '--mode', 'mesh', '--with-textures', '--force')
        run('install', str(mv1), '--with-textures', '--source', str(fbx), '--dest', str(DEST / f'{name}.mv1'))
        set_emissive(DEST / f'{name}.mv1')
        print(f'installed {name}')


def set_emissive(mv1):
    args = []
    for m in re.finditer(r'\[(\d+)\] (\S+)\s+diffuse=\(([\d.]+), ([\d.]+), ([\d.]+)', run('materials', str(mv1))):
        ratio = LEAF_EMISSIVE_RATIO if '_Leaf' in m.group(2) else EMISSIVE_RATIO
        rgb = ','.join(f'{float(c) * ratio:.4f}' for c in m.groups()[2:])
        args += ['--emissive', f'{m.group(1)}={rgb}']
    run('set-emissive', str(mv1), *args)


if __name__ == '__main__':
    if sys.argv[1:2] == ['--emissive-only']:
        for path in sorted(DEST.glob('*.mv1')):
            set_emissive(path)
    else:
        main()
