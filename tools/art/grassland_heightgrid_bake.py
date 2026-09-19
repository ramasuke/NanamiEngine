"""GrassLand.heightGridMap を今の地形とシーンの当たり判定から焼き直す (エディタの Bake Height Map の代わり)。

    python tools/art/grassland_heightgrid_bake.py [--dry-run]

エンジンの HeightGridMap::Bake と同じく、各セルの中心の samplingHeight_ から真下へ layerMask_ のレイを飛ばし、
最初に当たった高さを入れる。レイを物理で飛ばす代わりに:
- 地形: grassland_nature_scatter と同じ地形メッシュ (三角形補間で .mv1 と一致)
- BoxCollider: シーンの worldMatrix_ + offset_/offsetRotation_ + size_ から OBB を作り、真下へのレイと交差させる。
  始点が箱の中なら当たりは始点 (= samplingHeight_、エンジンの Jolt と同じ)
- 岩 (StaticMeshCollider): メッシュは読まず、外形の円柱 (長い石垣は箱) で近似する
敵の経路探索 (ChasePlayerForPathFinding / WanderMove) はこの高さの段差で歩けるかを決めるので、地形や置き物を
変えたら焼き直す。分割数・範囲などの設定は .meta の値をそのまま使う。
"""
import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.scene import reader  # noqa: E402

from grassland_nature_scatter import ROCK_SIZE, SCENE, Terrain, walk  # noqa: E402

META = REPO / 'Assets' / 'Data' / 'HeightGridMap' / 'GrassLand.heightGridMap.meta'
FENCE_SIZE = (35.0, 8.0)  # RuinedRockFence の長さ x 厚み (world @scale 1)。高さは ROCK_SIZE


def floats(obj):
    return [float(getattr(v, 'value', v)) for _, v in obj.items()]


def body_of(slot):
    return slot.body if isinstance(slot, Ver) else slot


def world_of(node):
    cols = [floats(c) for _, c in node.transform.world_matrix.items()]
    m = np.array(cols, dtype=np.float64).T  # 列優先で保存されている
    pos = m[:3, 3]
    basis = m[:3, :3]
    scale = np.linalg.norm(basis, axis=0)
    rot = basis / np.where(scale == 0, 1, scale)
    return pos, rot, scale


def euler_xyz(deg):
    """glm::quat(radians(euler)) = Rz * Ry * Rx"""
    x, y, z = (math.radians(a) for a in deg)
    cx, sx, cy, sy, cz, sz = math.cos(x), math.sin(x), math.cos(y), math.sin(y), math.cos(z), math.sin(z)
    rx = np.array([[1, 0, 0], [0, cx, -sx], [0, sx, cx]])
    ry = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
    rz = np.array([[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]])
    return rz @ ry @ rx


class Grid:
    def __init__(self, body):
        self.min = floats(body['areaMin_'])
        self.max = floats(body['areaMax_'])
        self.nx = int(body['divisionsX_'])
        self.nz = int(body['divisionsZ_'])
        self.top = float(body['samplingHeight_'])
        self.dist = float(body['rayDistance_'])
        self.mask = int(body['layerMask_'])
        self.cx = self.min[0] + (self.max[0] - self.min[0]) * (np.arange(self.nx) + 0.5) / self.nx
        self.cz = self.min[1] + (self.max[1] - self.min[1]) * (np.arange(self.nz) + 0.5) / self.nz
        self.height = np.full((self.nz, self.nx), np.finfo(np.float32).min, dtype=np.float64)

    def window(self, x0, x1, z0, z1):
        i0, i1 = np.searchsorted(self.cx, [x0, x1])
        j0, j1 = np.searchsorted(self.cz, [z0, z1])
        return slice(max(j0, 0), min(j1, self.nz)), slice(max(i0, 0), min(i1, self.nx))

    def hit(self, rows, cols, y):
        """y は当たった高さ (当たらないセルは -inf)。レイの届く範囲だけ、今より高いものを採る"""
        y = np.where(y >= self.top - self.dist, y, -np.inf)
        cur = self.height[rows, cols]
        self.height[rows, cols] = np.maximum(cur, y)

    def obb(self, center, rot, half):
        """真下へのレイと OBB の交差。center/half は world、rot は 3x3"""
        r = np.abs(rot) @ half
        rows, cols = self.window(center[0] - r[0], center[0] + r[0], center[2] - r[2], center[2] + r[2])
        if rows.start >= rows.stop or cols.start >= cols.stop:
            return 0
        X, Z = np.meshgrid(self.cx[cols], self.cz[rows])
        o = np.stack([X - center[0], np.full_like(X, self.top - center[1]), Z - center[2]], -1) @ rot  # local = R^T (o - c)
        d = np.array([0.0, -1.0, 0.0]) @ rot
        t_enter = np.full(X.shape, -np.inf)
        t_exit = np.full(X.shape, np.inf)
        for k in range(3):
            if abs(d[k]) < 1e-12:
                outside = np.abs(o[..., k]) > half[k]
                t_enter = np.where(outside, np.inf, t_enter)
                continue
            t1 = (-half[k] - o[..., k]) / d[k]
            t2 = (half[k] - o[..., k]) / d[k]
            t_enter = np.maximum(t_enter, np.minimum(t1, t2))
            t_exit = np.minimum(t_exit, np.maximum(t1, t2))
        ok = (t_enter <= t_exit) & (t_exit >= 0)
        t = np.clip(t_enter, 0, None)
        self.hit(rows, cols, np.where(ok, self.top - t, -np.inf))
        return int(ok.sum())

    def cylinder(self, x, z, radius, top):
        rows, cols = self.window(x - radius, x + radius, z - radius, z + radius)
        if rows.start >= rows.stop or cols.start >= cols.stop:
            return 0
        X, Z = np.meshgrid(self.cx[cols], self.cz[rows])
        inside = (X - x) ** 2 + (Z - z) ** 2 <= radius * radius
        self.hit(rows, cols, np.where(inside, min(top, self.top), -np.inf))
        return int(inside.sum())


def collider_parts(comp):
    base = body_of(comp.data['value0'])
    return (floats(base['offset_']), floats(base['offsetRotation_']), int(base['layer_'].value),
            bool(body_of(base['value0'])['isEnable_']))


def bake(grid, scene):
    t = Terrain()
    X, Z = np.meshgrid(grid.cx, grid.cz)
    grid.hit(slice(None), slice(None), t.height(X, Z))
    boxes = rocks = 0
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
            name = node.name
            if name in ROCK_SIZE and any(c.name == 'Model' for c in node.transform.children):
                model = next(c for c in node.transform.children if c.name == 'Model')
                if not any(c.fqn.endswith('::StaticMeshCollider') for c in model.components):
                    continue
                pos, rot, scale = world_of(node)
                r, h = ROCK_SIZE[name]
                s = float(scale[0])
                if name == 'RuinedRockFence':
                    half = np.array([FENCE_SIZE[0] / 2, h / 2, FENCE_SIZE[1] / 2]) * s
                    grid.obb(pos + rot @ np.array([0.0, h * s / 2, 0.0]), rot, half)
                else:
                    grid.cylinder(pos[0], pos[2], r * s * 0.8, pos[1] + h * s)
                rocks += 1
    print(f'  terrain + {boxes} box colliders + {rocks} rocks')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    raw = META.read_bytes()
    crlf = b'\r\n' in raw[:200]
    meta = json.loads(raw.decode('utf-8-sig'))
    body = meta['value0']['ptr_wrapper']['data']
    grid = Grid(body)
    scene = reader.read_scene_file(SCENE)
    bake(grid, scene)

    h = grid.height.astype(np.float32).ravel()
    change = np.flatnonzero(np.diff(h) != 0) + 1
    starts = np.concatenate([[0], change])
    lengths = np.diff(np.concatenate([starts, [len(h)]]))
    print(f'  {len(h)} cells, {len(starts)} runs, height {h.min():.1f}..{h.max():.1f}')
    if args.dry_run:
        return

    # 旧データのキーを捨てて、同じ並びで書き直す
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
