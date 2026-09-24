"""tools/art/asset_update.py の LAYOUT からタイトル画面のアセット更新プレハブ ("早馬の荷札", 案A) を組む。

    python tools/art/asset_update.py --emit          # 先にスプライトを書き出す
    python tools/art/asset_update_prefab.py          # Assets/Prefab/UI/AssetUpdate/AssetUpdateUI.prefab を組み直す
    python tools/art/asset_update_prefab.py --wire   # 加えて TitleScene の SampleTitleScene.assetUpdatePrefab_ を張る

ルートに AssetUpdateTagUi (見た目) と AssetUpdatePresenter (更新の進行と入力) を付ける。
SampleTitleScene が起動時に Instantiate し、更新が済むまでゲームを始めさせない。
座標はすべて画面の px (ルートは原点)。荷札の中身は Tag の子にして、降りてくる動きは Tag を動かすだけで済ませる。
操作ヒントは Tag の外に置き、札と一緒には動かさない。
.meta(asset guid)は既存があれば保つので、組み直しても TitleScene からの参照は切れない。
組み方の道具は game_over_prefab.py / event_board_prefab.py のものを使う。
"""
import argparse
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num  # noqa: E402
from tools.scene import edits, model  # noqa: E402

import asset_update as art  # noqa: E402
from event_board_prefab import ALIGN_CENTER, ALIGN_RIGHT, FONT_BODY, FONT_BRUSH, image, text  # noqa: E402
from game_over_prefab import BLACK_MASK, Builder, asset_guid, guid_of, new_prefab, save_prefab  # noqa: E402

UI_PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'AssetUpdate'
TITLE_SCENE = REPO / 'Assets' / 'Scene' / 'TitleScene.scene'
BACKDROP = asset_guid(str(art.BACKDROP_SPRITE) + '.meta')
HINT_CONFIRM = asset_guid(str(art.HINT_CONFIRM_SPRITE) + '.meta')
HINT_CANCEL = asset_guid(str(art.HINT_CANCEL_SPRITE) + '.meta')
# 判子を押す音は掲示板の受注印と同じ鈍い打撃、答えたときの音は店のカーソルと同じ
STAMP_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/打撃2.mp3.meta')
CONFIRM_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/ButtonClick.mp3.meta')
ALIGN_LEFT = 0

# タイトルの上に出す。ほかの画面 (7000〜8000 台) とは同時に出ないが、念のためその上の帯を使う
ORDER_VEIL_BLACK = 9000
ORDER_VEIL = 9001
ORDER_TAG = 9010
ORDER_TAG_RULE = 9011
ORDER_TAG_MARK = 9012
ORDER_TAG_TEXT = 9013
ORDER_STAMP = 9020
ORDER_HINT = 9030
ORDER_HINT_TEXT = 9031


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


def blend_image(b, parent, name, pos, sprite, order, blend, scale=1.0, enabled=True):
    _, comp = b.image(parent, name, pos, sprite, order, blend, scale=scale)
    if not enabled:
        model.set_component_enabled(comp, False)
    return comp


def put_text(b, parent, name, pos_px, s, color, order, font=FONT_BODY, align=ALIGN_CENTER):
    pos, px = pos_px
    return text(b, parent, name, pos, px, s, color, order, font=font, align=align)


def build_details(b, tag):
    L = art.LAYOUT
    details = b.node(tag, 'Details')
    values = []
    lp, vp = L['detail_label_px'], L['detail_value_px']
    for i, (y, label) in enumerate(zip(L['detail_rows'], art.DETAIL_LABELS)):
        put_text(b, details, f'Label{i}', ((L['detail_label_x'], y - lp // 2), lp), label, art.INK_FADE,
                 ORDER_TAG_TEXT, align=ALIGN_LEFT)
        values.append(put_text(b, details, f'Value{i}', ((L['detail_value_x'], y - vp // 2), vp), '', art.INK,
                               ORDER_TAG_TEXT, font=FONT_BRUSH, align=ALIGN_RIGHT))
        image(b, details, f'Rule{i}', (art.TAG_CENTER[0], y + L['detail_rule_dy']), sprite_guid('AssetUpdate_Rule'),
              ORDER_TAG_RULE)
    note = put_text(b, details, 'Note', L['note'], art.NOTE_OFFER, art.INK_FADE, ORDER_TAG_TEXT)
    return details, values, note


def build_progress(b, tag):
    L = art.LAYOUT
    progress = b.node(tag, 'Progress')
    hoofs = [image(b, progress, f'Hoof{i}', pos, sprite_guid('AssetUpdate_Hoof_Empty'), ORDER_TAG_MARK)
             for i, pos in enumerate(art.hoof_positions())]
    percent = put_text(b, progress, 'Percent', L['percent'], '0%', art.INK, ORDER_TAG_TEXT, font=FONT_BRUSH)
    amount = put_text(b, progress, 'Amount', L['amount'], '', art.INK_FADE, ORDER_TAG_TEXT)
    wait = put_text(b, progress, 'Wait', L['wait'], '', art.INK_FADE, ORDER_TAG_TEXT)
    return progress, hoofs, percent, amount, wait


def build_failure(b, tag):
    L = art.LAYOUT
    failure = b.node(tag, 'Failure')
    warning = put_text(b, failure, 'Warning', L['warning'], '', art.STAMP_RED, ORDER_TAG_TEXT)
    error = put_text(b, failure, 'Error', L['error'], '', art.INK_FADE, ORDER_TAG_TEXT)
    return failure, warning, error


def build_hint(b, parent, name, tag_key, label_key, button_key, sprite):
    L = art.LAYOUT
    hint = b.node(parent, name)
    image(b, hint, 'Tag', L[tag_key], sprite, ORDER_HINT)
    label = put_text(b, hint, 'Label', L[label_key], '', art.HINT_COLOR, ORDER_HINT_TEXT, align=ALIGN_LEFT)
    (cx, cy), (w, h) = L[button_key]
    click = b.node(hint, 'Click', (cx, cy))
    # Button::eventAreaSize_ は中心からの半分の幅と高さ
    button = b.component(click, 'Button', eventAreaSize_=f'{w / 2},{h / 2}')
    return hint, label, button


def build_ui():
    L = art.LAYOUT
    prefab = new_prefab('AssetUpdateUI')
    b = Builder(prefab)
    root = prefab.root

    visual = b.node(root, 'Visual')
    (pos, scale, _) = L['veil_black']
    veil_black = blend_image(b, visual, 'VeilBlack', pos, BLACK_MASK, ORDER_VEIL_BLACK, 0, scale=scale)
    (pos, scale, _) = L['veil']
    veil = blend_image(b, visual, 'Veil', pos, BACKDROP, ORDER_VEIL, 0, scale=scale)

    _, tag_center = art.tag_sprite()
    tag = b.node(visual, 'Tag')
    image(b, tag, 'Paper', tag_center, sprite_guid('AssetUpdate_Tag'), ORDER_TAG)
    headline = put_text(b, tag, 'Headline', L['headline'], '', art.INK, ORDER_TAG_TEXT, font=FONT_BRUSH)
    details, values, note = build_details(b, tag)
    progress, hoofs, percent, amount, wait = build_progress(b, tag)
    failure, warning, error = build_failure(b, tag)
    received = blend_image(b, tag, 'StampReceived', L['stamp_received'], sprite_guid('AssetUpdate_Stamp_Received'),
                           ORDER_STAMP, 0, enabled=False)
    undelivered = blend_image(b, tag, 'StampUndelivered', L['stamp'], sprite_guid('AssetUpdate_Stamp_Undelivered'),
                              ORDER_STAMP, 0, enabled=False)
    wrong = blend_image(b, tag, 'StampWrongVersion', L['stamp'], sprite_guid('AssetUpdate_Stamp_WrongVersion'),
                        ORDER_STAMP, 0, enabled=False)

    hints = b.node(visual, 'Hints')
    confirm_hint, confirm_label, confirm_button = build_hint(
        b, hints, 'Confirm', 'hint_confirm_tag', 'hint_confirm_label', 'hint_confirm_button', HINT_CONFIRM)
    cancel_hint, cancel_label, cancel_button = build_hint(
        b, hints, 'Cancel', 'hint_cancel_tag', 'hint_cancel_label', 'hint_cancel_button', HINT_CANCEL)

    ui = b.component(root, 'AssetUpdateTagUi')
    ui.data['veilBlendRate_'] = Num.of_int(L['veil'][2])
    ui.data['veilBlackBlendRate_'] = Num.of_int(L['veil_black'][2])
    ui.data['errorLineUnits_'] = Num.of_int(L['error_line_units'])
    ui.data['errorMaxLines_'] = Num.of_int(L['error_max_lines'])
    b.field(ui, 'visualRoot_', visual.guid)
    b.field(ui, 'tagRoot_', tag.guid)
    b.field(ui, 'veilBlack_', guid_of(veil_black))
    b.field(ui, 'veil_', guid_of(veil))
    b.field(ui, 'headlineText_', guid_of(headline))
    b.field(ui, 'detailsRoot_', details.guid)
    b.field(ui, 'fileCountText_', guid_of(values[0]))
    b.field(ui, 'sizeText_', guid_of(values[1]))
    b.field(ui, 'versionText_', guid_of(values[2]))
    b.field(ui, 'noteText_', guid_of(note))
    b.field(ui, 'progressRoot_', progress.guid)
    ui.data['hoofPrints_'] = [edits.field_blob('ImageRenderer', guid_of(h)) for h in hoofs]
    b.field(ui, 'hoofFilledSprite_', sprite_guid('AssetUpdate_Hoof_Filled'))
    b.field(ui, 'hoofEmptySprite_', sprite_guid('AssetUpdate_Hoof_Empty'))
    b.field(ui, 'percentText_', guid_of(percent))
    b.field(ui, 'amountText_', guid_of(amount))
    b.field(ui, 'waitText_', guid_of(wait))
    b.field(ui, 'failureRoot_', failure.guid)
    b.field(ui, 'warningText_', guid_of(warning))
    b.field(ui, 'errorText_', guid_of(error))
    b.field(ui, 'receivedStamp_', guid_of(received))
    b.field(ui, 'undeliveredStamp_', guid_of(undelivered))
    b.field(ui, 'wrongVersionStamp_', guid_of(wrong))
    b.field(ui, 'confirmHint_', confirm_hint.guid)
    b.field(ui, 'confirmLabel_', guid_of(confirm_label))
    b.field(ui, 'confirmButton_', guid_of(confirm_button))
    b.field(ui, 'cancelHint_', cancel_hint.guid)
    b.field(ui, 'cancelLabel_', guid_of(cancel_label))
    b.field(ui, 'cancelButton_', guid_of(cancel_button))

    presenter = b.component(root, 'AssetUpdatePresenter')
    b.field(presenter, 'stampSound_', STAMP_SOUND)
    b.field(presenter, 'confirmSound_', CONFIRM_SOUND)
    return save_prefab(prefab, UI_PREFAB_DIR, 'AssetUpdateUI')


def wire_title_scene(prefab_guid):
    """TitleScene の SampleTitleScene.assetUpdatePrefab_ にこの prefab を張る。
    v0 のまま保存されている SampleTitleScene は v1 に上げて欄を足す (欄は v1 で増えた)"""
    from tools.common.cereal_json import to_file_bytes
    from tools.scene import reader, validate, writer
    from game_over_prefab import all_nodes, check, let_writer_place_versions

    scene = reader.read_scene_file(TITLE_SCENE)
    comps = [c for root in scene.roots for n in all_nodes(root) for c in n.components
             if c.fqn == 'GamePlay::Ui::SampleTitleScene']
    if len(comps) != 1:
        raise SystemExit(f'{TITLE_SCENE.name}: expected one SampleTitleScene, found {len(comps)}')
    comp = comps[0]
    body = comp.data.body if hasattr(comp.data, 'body') else comp.data
    if 'assetUpdatePrefab_' in body:
        edits._set_field_guid(body['assetUpdatePrefab_'], prefab_guid)
    else:
        body['assetUpdatePrefab_'] = edits.field_blob('PrefabGameObjectFile', prefab_guid)
    comp.class_version = 1
    let_writer_place_versions(comp.data)

    text_out = writer.write_scene(scene)
    check(text_out, validate.validate_scene(scene), TITLE_SCENE.name)
    TITLE_SCENE.write_bytes(to_file_bytes(text_out))
    reader.read_scene_file(TITLE_SCENE)
    print(f'wired {TITLE_SCENE.relative_to(REPO)}: SampleTitleScene.assetUpdatePrefab_ = {prefab_guid}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--wire', action='store_true', help='TitleScene の SampleTitleScene.assetUpdatePrefab_ を張る')
    args = ap.parse_args()

    guid, _ = build_ui()
    if args.wire:
        wire_title_scene(guid)


if __name__ == '__main__':
    main()
