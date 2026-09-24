"""Desert.heightGridMap (砂漠の敵の経路探索用) を、地形とシーンの当たり判定から焼く。

    python tools/art/desert_heightgrid_bake.py [--dry-run]

やり方は grassland_heightgrid_bake.py と同じ (Grid / BoxCollider の OBB をそのまま使う)。違うのは:
- 地形は data/desert_terrain.npz (world 高さそのまま)
- StaticMeshCollider の小物 (城塞の壁・神殿・台地・崖・柱) はメッシュを読まず、desert_prefabs.py に切り出した
  寸法 (m) から、壁や建物は向き付きの箱、岩は円柱で近似する
.meta が無ければ GrassLand.heightGridMap.meta を写して作る (範囲・分割数は同じ、GUID は新しく振る)。
地形や置き物を変えたら (desert_terrain.py / desert_scene.py の後に) 焼き直す。
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common import meta_base  # noqa: E402
from tools.scene import reader  # noqa: E402

from grassland_heightgrid_bake import Grid, body_of, collider_parts, euler_xyz, floats, world_of  # noqa: E402
from grassland_nature_scatter import walk  # noqa: E402
import desert_terrain as dt  # noqa: E402

META = REPO / 'Assets' / 'Data' / 'HeightGridMap' / 'Desert.heightGridMap.meta'
TEMPLATE = REPO / 'Assets' / 'Data' / 'HeightGridMap' / 'GrassLand.heightGridMap.meta'
SCENE = REPO / 'Assets' / 'Scene' / 'DesertScene.scene'
M = 8.0

# プレハブ名 -> ('box', 幅 x, 高さ, 奥行き z) / ('cyl', 半径, 高さ)。m 単位、原点は底面の中心 (process_props.py の出力)
SOLIDS = {
    'FortressWallTall': ('box', 12.0, 11.1, 2.5),
    'FortressWallBlock': ('box', 15.4, 8.0, 5.3),
    'FortressWallBlockB': ('box', 8.4, 8.1, 4.5),
    'FortressWallBlockC': ('box', 15.4, 8.0, 3.5),
    'FortressWallAngled': ('box', 12.0, 6.6, 5.2),
    'FortressWallAngledB': ('box', 11.9, 6.6, 5.3),
    'FortressWallEnd': ('box', 1.2, 10.4, 3.6),
    'FortressGate': ('box', 19.8, 11.5, 15.8),
    'FortressArch': ('box', 7.0, 6.7, 2.5),
    'FortressArchSmall': ('box', 6.0, 6.0, 2.4),
    'FortressBalcony': ('box', 5.4, 24.0, 5.5),
    'FortressSpire': ('box', 6.4, 19.1, 4.5),
    'FortressTower': ('box', 3.9, 13.9, 3.9),
    'DesertTemple': ('box', 17.4, 25.2, 25.2),
    'SunRing': ('box', 18.5, 12.4, 3.2),
    'SandstoneCliff': ('box', 7.3, 5.9, 15.0),
    'DesertMesa': ('cyl', 12.0, 18.5),
    'SandstonePillar': ('cyl', 3.5, 11.4),
}


class DesertHeights:
    def __init__(self):
        d = np.load(dt.NPZ)
        self.h = d['height'].astype(np.float64)
        self.n = self.h.shape[0]
        self.step = dt.SIZE / (self.n - 1)

    def height(self, x, z):
        fx = np.clip(np.asarray(x, float) / self.step, 0, self.n - 1.0001)
        fz = np.clip(np.asarray(z, float) / self.step, 0, self.n - 1.0001)
        i0, j0 = fx.astype(int), fz.astype(int)
        tx, tz = fx - i0, fz - j0
        g = self.h
        h00, h10, h01, h11 = g[j0, i0], g[j0, i0 + 1], g[j0 + 1, i0], g[j0 + 1, i0 + 1]
        lower = h00 + tx * (h10 - h00) + tz * (h01 - h00)
        upper = h11 + (1 - tx) * (h01 - h11) + (1 - tz) * (h10 - h11)
        return np.where(tx + tz <= 1.0, lower, upper)


def bake(grid, scene):
    X, Z = np.meshgrid(grid.cx, grid.cz)
    grid.hit(slice(None), slice(None), DesertHeights().height(X, Z))
    boxes = solids = 0
    for root in scene.roots:
        for node in walk(root):
            for comp in node.components:
                if not comp.fqn.endswith('::BoxCollider'):
                    continue
                offset, offset_rot, layer, enabled = collider_parts(comp)
                if not enabled or not (grid.mask >> layer) & 1:
                    continue
                pos, rot, scale = world_of(node)
                size = np.array(floats(comp.data['size_']))
                center = pos + rot @ (np.array(offset) * scale)
                boxes += grid.obb(center, rot @ euler_xyz(offset_rot), size * scale / 2) > 0
            if node.name not in SOLIDS or not any(c.name == 'Model' for c in node.transform.children):
                continue
            pos, rot, scale = world_of(node)
            s = float(scale[0]) * M
            spec = SOLIDS[node.name]
            if spec[0] == 'box':
                _, w, h, d = spec
                half = np.array([w / 2, h / 2, d / 2]) * s
                grid.obb(pos + rot @ np.array([0.0, h * s / 2, 0.0]), rot, half)
            else:
                _, r, h = spec
                grid.cylinder(pos[0], pos[2], r * s, pos[1] + h * s)
            solids += 1
    print(f'  terrain + {boxes} box colliders + {solids} solid props')


def ensure_meta():
    if META.exists():
        return
    meta = json.loads(TEMPLATE.read_bytes().decode('utf-8-sig'))
    body = meta['value0']['ptr_wrapper']['data']
    head = body['value0']
    head['contentPath_'] = 'Assets\\Data\\HeightGridMap/Desert.heightGridMap'
    head['guid_']['value_'] = meta_base.mint_guid().upper()
    META.write_bytes(json.dumps(meta, indent=4, ensure_ascii=False).replace('\n', '\r\n').encode('utf-8'))
    META.with_suffix('').write_bytes(b'')
    print(f'created {META.relative_to(REPO)}  (guid {head["guid_"]["value_"]})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()
    ensure_meta()

    raw = META.read_bytes()
    crlf = b'\r\n' in raw[:200]
    meta = json.loads(raw.decode('utf-8-sig'))
    body = meta['value0']['ptr_wrapper']['data']
    grid = Grid(body)
    bake(grid, reader.read_scene_file(SCENE))

    h = grid.height.astype(np.float32).ravel()
    change = np.flatnonzero(np.diff(h) != 0) + 1
    starts = np.concatenate([[0], change])
    lengths = np.diff(np.concatenate([starts, [len(h)]]))
    print(f'  {len(h)} cells, {len(starts)} runs, height {h.min():.1f}..{h.max():.1f}')
    if args.dry_run:
        return
    for key in [k for k in body if k.startswith(('runLen_', 'runHeight_'))]:
        del body[key]
    body['cellCount'] = int(len(h))
    body['runCount'] = int(len(starts))
    for i, (s, n) in enumerate(zip(starts, lengths)):
        body[f'runLen_{i}'] = int(n)
        body[f'runHeight_{i}'] = float(h[s])
    text = json.dumps(meta, indent=4, ensure_ascii=False)
    if crlf:
        text = text.replace('\n', '\r\n')
    META.write_bytes(text.encode('utf-8'))
    print(f'wrote {META.relative_to(REPO)}')


if __name__ == '__main__':
    main()
