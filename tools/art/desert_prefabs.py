"""砂漠ステージの小物プレハブ (岩・城塞・神殿・竜の骨・サボテン・ヤシ) を組む。

    python tools/art/desert_prefabs.py

モデルは Assets/Art/Models/Desert (Sketchfab の .glb から NanamiAssetsWork/Desert/process_props.py で m 単位に切り出し、
tools.model で変換。出典は docs/ThirdPartyAssets.md)。草原の岩と同じく「ルート(scale 1) + Model 子(scale 0.08)」で、
world = m x 8。原点は底面の中心。

- 岩・城塞・神殿・竜の骨: Model 子に StaticMeshCollider (grassland_nature_prefabs と同じ scale_ 100 / offsetRotation_ 90)
- サボテン・ヤシ: ルートに幹だけの BoxCollider。低いヤシ (PalmBent) は当たり判定なし

.meta (asset guid) は既存があれば保つので、組み直してもシーン側の参照は切れない。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab, save_prefab  # noqa: E402
from grassland_nature_prefabs import box_collider, model_child, static_mesh_collider  # noqa: E402

MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'Desert'
PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Desert'
M = 8.0  # world / m

# 名前: (モデルのフォルダ, 当たり判定の簡略化の許容誤差 world)
MESH_SOLID = {
    'DesertMesa': ('Rocks', 3.0),
    'SandstoneCliff': ('Rocks', 1.0),
    'SandstonePillar': ('Rocks', 1.0),
    'FortressWallTall': ('Fortress', 1.0),
    'FortressTower': ('Fortress', 1.0),
    'FortressWallAngled': ('Fortress', 1.0),
    'FortressWallAngledB': ('Fortress', 1.0),
    'FortressWallEnd': ('Fortress', 1.0),
    'FortressWallBlock': ('Fortress', 1.0),
    'FortressWallBlockB': ('Fortress', 1.0),
    'FortressWallBlockC': ('Fortress', 1.0),
    'FortressGate': ('Fortress', 1.0),
    'FortressBalcony': ('Fortress', 1.0),
    'FortressSpire': ('Fortress', 1.0),
    'FortressArch': ('Fortress', 0.5),
    'FortressArchSmall': ('Fortress', 0.5),
    'SunRing': ('Fortress', 1.0),
    'DesertTemple': ('Fortress', 1.0),
    'DragonBones': ('Props', 1.0),
}
# 名前: (幹の太さ m, 高さ m)。当たり判定は幹だけ
TRUNK = {
    'CactusCluster': (1.2, 6.5),
    'CactusGroup': (1.2, 4.0),
    'CactusSmall': (0.5, 2.3),
    'CactusTall': (0.45, 4.7),
    'CactusArm': (0.5, 4.8),
    'PalmA': (0.5, 7.9),
    'PalmB': (0.5, 7.7),
}
NO_COLLIDER = ['PalmBent']


def build_mesh_solid(name, folder, max_error):
    prefab = new_prefab(name)
    b = Builder(prefab)
    node = model_child(b, prefab, asset_guid(MODELS / folder / f'{name}.mv1.meta'))
    collider = static_mesh_collider()
    collider.data['maxSimplifyError_'] = Num.of_float(max_error)
    node.components.append(collider)
    return save_prefab(prefab, PREFAB_DIR, name)


def build_trunk(name, width, height):
    prefab = new_prefab(name)
    b = Builder(prefab)
    size = (width * M, height * M, width * M)
    prefab.root.components.append(box_collider(size, (0.0, size[1] / 2, 0.0)))
    model_child(b, prefab, asset_guid(MODELS / 'Plants' / f'{name}.mv1.meta'))
    return save_prefab(prefab, PREFAB_DIR, name)


def build_plain(name):
    prefab = new_prefab(name)
    b = Builder(prefab)
    model_child(b, prefab, asset_guid(MODELS / 'Plants' / f'{name}.mv1.meta'))
    return save_prefab(prefab, PREFAB_DIR, name)


def main():
    PREFAB_DIR.mkdir(parents=True, exist_ok=True)
    for name, (folder, err) in MESH_SOLID.items():
        build_mesh_solid(name, folder, err)
    for name, (width, height) in TRUNK.items():
        build_trunk(name, width, height)
    for name in NO_COLLIDER:
        build_plain(name)


if __name__ == '__main__':
    main()
