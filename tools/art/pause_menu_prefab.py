"""Build the 冒険者の手帳 (＠メニュー) prefabs from tools/art/pause_menu.py's layout().

    python tools/art/pause_menu.py --emit          # 先にスプライトを書き出す
    python tools/art/pause_menu_prefab.py          # PauseMenuUI / PauseMenuRow / PauseMenuItemCell を組み直す
    python tools/art/pause_menu_prefab.py --wire   # 加えて SwordManStatusPresenter.pauseMenuPrefab_ を張る

各 prefab の .meta(asset guid)は既存があれば保つので、組み直しても外からの参照は切れない
(中の GameObject / Component の guid は毎回新しくなる)。

tools.scene の CLI では届かない所をここで埋める:
  * enum(TextRenderer::textAlign_)は 0 固定で書かれるので、中央/右揃えは数値で直接書く
  * add_component は数値を 0 で書くので、Slider などはヘッダの初期値を明示する
  * worldMatrix_ はエンジンが読み込み時に計算し直さないので、親から合成して書く
  * vector<FIELD(T)>(PauseMenuUi::statPips_)はカタログが単一の field と見なすので、配列を自前で作る
"""
import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, model, reader, validate, writer  # noqa: E402

import pause_menu as art  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'PauseMenu'
STATUS_PRESENTER_PREFAB = REPO / 'Assets' / 'Prefab' / 'PlayerAvatar' / 'Swordman' / 'SwordManStatusPresenter.prefab'

FONT_PX = 60  # ipam.ttf / onryou.ttf の TtfFontFile はどちらも 60px。TextRenderer は scale で縮める
ORDER_BACKDROP = 5000
ORDER_BOOK = 5010
ORDER_PARTS = 5020
ORDER_TEXT = 5030
TEXT_ALIGN = {'left': 0, 'center': 1, 'right': 2}


# ---------------------------------------------------------------- asset guids
def asset_guid(meta_path):
    text = Path(meta_path).read_text(encoding='utf-8-sig')
    m = re.search(r'"guid_"\s*:\s*\{[^{}]*?"value_"\s*:\s*"([0-9A-Fa-f-]{36})"', text)
    if not m:
        raise SystemExit(f'no guid_ in {meta_path}')
    return m.group(1).upper()


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


FONT_BODY = asset_guid(REPO / 'Assets/Art/Font/ipam.ttf.meta')
FONT_BRUSH = asset_guid(REPO / 'Assets/Art/Font/onryou.ttf.meta')
CHARACTER = asset_guid(REPO / 'Assets/Data/Character/SwordManCharacter.characterData.meta')
PIP_FILLED = asset_guid(REPO / 'Assets/Art/UI/CharacterSelect/Pip_Filled.png.meta')
PIP_EMPTY = asset_guid(REPO / 'Assets/Art/UI/CharacterSelect/Pip_Empty.png.meta')


# ---------------------------------------------------------------- building blocks
class PrefabBuilder:
    def __init__(self, name):
        self.cat = catalog_mod.load()
        root = edits.new_gameobject(name, kind=model.KIND_PREFAB_ROOT)
        self.prefab = model.Prefab(root=root, copied_object_guids=[])

    @property
    def root(self):
        return self.prefab.root

    def node(self, parent, name, pos=(0.0, 0.0), scale=1.0):
        """pos は親からの相対。scale は ImageRenderer / TextRenderer が x を見るので縦横同じにする"""
        return edits.add_gameobject(self.prefab, parent=parent.guid, name=name,
                                    pos=(float(pos[0]), float(pos[1]), 0.0),
                                    scale=(float(scale), float(scale), 1.0))

    def component(self, node, type_name, **params):
        comp = edits.add_component(self.prefab, node.guid, type_name, cat=self.cat,
                                   params={k: str(v) for k, v in params.items()})
        return comp

    def image(self, parent, name, pos, sprite, order, scale=1.0):
        node = self.node(parent, name, pos, scale)
        comp = self.component(node, 'ImageRenderer', spriteFile_=sprite or edits.EMPTY_GUID, renderPriority_=order)
        return node, comp

    def text(self, parent, name, spec, text=''):
        """spec は layout() の文字の指定。pos は TextRenderer の基準点(左上/上辺中央/右上)"""
        node = self.node(parent, name, spec['pos'], spec['px'] / FONT_PX)
        font = FONT_BRUSH if spec.get('font') == 'brush' else FONT_BODY
        comp = self.component(node, 'TextRenderer', fontFile_=font, renderOrder_=ORDER_TEXT,
                              text_=text or spec.get('text', ''), isWorldPos_='false',
                              textColor_=','.join(str(c) for c in spec['color']))
        comp.data['textAlign_'] = Num.of_int(TEXT_ALIGN[spec['align']])
        return node, comp

    def field(self, comp, key, guid):
        edits._set_field_guid(comp.data[key], guid)


def guid_of(comp):
    return model.find_component_guid(comp)


def slider(builder, parent, name, spec, fill_sprite):
    node = builder.node(parent, name, spec['pos'])
    w, h = spec['size']
    comp = builder.component(
        node, 'Slider',
        gaugeSprite_=fill_sprite, backgroundSprite_=sprite_guid('Bar_Frame'),
        drawPosition_='0,0', drawSize_=f'{w},{h}', value_='1', renderOrder_=ORDER_PARTS,
        isStretchToDrawSize_='true')
    # 目盛りは Slider の上に GaugeEffects で重ねる
    builder.component(
        node, 'GaugeEffects',
        renderOrder_=ORDER_PARTS + 1, trailDelay_secs_='0.5', trailDuration_secs_='0.8', tipWidth_='18',
        tickCount_='5', tickInsetY_='1.5', tickShadowAlpha_='110', tickHighlightAlpha_='0', bandInsetY_='3',
        gaugeFadeDuration_secs_='0.3', pulseFrequency_hz_='1.4', pulseMaxAlpha_='0')
    return node, comp


# ---------------------------------------------------------------- world matrices / class versions
def world_matrix_blob(scale, pos):
    cols = [(scale[0], 0.0, 0.0, 0.0), (0.0, scale[1], 0.0, 0.0), (0.0, 0.0, scale[2], 0.0),
            (pos[0], pos[1], pos[2], 1.0)]
    obj = OrderedObj()
    for i, col in enumerate(cols):
        obj[f'value{i}'] = OrderedObj((f'value{j}', Num.of_float(float(v))) for j, v in enumerate(col))
    return obj


def bake_world_matrices(node, parent_scale=(1.0, 1.0, 1.0), parent_pos=(0.0, 0.0, 0.0)):
    """回転は使わないので、world = 親の位置 + 親の拡縮 * 自分の位置、拡縮は掛け算で足りる"""
    t = node.transform
    lp = (edits._f(t.local_pos.x), edits._f(t.local_pos.y), edits._f(t.local_pos.z))
    ls = (edits._f(t.local_scale.x), edits._f(t.local_scale.y), edits._f(t.local_scale.z))
    ws = tuple(parent_scale[i] * ls[i] for i in range(3))
    wp = tuple(parent_pos[i] + parent_scale[i] * lp[i] for i in range(3))
    t.world_matrix = world_matrix_blob(ws, wp)
    for child in t.children:
        bake_world_matrices(child, ws, wp)


def let_writer_place_versions(blob):
    """新しく作った Field<T> / Color32 / 空の基底(IInitRenderable など)は、版キーの有無が固定で作られる。
    ここで作る prefab は丸ごと新規で型の出現順が分かっているので、エンジンと同じく
    「書き出し順で最初の1回だけ付ける」writer の判断に任せる"""
    if isinstance(blob, Ver):
        leaf = blob.key[1] if isinstance(blob.key, tuple) and len(blob.key) > 1 else ''
        is_empty_base = isinstance(blob.body, OrderedObj) and len(blob.body) == 0
        if is_empty_base or (isinstance(leaf, str) and (leaf.startswith(('Field<', 'FieldHolder<')) or leaf == 'Color32')):
            blob.literal_presence = None
        let_writer_place_versions(blob.body)
    elif isinstance(blob, OrderedObj):
        for _, value in blob.items():
            let_writer_place_versions(value)
    elif isinstance(blob, list):
        for value in blob:
            let_writer_place_versions(value)
    elif hasattr(blob, 'data'):
        let_writer_place_versions(blob.data)


def all_nodes(node):
    yield node
    for child in node.transform.children:
        yield from all_nodes(child)


def save_prefab(builder, name):
    PREFAB_DIR.mkdir(parents=True, exist_ok=True)
    path = PREFAB_DIR / f'{name}.prefab'
    for node in all_nodes(builder.root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(builder.root)

    text = writer.write_prefab(builder.prefab)
    problems = validate.validate_prefab(builder.prefab) + validate.validate_class_versions(text, catalog_mod.load())
    hard = [p for p in problems if not p.startswith('note:')]
    for p in problems:
        print('  ' + (p if p.startswith('note:') else 'FAIL: ' + p))
    if hard:
        raise SystemExit(f'{path.name}: validation failed - nothing written')
    path.write_bytes(to_file_bytes(text))

    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, name, PREFAB_DIR, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, name, guid, content_path)
    reader.read_prefab_file(path)  # 読み戻せることだけ確かめる
    print(f'wrote {path.relative_to(REPO)}  (asset guid {guid})')
    return guid


# ---------------------------------------------------------------- prefabs
def build_row(geo):
    b = PrefabBuilder('PauseMenuRow')
    row = geo['row']
    _, marker = b.image(b.root, 'Marker', row['marker'], sprite_guid('Row_Marker'), ORDER_PARTS)
    _, underline = b.image(b.root, 'Underline', row['underline'], sprite_guid('Row_Underline'), ORDER_PARTS)
    _, name = b.text(b.root, 'Name', row['name'])
    _, description = b.text(b.root, 'Description', row['description'])
    _, number = b.text(b.root, 'Number', row['number'])

    comp = b.component(b.root, 'PauseMenuRow')
    b.field(comp, 'nameText_', guid_of(name))
    b.field(comp, 'descriptionText_', guid_of(description))
    b.field(comp, 'numberText_', guid_of(number))
    b.field(comp, 'marker_', guid_of(marker))
    b.field(comp, 'underline_', guid_of(underline))
    return save_prefab(b, 'PauseMenuRow')


def build_item_cell(geo):
    b = PrefabBuilder('PauseMenuItemCell')
    cell = geo['item_cell']
    _, icon = b.image(b.root, 'Icon', cell['icon'], None, ORDER_PARTS)
    _, count = b.text(b.root, 'Count', cell['count'])

    comp = b.component(b.root, 'PauseMenuItemCell')
    b.field(comp, 'icon_', guid_of(icon))
    b.field(comp, 'countText_', guid_of(count))
    return save_prefab(b, 'PauseMenuItemCell')


def build_menu(geo, row_prefab, item_cell_prefab):
    b = PrefabBuilder('PauseMenuUI')
    # 開閉は Contents ごと SetEnable で切り替える。PauseMenuUi / Presenter は常駐させるので root に置く
    contents = b.node(b.root, 'Contents')

    b.image(contents, 'Backdrop', geo['backdrop']['center'], sprite_guid('Backdrop'), ORDER_BACKDROP,
            scale=geo['backdrop']['scale'])
    b.image(contents, 'Book', (art.SCREEN_W / 2, art.SCREEN_H / 2), sprite_guid('Book'), ORDER_BOOK)
    b.text(contents, 'Title', geo['title'])

    # 左の頁: 目次
    b.text(contents, 'IndexTitle', geo['index_title'])
    rows_root = b.node(contents, 'Rows', geo['rows_root'])
    b.component(rows_root, 'VerticalLayoutGroup', cellSize_=f"560,{geo['row_pitch']}", spacing_='0')

    # 右の頁: ステータス
    b.text(contents, 'StatusTitle', geo['status_title'])
    b.image(contents, 'Portrait', geo['portrait'], sprite_guid('Portrait_SwordMan'), ORDER_PARTS)
    _, name = b.text(contents, 'Name', geo['name'])
    _, reading = b.text(contents, 'Reading', geo['reading'])
    _, tagline = b.text(contents, 'Tagline', geo['tagline'])

    b.text(contents, 'HealthLabel', geo['health_label'])
    _, health_bar = slider(b, contents, 'HealthBar', geo['health_bar'], sprite_guid('Bar_Fill_Health'))
    _, health_text = b.text(contents, 'HealthText', geo['health_text'])
    b.text(contents, 'StaminaLabel', geo['stamina_label'])
    _, stamina_bar = slider(b, contents, 'StaminaBar', geo['stamina_bar'], sprite_guid('Bar_Fill_Stamina'))
    _, stamina_text = b.text(contents, 'StaminaText', geo['stamina_text'])

    stat_pips = []
    for i, stat in enumerate(geo['stats']):
        b.text(contents, f'StatLabel{i}', stat['label'])
        pips = b.node(contents, f'StatPips{i}', stat['pips'])
        comp = b.component(pips, 'StageDifficultyPips', filledSprite_=PIP_FILLED, emptySprite_=PIP_EMPTY)
        for k in range(5):
            b.image(pips, f'Pip{k}', (k * geo['pip_gap'], 0), PIP_EMPTY, ORDER_PARTS, scale=geo['pip_scale'])
        stat_pips.append(guid_of(comp))

    b.image(contents, 'MoneyPlate', geo['money_plate'], sprite_guid('MoneyPlate'), ORDER_PARTS)
    b.text(contents, 'MoneyLabel', geo['money_label'])
    _, money_text = b.text(contents, 'MoneyText', geo['money_text'])

    b.text(contents, 'ItemsLabel', geo['items_label'])
    items_root = b.node(contents, 'Items', geo['items_root'])
    b.component(items_root, 'HorizontalLayoutGroup', cellSize_=f"{geo['item_pitch']},76", spacing_='0')

    for hint in geo['hints']:
        stem = hint['sprite'].removeprefix('Hint_')
        b.image(contents, f'Hint{stem}', hint['chip'], sprite_guid(hint['sprite']), ORDER_PARTS)
        b.text(contents, f'Hint{stem}Text', hint['label'])

    ui = b.component(b.root, 'PauseMenuUi', maxItemCells_='6')
    b.field(ui, 'book_', contents.guid)
    b.field(ui, 'rowPrefab_', row_prefab)
    b.field(ui, 'rowsRoot_', rows_root.guid)
    b.field(ui, 'character_', CHARACTER)
    b.field(ui, 'nameText_', guid_of(name))
    b.field(ui, 'readingText_', guid_of(reading))
    b.field(ui, 'taglineText_', guid_of(tagline))
    ui.data['statPips_'] = [edits.field_blob('StageDifficultyPips', g) for g in stat_pips]
    b.field(ui, 'healthBar_', guid_of(health_bar))
    b.field(ui, 'healthText_', guid_of(health_text))
    b.field(ui, 'staminaBar_', guid_of(stamina_bar))
    b.field(ui, 'staminaText_', guid_of(stamina_text))
    b.field(ui, 'moneyText_', guid_of(money_text))
    b.field(ui, 'itemCellPrefab_', item_cell_prefab)
    b.field(ui, 'itemsRoot_', items_root.guid)

    b.component(b.root, 'PauseMenuPresenter')
    return save_prefab(b, 'PauseMenuUI')


# ---------------------------------------------------------------- SwordManStatusPresenter
def wire_status_presenter(menu_prefab):
    """StatusPresenter v4 -> v5: pauseMenuPrefab_ を末尾に足す(save() の順)"""
    prefab = reader.read_prefab_file(STATUS_PRESENTER_PREFAB)
    target = None
    for node in all_nodes(prefab.root):
        for comp in node.components:
            if comp.fqn == 'GamePlay::PlayerAvatar::SwordMan::StatusPresenter':
                target = comp
    if target is None:
        raise SystemExit('StatusPresenter not found in SwordManStatusPresenter.prefab')

    if 'pauseMenuPrefab_' in target.data:
        edits._set_field_guid(target.data['pauseMenuPrefab_'], menu_prefab)
    else:
        if target.class_version != 4:
            raise SystemExit(f'expected StatusPresenter v4, found v{target.class_version}')
        target.class_version = 5
        # itemBarPrefab_ が先にあるので Field<PrefabGameObjectFile> の2回目以降。版キーは付けない
        target.data.append('pauseMenuPrefab_', edits.field_blob('PrefabGameObjectFile', menu_prefab))

    text = writer.write_prefab(prefab)
    problems = validate.validate_prefab(prefab) + validate.validate_class_versions(text, catalog_mod.load())
    hard = [p for p in problems if not p.startswith('note:')]
    for p in problems:
        print('  ' + (p if p.startswith('note:') else 'FAIL: ' + p))
    if hard:
        raise SystemExit('SwordManStatusPresenter.prefab: validation failed - nothing written')
    STATUS_PRESENTER_PREFAB.write_bytes(to_file_bytes(text))
    print(f'wrote {STATUS_PRESENTER_PREFAB.relative_to(REPO)}  (pauseMenuPrefab_ -> {menu_prefab})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--wire', action='store_true', help='SwordManStatusPresenter.prefab に pauseMenuPrefab_ を張る')
    args = ap.parse_args()

    geo = art.layout()
    row = build_row(geo)
    cell = build_item_cell(geo)
    menu = build_menu(geo, row, cell)
    if args.wire:
        wire_status_presenter(menu)


if __name__ == '__main__':
    main()
