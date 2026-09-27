"""古竜の巣の小物プレハブ (岩の爪・竜の骸・頭骨・竜撃ちの銛・積まれた心臓・嵐の壁) を組む。

    python tools/art/nest_prefabs.py

モデルは Assets/Art/Models/Nest (nest_models_blender.py と NanamiAssetsWork/Nest/process_nest.py から tools.model で変換。
出典は docs/ThirdPartyAssets.md)。砂漠と同じく「ルート(scale 1) + Model 子(scale 0.08)」で world = m x 8、原点は底面の中心。
心臓 (NestHeart*) だけは IslandHeart の結晶と同じく world 単位のモデルなので Model 子は scale 1。

- 岩の爪・竜の骸・頭骨: Model 子に StaticMeshCollider
- 銛・心臓・嵐の壁: 当たり判定なし (銛は細く、心臓は山の上、壁は遠い)

.meta (asset guid) は既存があれば保つので、組み直してもシーン側の参照は切れない。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num  # noqa: E402
from tools.scene import edits  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab, save_prefab  # noqa: E402
from grassland_nature_prefabs import model_child, static_mesh_collider  # noqa: E402

MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'Nest'
HEART_MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'IslandHeart'
PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Nest'

# 名前: 当たり判定の簡略化の許容誤差 world
MESH_SOLID = {
    'NestSpireA': 1.0,
    'NestSpireB': 1.0,
    'NestSpireC': 1.0,
    'NestSkeleton': 2.0,
    'NestSkull': 1.0,
}
NO_COLLIDER = ['NestHarpoon', 'NestStormWall']
# 積まれた心臓: 名前 -> モデル。緑・光・火は拠点の島や狩り場の心臓と同じ結晶、潮 (青) と宵 (紫) は巣にしかない色
HEARTS = {
    'NestHeartGreen': HEART_MODELS / 'GreenCoreShard.mv1.meta',
    'NestHeartLight': HEART_MODELS / 'LightCoreShard.mv1.meta',
    'NestHeartFire': HEART_MODELS / 'FireCoreShard.mv1.meta',
    'NestHeartTide': MODELS / 'TideCoreShard.mv1.meta',
    'NestHeartDusk': MODELS / 'DuskCoreShard.mv1.meta',
}


def build_mesh_solid(name, max_error):
    prefab = new_prefab(name)
    b = Builder(prefab)
    node = model_child(b, prefab, asset_guid(MODELS / f'{name}.mv1.meta'))
    collider = static_mesh_collider()
    collider.data['maxSimplifyError_'] = Num.of_float(max_error)
    node.components.append(collider)
    return save_prefab(prefab, PREFAB_DIR, name)


def build_plain(name):
    prefab = new_prefab(name)
    b = Builder(prefab)
    model_child(b, prefab, asset_guid(MODELS / f'{name}.mv1.meta'))
    return save_prefab(prefab, PREFAB_DIR, name)


def build_heart(name, meta):
    prefab = new_prefab(name)
    b = Builder(prefab)
    node = edits.add_gameobject(prefab, parent=prefab.root.guid, name='Model', pos=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0))
    b.component(node, 'ModelRenderer', mv1File_=asset_guid(meta), useFixedInterpolation_='false')
    return save_prefab(prefab, PREFAB_DIR, name)


def main():
    PREFAB_DIR.mkdir(parents=True, exist_ok=True)
    for name, err in MESH_SOLID.items():
        build_mesh_solid(name, err)
    for name in NO_COLLIDER:
        build_plain(name)
    for name, meta in HEARTS.items():
        build_heart(name, meta)


if __name__ == '__main__':
    main()
