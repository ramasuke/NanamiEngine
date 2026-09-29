"""tools/art/stage_arrival_caption.py の LAYOUT から、空撮の字幕プレハブ (案C「中央の題字」) を組む。

    python tools/art/stage_arrival_caption.py --emit          # 先にスプライトを書き出す
    python tools/art/stage_arrival_caption_prefab.py          # Assets/Prefab/UI/StageArrival/StageArrivalCaption.prefab

ルートに StageArrivalCaption を付ける。StageArrivalMovie が空撮を流すときだけ Instantiate し、終わったら消す。
座標はすべて画面の px (ルートは原点)。出すまでは画像の blendRate_ を 0、文字を空にしておく。
.meta(asset guid)は既存があれば保つので、組み直しても GameManage.scene の各コンテキストからの参照は切れない。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num  # noqa: E402

import stage_arrival_caption as art  # noqa: E402
from event_board_prefab import ALIGN_CENTER, FONT_BODY, FONT_BRUSH, text  # noqa: E402
from game_over_prefab import Builder, asset_guid, guid_of, new_prefab, save_prefab  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'StageArrival'
# HUD より上、会話ウィンドウ (7000) や画面 (7000〜9000 台) より下
ORDER_VIGNETTE = 6900
ORDER_BAND = 6901
ORDER_RULE = 6902
ORDER_TEXT = 6903


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


def image(b, parent, name, pos, sprite, order, scale=1.0):
    _, comp = b.image(parent, name, pos, sprite, order, 0, scale=scale)
    return comp


def label(b, parent, name, key, color, font):
    pos, px = art.LAYOUT[key]
    return text(b, parent, name, pos, px, '', color, ORDER_TEXT, font=font, align=ALIGN_CENTER)


def build():
    L = art.LAYOUT
    prefab = new_prefab('StageArrivalCaption')
    b = Builder(prefab)
    root = prefab.root

    island = b.node(root, 'Island')
    (pos, scale) = L['vignette']
    vignette = image(b, island, 'Vignette', pos, sprite_guid('StageArrival_Vignette'), ORDER_VIGNETTE, scale)
    rule_top = image(b, island, 'RuleTop', L['island_rule_top'], sprite_guid('StageArrival_RuleLong'), ORDER_RULE)
    island_title = label(b, island, 'Title', 'island_title', art.TITLE_COLOR, FONT_BRUSH)
    rule_bottom = image(b, island, 'RuleBottom', L['island_rule_bottom'], sprite_guid('StageArrival_RuleLong'), ORDER_RULE)
    island_subtitle = label(b, island, 'Subtitle', 'island_subtitle', art.SUBTITLE_COLOR, FONT_BODY)

    landmark = b.node(root, 'Landmark')
    band = image(b, landmark, 'Band', L['landmark_band'], sprite_guid('StageArrival_Band'), ORDER_BAND)
    landmark_title = label(b, landmark, 'Title', 'landmark_title', art.TITLE_COLOR, FONT_BRUSH)
    rule = image(b, landmark, 'Rule', L['landmark_rule'], sprite_guid('StageArrival_RuleShort'), ORDER_RULE)
    landmark_subtitle = label(b, landmark, 'Subtitle', 'landmark_subtitle', art.SUBTITLE_COLOR, FONT_BODY)

    caption = b.component(root, 'StageArrivalCaption')
    b.field(caption, 'islandVignette_', guid_of(vignette))
    b.field(caption, 'islandRuleTop_', guid_of(rule_top))
    b.field(caption, 'islandRuleBottom_', guid_of(rule_bottom))
    b.field(caption, 'islandTitle_', guid_of(island_title))
    b.field(caption, 'islandSubtitle_', guid_of(island_subtitle))
    b.field(caption, 'landmarkBand_', guid_of(band))
    b.field(caption, 'landmarkRule_', guid_of(rule))
    b.field(caption, 'landmarkTitle_', guid_of(landmark_title))
    b.field(caption, 'landmarkSubtitle_', guid_of(landmark_subtitle))
    caption.data['fadeIn_secs_'] = Num.of_float(0.8)
    caption.data['fadeOut_secs_'] = Num.of_float(0.6)
    caption.data['vignetteBlendRate_'] = Num.of_int(art.VIGNETTE_BLEND_RATE)
    caption.data['bandBlendRate_'] = Num.of_int(art.BAND_BLEND_RATE)
    return save_prefab(prefab, PREFAB_DIR, 'StageArrivalCaption')


if __name__ == '__main__':
    build()
