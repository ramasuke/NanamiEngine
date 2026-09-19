"""GrassLand の自然物プレハブ(岩9種 + 茂み/花/倒木/切り株)を組む。

    python tools/art/grassland_nature_prefabs.py

モデルは Assets/Art/Models/Nature/Rocks, Props (Blender で m 単位に揃えて書き出し、tools.model で変換済み)。
木のプレハブと同じく「ルート(scale 1) + Model 子(scale 0.08)」で、world = m x 8 になる。

- 岩: Model 子に ModelRenderer + StaticMeshCollider (scale_ 100 / offsetRotation_ 90 = m 出力 FBX のフレーム変換を掛け直す)
- 茂み/花: Model 子に ModelRenderer + TreeLeafSway (材質名の _Leaf で揺れる)。当たり判定なし
- 倒木/切り株: ルートに BoxCollider

.meta(asset guid)は既存があれば保つので、組み直してもシーン側の参照は切れない。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab, save_prefab  # noqa: E402

MODEL_SCALE = 0.08
ROCK_DIR = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Rock'
NATURE_DIR = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Nature'
ROCK_MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'Nature' / 'Rocks'
PROP_MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'Nature' / 'Props'
TREE_VS = asset_guid(REPO / 'Assets/Art/Shaders/Tree/Tree_VS.vso.meta')
TREE_PS = asset_guid(REPO / 'Assets/Art/Shaders/Tree/Tree_PS.pso.meta')

ROCKS = ['RuinedRockFence', 'Rock17', 'SandyRock', 'FantasyRock', 'StylizedRock2',
         'DesertRockBase', 'StylizedRock', 'StylizedRock1', 'CyberpunkRock']
# 当たり判定の簡略化の許容誤差(world 単位)。既定の 5 は地形向けで、2m の岩(16 world)には粗すぎる
ROCK_SIMPLIFY_ERROR = 0.5
# m 出力 FBX は .mv1 のフレームに +90°X が入るが、StaticMeshCollider は生の頂点を読むので回転も掛け直す
ROCK_FRAME_ROTATION = (90.0, 0.0, 0.0)

# 名前: 揺れ幅(world 単位。木は 8)
SWAYING = {'Bush_A': 2.0, 'Bush_B': 1.5, 'FlowerPatch_A': 1.0, 'FlowerPatch_B': 1.0}
# 名前: (BoxCollider の size, offset)。モデルの外形(m) x 8 より少し細め
SOLID = {
    'FallenLog': ((28.0, 4.4, 4.2), (0.0, 2.2, 0.0)),
    'TreeStump': ((5.6, 4.8, 5.6), (0.0, 2.4, 0.0)),
}


def vec3_blob(v):
    return OrderedObj([(f'value{i}', Num.of_float(float(c))) for i, c in enumerate(v)])


def collider_base(offset, rotation):
    guid = edits.mint_guid().upper()
    guid_ver = Ver(('type', 'Guid'), 0, OrderedObj([('value_', guid)]))
    component_base = Ver(('type', 'ComponentBase'), 0, OrderedObj([('guid_', guid_ver), ('isEnable_', True)]))
    return Ver(('type', 'ColliderBase'), 6, OrderedObj([
        ('value0', component_base),
        ('offset_', vec3_blob(offset)),
        ('offsetRotation_', vec3_blob(rotation)),
        ('layer_', Num.of_int(0)),
        ('isSensor_', False),
        ('friction_', Num.of_float(0.2)),
    ]))


def box_collider(size, offset):
    """ColliderBase は tools.scene が組めないので、エンジンの save() の順に直接組む"""
    cat = catalog_mod.load()
    data = OrderedObj([('value0', collider_base(offset, (0.0, 0.0, 0.0)))])
    for i, leaf in enumerate(('IAwakable', 'IBeginPhysics', 'IEndPhysics'), start=1):
        data.append(f'value{i}', edits._empty_base_blob(leaf, (cat.base_info(leaf) or {}).get('version', 0)))
    data.append('size_', vec3_blob(size))
    return model.Component(fqn='NanamiEngine::Module::Component::BoxCollider', class_version=6, data=data)


def static_mesh_collider():
    data = OrderedObj([
        ('value0', collider_base((0.0, 0.0, 0.0), ROCK_FRAME_ROTATION)),
        ('scale_', vec3_blob((100.0, 100.0, 100.0))),
        ('simplifyEnabled_', True),
        ('maxSimplifyError_', Num.of_float(ROCK_SIMPLIFY_ERROR)),
        ('minTriangleRatio_', Num.of_float(0.05)),
    ])
    return model.Component(fqn='NanamiEngine::Module::Component::StaticMeshCollider', class_version=4, data=data)


def model_child(b, prefab, mv1_guid):
    node = edits.add_gameobject(prefab, parent=prefab.root.guid, name='Model', pos=(0.0, 0.0, 0.0),
                                scale=(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE))
    b.component(node, 'ModelRenderer', mv1File_=mv1_guid, useFixedInterpolation_='false')
    return node


def build_rock(name):
    prefab = new_prefab(name)
    b = Builder(prefab)
    node = model_child(b, prefab, asset_guid(ROCK_MODELS / f'{name}.mv1.meta'))
    node.components.append(static_mesh_collider())
    return save_prefab(prefab, ROCK_DIR, name)


def build_swaying(name, amplitude):
    prefab = new_prefab(name)
    b = Builder(prefab)
    node = model_child(b, prefab, asset_guid(PROP_MODELS / f'{name}.mv1.meta'))
    b.component(node, 'TreeLeafSway', vsFile_=TREE_VS, psFile_=TREE_PS, leafMaterialSuffix_='_Leaf',
                swayAmplitude_=amplitude, ambient_=0.35)
    return save_prefab(prefab, NATURE_DIR, name)


def build_solid(name, size, offset):
    prefab = new_prefab(name)
    b = Builder(prefab)
    prefab.root.components.append(box_collider(size, offset))
    model_child(b, prefab, asset_guid(PROP_MODELS / f'{name}.mv1.meta'))
    return save_prefab(prefab, NATURE_DIR, name)


def main():
    for name in ROCKS:
        build_rock(name)
    for name, amplitude in SWAYING.items():
        build_swaying(name, amplitude)
    for name, (size, offset) in SOLID.items():
        build_solid(name, size, offset)


if __name__ == '__main__':
    main()
