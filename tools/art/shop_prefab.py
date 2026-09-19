"""Build the shop (merchant stall) UI prefabs from tools/art/shop.py's LAYOUT.

    python tools/art/shop.py --emit          # 先にスプライトを書き出す
    python tools/art/shop_prefab.py          # 店の UI プレハブ (ShopRow / ShopUI) を組み直す

組むもの: 黒板の1行 ShopRow.prefab と、それを生やす画面 ShopUI.prefab (黒板・勘定書き・財布・操作ヒント)。
ShopUI のルートに ShopUi (見た目) と ShopPresenter (開閉と入力) を付け、品揃えは GeneralStore.shopData を指す。
.meta(asset guid)は既存があれば保つので、組み直しても BT の GameObject::Instantiate からの参照は切れない。
組み方の道具は game_over_prefab.py / event_board_prefab.py のものを使う。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, OrderedObj  # noqa: E402
from tools.scene import edits, model  # noqa: E402

import shop as art  # noqa: E402
from event_board_prefab import ALIGN_CENTER, ALIGN_RIGHT, FONT_BRUSH, image, rgb, text  # noqa: E402
from game_over_prefab import BLACK_MASK, FONT_BODY, Builder, asset_guid, guid_of, new_prefab, save_prefab  # noqa: E402

UI_PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'Shop'
SHOP_DATA = asset_guid(REPO / 'Assets/Data/Shop/GeneralStore.shopData.meta')
PURCHASE_SOUND = asset_guid(REPO / 'Assets/Audio/UI/Shop_Purchase.mp3.meta')
REFUSE_SOUND = asset_guid(REPO / 'Assets/Audio/UI/Shop_Refuse.mp3.meta')
CURSOR_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/ButtonClick.mp3.meta')
PLACEHOLDER_ICON = asset_guid(art.ITEM_ICON_DIR / 'Icon_Potion.png.meta')

# 掲示板 (7000 台) と同じ帯。ゲームオーバー (8000 台) より下
ORDER_VEIL = 7000
ORDER_BOARD = 7010
ORDER_CHALK_TEXT = 7021
ORDER_CHALK_MARK = 7022
ORDER_CHALK_GRAIN = 7023   # チョークの文字と印だけをかすれさせる。紙片とアイコンはこれより上
ORDER_CHIP = 7024
ORDER_CHIP_ICON = 7025
ORDER_TWINE = 7030
ORDER_RECEIPT = 7031
ORDER_RECEIPT_ICON = 7032
ORDER_RECEIPT_TEXT = 7033
ORDER_STAMP = 7034
ORDER_PURSE = 7040
ORDER_MONEY_TAG = 7041
ORDER_MONEY_TEXT = 7042
ORDER_HINT = 7050
ORDER_HINT_TEXT = 7051


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


def blend_image(b, parent, name, pos, sprite, order, blend, enabled=True):
    node, comp = b.image(parent, name, pos, sprite, order, blend)
    if not enabled:
        model.set_component_enabled(comp, False)
    return comp


# ---------------------------------------------------------------- 黒板の1行 (root が行の中心)
def build_row():
    L = art.LAYOUT
    prefab = new_prefab('ShopRow')
    b = Builder(prefab)
    root = prefab.root

    w, h = L['row_button']
    button = b.component(root, 'Button', eventAreaSize_=f'{w},{h}')
    image(b, root, 'Chip', L['row_chip'], sprite_guid('Shop_PaperChip'), ORDER_CHIP)
    pos, scale = L['row_icon']
    icon = image(b, root, 'Icon', pos, PLACEHOLDER_ICON, ORDER_CHIP_ICON, scale=scale)
    pos, px = L['row_name']
    name = text(b, root, 'NameText', pos, px, '', art.CHALK, ORDER_CHALK_TEXT, font=FONT_BRUSH)
    pos, px = L['row_owned']
    owned = text(b, root, 'OwnedText', pos, px, '', art.CHALK_DIM, ORDER_CHALK_TEXT)
    pos, px = L['row_price']
    price = text(b, root, 'PriceText', pos, px, '', art.CHALK_YELLOW, ORDER_CHALK_TEXT, font=FONT_BRUSH,
                 align=ALIGN_RIGHT)
    mark = b.node(root, 'SelectMark')
    image(b, mark, 'Circle', L['row_circle'], sprite_guid('Shop_ChalkCircle'), ORDER_CHALK_MARK)
    image(b, mark, 'Arrow', L['row_arrow'], sprite_guid('Shop_ChalkArrow'), ORDER_CHALK_MARK)

    row = b.component(root, 'ShopRow', priceColor_=rgb(art.CHALK_YELLOW), unaffordablePriceColor_=rgb(art.CHALK_RED))
    b.field(row, 'iconRenderer_', guid_of(icon))
    b.field(row, 'nameText_', guid_of(name))
    b.field(row, 'ownedText_', guid_of(owned))
    b.field(row, 'priceText_', guid_of(price))
    b.field(row, 'selectMark_', mark.guid)
    b.field(row, 'selectButton_', guid_of(button))
    return save_prefab(prefab, UI_PREFAB_DIR, 'ShopRow')


# ---------------------------------------------------------------- 勘定書き (Receipt が紙の中心)
def build_receipt(b, root):
    L = art.LAYOUT
    image(b, root, 'Twine', L['twine'], sprite_guid('Shop_Twine'), ORDER_TWINE)
    paper = b.node(root, 'Receipt', L['receipt'])
    b.component(paper, 'ImageRenderer', spriteFile_=sprite_guid('Shop_Receipt'), renderPriority_=ORDER_RECEIPT)
    content = b.node(paper, 'Content')

    pos, scale = L['rc_icon']
    icon = image(b, content, 'Icon', pos, PLACEHOLDER_ICON, ORDER_RECEIPT_ICON, scale=scale)
    pos, px = L['rc_name']
    name = text(b, content, 'NameText', pos, px, '', art.INK, ORDER_RECEIPT_TEXT, font=FONT_BRUSH)
    pos, px = L['rc_owned']
    owned = text(b, content, 'OwnedText', pos, px, '', art.INK_FADE, ORDER_RECEIPT_TEXT)
    pos, px, step = L['rc_desc']
    lines = [text(b, content, f'DescLine{i}', (pos[0], pos[1] + i * step), px, '', art.INK, ORDER_RECEIPT_TEXT)
             for i in range(L['rc_desc_lines'])]
    pos, px = L['rc_unit_price']
    unit = text(b, content, 'UnitPriceText', pos, px, '', art.INK, ORDER_RECEIPT_TEXT, align=ALIGN_RIGHT)
    decrease = blend_image(b, content, 'QtyLeft', L['rc_decrease'], sprite_guid('Shop_QtyLeft'), ORDER_RECEIPT_TEXT, 255)
    pos, px = L['rc_quantity']
    quantity = text(b, content, 'QuantityText', pos, px, '', art.INK, ORDER_RECEIPT_TEXT, font=FONT_BRUSH,
                    align=ALIGN_CENTER)
    increase = blend_image(b, content, 'QtyRight', L['rc_increase'], sprite_guid('Shop_QtyRight'), ORDER_RECEIPT_TEXT,
                           255)
    pos, px = L['rc_total']
    total = text(b, content, 'TotalText', pos, px, '', art.STAMP_RED, ORDER_RECEIPT_TEXT, font=FONT_BRUSH,
                 align=ALIGN_RIGHT)
    pos, px = L['rc_after']
    after = text(b, content, 'AfterPaymentText', pos, px, '', art.INK_FADE, ORDER_RECEIPT_TEXT, align=ALIGN_RIGHT)
    pos, px = L['rc_refusal']
    refusal = text(b, content, 'RefusalText', pos, px, '', art.STAMP_RED, ORDER_RECEIPT_TEXT, align=ALIGN_RIGHT,
                   enabled=False)
    stamp = blend_image(b, paper, 'PaidStamp', L['rc_stamp'], sprite_guid('Shop_PaidStamp'), ORDER_STAMP, 0,
                        enabled=False)

    receipt = b.component(paper, 'ShopReceipt', stampDuration_secs_='0.6', stampStartScale_='1.6')
    receipt.data['markActiveBlendRate_'] = Num.of_int(255)
    receipt.data['markInactiveBlendRate_'] = Num.of_int(L['rc_mark_inactive'])
    b.field(receipt, 'contentRoot_', content.guid)
    b.field(receipt, 'iconRenderer_', guid_of(icon))
    b.field(receipt, 'nameText_', guid_of(name))
    b.field(receipt, 'ownedText_', guid_of(owned))
    receipt.data['descriptionLines_'] = [edits.field_blob('TextRenderer', guid_of(t)) for t in lines]
    b.field(receipt, 'unitPriceText_', guid_of(unit))
    b.field(receipt, 'quantityText_', guid_of(quantity))
    b.field(receipt, 'decreaseMark_', guid_of(decrease))
    b.field(receipt, 'increaseMark_', guid_of(increase))
    b.field(receipt, 'totalText_', guid_of(total))
    b.field(receipt, 'afterPaymentText_', guid_of(after))
    b.field(receipt, 'refusalText_', guid_of(refusal))
    b.field(receipt, 'paidStamp_', guid_of(stamp))
    return receipt


def build_hints(b, root):
    group = b.node(root, 'Hints')
    for i, item in enumerate(art.hint_layout()):
        if item['kind'] == 'tag':
            image(b, group, f'Tag{i}', item['pos'], asset_guid(str(art.HINT_SPRITES[item['sprite']]) + '.meta'),
                  ORDER_HINT)
        else:
            text(b, group, f'Text{i}', item['pos'], art.LAYOUT['hint_px'], item['text'], art.HINT_COLOR,
                 ORDER_HINT_TEXT)


# ---------------------------------------------------------------- 店の画面 (root は画面の原点)
def build_ui(row_prefab_guid):
    L = art.LAYOUT
    prefab = new_prefab('ShopUI')
    b = Builder(prefab)
    root = prefab.root

    veil_pos, veil_scale, veil_blend = L['veil']
    b.image(root, 'Veil', veil_pos, BLACK_MASK, ORDER_VEIL, veil_blend, scale=veil_scale)
    image(b, root, 'Board', L['board'], sprite_guid('Shop_Board'), ORDER_BOARD)
    pos, px = L['title']
    title = text(b, root, 'TitleText', pos, px, '', art.CHALK, ORDER_CHALK_TEXT, font=FONT_BRUSH, align=ALIGN_CENTER)
    image(b, root, 'TitleRule', L['title_rule'], sprite_guid('Shop_ChalkRule'), ORDER_CHALK_MARK)
    rows = b.node(root, 'Rows', L['rows_origin'])
    pos, px = L['restock']
    restock = text(b, rows, 'RestockText', pos, px, '(入荷待ち)', art.CHALK_DIM, ORDER_CHALK_TEXT, font=FONT_BRUSH,
                   enabled=False)
    up = image(b, root, 'MoreAbove', L['more_above'], sprite_guid('Shop_ChalkMoreUp'), ORDER_CHALK_MARK, enabled=False)
    down = image(b, root, 'MoreBelow', L['more_below'], sprite_guid('Shop_ChalkMoreDown'), ORDER_CHALK_MARK,
                 enabled=False)
    image(b, root, 'ChalkGrain', L['grain'], sprite_guid('Shop_ChalkGrain'), ORDER_CHALK_GRAIN)

    receipt = build_receipt(b, root)

    image(b, root, 'Purse', L['purse'], sprite_guid('Shop_Purse'), ORDER_PURSE)
    pos, px = L['money_label']
    text(b, root, 'MoneyLabel', pos, px, '所持金', art.HINT_COLOR, ORDER_MONEY_TAG)
    image(b, root, 'MoneyTag', L['money_tag'], sprite_guid('Shop_MoneyTag'), ORDER_MONEY_TAG)
    pos, px = L['money_text']
    money = text(b, root, 'MoneyText', pos, px, '', art.INK, ORDER_MONEY_TEXT, font=FONT_BRUSH, align=ALIGN_CENTER)
    build_hints(b, root)

    ui = b.component(root, 'ShopUi', rowSpacing_px_=str(art.ROW_PITCH), maxVisibleRows_=str(art.MAX_ROWS))
    b.field(ui, 'rowPrefab_', row_prefab_guid)
    b.field(ui, 'rowsRoot_', rows.guid)
    b.field(ui, 'moreAboveMark_', guid_of(up))
    b.field(ui, 'moreBelowMark_', guid_of(down))
    b.field(ui, 'titleText_', guid_of(title))
    b.field(ui, 'restockText_', guid_of(restock))
    b.field(ui, 'moneyText_', guid_of(money))
    b.field(ui, 'receipt_', guid_of(receipt))

    presenter = b.component(root, 'ShopPresenter', quantityRepeatDelay_secs_='0.35',
                            quantityRepeatInterval_secs_='0.08')
    b.field(presenter, 'shop_', SHOP_DATA)
    b.field(presenter, 'purchaseSound_', PURCHASE_SOUND)
    b.field(presenter, 'refuseSound_', REFUSE_SOUND)
    b.field(presenter, 'cursorSound_', CURSOR_SOUND)
    return save_prefab(prefab, UI_PREFAB_DIR, 'ShopUI')


def build_all():
    row_guid, _ = build_row()
    ui_guid, _ = build_ui(row_guid)
    return ui_guid


# ---------------------------------------------------------------- 3D の露店 (Prop)
PROP_DIR = REPO / 'Assets' / 'Prefab' / 'Prop'
STALL_MODEL = asset_guid(REPO / 'Assets/Art/Models/Prop/MerchantStall/MerchantStall.mv1.meta')
GOODS = [  # (名前, .mv1, カウンター上の位置 cm (x, z), 向き deg)
    ('HealPotion', 'Assets/Art/Models/Item/Item_HealPotion.mv1.meta', (-70.0, -15.0), 20.0),
    ('RoastedMeat', 'Assets/Art/Models/Item/Item_RoastedMeat.mv1.meta', (-10.0, -12.0), -35.0),
    ('BarrelBomb', 'Assets/Art/Models/Item/Item_BarrelBomb.mv1.meta', (55.0, -8.0), 10.0),
]
# 露店の FBX は cm。前は -Z (DxLib が Z を反転するので Blender の -Y 側)。プレイヤーと同じ 0.08 倍で実寸比
MODEL_SCALE = 0.08
COUNTER_CM = (256.0, 101.0, 92.0)
# 店を開いている間のカメラ。露店を画面の左 1/3 に入れ、右の黒板と勘定書きに被らないようにする (ルートからの相対)
SHOP_CAMERA_POS = (34.0, 16.0, -55.0)
SHOP_CAMERA_TARGET = (34.0, 11.0, 0.0)
SHOP_CAMERA_PRIORITY = 100


def yaw_quat(deg):
    import math
    h = math.radians(deg) / 2
    return 0.0, math.sin(h), 0.0, math.cos(h)


def build_prop():
    from event_board_prefab import box_collider

    prefab = new_prefab('MerchantStall')
    b = Builder(prefab)
    root = prefab.root

    stall = b.component(root, 'MerchantStall', focusPriority_=str(SHOP_CAMERA_PRIORITY))
    w, h, d = (c * MODEL_SCALE for c in COUNTER_CM)
    root.components.append(box_collider((w, h, d), (0.0, h / 2, 0.0)))

    model_node = edits.add_gameobject(prefab, parent=root.guid, name='Model', pos=(0.0, 0.0, 0.0),
                                      scale=(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE))
    b.component(model_node, 'ModelRenderer', mv1File_=STALL_MODEL, useFixedInterpolation_='false')

    goods = edits.add_gameobject(prefab, parent=root.guid, name='Goods', pos=(0.0, h, 0.0))
    for name, meta, (x, z), yaw in GOODS:
        node = edits.add_gameobject(prefab, parent=goods.guid, name=name,
                                    pos=(x * MODEL_SCALE, 0.0, z * MODEL_SCALE),
                                    scale=(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE))
        node.transform.local_rot = edits._quat_from_floats(yaw_quat(yaw))
        b.component(node, 'ModelRenderer', mv1File_=asset_guid(REPO / meta), useFixedInterpolation_='false')

    target = edits.add_gameobject(prefab, parent=root.guid, name='ShopCameraTarget', pos=SHOP_CAMERA_TARGET)
    camera_node = edits.add_gameobject(prefab, parent=root.guid, name='ShopCamera', pos=SHOP_CAMERA_POS)
    camera = b.component(camera_node, 'CineMachineVirtualCamera', overrideFov_='false', fov_='60')
    camera.data['priority_'] = OrderedObj([('value', Num.of_int(-1))])
    look = b.component(camera_node, 'VirtualCameraLookAtBehaviour', lookAtTargetOffset_='0,0,0')
    b.field(look, 'target_', target.guid)

    b.field(stall, 'shopCamera_', guid_of(camera))
    return save_prefab(prefab, PROP_DIR, 'MerchantStall')


# ---------------------------------------------------------------- MainIslandScene へ置く
MAIN_ISLAND_SCENE = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
ISLAND = 'FirstIsland'
PLACE_NEXT_TO = 'EventNoticeBoard'
PLACE_SIDE_OFFSET = 75.0          # 掲示板の +X 方向へ (FirstIsland のローカル単位)
STALL_ROOT_SCALE = 2.5            # 掲示板と同じ。FirstIsland 0.4 x 2.5 x モデル 0.08 = プレイヤーと同じ 0.08
KEEPER_BEHIND = 75.0 * MODEL_SCALE * STALL_ROOT_SCALE   # カウンターの奥 75cm
KEEPER_SOURCE = 'CharacterBrokerNpc'
KEEPER_NAME = 'MerchantNpc'
KEEPER_DISPLAY_NAME = 'よろず屋の店主'
KEEPER_MODEL = asset_guid(REPO / 'Assets/Art/Models/Woman/Kachujin G Rosales.mv1.meta')
KEEPER_BEHAVIOUR = asset_guid(REPO / 'Assets/Data/FriendlyNpcBehviour/Merchant.friendBehaviourData.meta')
# 仲介人は専用の animTree に替わったので、店主は汎用の NPC 待機 (Kachujin の骨と名前が合うことを確認済み) に固定する
KEEPER_ANIMATION_TREE = asset_guid(REPO / 'Assets/Animations/ActionInstructure.animTree.meta')


def audit_path_of(tree, guid):
    """validate の監査と同じ書き方で、guid を持つ GameObject の data までの道のりを返す"""
    from tools.common.cereal_json import OrderedObj as Obj

    def walk(node, where):
        if isinstance(node, Obj):
            counts = {}
            for key, value in node.items():
                counts[key] = counts.get(key, 0) + 1
                label = key if counts[key] == 1 else f'{key}#{counts[key] - 1}'
                if key == 'value_' and value == guid:
                    return where
                found = walk(value, f'{where}/{label}')
                if found:
                    return found
        elif isinstance(node, list):
            for index, value in enumerate(node):
                found = walk(value, f'{where}/[{index}]')
                if found:
                    return found
        return None

    hit = walk(tree, '')
    if hit is None:
        raise SystemExit(f'{guid} not found in the written scene')
    return hit[:hit.rfind('/ptr_wrapper/data') + len('/ptr_wrapper/data')] + '/'


def strip_repeat_versions_in(text, guids):
    from tools.common.cereal_json import dumps, loads
    from tools.scene import catalog as catalog_mod
    from event_board_prefab import StrayVersionStripper

    tree = loads(text)
    if dumps(tree) != text:
        raise SystemExit('cereal_json round trip is not exact - refusing to rewrite')
    for guid in guids:
        stripper = StrayVersionStripper(catalog_mod.load(), audit_path_of(tree, guid))
        stripper.run(tree, '')
        print(f'  stripped {len(stripper.stripped)} repeat cereal_class_version key(s) inside {guid}')
    return dumps(tree)


def copy_keeper(anchor):
    """酒場の仲介人を複製し、名前・BT・見た目だけ差し替える (Dynamic + constraints 61 の会話できる形を保つ)"""
    import copy

    node = copy.deepcopy(anchor)
    remap = {}
    edits._remint_guids(node, remap)
    edits._remap_guid_references(node, remap)
    node.name = KEEPER_NAME
    for comp in node.components:
        if comp.fqn.endswith('::FriendlyNpc'):
            comp.data['name_'] = KEEPER_DISPLAY_NAME
            edits._set_field_guid(comp.data['friendlyNpcBehaviourFile_'], KEEPER_BEHAVIOUR)
        elif comp.fqn.endswith('::ModelRenderer'):
            edits._set_field_guid(comp.data['mv1File_'], KEEPER_MODEL)
        elif comp.fqn.endswith('::Animator'):
            edits._set_field_guid(comp.data['animationTreeFile_'], KEEPER_ANIMATION_TREE)
    return node


def place_in_main_island(prop_path):
    """FirstIsland の掲示板の横に露店を、そのカウンターの奥に店主を置く。置き直すときは前のものを消してから置く"""
    from tools.common.cereal_json import to_file_bytes
    from tools.scene import mathutil, reader, validate, writer
    from event_board_prefab import bake_rotated_world_matrices, check, first_versions, to_collider_base_v5

    scene = reader.read_scene_file(MAIN_ISLAND_SCENE)
    island = next((r for r in scene.roots if r.name == ISLAND), None)
    if island is None:
        raise SystemExit(f'{ISLAND} not found at the root of {MAIN_ISLAND_SCENE.name}')
    children = island.transform.children
    children[:] = [c for c in children if c.name not in ('MerchantStall', KEEPER_NAME)]
    anchor = next((c for c in children if c.name == PLACE_NEXT_TO), None)
    source = next((c for c in children if c.name == KEEPER_SOURCE), None)
    if anchor is None or source is None:
        raise SystemExit(f'{PLACE_NEXT_TO} / {KEEPER_SOURCE} not found under {ISLAND}')

    trs = edits._node_local_trs(anchor)
    side = mathutil.quat_rotate_vec(trs.rot, (1.0, 0.0, 0.0))
    back = mathutil.quat_rotate_vec(trs.rot, (0.0, 0.0, 1.0))
    stall_pos = tuple(trs.pos[i] + side[i] * PLACE_SIDE_OFFSET for i in range(3))
    keeper_pos = tuple(stall_pos[i] + back[i] * KEEPER_BEHIND for i in range(3))

    stall = edits.instantiate_prefab(scene, reader.read_prefab_file(prop_path), parent=island.guid)
    collider_version = first_versions(MAIN_ISLAND_SCENE.read_text(encoding='utf-8')).get('ColliderBase')
    if collider_version == 5:
        to_collider_base_v5(stall)
    elif collider_version not in (None, 6):
        raise SystemExit(f'{MAIN_ISLAND_SCENE.name}: ColliderBase v{collider_version} is not handled')
    stall.transform.local_pos = edits._vec3_from_floats(stall_pos)
    stall.transform.local_rot = copy_rot(anchor)
    stall.transform.local_scale = edits._vec3_from_floats((STALL_ROOT_SCALE,) * 3)

    keeper = copy_keeper(source)
    keeper.transform.local_pos = edits._vec3_from_floats(keeper_pos)
    keeper.transform.local_rot = copy_rot(anchor)
    children.append(keeper)

    island_world = edits._world_trs_of_chain([island])
    bake_rotated_world_matrices(stall, island_world)
    bake_rotated_world_matrices(keeper, island_world)

    text = strip_repeat_versions_in(writer.write_scene(scene), [stall.guid, keeper.guid])
    check(text, validate.validate_scene(scene), MAIN_ISLAND_SCENE.name)
    MAIN_ISLAND_SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(MAIN_ISLAND_SCENE)
    print(f'placed MerchantStall under {ISLAND} at ({stall_pos[0]:.1f}, {stall_pos[1]:.1f}, {stall_pos[2]:.1f}) '
          f'and {KEEPER_NAME} at ({keeper_pos[0]:.1f}, {keeper_pos[1]:.1f}, {keeper_pos[2]:.1f})')


def copy_rot(node):
    import copy
    return copy.deepcopy(node.transform.local_rot)


def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('--prop', action='store_true', help='3D の露店 Prop/MerchantStall.prefab も組む')
    ap.add_argument('--place', action='store_true', help='露店と店主を MainIslandScene の掲示板の横に置く(--prop を含む)')
    args = ap.parse_args()

    build_all()
    if args.prop or args.place:
        _, prop_path = build_prop()
        if args.place:
            place_in_main_island(prop_path)


if __name__ == '__main__':
    main()
