"""GrassLandScene の北の棚のキャンプに、ティラノ (狩猟民の呼び名で「大顎」) に村を追われた避難民を4人置く。

    python tools/art/camp_people.py models [--role Elder ...]   # C:\\Temp\\CampPeople の Mixamo FBX -> Assets の .mv1
    python tools/art/camp_people.py data                        # AnimTree を作り直す (GUID は保つ)
    python tools/art/camp_people.py place                       # GrassLandScene の CampPeople ルートを置き直す

- 見た目と動きは Mixamo。キャラごとに T ポーズ (FBX Binary) と、同じキャラで書き出したクリップ2本
  (FBX Binary / With Skin / 30 fps) を C:\\Temp\\CampPeople\\<Role>\\<Role>.fbx, <クリップ>.fbx に置く。
  Mixamo のクリップは書き出したキャラでしか正しく動かず、DxLibModelViewer は日本語パスを開けない。
- AnimTree は State 0 = 待機 / 1 = 会話。AnyState 遷移は「クリップ長 - blendAnimationOffset」を過ぎてから判定されるので、
  offset を巨大にして毎フレーム判定させる (0 だと話しかけても今のクリップが終わるまで切り替わらない)。
- 会話 (.npcChat) と BT は story_npcs.py が物語の進み具合に合わせて作る。AnimTree の State 0 = 待機 / 1 = 会話はそちらと揃える。
- NPC は MainIslandScene の仲介人 (CharacterBrokerNpc) の複製。Dynamic + constraints 61 でないと会話センサーに拾われない。
  長老は capsule の下端を座面の高さにして、切り株 (TreeStump) の上に重力で乗せる。村娘は物資の山の前に背中を預けて床に座る。
- Settlement (settlement_scatter.py) とは別のルートなので、キャンプを置き直しても消えない。置き物を動かしたらこちらも見直す。
  抜いた草は戻らない。
"""
import argparse
import copy
import math
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, OrderedObj, dumps, loads, read_text, to_file_bytes  # noqa: E402
from tools.common import meta_base  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import SCENE, Terrain, bake_world_matrices, first_versions, quat_axis, walk  # noqa: E402

TEMP = Path(r'C:\Temp\CampPeople')
MODEL_DIR = Path('Assets/Art/Models/CampPeople')
ANIM_DIR = Path('Assets/Art/Animation/CampPeople')
TREE_DIR = REPO / 'Assets' / 'Animations'
BT_DIR = REPO / 'Assets' / 'Data' / 'FriendlyNpcBehviour'
CHAT_DIR = REPO / 'Assets' / 'Data' / 'NpcChatText'
MAIN_ISLAND = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
STUMP_PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Nature' / 'TreeStump.prefab'
GRASS_META = REPO / 'Assets' / 'Data' / 'GrassField' / 'GrassLandGrass.grassField.meta'

TEMPLATE_NPC = 'CharacterBrokerNpc'
ROOT_NAME = 'CampPeople'
EMISSIVE = '0.4,0.4,0.4'      # 仲介人と同じ
MAX_TEXTURE = 2048
CLIP_FPS = 30.0
TRANSITION_SECS = 0.3
SWITCH_ANY_TIME = 100000.0    # blendAnimationOffset_secs_。ドラゴン / ティラノ / ハイエナと同じ
FONT_CHAT = '02951627-F120-4EDC-B4F7-C27985C7F643'   # 会話ウィンドウの本文 (ZenOldMincho-Bold.ttf)
MAX_LINE_CHARS = 22

CAMPFIRE = (978.0, 1160.0)
BASIN = (840.0, 610.0)
SUPPLY_PILE_FRONT = (1043.2, 1164.5)   # 物資の山の当たり判定の正面 (焚き火側)
STUMP_BOX_HEIGHT = 4.8                 # TreeStump の BoxCollider の高さ (prefab scale 1)
STUMP_SINK = 0.2
ICON_WORLD_SCALE = 3.375               # 仲介人の吹き出しと同じ大きさ
ICON_X = 0.88


@dataclass
class Role:
    key: str
    source: str            # Mixamo のキャラ名
    display: str           # FriendlyNpc.name_ (会話の名前欄)
    base: str              # State 0 のクリップ (C:\Temp\CampPeople\<key>\<base>.fbx)
    talk: str              # State 1 のクリップ
    pos: tuple             # 足元の (x, z)
    face: tuple            # 向く先の (x, z)
    scale: float
    capsule: tuple         # (offset (x, y, z), radius, height)。モデルの単位 (cm)
    icon_y: float          # 吹き出しの高さ (モデルの単位)
    seat_cm: float         # 0 なら立ち/床。>0 なら切り株に腰掛ける座面の高さ (cm)
    seat_ahead: float      # 切り株の中心を足元からどれだけ前に置くか (world)
    clear_grass: float     # 足元の草を抜く半径 (world)。0 なら抜かない


ROLES = [
    Role('Elder', 'Abe', '狩猟民の長', 'SittingIdle', 'SittingTalking',
         pos=(972.0, 1188.0), face=CAMPFIRE, scale=0.1,
         capsule=((0.0, 91.5, 0.0), 28.0, 41.0), icon_y=175.0,
         seat_cm=43.0, seat_ahead=1.5, clear_grass=4.0),
    Role('Lookout', 'Morak', '見張りの若者', 'LookingAround', 'Talking',
         pos=(1003.0, 1112.0), face=BASIN, scale=0.1,
         capsule=((0.0, 90.0, 0.0), 35.0, 110.0), icon_y=215.0,
         seat_cm=0.0, seat_ahead=0.0, clear_grass=0.0),
    Role('Huntress', 'Erika Archer', '弓の狩人', 'NeutralIdle', 'Talking',
         pos=(1008.0, 1146.0), face=CAMPFIRE, scale=0.1,
         capsule=((0.0, 88.0, 0.0), 30.0, 116.0), icon_y=212.0,
         seat_cm=0.0, seat_ahead=0.0, clear_grass=0.0),
    Role('Wounded', 'Peasant Girl', 'けがをした村娘', 'SittingDazed', 'SittingFloor',
         pos=(1041.3, 1164.4), face=CAMPFIRE, scale=0.085,
         capsule=((0.0, 48.0, -15.0), 25.0, 46.0), icon_y=135.0,
         seat_cm=0.0, seat_ahead=0.0, clear_grass=9.0),
]


def run(*args):
    print('  $', ' '.join(str(a) for a in args))
    subprocess.run([sys.executable, '-m', *map(str, args)], cwd=REPO, check=True)


def asset_guid(meta_path):
    def find(node):
        if isinstance(node, OrderedObj):
            if 'guid_' in node:
                return node['guid_']['value_']
            for _, value in node.items():
                found = find(value)
                if found:
                    return found
        return None
    guid = find(loads(read_text(meta_path)))
    if not guid:
        raise SystemExit(f'no guid_ in {meta_path}')
    return guid


# ---------------------------------------------------------------- models
def shrink_textures(folder):
    for path in sorted(folder.glob('*')):
        if path.suffix.lower() not in ('.png', '.jpg', '.jpeg', '.tga'):
            continue
        im = Image.open(path)
        if max(im.size) <= MAX_TEXTURE:
            continue
        rate = MAX_TEXTURE / max(im.size)
        size = (round(im.size[0] * rate), round(im.size[1] * rate))
        im.resize(size, Image.LANCZOS).save(path)
        print(f'    {path.name}: {im.size} -> {size}')


def models(roles):
    for r in roles:
        src = TEMP / r.key
        fbx = src / f'{r.key}.fbx'
        if not fbx.exists() and (src / 'Character.fbx').exists():
            (src / 'Character.fbx').rename(fbx)
        out = src / 'out'
        mv1 = out / f'{r.key}.mv1'
        print(f'{r.key} ({r.source})')
        run('tools.model', 'convert', fbx, mv1, '--mode', 'mesh', '--with-textures', '--emissive', EMISSIVE,
            '--timeout', '300', '--force')
        shrink_textures(out / f'{r.key}.fbm')
        run('tools.model', 'install', mv1, '--dest', (MODEL_DIR / f'{r.key}.mv1').as_posix(), '--with-textures')
        for clip in (r.base, r.talk):
            clip_mv1 = out / f'{clip}.mv1'
            run('tools.model', 'convert', src / f'{clip}.fbx', clip_mv1, '--mode', 'anim', '--timeout', '300', '--force')
            run('tools.model', 'install', clip_mv1, '--dest', (ANIM_DIR / r.key / f'{clip}.mv1').as_posix())


# ---------------------------------------------------------------- data
def recreate(kind_module, name, data_path, meta_path, *extra):
    """new-tree --force で作り直し、前の GUID があれば .meta に戻す (シーンや BT からの参照を切らない)"""
    old = asset_guid(meta_path) if meta_path.exists() else None
    run(kind_module, 'new-tree', name, *extra, '--force')
    if old:
        new = asset_guid(meta_path)
        meta_path.write_bytes(read_text(meta_path).replace(new, old).encode('utf-8'))
        text = data_path.read_bytes()
        if new.encode() in text:
            data_path.write_bytes(text.replace(new.encode(), old.encode()))
    return asset_guid(meta_path)


def check_page(text):
    for line in text.split('\n'):
        if len(line) > MAX_LINE_CHARS:
            raise SystemExit(f'too long for the chat box ({len(line)} > {MAX_LINE_CHARS}): {line}')
    text.encode('cp932')   # DxLib へは Shift-JIS で渡るので、CP932 に無い字 (〜 など) は消える


def strip_versions(obj):
    if isinstance(obj, OrderedObj):
        for key in [k for k, _ in obj.items() if k == 'cereal_class_version']:
            obj.pop(key)
        for _, value in obj.items():
            strip_versions(value)
    return obj


def write_npc_chat(name, pages):
    """Merchant.npcChat.meta と同じ並びで書く。本体の .npcChat は 0 バイト、中身は .meta"""
    for page in pages:
        check_page(page)
    template = loads(read_text(CHAT_DIR / 'Merchant.npcChat.meta'))
    data = template['value0']['ptr_wrapper']['data']
    first_item = data['item_0']
    # NOTE: cereal は型ごとに初出だけ版を書くので、2件目以降は版の無い形にする
    other_item = data['item_1'] if 'item_1' in data.keys() else strip_versions(copy.deepcopy(first_item))

    meta_path = CHAT_DIR / f'{name}.npcChat.meta'
    guid = asset_guid(meta_path) if meta_path.exists() else meta_base.mint_guid()
    data['value0']['contentPath_'] = f'Assets\\Data\\NpcChatText/{name}.npcChat'
    data['value0']['guid_']['value_'] = guid

    items = []
    for i, page in enumerate(pages):
        item = copy.deepcopy(first_item if i == 0 else other_item)
        wrapper = item['font_']['value0']['ptr_wrapper']
        wrapper['id'] = Num.of_int(2147483650 + i)
        wrapper['data']['value0']['value_'] = FONT_CHAT
        item['text_'] = page
        items.append((f'item_{i}', item))
    rebuilt = OrderedObj([('cereal_class_version', data['cereal_class_version']), ('value0', data['value0']),
                          ('count', Num.of_int(len(pages)))] + items)
    template['value0']['ptr_wrapper']['data'] = rebuilt

    meta_path.write_bytes(to_file_bytes(dumps(template)).rstrip(b'\r\n'))
    (CHAT_DIR / f'{name}.npcChat').write_bytes(b'')
    return guid


def anim_tree(r):
    name = f'CampPeople{r.key}'
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


def apply_ops(kind_module, path, ops):
    import json
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.json', delete=False, encoding='utf-8') as f:
        json.dump(ops, f, ensure_ascii=False)
    try:
        run(kind_module, 'apply', path, f.name)
    finally:
        Path(f.name).unlink()


def data(roles):
    for r in roles:
        print(f'{r.key}')
        anim_tree(r)


# ---------------------------------------------------------------- scene
class VersionFixer(validate._ClassVersionAudit):
    """別のファイルから写した NPC の版キーを、このシーンでの初出/2回目に合わせる。
    写し元で2回目以降だった型はここで初出になると版キーが足りず、prefab の初出の版キーは2回目では余る"""

    def __init__(self, cat, owner_path, source_versions):
        super().__init__(cat)
        self.owner_path = owner_path
        self.source_versions = source_versions
        self.added = []
        self.stripped = 0

    def _check(self, type_key, node, where):
        if where.startswith(self.owner_path):
            if type_key not in self._first and validate._VER not in node:
                version = self.source_versions.get(type_key)
                if version is None:
                    raise SystemExit(f'{where}: no source version for {type_key}')
                node.insert(0, validate._VER, Num.of_int(int(version)))
                self.added.append(type_key)
            elif type_key in self._first and validate._VER in node:
                node.pop(validate._VER)
                self.stripped += 1
                return
        super()._check(type_key, node, where)


def yaw_facing(pos, target):
    """キャラクターは local -Z が正面: yaw θ の正面は (-sinθ, -cosθ)"""
    fx, fz = target[0] - pos[0], target[1] - pos[1]
    return math.atan2(-fx, -fz), np.array([fx, fz]) / math.hypot(fx, fz)


def set_vec3(obj, values):
    for i, v in enumerate(values):
        obj[f'value{i}'] = Num.of_float(float(v))


def npc_from_template(template, r, guids, t):
    node = copy.deepcopy(template)
    remap = {}
    edits._remint_guids(node, remap)
    edits._remap_guid_references(node, remap)
    node.name = f'CampPeople{r.key}'

    yaw, facing = yaw_facing(r.pos, r.face)
    ground = float(t.height(*r.pos))
    node.transform.local_pos = edits._vec3_from_floats((r.pos[0], ground + 1.0, r.pos[1]))
    node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), yaw))
    node.transform.local_scale = edits._vec3_from_floats((r.scale,) * 3)

    offset, radius, height = r.capsule
    for comp in node.components:
        leaf = comp.fqn.rsplit('::', 1)[-1]
        if leaf == 'FriendlyNpc':
            comp.data['name_'] = r.display
            edits._set_field_guid(comp.data['friendlyNpcBehaviourFile_'], guids['bt'])
        elif leaf == 'ModelRenderer':
            edits._set_field_guid(comp.data['mv1File_'], guids['model'])
        elif leaf == 'Animator':
            edits._set_field_guid(comp.data['animationTreeFile_'], guids['tree'])
        elif leaf == 'CapsuleCollider':
            base = comp.data['value0']
            base = base.body if hasattr(base, 'body') else base
            if 'mass_' in base:
                raise SystemExit('template CapsuleCollider is ColliderBase v5 - migrate it first')
            set_vec3(base['offset_'], offset)
            comp.data['radius_'] = Num.of_float(radius)
            comp.data['height_'] = Num.of_float(height)
        elif leaf == 'RigidBody':
            if int(comp.data['motionType_'].value) != 2 or int(comp.data['constraints_'].value) != 61:
                raise SystemExit('template RigidBody is no longer Dynamic + constraints 61')

    icon = next(c for c in node.transform.children if c.name == 'BillBoardNpcChatIcon')
    icon.transform.local_pos = edits._vec3_from_floats((ICON_X, r.icon_y, 0.0))
    icon.transform.local_scale = edits._vec3_from_floats((ICON_WORLD_SCALE / r.scale,) * 3)

    capsule_bottom = offset[1] - height / 2 - radius
    print(f'  {node.name:20s} ({r.pos[0]:7.1f}, {ground:6.1f}, {r.pos[1]:7.1f})  yaw {math.degrees(yaw):6.1f}  '
          f'slope {float(t.slope(*r.pos)):4.1f}  capsule {capsule_bottom:.0f}..{offset[1] + height / 2 + radius:.0f} cm')
    return node, facing, ground


def stump_under(scene, root, stump_prefab, r, facing, ground, t):
    """座面が seat_cm (world = seat_cm * scale) に来る大きさの切り株を、腰の下 (足元の少し前) に置く"""
    cx, cz = r.pos[0] + facing[0] * r.seat_ahead, r.pos[1] + facing[1] * r.seat_ahead
    base = t.lowest(cx, cz, 2.7) - STUMP_SINK
    seat = ground + r.seat_cm * r.scale
    scale = (seat - base) / STUMP_BOX_HEIGHT
    node = edits.instantiate_prefab(scene, stump_prefab, parent=root.guid)
    node.name = f'CampPeople{r.key}Seat'
    node.transform.local_pos = edits._vec3_from_floats((cx, base, cz))
    node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), math.radians(37.0)))
    node.transform.local_scale = edits._vec3_from_floats((scale,) * 3)
    print(f'  {node.name:20s} ({cx:7.1f}, {base:6.1f}, {cz:7.1f})  scale {scale:.2f}  seat top {seat:.1f}')
    return node


def clear_grass(roles):
    import base64
    root = loads(GRASS_META.read_bytes().decode('utf-8-sig'))
    body = root['value0']['ptr_wrapper']['data']
    chunk = float(body['chunkSize_'].value)
    spots = [(r.pos[0], r.pos[1], r.clear_grass) for r in roles if r.clear_grass > 0]
    removed = 0
    for rec in body['chunks']:
        cx, cz = int(rec['cx'].value), int(rec['cz'].value)
        raw = np.frombuffer(base64.b64decode(rec['blades']), dtype='<u2').reshape(-1, 3)
        x = (raw[:, 0] / 65535.0 + cx) * chunk
        z = (raw[:, 1] / 65535.0 + cz) * chunk
        keep = np.ones(len(raw), bool)
        for sx, sz, radius in spots:
            keep &= (x - sx) ** 2 + (z - sz) ** 2 > radius * radius
        if keep.all():
            continue
        removed += int((~keep).sum())
        rec['count'] = Num.of_int(int(keep.sum()))
        rec['blades'] = base64.b64encode(raw[keep].astype('<u2').tobytes()).decode('ascii')
    if removed:
        GRASS_META.write_bytes(to_file_bytes(dumps(root)))
    print(f'  grass: removed {removed} blades around seated people')


def place(roles):
    t = Terrain()
    scene = reader.read_scene_file(SCENE)
    scene.roots = [r for r in scene.roots if r.name != ROOT_NAME]
    source = reader.read_scene_file(MAIN_ISLAND)
    template = next((n for r in source.roots for n in walk(r) if n.name == TEMPLATE_NPC), None)
    if template is None:
        raise SystemExit(f'{TEMPLATE_NPC} not found in {MAIN_ISLAND.name}')
    stump_prefab = reader.read_prefab_file(STUMP_PREFAB)

    root = edits.add_gameobject(scene, parent=None, name=ROOT_NAME)
    for r in roles:
        guids = {
            'model': asset_guid(REPO / MODEL_DIR / f'{r.key}.mv1.meta'),
            'tree': asset_guid(TREE_DIR / f'CampPeople{r.key}.animTree.meta'),
            'bt': asset_guid(BT_DIR / f'CampPeople{r.key}.friendBehaviourData.meta'),
        }
        node, facing, ground = npc_from_template(template, r, guids, t)
        root.transform.children.append(node)
        if r.seat_cm > 0:
            stump_under(scene, root, stump_prefab, r, facing, ground, t)
    for node in walk(root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root)

    text = writer.write_scene(scene)
    tree = loads(text)
    fixer = VersionFixer(catalog_mod.load(), f'/gameObject_{len(scene.roots) - 1}/', first_versions(read_text(MAIN_ISLAND)))
    fixer.run(tree, '')
    text = dumps(tree)
    print(f'  versions: added {sorted(set(fixer.added))}, stripped {fixer.stripped} repeat key(s)')

    versions = first_versions(text)
    if versions.get('ColliderBase') != 6:
        raise SystemExit(f'ColliderBase first occurrence is v{versions.get("ColliderBase")} - nothing written')
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({len(roles)} people under {ROOT_NAME})')
    clear_grass(roles)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('step', choices=('models', 'data', 'place'))
    ap.add_argument('--role', action='append', help='models / data の対象を絞る (Elder / Lookout / Huntress / Wounded)')
    args = ap.parse_args()
    roles = [r for r in ROLES if not args.role or r.key in args.role]
    if args.step == 'models':
        models(roles)
    elif args.step == 'data':
        data(roles)
    else:
        place(ROLES)


if __name__ == '__main__':
    main()
