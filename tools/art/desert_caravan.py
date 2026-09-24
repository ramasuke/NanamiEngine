"""砂漠ステージのオアシスに足止めされた隊商の4人 (隊商頭・水守りの娘・駱駝番の少年・はぐれた護衛) を作る。

    python tools/art/desert_caravan.py models [--role Master ...]   # <work>/mixamo の Mixamo FBX -> Assets の .mv1
    python tools/art/desert_caravan.py data                         # AnimTree を作り直す (GUID は保つ)
    python tools/art/desert_caravan.py place                        # DesertScene の Caravan ルートに4人とクノイチを置き直す

- 作り方は草原の野営地 (camp_people.py) と同じ。見た目と動きは Mixamo で、キャラごとに T ポーズと、
  同じキャラで書き出したクリップ2本 (FBX Binary / Without Skin / 30 fps) を <work>/mixamo/<Role>/ に置く。
  T ポーズの FBX はテクスチャを中に持っているので、先に Blender で <Role>.fbm/ へ取り出しておく
  (<work>/mixamo/unpack.py)。<work> は既定で %USERPROFILE%\\NanamiAssetsWork\\Desert。
- AnimTree は State 0 = 待機 / 1 = 会話 (story_npcs.py の BT と揃える)。
- 会話と BT は story_npcs.py --only desert が物語の進み具合に合わせて作る (place の前に流す)。
- place は草原の野営地 (camp_people.place) と同じく、拠点の島の仲介人 (CharacterBrokerNpc) を写して NPC にする。
  クノイチは序章の Kunoichi-Adventure の見た目 (モデル・AnimTree・当たり判定・大きさ) を借りる (story_npcs.place_newcomers と同じ)。
  置き場所は desert_terrain.py の配置の目安に合わせた。護衛は城塞の東の割れ目の外で座り込んでいる。
"""
import argparse
import os
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common import meta_base  # noqa: E402

from camp_people import (CLIP_FPS, EMISSIVE, SWITCH_ANY_TIME, TRANSITION_SECS, TREE_DIR, apply_ops, recreate,  # noqa: E402
                         run, shrink_textures)

WORK = Path(os.environ.get('NANAMI_DESERT_WORK', Path.home() / 'NanamiAssetsWork' / 'Desert')) / 'mixamo'
MODEL_DIR = Path('Assets/Art/Models/Caravan')
ANIM_DIR = Path('Assets/Art/Animation/Caravan')


@dataclass
class Role:
    key: str
    source: str       # Mixamo のキャラ名
    display: str      # FriendlyNpc.name_ (会話の名前欄)
    base: str         # State 0 のクリップ
    talk: str         # State 1 のクリップ
    # 以下は place 用 (camp_people.Role と同じ意味)
    pos: tuple = (0.0, 0.0)
    face: tuple = (0.0, 0.0)
    scale: float = 0.1
    capsule: tuple = ((0.0, 90.0, 0.0), 30.0, 116.0)
    icon_y: float = 212.0


# 足元の (x, z) と向く先。desert_terrain.py の CAMP (420, 1000) / OASIS (600, 1040) / PLAZA (880, 420)
ROLES = [
    Role('Master', 'Heraklios By A. Dizon', '隊商頭', 'BreathingIdle', 'Talking',
         pos=(405.0, 1025.0), face=(420.0, 1000.0)),
    Role('Keeper', 'Arissa', '水守りの娘', 'SadIdle', 'Talking',
         pos=(515.0, 1030.0), face=(600.0, 1040.0)),
    Role('Boy', 'Kaya', '駱駝番の少年', 'HappyIdle', 'Talking',
         pos=(455.0, 975.0), face=(380.0, 1150.0)),
    Role('Guard', 'Castle Guard 01', '隊商の護衛', 'SittingDazed', 'SittingFloor',
         pos=(1095.0, 440.0), face=(1180.0, 470.0), capsule=((0.0, 48.0, -15.0), 25.0, 46.0), icon_y=135.0),
]
# クノイチ: 竜の骨 (desert_terrain.BONES) の南で、骨の方を向いて立つ
KUNOICHI = ('DesertKunoichi', 'Kunoichi-Adventure', (1060.0, 1000.0), (1100.0, 900.0))


def tree_name(r):
    return f'Caravan{r.key}'


def models(roles):
    for r in roles:
        src = WORK / r.key
        out = src / 'out'
        mv1 = out / f'{r.key}.mv1'
        print(f'{r.key} ({r.source})')
        run('tools.model', 'convert', src / f'{r.key}.fbx', mv1, '--mode', 'mesh', '--with-textures',
            '--emissive', EMISSIVE, '--timeout', '300', '--force')
        shrink_textures(out / f'{r.key}.fbm')
        run('tools.model', 'install', mv1, '--dest', (MODEL_DIR / f'{r.key}.mv1').as_posix(), '--with-textures')
        for clip in (r.base, r.talk):
            clip_mv1 = out / f'{clip}.mv1'
            run('tools.model', 'convert', src / f'{clip}.fbx', clip_mv1, '--mode', 'anim', '--timeout', '300', '--force')
            run('tools.model', 'install', clip_mv1, '--dest', (ANIM_DIR / r.key / f'{clip}.mv1').as_posix())


def anim_tree(r):
    import subprocess
    name = tree_name(r)
    guid = recreate('tools.animtree', name, TREE_DIR / f'{name}.animTree', TREE_DIR / f'{name}.animTree.meta')
    shown = subprocess.run([sys.executable, '-m', 'tools.animtree', 'show', f'Assets/Animations/{name}.animTree'],
                           cwd=REPO, check=True, capture_output=True, text=True).stdout
    entry = next(line.split()[1] for line in shown.splitlines() if line.startswith('Entry'))
    any_state = next(line.split()[1] for line in shown.splitlines() if line.startswith('AnyState'))
    idle, talk = meta_base.mint_guid(), meta_base.mint_guid()
    ops = [
        {'op': 'add-clip-node', 'name': 'Idle', 'clip': (ANIM_DIR / r.key / f'{r.base}.mv1').as_posix(),
         'speed': CLIP_FPS, 'blend_offset_secs': SWITCH_ANY_TIME, 'is_loop': True, 'guid': idle, 'pos': [276, 380]},
        {'op': 'add-clip-node', 'name': 'Talk', 'clip': (ANIM_DIR / r.key / f'{r.talk}.mv1').as_posix(),
         'speed': CLIP_FPS, 'blend_offset_secs': SWITCH_ANY_TIME, 'is_loop': True, 'guid': talk, 'pos': [276, 260]},
        {'op': 'add-param', 'name': 'State', 'kind': 'int', 'value': 0},
        {'op': 'add-transition', 'from': entry, 'next': idle},
    ]
    for node, state in ((idle, 0), (talk, 1)):
        ops.append({'op': 'add-transition', 'from': any_state, 'next': node, 'any_state': True,
                    'duration_secs': TRANSITION_SECS})
        ops.append({'op': 'add-condition', 'any_state': True, 'from': any_state, 'next': node,
                    'name': 'State', 'kind': 'int', 'value': state})
    apply_ops('tools.animtree', f'Assets/Animations/{name}.animTree', ops)
    return guid


def data(roles):
    for r in roles:
        print(f'{r.key}: {anim_tree(r)}')


def place(roles):
    import copy
    import math

    from camp_people import ICON_WORLD_SCALE, ICON_X, VersionFixer, npc_from_template, set_vec3, yaw_facing
    from game_over_prefab import asset_guid, check, let_writer_place_versions
    from grassland_nature_scatter import bake_world_matrices, first_versions, quat_axis, walk
    from tools.common.cereal_json import Num, dumps, loads, read_text, to_file_bytes
    from tools.scene import catalog as catalog_mod, edits, reader, validate, writer
    from desert_scene import SCENE, Terrain
    from story_npcs import MAIN_ISLAND, PROLOGUE_SCENE, TEMPLATE_NPC, field_guid

    bt_dir = REPO / 'Assets' / 'Data' / 'FriendlyNpcBehviour'
    t = Terrain()
    scene = reader.read_scene_file(SCENE)
    root = next(r for r in scene.roots if r.name == 'Caravan')
    root.transform.children = []
    source = reader.read_scene_file(MAIN_ISLAND)
    template = next(n for r in source.roots for n in walk(r) if n.name == TEMPLATE_NPC)

    for r in roles:
        guids = {'model': asset_guid(REPO / MODEL_DIR / f'{r.key}.mv1.meta'),
                 'tree': asset_guid(TREE_DIR / f'{tree_name(r)}.animTree.meta'),
                 'bt': asset_guid(bt_dir / f'{tree_name(r)}.friendBehaviourData.meta')}
        node, _facing, _ground = npc_from_template(template, r, guids, t)
        node.name = tree_name(r)
        root.transform.children.append(node)

    # クノイチ (序章の Kunoichi-Adventure の見た目。縮小された飛行船の子なので親の scale も掛ける)
    key, source_name, pos, face = KUNOICHI
    prologue = reader.read_scene_file(PROLOGUE_SCENE)

    def rec(node, scale):
        scale *= float(node.transform.local_scale.x.value)
        if node.name == source_name:
            return node, scale
        for child in node.transform.children:
            found = rec(child, scale)
            if found:
                return found
        return None
    src_node, scale = next(f for f in (rec(r, 1.0) for r in prologue.roots) if f)
    src = {c.fqn.rsplit('::', 1)[-1]: c.data for c in src_node.components}
    src_icon = next(c for c in src_node.transform.children if c.name == 'BillBoardNpcChatIcon')
    node = copy.deepcopy(template)
    remap = {}
    edits._remint_guids(node, remap)
    edits._remap_guid_references(node, remap)
    node.name = key
    node.transform.children = [c for c in node.transform.children if c.name == 'BillBoardNpcChatIcon']
    yaw, _ = yaw_facing(pos, face)
    node.transform.local_pos = edits._vec3_from_floats((pos[0], t.height(*pos) + 1.0, pos[1]))
    node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), yaw))
    node.transform.local_scale = edits._vec3_from_floats((scale,) * 3)
    for comp in node.components:
        leaf = comp.fqn.rsplit('::', 1)[-1]
        if leaf == 'FriendlyNpc':
            comp.data['name_'] = src['FriendlyNpc']['name_']
            edits._set_field_guid(comp.data['friendlyNpcBehaviourFile_'],
                                  asset_guid(bt_dir / f'{key}.friendBehaviourData.meta'))
        elif leaf == 'ModelRenderer':
            edits._set_field_guid(comp.data['mv1File_'], field_guid(src['ModelRenderer']['mv1File_']))
        elif leaf == 'Animator':
            edits._set_field_guid(comp.data['animationTreeFile_'], field_guid(src['Animator']['animationTreeFile_']))
        elif leaf == 'CapsuleCollider':
            base = comp.data['value0']
            base = base.body if hasattr(base, 'body') else base
            src_base = src['CapsuleCollider']['value0']
            src_base = src_base.body if hasattr(src_base, 'body') else src_base
            set_vec3(base['offset_'], [float(src_base['offset_'][k].value) for k in ('value0', 'value1', 'value2')])
            comp.data['radius_'] = Num.of_float(float(src['CapsuleCollider']['radius_'].value))
            comp.data['height_'] = Num.of_float(float(src['CapsuleCollider']['height_'].value))
    icon = node.transform.children[0]
    icon.transform.local_pos = edits._vec3_from_floats((ICON_X, float(src_icon.transform.local_pos.y.value), 0.0))
    icon.transform.local_scale = edits._vec3_from_floats((ICON_WORLD_SCALE / scale,) * 3)
    root.transform.children.append(node)
    print(f'  {key:20s} ({pos[0]:7.1f}, {t.height(*pos):6.1f}, {pos[1]:7.1f})  yaw {math.degrees(yaw):6.1f}  scale {scale}')

    for n in walk(root):
        for comp in n.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root)
    text = writer.write_scene(scene)
    tree = loads(text)
    index = next(i for i, r in enumerate(scene.roots) if r.name == 'Caravan')
    fixer = VersionFixer(catalog_mod.load(), f'/gameObject_{index}/', first_versions(read_text(MAIN_ISLAND)))
    fixer.run(tree, '')
    text = dumps(tree)
    print(f'  versions: added {sorted(set(fixer.added))}, stripped {fixer.stripped} repeat key(s)')
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({len(roles) + 1} people under Caravan)')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('step', choices=['models', 'data', 'place'])
    ap.add_argument('--role', action='append')
    a = ap.parse_args()
    roles = [r for r in ROLES if not a.role or r.key in a.role]
    if a.step == 'place':
        place(ROLES)
        return
    {'models': models, 'data': data}[a.step](roles)


if __name__ == '__main__':
    main()
