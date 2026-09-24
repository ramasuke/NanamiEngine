"""MagicCaster の魔法パレットのプレハブを tools/art/spell_palette.py の layout() から組む。

    python tools/art/spell_palette.py              # 先にスプライトを書き出す
    python tools/art/spell_palette_prefab.py       # SpellSlot / SpellPaletteUI を組み直す
    python tools/art/spell_palette_prefab.py --wire  # 加えて MagicCasterStatusPresenter.prefab を MagicCaster::StatusPresenter にする

各 prefab の .meta(asset guid)は既存があれば保つので、組み直しても外からの参照は切れない
(中の GameObject / Component の guid は毎回新しくなる)。組み方は tools/art/pause_menu_prefab.py と同じ。
"""
import argparse
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402

import pause_menu_prefab as base  # noqa: E402
import spell_palette as art  # noqa: E402

base.PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'SpellPalette'
STATUS_PRESENTER_PREFAB = REPO / 'Assets' / 'Prefab' / 'PlayerAvatar' / 'MagicCaster' / 'MagicCasterStatusPresenter.prefab'
SPRITE_DIR = REPO / 'Assets' / 'Art' / 'UI' / 'SpellPalette'
CONTROL_GUIDE_DIR = REPO / 'Assets' / 'Art' / 'UI' / 'ControlGuide'

FONT_PX = 60
FONT_BRUSH = base.FONT_BRUSH
TEXT_ALIGN = base.TEXT_ALIGN

ORDER_HALO = 30
ORDER_CIRCLE = 31
ORDER_MANA = 32
ORDER_SLOT = 34
ORDER_TEXT = 44

BLEND_ALPHA = 1
BLEND_ADD = 2


def sprite(name, directory=SPRITE_DIR):
    return base.asset_guid(directory / f'{name}.png.meta')


class Builder(base.PrefabBuilder):
    def blend_image(self, parent, name, pos, sprite_guid, order, blend=BLEND_ALPHA, scale=1.0):
        node = self.node(parent, name, pos, scale)
        comp = self.component(node, 'BlendImageRenderer', spriteFile_=sprite_guid or edits.EMPTY_GUID,
                              blendRate_='255', renderOrder_=order)
        comp.data['blendMode_'] = Num.of_int(blend)
        return node, comp

    def gauge(self, parent, name, pos, sprite_guid, order, start, span, fill):
        node = self.node(parent, name, pos)
        comp = self.component(node, 'CircleGaugeRenderer', spriteFile_=sprite_guid, blendRate_='255',
                              renderOrder_=order, startPercent_=start, spanPercent_=span, fillRate_=fill)
        comp.data['blendMode_'] = Num.of_int(BLEND_ALPHA)
        return node, comp

    def label(self, parent, name, spec, text, color, order=ORDER_TEXT):
        node = self.node(parent, name, spec['pos'], spec['px'] / FONT_PX)
        comp = self.component(node, 'TextRenderer', fontFile_=FONT_BRUSH, renderOrder_=order, text_=text,
                              isWorldPos_='false', textColor_=','.join(str(c) for c in color))
        comp.data['textAlign_'] = Num.of_int(TEXT_ALIGN[spec['align']])
        return node, comp


def build_slot(geo):
    b = Builder('SpellSlot')
    slot = geo['slot']
    content = b.node(b.root, 'Content')
    _, socket = b.blend_image(content, 'Socket', (0, 0), sprite('SpellSlot_Socket'), ORDER_SLOT)
    _, icon = b.blend_image(content, 'Icon', slot['icon'], None, ORDER_SLOT + 1)
    _, lack = b.blend_image(content, 'ManaLackShade', (0, 0), sprite('SpellSlot_ManaLackShade'), ORDER_SLOT + 2)
    _, cooldown = b.gauge(content, 'Cooldown', (0, 0), sprite('SpellSlot_CooldownShade'), ORDER_SLOT + 3, '0', '100', '0')
    _, active = b.blend_image(content, 'Active', (0, 0), sprite('SpellSlot_SocketActive'), ORDER_SLOT + 4, BLEND_ADD)
    _, cooldown_text = b.label(content, 'CooldownText', slot['cooldown_text'], '', (240, 244, 250))
    _, cost_pill = b.blend_image(content, 'CostPill', slot['cost_pill'], sprite('SpellSlot_CostPill'), ORDER_SLOT + 5)
    _, cost_text = b.label(content, 'CostText', slot['cost_text'], '', (214, 192, 255))
    _, glyph = b.blend_image(content, 'Glyph', (0, -slot['glyph_distance']), None, ORDER_SLOT + 6)

    comp = b.component(b.root, 'SpellSlot', glyphDistance_=slot['glyph_distance'])
    b.field(comp, 'content_', content.guid)
    b.field(comp, 'socket_', base.guid_of(socket))
    b.field(comp, 'active_', base.guid_of(active))
    b.field(comp, 'icon_', base.guid_of(icon))
    b.field(comp, 'manaLackShade_', base.guid_of(lack))
    b.field(comp, 'cooldown_', base.guid_of(cooldown))
    b.field(comp, 'cooldownText_', base.guid_of(cooldown_text))
    b.field(comp, 'costPill_', base.guid_of(cost_pill))
    b.field(comp, 'costText_', base.guid_of(cost_text))
    b.field(comp, 'glyph_', base.guid_of(glyph))
    comp.data['costColor_'] = edits.color32_blob(214, 192, 255)
    comp.data['costLackColor_'] = edits.color32_blob(255, 120, 104)
    return base.save_prefab(b, 'SpellSlot')


def build_palette(geo, slot_prefab):
    b = Builder('SpellPaletteUI')
    b.root.transform.local_pos = edits._vec3_from_floats((geo['root'][0], geo['root'][1], 0.0))

    _, halo = b.blend_image(b.root, 'Halo', (0, 0), sprite('SpellPalette_Halo'), ORDER_HALO)
    _, circle = b.blend_image(b.root, 'Circle', (0, 0), sprite('SpellPalette_Circle'), ORDER_CIRCLE)
    _, track = b.blend_image(b.root, 'ManaTrack', (0, 0), sprite('SpellPalette_ManaTrack'), ORDER_MANA)
    arc = geo['arc']
    _, fill = b.gauge(b.root, 'ManaFill', (0, 0), sprite('SpellPalette_ManaFill'), ORDER_MANA + 1,
                      str(arc['start_deg'] / 360.0 * 100.0), str(arc['span_deg'] / 360.0 * 100.0), '1')
    _, tip = b.blend_image(b.root, 'ManaTip', (0, -arc['radius']), sprite('SpellPalette_ManaTip'), ORDER_MANA + 2, BLEND_ADD)

    slots_root = b.node(b.root, 'Slots')
    anchors_root = b.node(b.root, 'Anchors')
    anchors = {}
    for key, pos in list(geo['front_anchors'].items()) + list(geo['back_anchors'].items()):
        anchors[key] = b.node(anchors_root, f'Anchor{key}', pos)

    _, palette_glyph = b.blend_image(b.root, 'PaletteGlyph', geo['palette_glyph'], sprite('SpellPalette_Pad_LT'), ORDER_TEXT)
    _, mana_label = b.label(b.root, 'ManaLabel', geo['mana_label'], 'MP', (206, 186, 255))
    _, mana_value = b.label(b.root, 'ManaValue', geo['mana_value'], '100', (248, 246, 255))
    _, page_glyph = b.blend_image(b.root, 'PageGlyph', geo['page_glyph'], sprite('SpellPalette_Pad_RB'), ORDER_TEXT)
    _, page_text = b.label(b.root, 'PageText', geo['page_text'], 'Ⅱ', (220, 210, 250))

    names = {}
    for key, spec in geo['names'].items():
        _, names[key] = b.label(b.root, f'Name{key}', spec, '', (238, 242, 240))

    comp = b.component(b.root, 'SpellPalette',
                       manaArcRadius_=arc['radius'], manaArcStartDeg_=arc['start_deg'], manaArcSpanDeg_=arc['span_deg'],
                       idleAlphaRate_='0.75', backScale_=geo['back_scale'], backAlphaRate_='0.6', manaLackIconRate_='0.45',
                       fadeDuration_secs_='0.2', pageSwapDuration_secs_='0.15')
    b.field(comp, 'slotsRoot_', slots_root.guid)
    b.field(comp, 'slotPrefab_', slot_prefab)
    for key in ('Top', 'Right', 'Bottom', 'Left', 'TopRight', 'BottomRight', 'BottomLeft', 'TopLeft'):
        b.field(comp, f'anchor{key}_', anchors[key].guid)
    for key in ('Top', 'Right', 'Bottom', 'Left'):
        b.field(comp, f'name{key}_', base.guid_of(names[key]))
    b.field(comp, 'halo_', base.guid_of(halo))
    b.field(comp, 'circle_', base.guid_of(circle))
    b.field(comp, 'manaTrack_', base.guid_of(track))
    b.field(comp, 'manaFill_', base.guid_of(fill))
    b.field(comp, 'manaTip_', base.guid_of(tip))
    b.field(comp, 'manaLabel_', base.guid_of(mana_label))
    b.field(comp, 'manaValue_', base.guid_of(mana_value))
    b.field(comp, 'paletteGlyph_', base.guid_of(palette_glyph))
    b.field(comp, 'pageGlyph_', base.guid_of(page_glyph))
    b.field(comp, 'pageText_', base.guid_of(page_text))
    # 時計回りに 上=Y(1) 右=B(2) 下=A(3) 左=X(4)。パッドの面ボタンは操作ガイドの画像を使う
    for key, pad, keyboard in (('Top', 'Y', '1'), ('Right', 'B', '2'), ('Bottom', 'A', '3'), ('Left', 'X', '4')):
        b.field(comp, f'padGlyph{key}_', sprite(f'ControlGuide_Pad_{pad}', CONTROL_GUIDE_DIR))
        b.field(comp, f'keyGlyph{key}_', sprite(f'SpellPalette_Key_{keyboard}'))
    b.field(comp, 'padPaletteSprite_', sprite('SpellPalette_Pad_LT'))
    b.field(comp, 'keyPaletteSprite_', sprite('SpellPalette_Key_Range'))
    b.field(comp, 'padPageSprite_', sprite('SpellPalette_Pad_RB'))
    b.field(comp, 'keyPageSprite_', sprite('SpellPalette_Mouse_Right'))
    comp.data['manaValueColor_'] = edits.color32_blob(248, 246, 255)
    return base.save_prefab(b, 'SpellPaletteUI')


def wire_status_presenter(palette_prefab):
    """OtherPlayer::StatusPresenter を MagicCaster::StatusPresenter(spellPalettePrefab_) に差し替える"""
    prefab = reader.read_prefab_file(STATUS_PRESENTER_PREFAB)
    root = prefab.root
    cat = catalog_mod.load()
    for i, comp in enumerate(list(root.components)):
        if comp.fqn in ('GamePlay::PlayerAvatar::OtherPlayer::StatusPresenter',
                        'GamePlay::PlayerAvatar::MagicCaster::StatusPresenter'):
            edits.remove_component(prefab, root.guid, i)
            break
    comp = edits.add_component(prefab, root.guid, 'GamePlay::PlayerAvatar::MagicCaster::StatusPresenter', cat=cat,
                               params={'spellPalettePrefab_': palette_prefab})
    base.let_writer_place_versions(comp.data)

    text = writer.write_prefab(prefab)
    problems = validate.validate_prefab(prefab) + validate.validate_class_versions(text, cat)
    hard = [p for p in problems if not p.startswith('note:')]
    for p in problems:
        print('  ' + (p if p.startswith('note:') else 'FAIL: ' + p))
    if hard:
        raise SystemExit('MagicCasterStatusPresenter.prefab: validation failed - nothing written')
    STATUS_PRESENTER_PREFAB.write_bytes(to_file_bytes(text))
    print(f'wrote {STATUS_PRESENTER_PREFAB.relative_to(REPO)}  (spellPalettePrefab_ -> {palette_prefab})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--wire', action='store_true', help='MagicCasterStatusPresenter.prefab を MagicCaster::StatusPresenter にする')
    args = ap.parse_args()

    geo = art.layout()
    slot = build_slot(geo)
    palette = build_palette(geo, slot)
    if args.wire:
        wire_status_presenter(palette)


if __name__ == '__main__':
    main()
