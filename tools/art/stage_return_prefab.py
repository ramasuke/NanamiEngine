"""tools/art/stage_return.py の LAYOUT からステージの「帰 還」貼り紙プレハブ (案A 掲示板の貼り紙) を組む。

    python tools/art/stage_return.py --emit          # 先にスプライトを書き出す
    python tools/art/stage_return_prefab.py          # Assets/Prefab/UI/StageReturn/StageReturnUI.prefab を組み直す
    python tools/art/stage_return_prefab.py --into Assets/Scene/GrassLandScene.scene   # 加えてステージに置く

ルートに StageReturnNoticeUi (見た目) と StageReturnPresenter (ESC と入力) を付ける。ステージのシーンに置けば働く。
3行目の「設定」は settings_prefab.py の SettingsScreen.prefab を開く (先に組んでおく)。
座標はすべて画面の px (ルートは原点)。貼り紙の中身は Notice の子にして、降りてくる動きは Notice を動かすだけで済ませる。
操作ヒントは Notice の外に置き、札と一緒には動かさない。
.meta(asset guid)は既存があれば保つので、組み直してもシーンからの参照は切れない。
組み方の道具は game_over_prefab.py / event_board_prefab.py のものを使う。
"""
import argparse
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.scene import model  # noqa: E402

import stage_return as art  # noqa: E402
from event_board_prefab import ALIGN_CENTER, FONT_BODY, FONT_BRUSH_INK, text  # noqa: E402
from game_over_prefab import BLACK_MASK, Builder, asset_guid, guid_of, new_prefab, save_prefab  # noqa: E402

UI_PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'StageReturn'
BACKDROP = asset_guid(str(art.BACKDROP_SPRITE) + '.meta')
UI_SOUNDS = asset_guid(REPO / 'Assets/Data/UiSound/UiSoundBank.uiSoundBank.meta')
SETTINGS_PREFAB = asset_guid(REPO / 'Assets/Prefab/UI/Settings/SettingsScreen.prefab.meta')
ALIGN_LEFT = 0

# ほかの画面 (7000 台前半) とゲームオーバー (8000 台) の間の空いている帯
ORDER_VEIL_BLACK = 7600
ORDER_VEIL = 7601
ORDER_NOTICE = 7610
ORDER_UNDERLINE = 7611
ORDER_TEXT = 7612
ORDER_STAMP = 7613
ORDER_HINT = 7620
ORDER_HINT_TEXT = 7621


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


def blend_image(b, parent, name, pos, sprite, order, blend=255, scale=1.0, enabled=True):
    _, comp = b.image(parent, name, pos, sprite, order, blend, scale=scale)
    if not enabled:
        model.set_component_enabled(comp, False)
    return comp


def put_text(b, parent, name, pos_px, s, color, order, font=FONT_BODY, align=ALIGN_CENTER, enabled=True):
    """pos_px は (文字の中心, px)。TextRenderer は上辺に置くので art.text_top で直す"""
    (x, cy), px = pos_px
    return text(b, parent, name, (x, art.text_top(cy, px)), px, s, color, order, font=font, align=align,
                enabled=enabled)


def build_row(b, parent, name, index, L):
    y = L['rows'][index]
    row = b.node(parent, name)
    label = put_text(b, row, 'Label', ((art.NOTICE_CX, y), L['row_px']), art.CHOICES[index],
                     art.INK if index == art.SELECTED else art.INK_FADE, ORDER_TEXT)
    underline = blend_image(b, row, 'Underline', (art.NOTICE_CX, y + L['underline_dy']),
                            sprite_guid('StageReturn_Underline'), ORDER_UNDERLINE, enabled=index == art.SELECTED)
    stamp = blend_image(b, row, 'Stamp', (art.NOTICE_CX + L['stamp_dx'], y), sprite_guid(art.stamp_name(index)),
                        ORDER_STAMP, enabled=index == art.SELECTED)
    return label, underline, stamp


def build_hint(b, parent, hint):
    node = b.node(parent, hint['key'])
    blend_image(b, node, 'Tag', hint['tag'], sprite_guid(f'StageReturn_Hint_{hint["key"]}'), ORDER_HINT)
    put_text(b, node, 'Label', hint['label_pos'], hint['label'], art.HINT_COLOR[:3], ORDER_HINT_TEXT, align=ALIGN_LEFT)
    (cx, cy), (w, h) = hint['button']
    click = b.node(node, 'Click', (cx, cy))
    # Button::eventAreaSize_ は中心からの半分の幅と高さ
    return b.component(click, 'Button', eventAreaSize_=f'{w / 2},{h / 2}')


def build_tree(b, root):
    """root の下に貼り紙を組み、root に2つのコンポーネントを付ける"""
    L = art.layout()
    visual = b.node(root, 'Visual')
    (pos, scale, _) = L['veil_black']
    veil_black = blend_image(b, visual, 'VeilBlack', pos, BLACK_MASK, ORDER_VEIL_BLACK, 0, scale=scale)
    (pos, scale, _) = L['veil']
    veil = blend_image(b, visual, 'Veil', pos, BACKDROP, ORDER_VEIL, 0, scale=scale)

    notice = b.node(visual, 'Notice')
    solo = blend_image(b, notice, 'PaperSolo', art.notice_center('solo'), sprite_guid('StageReturn_Notice_Solo'),
                       ORDER_NOTICE)
    host = blend_image(b, notice, 'PaperHost', art.notice_center('host'), sprite_guid('StageReturn_Notice_Host'),
                       ORDER_NOTICE, enabled=False)
    put_text(b, notice, 'Title', L['title'], art.TITLE_TEXT, art.INK, ORDER_TEXT, font=FONT_BRUSH_INK)
    put_text(b, notice, 'Body', L['body'], art.BODY_TEXT, art.INK, ORDER_TEXT)
    host_note = put_text(b, notice, 'HostNote', L['host_note'], art.HOST_NOTE, art.STAMP_RED, ORDER_TEXT,
                         enabled=False)
    return_label, return_underline, return_stamp = build_row(b, notice, 'Return', 0, L)
    stay_label, stay_underline, stay_stamp = build_row(b, notice, 'Stay', 1, L)
    settings_label, settings_underline, settings_stamp = build_row(b, notice, 'Settings', 2, L)

    hints = b.node(visual, 'Hints')
    buttons = {h['key']: build_hint(b, hints, h) for h in L['hints']}

    ui = b.component(root, 'StageReturnNoticeUi',
                     selectedColor_=','.join(map(str, art.INK)),
                     unselectedColor_=','.join(map(str, art.INK_FADE)),
                     veilBlendRate_=L['veil'][2], veilBlackBlendRate_=L['veil_black'][2],
                     dropDistance_px_=float(L['drop_px']), enterDuration_secs_=0.2,
                     stampDuration_secs_=0.3, stampStartScale_=1.6)
    b.field(ui, 'visualRoot_', visual.guid)
    b.field(ui, 'noticeRoot_', notice.guid)
    b.field(ui, 'veilBlack_', guid_of(veil_black))
    b.field(ui, 'veil_', guid_of(veil))
    b.field(ui, 'soloNotice_', guid_of(solo))
    b.field(ui, 'hostNotice_', guid_of(host))
    b.field(ui, 'hostNoteText_', guid_of(host_note))
    b.field(ui, 'returnLabel_', guid_of(return_label))
    b.field(ui, 'returnUnderline_', guid_of(return_underline))
    b.field(ui, 'returnStamp_', guid_of(return_stamp))
    b.field(ui, 'stayLabel_', guid_of(stay_label))
    b.field(ui, 'stayUnderline_', guid_of(stay_underline))
    b.field(ui, 'stayStamp_', guid_of(stay_stamp))
    b.field(ui, 'settingsLabel_', guid_of(settings_label))
    b.field(ui, 'settingsUnderline_', guid_of(settings_underline))
    b.field(ui, 'settingsStamp_', guid_of(settings_stamp))
    b.field(ui, 'confirmButton_', guid_of(buttons['Enter']))
    b.field(ui, 'cancelButton_', guid_of(buttons['Esc']))

    presenter = b.component(root, 'StageReturnPresenter')
    b.field(presenter, 'uiSounds_', UI_SOUNDS)
    b.field(presenter, 'settingsPrefab_', SETTINGS_PREFAB)


def build_ui():
    prefab = new_prefab('StageReturnUI')
    build_tree(Builder(prefab), prefab.root)
    return save_prefab(prefab, UI_PREFAB_DIR, 'StageReturnUI')


def suppress_versions(node, leaves):
    """leaves の型の版キーを、この木では出さない (シーンの前の方で既に出ている)"""
    from tools.common.blob import Ver
    from tools.common.cereal_json import OrderedObj
    from game_over_prefab import all_nodes

    def walk(blob):
        if isinstance(blob, Ver):
            leaf = blob.key[1] if isinstance(blob.key, tuple) and len(blob.key) > 1 else ''
            if leaf in leaves:
                blob.literal_presence = False
            walk(blob.body)
        elif isinstance(blob, OrderedObj):
            for _, value in blob.items():
                walk(value)
        elif isinstance(blob, list):
            for value in blob:
                walk(value)
        elif hasattr(blob, 'data'):
            walk(blob.data)

    for n in all_nodes(node):
        for comp in n.components:
            walk(comp.data)


def place_in_scene(scene_path):
    """シーンの一番上に同じ木を組んで置く。組み直すときは前のものを外してから置く。
    prefab をコピーすると、読み戻した版キーが型名ではなく形の指紋になり、版を置き直せないので組み直す"""
    import re
    from tools.common.cereal_json import to_file_bytes
    from tools.scene import catalog as catalog_mod, reader, validate, writer
    from game_over_prefab import check, prepare

    scene = reader.read_scene_file(scene_path)
    scene.roots = [r for r in scene.roots if r.name != 'StageReturnUI']
    b = Builder(scene)
    root = b.node(None, 'StageReturnUI')
    build_tree(b, root)
    prepare([root])

    # NOTE: 読み込んだシーンの版キーは形の指紋なので、writer には同じ型が既に出ていることが分からない。
    #       validate が「2度目に版がある」と言った型を、この木では出さないようにする
    for _ in range(8):
        text_out = writer.write_scene(scene)
        problems = validate.validate_scene(scene) + validate.validate_class_versions(text_out, catalog_mod.load())
        repeated = {m.group(1) for p in problems
                    if (m := re.search(r'repeat occurrence of (\S+) carries a stray', p)) and not p.startswith('note:')}
        if not repeated:
            break
        suppress_versions(root, repeated)
    check(text_out, validate.validate_scene(scene), scene_path.name)
    scene_path.write_bytes(to_file_bytes(text_out))
    reader.read_scene_file(scene_path)
    print(f'placed StageReturnUI in {scene_path.relative_to(REPO)}')


def remove_from_scene(scene_path):
    """シーンの一番上に置いた StageReturnUI を外す"""
    from tools.common.cereal_json import to_file_bytes
    from tools.scene import reader, validate, writer
    from game_over_prefab import check

    scene = reader.read_scene_file(scene_path)
    kept = [r for r in scene.roots if r.name != 'StageReturnUI']
    if len(kept) == len(scene.roots):
        print(f'no StageReturnUI in {scene_path.relative_to(REPO)}')
        return
    scene.roots = kept
    text_out = writer.write_scene(scene)
    check(text_out, validate.validate_scene(scene), scene_path.name)
    scene_path.write_bytes(to_file_bytes(text_out))
    reader.read_scene_file(scene_path)
    print(f'removed StageReturnUI from {scene_path.relative_to(REPO)}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--into', action='append', default=[],
                    help='この .scene にも置く (SubScene の Assets/Scene/StageReturnUiScene.scene)')
    ap.add_argument('--remove-from', action='append', default=[], help='この .scene から外す')
    args = ap.parse_args()

    for scene in args.remove_from:
        remove_from_scene(Path(scene).resolve())
    build_ui()
    for scene in args.into:
        place_in_scene(Path(scene).resolve())


if __name__ == '__main__':
    main()
