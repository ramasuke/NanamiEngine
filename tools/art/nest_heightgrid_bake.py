"""Nest.heightGridMap (古竜の巣で古竜が経路探索に使う) を、巣の地形から焼く。

    python tools/art/nest_heightgrid_bake.py [--dry-run]

やり方は desert_heightgrid_bake.py と同じ。違うのは:
- 地形は data/nest_terrain.npz (world 高さそのまま)
- 外輪の岩の爪 (NestSpire) は巣の底 (戦う所) の外なので焼かない。シーンの BoxCollider だけ足す
.meta が無ければ GrassLand.heightGridMap.meta を写して作る (範囲・分割数は同じ、GUID は新しく振る)。
地形を変えたら (nest_terrain.py の後に) 焼き直す。
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

from grassland_heightgrid_bake import Grid, collider_parts, euler_xyz, floats, world_of  # noqa: E402
from grassland_nature_scatter import walk  # noqa: E402
import nest_terrain as nt  # noqa: E402

META = REPO / 'Assets' / 'Data' / 'HeightGridMap' / 'Nest.heightGridMap.meta'
TEMPLATE = REPO / 'Assets' / 'Data' / 'HeightGridMap' / 'GrassLand.heightGridMap.meta'
SCENE = REPO / 'Assets' / 'Scene' / 'DragonNestScene.scene'


class NestHeights:
    def __init__(self):
        d = np.load(nt.NPZ)
        self.h = d['height'].astype(np.float64)
        self.n = self.h.shape[0]
        self.step = nt.SIZE / (self.n - 1)

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
    grid.hit(slice(None), slice(None), NestHeights().height(X, Z))
    boxes = 0
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
    print(f'  terrain + {boxes} box colliders')


def ensure_meta():
    if META.exists():
        return
    meta = json.loads(TEMPLATE.read_bytes().decode('utf-8-sig'))
    body = meta['value0']['ptr_wrapper']['data']
    head = body['value0']
    head['contentPath_'] = 'Assets\\Data\\HeightGridMap/Nest.heightGridMap'
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
