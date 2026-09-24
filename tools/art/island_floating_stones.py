"""島の底の浮遊石 (docs/Story.md: 島は底に埋まった浮遊石の力で浮いている)。島ごとに色が違う。

    python tools/art/island_floating_stones.py              # prefab を組み、2つのシーンに置き直す
    python tools/art/island_floating_stones.py --no-place   # prefab だけ組む
    python tools/art/island_floating_stones.py --textures   # Sky / Pearl / Rose の Core_Crystal_<Color>.png を作り直す (モデルの変換の前)

- Assets/Prefab/Prop/Story/<Color>IslandStone.prefab : Stone (IslandFloatingStone_<Color>.mv1) + <Color>IslandAura
- 石は島の尖った底に根元を埋めて、底から結晶が突き出して見えるように置く (pos はモデルの原点 = 埋める深さの目安)
- FirstTouchDownMainIsLandScene: 落ちる島 (Second = 緑 / Third = 光) の石は抜かれて草原・砂漠へ飛んでいく
  (昔は拠点の島の心臓の地中に埋めた IslandHeartStones から抜けていた。そのシーンの root は消す)。
  <Island>StoneRoot の子が1つの石で、FirstEventDragon の ScatterFloatingStones がそれを飛ばす
  (riseHeight_ を負にすると下へ抜け、root から石への水平の向きへ飛ぶ)。石の子に <Color>StoneFlight (Manual) を付ける
- 両シーン: 浮いたまま残る島 (拠点の島 = 紅 / Training = 蒼 / Forth = 白) の底に石を1つずつ
置き直すときは前の IslandFloatingStones をシーンから消してから置く。
モデルは tools/art/island_floating_stone_blender.py、エフェクトは tools/art/green_core_effect.py (ISLAND_STONES)。
"""
import argparse
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, to_file_bytes  # noqa: E402
from tools.scene import edits, reader, validate, writer  # noqa: E402

from event_board_prefab import bake_rotated_world_matrices, strip_repeat_versions  # noqa: E402
from game_over_prefab import Builder, asset_guid, check, let_writer_place_versions, new_prefab  # noqa: E402
from grassland_nature_scatter import quat_axis  # noqa: E402
from restoration_sites import all_nodes  # noqa: E402
from tools.scene import meta as scene_meta  # noqa: E402

PREFAB_DIR = REPO / 'Assets/Prefab/Prop/Story'
MODEL_DIR = REPO / 'Assets/Art/Models/IslandHeart'
PARTICLE_DIR = REPO / 'Assets/Prefab/Particle'
COLORS = ['Green', 'Light', 'Sky', 'Pearl', 'Rose']
# 色 -> 緑の Core_Crystal.png からの色替え (PIL の HSV の色相 0..255, 彩度の倍率, 明度の倍率, 明度の足し分)。
# Green / Light は物語の石のテクスチャ (Core_Crystal.png / Core_Crystal_Light.png) をそのまま使う
RECOLORS = {
    'Sky': (145, 0.85, 1.0, 0),
    'Pearl': (150, 0.12, 0.8, 55),
    'Rose': (242, 0.8, 1.0, 0),
}
OLD_HEART_STONES = 'IslandHeartStones'
GREEN_HUE = 102              # Core_Crystal.png の色相の平均
FIRST_TOUCH_DOWN = REPO / 'Assets/Scene/FirstTouchDownMainIsLandScene.scene'
MAIN_ISLAND = REPO / 'Assets/Scene/MainIslandScene.scene'
GROUP_NAME = 'IslandFloatingStones'
MANUAL = 2                   # ParticleSystem PlayMode: ScatterFloatingStones が飛び立つときに Play する
MAIN_ISLAND_CENTER = (-18.8, 57.1)   # 石が飛んでいく向きの基準 (拠点の島の心臓の真上)
ROOT_BACK = 10.0             # 抜かれる石の root を石から拠点側へずらす量。この向きへ飛ぶ

# pos はモデルの原点 (ワールド座標、エディタで見て決めた)。その上 30 * scale ほどが島の岩に埋まる。yaw は度
STONES = [
    # root_guid は FirstEventDragon の ScatterFloatingStones が指すので、置き直しても変えない
    dict(name='SecondIsland', color='Green', pos=(-279.0, -203.0, 484.0), scale=2.0, yaw=20.0, falls=True,
         root_guid='B2908990-9972-4F05-B62E-046D55C07DFC'),
    dict(name='ThirdIsland', color='Light', pos=(269.0, -75.0, 654.0), scale=1.7, yaw=75.0, falls=True,
         root_guid='56373276-EDF2-4997-A32E-C3B401C19F89'),
    # 拠点の島。心臓の緑・光は序章で落ちる島の石に移したので、自分の石は紅
    dict(name='FirstIsland', color='Rose', pos=(-20.0, -150.0, 60.0), scale=2.2, yaw=0.0, falls=False),
    dict(name='TrainingIsland', color='Sky', pos=(156.0, -162.0, 281.0), scale=1.4, yaw=140.0, falls=False),
    dict(name='ForthIsland', color='Pearl', pos=(-64.0, 97.0, 340.0), scale=1.3, yaw=250.0, falls=False),
]


def make_textures():
    import numpy as np
    from PIL import Image
    src = np.array(Image.open(MODEL_DIR / 'Core_Crystal.png').convert('RGB').convert('HSV')).astype(float)
    for color, (hue, sat, val, val_add) in RECOLORS.items():
        hsv = src.copy()
        hsv[..., 0] = (hsv[..., 0] - GREEN_HUE + hue) % 256
        hsv[..., 1] = np.clip(hsv[..., 1] * sat, 0, 255)
        hsv[..., 2] = np.clip(hsv[..., 2] * val + val_add, 0, 255)
        Image.fromarray(hsv.astype('uint8'), 'HSV').convert('RGB').save(MODEL_DIR / f'Core_Crystal_{color}.png')
        print(f'wrote Core_Crystal_{color}.png')


def prefab_path(color):
    return PREFAB_DIR / f'{color}IslandStone.prefab'


def yaw_rot(deg):
    return edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), math.radians(deg)))


def build_prefab(color):
    name = f'{color}IslandStone'
    prefab = new_prefab(name)
    b = Builder(prefab)
    stone = edits.add_gameobject(prefab, parent=prefab.root.guid, name='Stone')
    comp = b.component(stone, 'ModelRenderer')
    b.field(comp, 'mv1File_', asset_guid(MODEL_DIR / f'IslandFloatingStone_{color}.mv1.meta'))
    edits.instantiate_prefab(prefab, reader.read_prefab_file(PARTICLE_DIR / f'{color}IslandAura.prefab'),
                             parent=stone.guid)

    path = prefab_path(color)
    for node in all_nodes(prefab.root):
        for c in node.components:
            let_writer_place_versions(c.data)
    bake_rotated_world_matrices(prefab.root)
    text = strip_repeat_versions(writer.write_prefab(prefab), '/transform_/child/')
    check(text, validate.validate_prefab(prefab), path.name)
    path.write_bytes(to_file_bytes(text))
    meta = Path(str(path) + '.meta')
    if not meta.exists():
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, name, PREFAB_DIR, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, name, guid, content_path)
    reader.read_prefab_file(path)
    print(f'wrote {path.relative_to(REPO)}  ({asset_guid(meta)})')
    return path


def find_child(node, name):
    return next(c for c in node.transform.children if c.name == name)


def add_stone(scene, parent, spec, falls):
    stone_prefab = reader.read_prefab_file(prefab_path(spec['color']))
    x, y, z = spec['pos']
    if not falls:
        node = edits.instantiate_prefab(scene, stone_prefab, parent=parent.guid)
        node.name = f"{spec['name']}Stone"
        node.transform.local_pos = edits._vec3_from_floats((x, y, z))
    else:
        dx, dz = x - MAIN_ISLAND_CENTER[0], z - MAIN_ISLAND_CENTER[1]
        length = math.hypot(dx, dz)
        dx, dz = dx / length, dz / length
        root = edits.add_gameobject(scene, parent=parent.guid, name=f"{spec['name']}StoneRoot",
                                    pos=(x - dx * ROOT_BACK, y, z - dz * ROOT_BACK))
        root.guid = spec['root_guid']
        node = edits.instantiate_prefab(scene, stone_prefab, parent=root.guid)
        node.name = f"{spec['name']}Stone"
        node.transform.local_pos = edits._vec3_from_floats((dx * ROOT_BACK, 0.0, dz * ROOT_BACK))
        flight = edits.instantiate_prefab(scene, reader.read_prefab_file(PARTICLE_DIR / f"{spec['color']}StoneFlight.prefab"),
                                          parent=find_child(node, 'Stone').guid)
        flight.components[0].data['playMode_'] = Num.of_int(MANUAL)
    node.transform.local_rot = yaw_rot(spec['yaw'])
    s = spec['scale']
    node.transform.local_scale = edits._vec3_from_floats((s, s, s))


def place(scene_path, with_falling):
    scene = reader.read_scene_file(scene_path)
    scene.roots = [r for r in scene.roots if r.name not in (GROUP_NAME, OLD_HEART_STONES)]
    index = len(scene.roots)
    group = edits.add_gameobject(scene, parent=None, name=GROUP_NAME)
    for spec in STONES:
        if spec['falls'] and not with_falling:
            continue
        add_stone(scene, group, spec, spec['falls'])
    for node in all_nodes(group):
        for c in node.components:
            let_writer_place_versions(c.data)
    bake_rotated_world_matrices(group)

    text = strip_repeat_versions(writer.write_scene(scene), f'/gameObject_{index}/')
    check(text, validate.validate_scene(scene), scene_path.name)
    scene_path.write_bytes(to_file_bytes(text))
    placed = reader.read_scene_file(scene_path)
    group = next(r for r in placed.roots if r.name == GROUP_NAME)
    for child in group.transform.children:
        print(f'{scene_path.name}: {child.name} [{child.guid}]')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--no-place', action='store_true', help='prefab だけ組み、シーンには置かない')
    ap.add_argument('--textures', action='store_true', help='Core_Crystal_<Color>.png を作り直す')
    args = ap.parse_args()
    if args.textures:
        make_textures()
        return
    for color in COLORS:
        build_prefab(color)
    if not args.no_place:
        place(FIRST_TOUCH_DOWN, with_falling=True)
        place(MAIN_ISLAND, with_falling=False)


if __name__ == '__main__':
    main()
