"""GrassLand の薬草と宝箱のプレハブ・データを組む。

    python tools/art/loot_prefabs.py

- Assets/Prefab/Pickup/HerbPickup.prefab       : 飛び出して拾う薬草 (HealPotionPickup の Model だけ差し替え)
- Assets/Data/Item/Herb.itemData               : 薬草 (HealHealthEffect 15)
- Assets/Data/Drop/TreasureChest.dropTable     : 宝箱の中身
- Assets/Prefab/Prop/Loot/HerbPatch.prefab     : 生えている薬草 (HerbPatch + 調べるアイコン)
- Assets/Prefab/Prop/Loot/TreasureChest.prefab : 宝箱 (TreasureChest + ヒンジ位置の Lid + 調べるアイコン)

モデルは Assets/Art/Models/Prop/Loot (tools/art/loot/ の Blender スクリプト)。world = m x 8、Blender (x, y, z) = エンジン (x, z, y)。
.meta (asset guid) は既存があれば保つので、組み直してもシーン側の参照は切れない。
"""
import copy
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ptr, Ver  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, model, reader  # noqa: E402

from game_over_prefab import Builder, all_nodes, asset_guid, guid_of, new_prefab, save_prefab  # noqa: E402
from grassland_nature_prefabs import MODEL_SCALE, box_collider  # noqa: E402

METERS = 8.0
MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'Prop' / 'Loot'
LOOT_PREFABS = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Loot'
PICKUP_PREFABS = REPO / 'Assets' / 'Prefab' / 'Pickup'
ITEM_DIR = REPO / 'Assets' / 'Data' / 'Item'
DROP_DIR = REPO / 'Assets' / 'Data' / 'Drop'

CHAT_ICON_PREFAB = REPO / 'Assets' / 'Prefab' / 'UI' / 'BillBoardNpcChatIcon.prefab'
# RigidBody は掲示板 (調べられる置物) のものを元にする
NOTICE_BOARD_PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop' / 'EventNoticeBoard.prefab'

HEAL_POTION = asset_guid(ITEM_DIR / 'HealPotion.itemData.meta')
ROASTED_MEAT = asset_guid(ITEM_DIR / 'RoastedMeat.itemData.meta')
HERB_ICON = asset_guid(REPO / 'Assets/Art/UI/Item/Icon_Herb.png.meta')
HARVEST_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/GrassSways.mp3.meta')
OPEN_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/HolyGlass.mp3.meta')
HARVEST_PARTICLE = asset_guid(REPO / 'Assets/Prefab/Particle/FootstepDust.prefab.meta')
OPEN_PARTICLE = asset_guid(REPO / 'Assets/Prefab/Particle/ChestOpenBurst.prefab.meta')   # tools/art/chest_open_effect.py

HERB_MODEL_SCALE = MODEL_SCALE * 1.8   # 草 (高さ 5 前後) に埋もれないよう大きめ
HERB = {
    'displayName_': '薬草',
    'heal': 15,
    'maxStack_': 10,
    'descriptionLines_': ['体力を少し回復する。', '草原のあちこちに生えている。'],
}
# (item guid, chance, count)
CHEST_ITEMS = [('HERB', 1.0, 2), (HEAL_POTION, 0.5, 1), (ROASTED_MEAT, 0.3, 1)]
CHEST_MONEY, CHEST_COINS = 30, 6

# Blender の寸法 (tools/art/loot/assets_loot.py の CHEST_* と合わせる)
CHEST_D, CHEST_H = 0.60, 0.46
# 実寸 (幅 1 m) だとプレイヤーの横で小さすぎるのでルートごと拡大する。調べるアイコンはその分だけ縮めて元の見た目に戻す
CHEST_SCALE = 2.0


def engine(x, y, z):
    """Blender の m -> エンジンの world 単位 (y と z を入れ替え)"""
    return x * METERS, z * METERS, y * METERS


# ---------------------------------------------------------------- data (.meta に中身を持つ ScriptableObject)
def ordered(text):
    return json.loads(text, object_pairs_hook=lambda pairs: dict(pairs))


def write_json(path, obj):
    path.write_text(json.dumps(obj, ensure_ascii=False, indent=4), encoding='utf-8')


def existing_guid(meta):
    return asset_guid(meta) if meta.exists() else scene_meta.mint_guid().upper()


def set_ptr_guid(field, guid, ptr_id=None):
    """Field<T> の blob (value0.ptr_wrapper.data.value0.value_) に guid を入れる"""
    wrapper = field['value0']['ptr_wrapper']
    if ptr_id is not None:
        wrapper['id'] = ptr_id
    wrapper['data']['value0']['value_'] = guid


def build_herb_item(pickup_prefab_guid):
    meta = ITEM_DIR / 'Herb.itemData.meta'
    guid = existing_guid(meta)
    obj = ordered((ITEM_DIR / 'HealPotion.itemData.meta').read_text(encoding='utf-8'))
    data = obj['value0']['ptr_wrapper']['data']
    data['value0']['contentPath_'] = 'Assets\\Data\\Item/Herb.itemData'
    data['value0']['guid_']['value_'] = guid
    data['displayName_'] = HERB['displayName_']
    set_ptr_guid(data['iconSprite_'], HERB_ICON)
    data['maxStack_'] = HERB['maxStack_']
    effects = data['effects_']
    assert len(effects) == 1 and effects[0]['polymorphic_name'].endswith('HealHealthEffect'), effects
    effect_body = effects[0]['ptr_wrapper']['data']
    assert 'amount_' in json.dumps(effect_body)
    _set_key(effect_body, 'amount_', HERB['heal'])
    set_ptr_guid(data['pickupPrefab_'], pickup_prefab_guid)
    data['descriptionLines_'] = HERB['descriptionLines_']
    write_json(meta, obj)
    (ITEM_DIR / 'Herb.itemData').write_bytes(b'')
    print(f'wrote Assets/Data/Item/Herb.itemData  (asset guid {guid})')
    return guid


def _set_key(blob, key, value):
    if isinstance(blob, dict):
        for k in blob:
            if k == key:
                blob[k] = value
                return True
            if _set_key(blob[k], key, value):
                return True
    elif isinstance(blob, list):
        return any(_set_key(v, key, value) for v in blob)
    return False


def build_chest_drop_table(herb_guid):
    meta = DROP_DIR / 'TreasureChest.dropTable.meta'
    guid = existing_guid(meta)
    obj = ordered((DROP_DIR / 'Barrel.dropTable.meta').read_text(encoding='utf-8'))
    data = obj['value0']['ptr_wrapper']['data']
    data['value0']['contentPath_'] = 'Assets\\Data\\Drop/TreasureChest.dropTable'
    data['value0']['guid_']['value_'] = guid
    _set_key(data['money_'], 'value_', CHEST_MONEY)
    data['moneyPickupCount_'] = CHEST_COINS
    first, rest = data['items_'][0], data['items_'][1]   # cereal は版番号を最初の1行にだけ書く
    next_ptr = first['item_']['value0']['ptr_wrapper']['id']
    rows = []
    for i, (item, chance, count) in enumerate(CHEST_ITEMS):
        row = copy.deepcopy(first if i == 0 else rest)
        set_ptr_guid(row['item_'], herb_guid if item == 'HERB' else item, next_ptr + i)
        row['chance_'] = chance
        row['count_'] = count
        rows.append(row)
    data['items_'] = rows
    write_json(meta, obj)
    (DROP_DIR / 'TreasureChest.dropTable').write_bytes(b'')
    print(f'wrote Assets/Data/Drop/TreasureChest.dropTable  (asset guid {guid})')
    return guid


# ---------------------------------------------------------------- prefabs
RIGIDBODY_KINEMATIC = 1


def kinematic_body():
    """InteractableArea (センサー) は静的な物体を拾わないので体は要る。押されて動かないよう Kinematic・重力なし"""
    board = reader.read_prefab_file(NOTICE_BOARD_PREFAB)
    body = fresh(next(c for c in board.root.components if c.fqn.endswith('::RigidBody')))
    body.data['motionType_'] = Num.of_int(RIGIDBODY_KINEMATIC)
    body.data['isGravity_'] = False
    return body


def fresh(component):
    comp = copy.deepcopy(component)
    model.set_component_guid(comp, edits.mint_guid().upper())
    return comp


def add_chat_icon(prefab, height):
    icon_prefab = reader.read_prefab_file(CHAT_ICON_PREFAB)
    node = edits.instantiate_prefab(prefab, icon_prefab, parent='root')
    node.name = 'ChatIcon'
    edits.set_transform(prefab, node.guid, pos=(0.0, height, 0.0))
    comp = next(c for c in node.components if c.fqn.endswith('BillBoardNpcChatIcon'))
    drop_game_object_field_versions(comp)
    return guid_of(comp)


def drop_game_object_field_versions(comp):
    """読み込んだアイコンの Field<IGameObject> は型の分からない blob のまま版番号を抱えている。
    ルートの HerbPatch / TreasureChest が先に同じ型を書くので、こちらは版番号なしにする"""
    entry = catalog_mod.load().component_by_fqn(comp.fqn)
    for param in entry['params']:
        if param.get('shape') != 'field' or param.get('type') != 'IGameObject':
            continue
        blob = comp.data[param['key']]
        if not isinstance(blob, Ver):
            continue   # 2つ目以降は元から版番号なし
        blob.literal_presence = False
        holder = blob.body['value0']
        if isinstance(holder, Ptr) and isinstance(holder.data, Ver):
            holder.data.literal_presence = False


def model_node(b, prefab, parent, mv1, scale=MODEL_SCALE, pos=(0.0, 0.0, 0.0)):
    node = edits.add_gameobject(prefab, parent=parent.guid, name='Model', pos=pos, scale=(scale, scale, scale))
    b.component(node, 'ModelRenderer', mv1File_=asset_guid(MODELS / f'{mv1}.mv1.meta'), useFixedInterpolation_='false')
    return node


def build_herb_pickup():
    """HealPotionPickup を元に、Model の .mv1 だけ薬草の束にする"""
    prefab = edits.copy_prefab(reader.read_prefab_file(PICKUP_PREFABS / 'HealPotionPickup.prefab'))
    prefab.root.name = 'HerbPickup'
    model_go = next(n for n in all_nodes(prefab.root) if n.name == 'Model')
    renderer = next(c for c in model_go.components if c.fqn.endswith('::ModelRenderer'))
    edits._set_field_guid(renderer.data['mv1File_'], asset_guid(MODELS / 'HerbPickup.mv1.meta'))
    edits.set_transform(prefab, model_go.guid, scale=(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE))
    return save_prefab(prefab, PICKUP_PREFABS, 'HerbPickup')[0]


def build_herb_patch(herb_guid):
    prefab = new_prefab('HerbPatch')
    b = Builder(prefab)
    root = prefab.root
    model_node(b, prefab, root, 'HerbPatch', HERB_MODEL_SCALE)
    drop = edits.add_gameobject(prefab, parent=root.guid, name='DropPoint', pos=(0.0, 2.0, 0.0))
    icon = add_chat_icon(prefab, 10.5)
    comp = b.component(root, 'HerbPatch', item_=herb_guid, count_=1, dropPoint_=drop.guid,
                       harvestParticle_=HARVEST_PARTICLE, harvestSound_=HARVEST_SOUND, chatIcon_=icon)
    root.components.remove(comp)
    # 茎のまわりだけの小さな箱。プレイヤーの InteractableArea が触れれば足りる
    root.components[:0] = [comp, box_collider((3.0, 4.0, 3.0), (0.0, 2.0, 0.0)), kinematic_body()]
    return save_prefab(prefab, LOOT_PREFABS, 'HerbPatch')[0]


def build_treasure_chest(drop_table_guid):
    prefab = new_prefab('TreasureChest')
    b = Builder(prefab)
    root = prefab.root
    body = model_node(b, prefab, root, 'TreasureChest_Base')
    lid = edits.add_gameobject(prefab, parent=root.guid, name='Lid', pos=engine(0.0, -CHEST_D / 2, CHEST_H))
    model_node(b, prefab, lid, 'TreasureChest_Lid')
    drop = edits.add_gameobject(prefab, parent=root.guid, name='DropPoint', pos=engine(0.0, 0.1, CHEST_H + 0.25))
    icon = add_chat_icon(prefab, 16.0 / CHEST_SCALE)
    icon_node = next(n for n in all_nodes(prefab.root) if n.name == 'ChatIcon')
    icon_scale = float(edits._vec3_floats(icon_node.transform.local_scale)[0]) / CHEST_SCALE
    edits.set_transform(prefab, icon_node.guid, scale=(icon_scale, icon_scale, icon_scale))
    edits.set_transform(prefab, root.guid, scale=(CHEST_SCALE, CHEST_SCALE, CHEST_SCALE))
    comp = b.component(root, 'TreasureChest', dropTable_=drop_table_guid, lid_=lid.guid, dropPoint_=drop.guid,
                       openParticle_=OPEN_PARTICLE, openSound_=OPEN_SOUND, chatIcon_=icon,
                       openAngle_deg_=-105.0, openDuration_secs_=0.45, body_=body.guid, shakeDuration_secs_=0.55,
                       shakeAngle_deg_=5.0, shakeFrequency_hz_=9.0)
    root.components.remove(comp)
    size = engine(1.0, CHEST_D + 0.04, CHEST_H + 0.3)
    root.components[:0] = [comp, box_collider(size, (0.0, size[1] / 2, 0.0)), kinematic_body()]
    return save_prefab(prefab, LOOT_PREFABS, 'TreasureChest')[0]


def main():
    pickup = build_herb_pickup()
    herb = build_herb_item(pickup)
    table = build_chest_drop_table(herb)
    build_herb_patch(herb)
    build_treasure_chest(table)


if __name__ == '__main__':
    main()
