"""tools/art/settings_screen.py の LAYOUT から設定画面のプレハブを組む。

    python tools/art/settings_screen.py --emit     # 先にスプライトを書き出す
    python tools/art/settings_prefab.py            # Assets/Prefab/UI/Settings/ の3つを組み直す

SettingsScreen.prefab  ルートに SettingsScreenUi (見た目) と SettingsScreenPresenter (入力)。
                       タイトルとステージの貼り紙が生成して開き、閉じると自分で消える。
SettingsTab.prefab     左のカテゴリの札1枚 (SettingsTabUi)。札0枚目の位置で組み、実行時に下へずらす。
SettingsRow.prefab     一覧の1行 (SettingsRowUi)。行0の位置で組み、実行時に下へずらす。
座標はすべて画面の px (ルートは原点)。.meta(asset guid)は既存があれば保つので、組み直しても参照は切れない。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.scene import model  # noqa: E402

import settings_screen as art  # noqa: E402
from event_board_prefab import ALIGN_CENTER, FONT_BODY, FONT_BRUSH, text  # noqa: E402
from game_over_prefab import BLACK_MASK, Builder, asset_guid, guid_of, new_prefab, save_prefab  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'Settings'
UI_SOUNDS = asset_guid(REPO / 'Assets/Data/UiSound/UiSoundBank.uiSoundBank.meta')
ALIGN_LEFT = 0

# 貼り紙 (7600 台) より上、ゲームオーバー (8000 台) より下
ORDER_BACKDROP = 7700
ORDER_PLATE = 7710
ORDER_RULE = 7711
ORDER_BAND = 7712
ORDER_MARK = 7713
ORDER_TEXT = 7720
ORDER_HINT = 7730
ORDER_HINT_TEXT = 7731

# BlackMask を画面いっぱいに敷く中心と倍率 (stage_return.py の veil_black と同じ)
BACKDROP = ((960, 540), 0.47)


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


def rgb(color):
    return ','.join(str(c) for c in color[:3])


def image(b, parent, name, pos, sprite, order, blend=255, scale=1.0, enabled=True):
    _, comp = b.image(parent, name, pos, sprite_guid(sprite) if not sprite.startswith('{') else sprite[1:-1],
                      order, blend, scale=scale)
    if not enabled:
        model.set_component_enabled(comp, False)
    return comp


def put_text(b, parent, name, spec, s, color, order=ORDER_TEXT, font=FONT_BODY, align=ALIGN_LEFT):
    """spec は (x, 中心 y, px)。TextRenderer は上辺に置くので art.text_top で直す"""
    x, cy, px = spec
    return text(b, parent, name, (x, art.text_top(cy, px)), px, s, color, order, font=font, align=align)


def build_tab():
    L = art.layout()
    prefab = new_prefab('SettingsTab')
    b = Builder(prefab)
    root = prefab.root
    plate = image(b, root, 'Plate', L['tab'], 'Settings_Tab', ORDER_PLATE)
    selected = image(b, root, 'PlateSelected', L['tab'], 'Settings_Tab_Selected', ORDER_PLATE, enabled=False)
    name = put_text(b, root, 'Name', L['tab_text'], '', art.CREAM_FADE)

    ui = b.component(root, 'SettingsTabUi', selectedColor_=rgb(art.CREAM), unselectedColor_=rgb(art.CREAM_FADE))
    b.field(ui, 'nameText_', guid_of(name))
    b.field(ui, 'plate_', guid_of(plate))
    b.field(ui, 'selectedPlate_', guid_of(selected))
    return save_prefab(prefab, PREFAB_DIR, 'SettingsTab')[0]


def build_row():
    L = art.layout()
    prefab = new_prefab('SettingsRow')
    b = Builder(prefab)
    root = prefab.root
    image(b, root, 'Rule', L['row_rule'], 'Settings_RowRule', ORDER_RULE)
    band = image(b, root, 'Band', L['row_band'], 'Settings_RowBand', ORDER_BAND, enabled=False)
    left = image(b, root, 'ArrowLeft', L['row_arrow_left'], 'Settings_Arrow_Left', ORDER_MARK, enabled=False)
    right = image(b, root, 'ArrowRight', L['row_arrow_right'], 'Settings_Arrow_Right', ORDER_MARK, enabled=False)
    label = put_text(b, root, 'Label', L['row_label'], '', art.CREAM_FADE)
    value = put_text(b, root, 'Value', L['row_value'], '', art.CREAM_FADE, align=ALIGN_CENTER)

    ui = b.component(root, 'SettingsRowUi', selectedColor_=rgb(art.CREAM), unselectedColor_=rgb(art.CREAM_FADE))
    b.field(ui, 'labelText_', guid_of(label))
    b.field(ui, 'valueText_', guid_of(value))
    b.field(ui, 'band_', guid_of(band))
    b.field(ui, 'leftArrow_', guid_of(left))
    b.field(ui, 'rightArrow_', guid_of(right))
    return save_prefab(prefab, PREFAB_DIR, 'SettingsRow')[0]


def build_screen(tab_guid, row_guid):
    L = art.layout()
    prefab = new_prefab('SettingsScreen')
    b = Builder(prefab)
    root = prefab.root

    # 幕は滑らせない
    (pos, scale) = BACKDROP
    image(b, root, 'Backdrop', pos, '{' + BLACK_MASK + '}', ORDER_BACKDROP, art.BACKDROP_BLEND, scale=scale)

    visual = b.node(root, 'Visual')
    tabs = b.node(visual, 'Tabs')
    image(b, visual, 'Panel', L['panel'], 'Settings_Panel', ORDER_PLATE)
    image(b, visual, 'Header', L['header'], 'Settings_Header', ORDER_RULE)
    put_text(b, visual, 'HeaderText', L['header_text'], art.HEADING, art.CREAM, font=FONT_BRUSH, align=ALIGN_CENTER)
    category = put_text(b, visual, 'CategoryText', L['category_text'], '', art.CREAM)
    image(b, visual, 'CategoryRule', L['category_rule'], 'Settings_CategoryRule', ORDER_RULE)
    rows = b.node(visual, 'Rows')
    track = image(b, visual, 'ScrollTrack', L['scroll_track'], 'Settings_ScrollTrack', ORDER_BAND, enabled=False)
    thumb = image(b, visual, 'ScrollThumb', L['scroll_thumb'], 'Settings_ScrollThumb', ORDER_MARK, enabled=False)
    image(b, visual, 'Desc', L['desc'], 'Settings_Desc', ORDER_PLATE)
    desc = put_text(b, visual, 'DescText', L['desc_text'], '', art.CREAM)

    hints = b.node(visual, 'Hints')
    for h in L['hints']:
        node = b.node(hints, h['key'])
        image(b, node, 'Tag', h['tag'], f'Settings_Hint_{h["key"]}', ORDER_HINT)
        put_text(b, node, 'Label', h['text'], h['label'], (240, 226, 202), order=ORDER_HINT_TEXT)

    ui = b.component(root, 'SettingsScreenUi',
                     tabPitch_px_=float(art.TAB_PITCH), rowPitch_px_=float(art.ROW_H),
                     maxVisibleRows_=art.VISIBLE_ROWS, scrollTravel_px_=float(L['scroll_travel']),
                     enterSlide_px_=24.0, enterDuration_secs_=0.18)
    b.field(ui, 'visualRoot_', visual.guid)
    b.field(ui, 'tabPrefab_', tab_guid)
    b.field(ui, 'tabsRoot_', tabs.guid)
    b.field(ui, 'rowPrefab_', row_guid)
    b.field(ui, 'rowsRoot_', rows.guid)
    b.field(ui, 'categoryText_', guid_of(category))
    b.field(ui, 'descriptionText_', guid_of(desc))
    b.field(ui, 'scrollTrack_', guid_of(track))
    b.field(ui, 'scrollThumb_', guid_of(thumb))

    presenter = b.component(root, 'SettingsScreenPresenter')
    b.field(presenter, 'uiSounds_', UI_SOUNDS)
    return save_prefab(prefab, PREFAB_DIR, 'SettingsScreen')[0]


def main():
    tab = build_tab()
    row = build_row()
    build_screen(tab, row)


if __name__ == '__main__':
    main()
