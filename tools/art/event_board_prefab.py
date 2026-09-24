"""Build the event-board UI prefabs from tools/art/event_board.py's LAYOUT / V2.

    python tools/art/event_board.py --emit        # 先にスプライトを書き出す
    python tools/art/event_board_prefab.py        # 掲示板の UI プレハブ一式を組み直す
    python tools/art/event_board_prefab.py --prop # 加えて 3D の看板 Prop/EventNoticeBoard.prefab
    python tools/art/event_board_prefab.py --place # 加えて看板を MainIslandScene の酒場の仲介人の右に置く

組むもの: 行(EventBoardRow / EventBoardQuestRow / EventBoardNoticeRow / EventBoardRestorationRow)、木札(EventBoardTab)、
頁(EventBoardEventPage / EventBoardQuestPage / EventBoardNoticePage / EventBoardRestorationPage)、
それらを生成するルートの EventBoardUI。
.meta(asset guid)は既存があれば保つので、組み直しても外(看板の eventBoardUiPrefab_ など)からの参照は切れない。
組み方の道具(Builder / save_prefab など)は game_over_prefab.py のものを使う。
"""
import argparse
import copy
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, mathutil, model, reader, validate, writer  # noqa: E402

import event_board as art  # noqa: E402
from game_over_prefab import (  # noqa: E402
    ALIGN_LEFT, BLACK_MASK, FONT_PX, Builder, asset_guid, check, guid_of, new_prefab, save_prefab)

PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'EventBoard'
FONT_BODY = asset_guid(REPO / 'Assets/Art/Font/ZenOldMincho-Bold.ttf.meta')
FONT_BRUSH = asset_guid(REPO / 'Assets/Art/Font/KaiseiDecol-Bold.ttf.meta')
BOARD_DATA = asset_guid(REPO / 'Assets/Data/EventNotice/MainIslandEventBoard.eventBoard.meta')
HINT_CANCEL = asset_guid(str(art.HINT_CANCEL_SPRITE) + '.meta')
HINT_CONFIRM = asset_guid(str(art.HINT_CONFIRM_SPRITE) + '.meta')
PIP_FILLED = asset_guid(str(art.PIP_FILLED_SPRITE) + '.meta')
PIP_EMPTY = asset_guid(str(art.PIP_EMPTY_SPRITE) + '.meta')
# 受注印を押す音。ゲームオーバーの石版が落ちる音と同じ鈍い打撃
ACCEPT_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/打撃2.mp3.meta')
# 普請の代金を払う音と、足りないときの断りの音は店と同じ
RESTORE_SOUND = asset_guid(REPO / 'Assets/Audio/UI/Shop_Purchase.mp3.meta')
REFUSE_SOUND = asset_guid(REPO / 'Assets/Audio/UI/Shop_Refuse.mp3.meta')
ALIGN_CENTER = 1
ALIGN_RIGHT = 2

# 他のHUDより上、ゲームオーバー(8000台)より下
ORDER_VEIL = 7000
ORDER_BOARD = 7010
ORDER_TAB_CORDS = 7011
ORDER_TAB_ROPE = 7012
ORDER_TAB = 7013
ORDER_TAB_LABEL = 7014
ORDER_TAB_BADGE = 7015
ORDER_TAB_BADGE_TEXT = 7016
ORDER_TICKET = 7020
ORDER_TICKET_NAIL = 7021
ORDER_TICKET_TEXT = 7022
ORDER_TICKET_STAMP = 7023
ORDER_TICKET_SEAL = 7024
ORDER_ARROW = 7025
ORDER_POSTER = 7030
ORDER_BANNER = 7031
ORDER_BANNER_SHADE = 7032
ORDER_BANNER_FRAME = 7033
ORDER_POSTER_TEXT = 7034
ORDER_POSTER_STAMP = 7035
ORDER_HINT = 7040
ORDER_HINT_TEXT = 7041


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


def rgb(color):
    return ','.join(str(c) for c in color)


def image(b, parent, name, pos, sprite, order, enabled=True, scale=1.0):
    node = b.node(parent, name, pos, scale)
    comp = b.component(node, 'ImageRenderer', spriteFile_=sprite, renderPriority_=order)
    if not enabled:
        model.set_component_enabled(comp, False)
    return comp


def text(b, parent, name, pos, px, s, color, order, font=FONT_BODY, align=ALIGN_LEFT, enabled=True):
    node = b.node(parent, name, pos, px / FONT_PX)
    comp = b.component(node, 'TextRenderer', fontFile_=font, renderOrder_=order, text_=s,
                       isWorldPos_='false', textColor_=rgb(color))
    comp.data['textAlign_'] = Num.of_int(align)
    if not enabled:
        model.set_component_enabled(comp, False)
    return comp


def pips(b, parent, name, first, gap, scale, order):
    """難度の点5つ。中身の塗り分けは C++ 側 (ShowQuestBoardRankPips) が毎回決める"""
    return [image(b, parent, f'{name}{i}', (first[0] + gap * i, first[1]), PIP_EMPTY, order, scale=scale)
            for i in range(5)]


def ticket_root(b, root):
    """札の土台。root に紙の絵と当たり判定、子に釘と選択中の蝋を付ける"""
    L = art.LAYOUT
    b.component(root, 'ImageRenderer', spriteFile_=sprite_guid('Ticket_Unselected'), renderPriority_=ORDER_TICKET)
    w, h = art.TICKET_SIZE
    b.component(root, 'Button', eventAreaSize_=f'{w},{h}')
    image(b, root, 'Nail', L['row_nail'], sprite_guid('Nail_Small'), ORDER_TICKET_NAIL)
    return image(b, root, 'WaxSeal', L['row_seal'], sprite_guid('WaxSeal'), ORDER_TICKET_SEAL, enabled=False)


def ticket_fields(b, row, seal, selected_key='selectedTicketSprite_', unselected_key='unselectedTicketSprite_'):
    b.field(row, 'waxSeal_', guid_of(seal))
    b.field(row, selected_key, sprite_guid('Ticket_Selected'))
    b.field(row, unselected_key, sprite_guid('Ticket_Unselected'))


# ---------------------------------------------------------------- 行
def build_row():
    """催しの札"""
    L = art.LAYOUT
    prefab = new_prefab('EventBoardRow')
    b = Builder(prefab)
    root = prefab.root

    seal = ticket_root(b, root)
    title = text(b, root, 'TitleText', L['row_title'][0], L['row_title'][1], '', art.INK,
                 ORDER_TICKET_TEXT, font=FONT_BRUSH)
    status = text(b, root, 'StatusText', L['row_status'][0], L['row_status'][1], '', art.INK_FADE,
                  ORDER_TICKET_TEXT)
    stamp = image(b, root, 'OngoingStamp', L['row_stamp'], sprite_guid('OngoingStamp_Small'),
                  ORDER_TICKET_STAMP, enabled=False)

    row = b.component(root, 'EventBoardRow', selectedScale_=str(L['row_selected_scale']),
                      ongoingStatusColor_=rgb(art.STAMP_RED), upcomingStatusColor_=rgb(art.INK_FADE))
    b.field(row, 'titleText_', guid_of(title))
    b.field(row, 'statusText_', guid_of(status))
    b.field(row, 'ongoingStamp_', guid_of(stamp))
    ticket_fields(b, row, seal, 'selectedNoticeSprite_', 'unselectedNoticeSprite_')
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardRow')


def build_quest_row():
    """依頼の札"""
    L, V = art.LAYOUT, art.V2
    prefab = new_prefab('EventBoardQuestRow')
    b = Builder(prefab)
    root = prefab.root

    seal = ticket_root(b, root)
    title = text(b, root, 'TitleText', V['qrow_title'][0], V['qrow_title'][1], '', art.INK,
                 ORDER_TICKET_TEXT, font=FONT_BRUSH)
    image(b, root, 'Coin', V['qrow_coin'], sprite_guid('Coin_Small'), ORDER_TICKET_TEXT)
    reward = text(b, root, 'RewardText', V['qrow_reward'][0], V['qrow_reward'][1], '', art.INK,
                  ORDER_TICKET_TEXT, font=FONT_BRUSH)
    place = text(b, root, 'PlaceText', V['qrow_place'][0], V['qrow_place'][1], '', art.INK_FADE, ORDER_TICKET_TEXT)
    first, gap, scale = V['qrow_pips']
    rank = pips(b, root, 'Pip', first, gap, scale, ORDER_TICKET_TEXT)
    chip = image(b, root, 'EventChip', V['qrow_event_chip'], sprite_guid('EventChip_Small'), ORDER_TICKET_STAMP,
                 enabled=False)
    stamp = image(b, root, 'StateStamp', V['qrow_stamp'], sprite_guid('QuestStamp_Taking'), ORDER_TICKET_STAMP,
                  enabled=False)

    row = b.component(root, 'EventBoardQuestRow', selectedScale_=str(L['row_selected_scale']))
    b.field(row, 'titleText_', guid_of(title))
    b.field(row, 'placeText_', guid_of(place))
    b.field(row, 'rewardText_', guid_of(reward))
    row.data['rankPips_'] = [edits.field_blob('ImageRenderer', guid_of(p)) for p in rank]
    b.field(row, 'eventChip_', guid_of(chip))
    b.field(row, 'stateStamp_', guid_of(stamp))
    b.field(row, 'filledPipSprite_', PIP_FILLED)
    b.field(row, 'emptyPipSprite_', PIP_EMPTY)
    b.field(row, 'takingStampSprite_', sprite_guid('QuestStamp_Taking'))
    b.field(row, 'clearedStampSprite_', sprite_guid('QuestStamp_Cleared'))
    b.field(row, 'preparingStampSprite_', sprite_guid('QuestStamp_Preparing'))
    ticket_fields(b, row, seal)
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardQuestRow')


def build_notice_row():
    """お知らせの札"""
    L, V = art.LAYOUT, art.V2
    prefab = new_prefab('EventBoardNoticeRow')
    b = Builder(prefab)
    root = prefab.root

    seal = ticket_root(b, root)
    chip = image(b, root, 'KindChip', V['nrow_chip'], sprite_guid('KindChip_Update'), ORDER_TICKET_TEXT)
    date = text(b, root, 'DateText', V['nrow_date'][0], V['nrow_date'][1], '', art.INK_FADE, ORDER_TICKET_TEXT,
                align=ALIGN_RIGHT)
    title = text(b, root, 'TitleText', V['nrow_title'][0], V['nrow_title'][1], '', art.INK, ORDER_TICKET_TEXT)
    unread = image(b, root, 'UnreadSeal', V['nrow_unread'], sprite_guid('UnreadSeal'), ORDER_TICKET_SEAL,
                   enabled=False)

    row = b.component(root, 'EventBoardNoticeRow', selectedScale_=str(L['row_selected_scale']))
    b.field(row, 'kindChip_', guid_of(chip))
    b.field(row, 'dateText_', guid_of(date))
    b.field(row, 'titleText_', guid_of(title))
    b.field(row, 'unreadSeal_', guid_of(unread))
    row.data['kindChipSprites_'] = [edits.field_blob('SpriteFile', sprite_guid(f'KindChip_{k}')) for k in art.V2_KINDS]
    ticket_fields(b, row, seal)
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardNoticeRow')


def build_restoration_row():
    """普請の札"""
    L, V = art.LAYOUT, art.V2
    prefab = new_prefab('EventBoardRestorationRow')
    b = Builder(prefab)
    root = prefab.root

    seal = ticket_root(b, root)
    name = text(b, root, 'NameText', V['rrow_name'][0], V['rrow_name'][1], '', art.INK,
                ORDER_TICKET_TEXT, font=FONT_BRUSH)
    image(b, root, 'Coin', V['rrow_coin'], sprite_guid('Coin_Small'), ORDER_TICKET_TEXT)
    cost = text(b, root, 'CostText', V['rrow_cost'][0], V['rrow_cost'][1], '', art.INK,
                ORDER_TICKET_TEXT, font=FONT_BRUSH)
    note = text(b, root, 'NoteText', V['rrow_note'][0], V['rrow_note'][1], '', art.INK_FADE, ORDER_TICKET_TEXT)
    stamp = image(b, root, 'StateStamp', V['rrow_stamp'], sprite_guid('RestorationStamp_Restored'),
                  ORDER_TICKET_STAMP, enabled=False)

    row = b.component(root, 'EventBoardRestorationRow', selectedScale_=str(L['row_selected_scale']),
                      defaultNoteColor_=rgb(art.INK_FADE), refusedNoteColor_=rgb(art.STAMP_RED))
    b.field(row, 'nameText_', guid_of(name))
    b.field(row, 'costText_', guid_of(cost))
    b.field(row, 'noteText_', guid_of(note))
    b.field(row, 'stateStamp_', guid_of(stamp))
    b.field(row, 'restoredStampSprite_', sprite_guid('RestorationStamp_Restored'))
    b.field(row, 'lockedStampSprite_', sprite_guid('RestorationStamp_Locked'))
    ticket_fields(b, row, seal)
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardRestorationRow')


def build_tab():
    """木札の見出し1枚。root が板の中心"""
    V = art.V2
    prefab = new_prefab('EventBoardTab')
    b = Builder(prefab)
    root = prefab.root

    b.component(root, 'ImageRenderer', spriteFile_=sprite_guid('Tab_Unselected'), renderPriority_=ORDER_TAB)
    w, h = art.TAB_SIZE
    b.component(root, 'Button', eventAreaSize_=f'{w},{h}')
    image(b, root, 'Cords', V['tab_cords'], sprite_guid('TabCords'), ORDER_TAB_CORDS)
    label = text(b, root, 'Label', V['tab_label'][0], V['tab_label'][1], '', V['tab_label_unselected'],
                 ORDER_TAB_LABEL, font=FONT_BRUSH, align=ALIGN_CENTER)
    badge = b.node(root, 'Badge', V['tab_badge'])
    image(b, badge, 'Seal', (0, 0), sprite_guid('TabBadge'), ORDER_TAB_BADGE)
    count = text(b, badge, 'CountText', V['tab_badge_text'][0], V['tab_badge_text'][1], '', (252, 236, 210),
                 ORDER_TAB_BADGE_TEXT, align=ALIGN_CENTER)

    tab = b.component(root, 'EventBoardTab', selectedLabelColor_=rgb(V['tab_label_selected']),
                      unselectedLabelColor_=rgb(V['tab_label_unselected']),
                      selectedScale_=str(V['tab_selected_scale']), selectedDrop_px_=str(V['tab_selected_drop']))
    b.field(tab, 'labelText_', guid_of(label))
    b.field(tab, 'badgeRoot_', badge.guid)
    b.field(tab, 'badgeCountText_', guid_of(count))
    b.field(tab, 'selectedSprite_', sprite_guid('Tab_Selected'))
    b.field(tab, 'unselectedSprite_', sprite_guid('Tab_Unselected'))
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardTab')


# ---------------------------------------------------------------- 頁 (root は画面の原点。中の座標は画面座標)
def page_list(b, root, page, row_prefab_guid):
    """左の札の並び。3頁とも同じ位置に置く"""
    V = art.V2
    rows = b.node(root, 'Rows', V['rows_origin'])
    up = image(b, root, 'MoreAbove', V['arrow_up'], sprite_guid('Arrow_Up'), ORDER_ARROW, enabled=False)
    down = image(b, root, 'MoreBelow', V['arrow_down'], sprite_guid('Arrow_Down'), ORDER_ARROW, enabled=False)
    page.data['rowSpacing_px_'] = Num.of_float(float(V['row_spacing']))
    page.data['maxVisibleRows_'] = Num.of_int(V['max_rows'])
    b.field(page, 'rowPrefab_', row_prefab_guid)
    b.field(page, 'rowsRoot_', rows.guid)
    b.field(page, 'moreAboveMark_', guid_of(up))
    b.field(page, 'moreBelowMark_', guid_of(down))


def page_paper(b, root, sprite, empty_label):
    """右の紙と釘。一覧が空のときは紙の中身ごと隠して empty_label を出す"""
    V = art.V2
    image(b, root, 'Paper', V['poster'], sprite, ORDER_POSTER)
    image(b, root, 'PaperNail', V['poster_nail'], sprite_guid('Nail'), ORDER_POSTER_STAMP)
    return text(b, root, 'EmptyText', V['empty'][0], V['empty'][1], empty_label, art.INK_FADE, ORDER_POSTER_TEXT,
                align=ALIGN_CENTER, enabled=False)


def build_event_page(row_prefab_guid):
    L = art.LAYOUT
    prefab = new_prefab('EventBoardEventPage')
    b = Builder(prefab)
    root = prefab.root
    page = b.component(root, 'EventBoardEventPage', ongoingStatusColor_=rgb(art.STAMP_RED),
                       upcomingStatusColor_=rgb(art.INK))
    page_list(b, root, page, row_prefab_guid)
    empty = page_paper(b, root, sprite_guid('Poster'), 'いま予定されている催しはありません')

    detail = b.node(root, 'Detail')
    banner = image(b, detail, 'Banner', L['banner'], edits.EMPTY_GUID, ORDER_BANNER)
    image(b, detail, 'BannerShade', L['banner'], sprite_guid('BannerShade'), ORDER_BANNER_SHADE)
    image(b, detail, 'BannerFrame', L['banner'], sprite_guid('BannerFrame'), ORDER_BANNER_FRAME)
    title = text(b, detail, 'TitleText', L['title'][0], L['title'][1], '', art.INK, ORDER_POSTER_TEXT,
                 font=FONT_BRUSH)
    tag = text(b, detail, 'TagText', L['tag'][0], L['tag'][1], '', art.INK_FADE, ORDER_POSTER_TEXT)
    status = text(b, detail, 'StatusText', L['status'][0], L['status'][1], '', art.INK, ORDER_POSTER_TEXT,
                  align=ALIGN_RIGHT)
    image(b, detail, 'Rule', L['rule'], sprite_guid('Rule'), ORDER_POSTER_TEXT)
    text(b, detail, 'PeriodLabel', L['period_label'][0], L['period_label'][1], '期 間', art.INK_FADE,
         ORDER_POSTER_TEXT)
    period = text(b, detail, 'PeriodText', L['period'][0], L['period'][1], '', art.INK, ORDER_POSTER_TEXT)
    (dx, dy), dpx, gap, count = L['desc']
    lines = [text(b, detail, f'DescLine{i}', (dx, dy + i * gap), dpx, '', art.INK, ORDER_POSTER_TEXT)
             for i in range(count)]
    stamp = image(b, detail, 'OngoingStamp', L['stamp'], sprite_guid('OngoingStamp'), ORDER_POSTER_STAMP,
                  enabled=False)

    b.field(page, 'detailRoot_', detail.guid)
    b.field(page, 'detailBanner_', guid_of(banner))
    b.field(page, 'detailTitleText_', guid_of(title))
    b.field(page, 'detailTagText_', guid_of(tag))
    b.field(page, 'detailPeriodText_', guid_of(period))
    b.field(page, 'detailStatusText_', guid_of(status))
    b.field(page, 'detailOngoingStamp_', guid_of(stamp))
    b.field(page, 'emptyText_', guid_of(empty))
    page.data['detailDescriptionLines_'] = [edits.field_blob('TextRenderer', guid_of(line)) for line in lines]
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardEventPage')


def build_quest_page(row_prefab_guid):
    V = art.V2
    prefab = new_prefab('EventBoardQuestPage')
    b = Builder(prefab)
    root = prefab.root
    page = b.component(root, 'EventBoardQuestPage', takingStateColor_=rgb(art.STAMP_RED),
                       defaultStateColor_=rgb(art.INK))
    page_list(b, root, page, row_prefab_guid)
    empty = page_paper(b, root, sprite_guid('QuestPoster'), 'いま貼り出されている依頼はありません')

    detail = b.node(root, 'Detail')
    event_chip = image(b, detail, 'EventChip', V['q_event_chip'], sprite_guid('EventChip'), ORDER_POSTER_TEXT)
    event_text = text(b, detail, 'EventText', V['q_event_text'][0], V['q_event_text'][1], '', art.V2_EVENT_COLOR,
                      ORDER_POSTER_TEXT)
    photo_root = b.node(detail, 'Photo', V['q_photo'])
    photo = image(b, photo_root, 'Picture', (0, 0), edits.EMPTY_GUID, ORDER_BANNER)
    image(b, photo_root, 'Shade', (0, 0), sprite_guid('QuestPhotoShade'), ORDER_BANNER_SHADE)
    image(b, photo_root, 'Frame', (0, 0), sprite_guid('QuestPhotoFrame'), ORDER_BANNER_FRAME)
    title = text(b, detail, 'TitleText', V['q_title'][0], V['q_title'][1], '', art.INK, ORDER_POSTER_TEXT,
                 font=FONT_BRUSH)
    client = text(b, detail, 'ClientText', V['q_client'][0], V['q_client'][1], '', art.INK, ORDER_POSTER_TEXT)
    place = text(b, detail, 'PlaceText', V['q_place'][0], V['q_place'][1], '', art.INK, ORDER_POSTER_TEXT)
    first, gap, scale = V['q_pips']
    rank = pips(b, detail, 'Pip', first, gap, scale, ORDER_POSTER_TEXT)
    state = text(b, detail, 'StateText', V['q_state'][0], V['q_state'][1], '', art.INK, ORDER_POSTER_TEXT)
    goal = text(b, detail, 'GoalText', V['q_goal'][0], V['q_goal'][1], '', art.INK, ORDER_POSTER_TEXT)
    reward = text(b, detail, 'RewardText', V['q_reward'][0], V['q_reward'][1], '', art.INK, ORDER_POSTER_TEXT,
                  font=FONT_BRUSH)
    limit = text(b, detail, 'LimitText', V['q_limit'][0], V['q_limit'][1], '', art.INK, ORDER_POSTER_TEXT)
    (dx, dy), dpx, line_gap, count = V['q_desc']
    lines = [text(b, detail, f'DescLine{i}', (dx, dy + i * line_gap), dpx, '', art.INK, ORDER_POSTER_TEXT)
             for i in range(count)]
    seal = image(b, detail, 'Seal', V['q_seal'], sprite_guid('QuestSeal_Open'), ORDER_POSTER_STAMP)

    b.field(page, 'detailRoot_', detail.guid)
    b.field(page, 'detailPhotoRoot_', photo_root.guid)
    b.field(page, 'detailPhoto_', guid_of(photo))
    b.field(page, 'detailEventChip_', guid_of(event_chip))
    b.field(page, 'detailEventText_', guid_of(event_text))
    b.field(page, 'detailTitleText_', guid_of(title))
    b.field(page, 'detailClientText_', guid_of(client))
    b.field(page, 'detailPlaceText_', guid_of(place))
    page.data['detailRankPips_'] = [edits.field_blob('ImageRenderer', guid_of(p)) for p in rank]
    b.field(page, 'detailStateText_', guid_of(state))
    b.field(page, 'detailGoalText_', guid_of(goal))
    b.field(page, 'detailRewardText_', guid_of(reward))
    b.field(page, 'detailLimitText_', guid_of(limit))
    page.data['detailDescriptionLines_'] = [edits.field_blob('TextRenderer', guid_of(line)) for line in lines]
    b.field(page, 'detailSeal_', guid_of(seal))
    b.field(page, 'emptyText_', guid_of(empty))
    b.field(page, 'filledPipSprite_', PIP_FILLED)
    b.field(page, 'emptyPipSprite_', PIP_EMPTY)
    b.field(page, 'openSealSprite_', sprite_guid('QuestSeal_Open'))
    b.field(page, 'takingSealSprite_', sprite_guid('QuestSeal_Taking'))
    b.field(page, 'clearedSealSprite_', sprite_guid('QuestSeal_Cleared'))
    b.field(page, 'preparingSealSprite_', sprite_guid('QuestSeal_Preparing'))
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardQuestPage')


def build_notice_page(row_prefab_guid):
    V = art.V2
    prefab = new_prefab('EventBoardNoticePage')
    b = Builder(prefab)
    root = prefab.root
    page = b.component(root, 'EventBoardNoticePage')
    page_list(b, root, page, row_prefab_guid)
    empty = page_paper(b, root, sprite_guid('Letter'), 'お知らせはありません')

    detail = b.node(root, 'Detail')
    hanko = image(b, detail, 'KindHanko', V['n_hanko'], sprite_guid('KindHanko_Update'), ORDER_POSTER_STAMP)
    date = text(b, detail, 'DateText', V['n_date'][0], V['n_date'][1], '', art.INK_FADE, ORDER_POSTER_TEXT,
                align=ALIGN_RIGHT)
    title = text(b, detail, 'TitleText', V['n_title'][0], V['n_title'][1], '', art.INK, ORDER_POSTER_TEXT,
                 font=FONT_BRUSH)
    (bx, by), bpx, gap, count = V['n_body']
    lines = [text(b, detail, f'BodyLine{i}', (bx, by + i * gap), bpx, '', art.INK, ORDER_POSTER_TEXT)
             for i in range(count)]

    b.field(page, 'detailRoot_', detail.guid)
    b.field(page, 'detailKindHanko_', guid_of(hanko))
    b.field(page, 'detailDateText_', guid_of(date))
    b.field(page, 'detailTitleText_', guid_of(title))
    page.data['detailBodyLines_'] = [edits.field_blob('TextRenderer', guid_of(line)) for line in lines]
    b.field(page, 'emptyText_', guid_of(empty))
    page.data['kindHankoSprites_'] = [edits.field_blob('SpriteFile', sprite_guid(f'KindHanko_{k}'))
                                      for k in art.V2_KINDS]
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardNoticePage')


def build_restoration_page(row_prefab_guid):
    """復興の頁。大きな紙の代わりに右下の見積の札だけを置き、真ん中は下見のカメラに空けておく"""
    V = art.V2
    prefab = new_prefab('EventBoardRestorationPage')
    b = Builder(prefab)
    root = prefab.root
    page = b.component(root, 'EventBoardRestorationPage', defaultColor_=rgb(art.INK), fadedColor_=rgb(art.INK_FADE),
                       refusedColor_=rgb(art.STAMP_RED), remainColor_=rgb(art.GOLD_INK),
                       notApplicableText_='―', noConditionText_='なし')
    page_list(b, root, page, row_prefab_guid)
    image(b, root, 'Card', V['r_card'], sprite_guid('RestorationCard'), ORDER_POSTER)
    image(b, root, 'CardNail', V['r_card_nail'], sprite_guid('Nail'), ORDER_POSTER_STAMP)
    empty = text(b, root, 'EmptyText', V['r_empty'][0], V['r_empty'][1], 'まだ普請の段取りは付いていない',
                 art.INK_FADE, ORDER_POSTER_TEXT, align=ALIGN_CENTER, enabled=False)

    detail = b.node(root, 'Detail')
    name = text(b, detail, 'NameText', V['r_name'][0], V['r_name'][1], '', art.INK, ORDER_POSTER_TEXT,
                font=FONT_BRUSH)
    state = text(b, detail, 'StateText', V['r_state'][0], V['r_state'][1], '', art.INK, ORDER_POSTER_TEXT,
                 align=ALIGN_RIGHT)
    condition = text(b, detail, 'ConditionText', V['r_condition'][0], V['r_condition'][1], '', art.INK_FADE,
                     ORDER_POSTER_TEXT)
    cost = text(b, detail, 'CostText', V['r_cost'][0], V['r_cost'][1], '', art.INK, ORDER_POSTER_TEXT,
                font=FONT_BRUSH)
    balance = text(b, detail, 'BalanceText', V['r_balance'][0], V['r_balance'][1], '', art.INK, ORDER_POSTER_TEXT,
                   font=FONT_BRUSH)
    remain = text(b, detail, 'RemainText', V['r_remain'][0], V['r_remain'][1], '', art.GOLD_INK, ORDER_POSTER_TEXT,
                  font=FONT_BRUSH)
    (dx, dy), dpx, line_gap, count = V['r_desc']
    lines = [text(b, detail, f'DescLine{i}', (dx, dy + i * line_gap), dpx, '', art.INK, ORDER_POSTER_TEXT)
             for i in range(count)]
    seal_pos, seal_scale = V['r_seal']
    seal = image(b, detail, 'Seal', seal_pos, sprite_guid('RestorationSeal_Open'), ORDER_POSTER_STAMP,
                 scale=seal_scale)

    b.field(page, 'detailRoot_', detail.guid)
    b.field(page, 'detailNameText_', guid_of(name))
    b.field(page, 'detailStateText_', guid_of(state))
    b.field(page, 'detailConditionText_', guid_of(condition))
    b.field(page, 'detailCostText_', guid_of(cost))
    b.field(page, 'detailBalanceText_', guid_of(balance))
    b.field(page, 'detailRemainText_', guid_of(remain))
    page.data['detailDescriptionLines_'] = [edits.field_blob('TextRenderer', guid_of(line)) for line in lines]
    b.field(page, 'detailSeal_', guid_of(seal))
    b.field(page, 'emptyText_', guid_of(empty))
    b.field(page, 'openSealSprite_', sprite_guid('RestorationSeal_Open'))
    b.field(page, 'lockedSealSprite_', sprite_guid('RestorationSeal_Locked'))
    b.field(page, 'restoredSealSprite_', sprite_guid('RestorationSeal_Restored'))
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardRestorationPage')


# ---------------------------------------------------------------- 掲示板の画面 (見出し + 頁 + 操作ガイド)
HINT_SPRITES = {
    'HintTag_Cancel': HINT_CANCEL,
    'HintTag_Confirm': HINT_CONFIRM,
}


def hint_group(b, parent, name, items):
    group = b.node(parent, name)
    for i, item in enumerate(art.v2_hint_layout(items)):
        if item['kind'] == 'tag':
            sprite = HINT_SPRITES.get(item['sprite']) or sprite_guid(item['sprite'])
            image(b, group, f'Tag{i}', item['pos'], sprite, ORDER_HINT)
        else:
            text(b, group, f'Text{i}', item['pos'], art.LAYOUT['hint_px'], item['text'], art.HINT_COLOR,
                 ORDER_HINT_TEXT)
    return group


def build_ui(tab_guid, quest_page_guid, event_page_guid, notice_page_guid, restoration_page_guid):
    L, V = art.LAYOUT, art.V2
    prefab = new_prefab('EventBoardUI')
    b = Builder(prefab)
    root = prefab.root

    veil, _ = b.image(root, 'Veil', (art.SCREEN_W / 2, art.SCREEN_H / 2), BLACK_MASK, ORDER_VEIL, L['veil_blend'],
                      scale=L['veil_scale'])
    image(b, root, 'Strip', V['strip'], sprite_guid('Strip'), ORDER_BOARD)
    image(b, root, 'TabRope', V['tab_rope'], sprite_guid('TabRope'), ORDER_TAB_ROPE)
    image(b, root, 'TabHintLB', V['tab_hint_lb'], sprite_guid('HintTag_LB'), ORDER_TAB)
    image(b, root, 'TabHintRB', V['tab_hint_rb'], sprite_guid('HintTag_RB'), ORDER_TAB)
    tabs = b.node(root, 'Tabs', V['tabs_root'])
    pages = b.node(root, 'Pages')
    with_accept = hint_group(b, root, 'HintsWithAccept', art.V2_HINTS_WITH_ACCEPT)
    without_accept = hint_group(b, root, 'HintsWithoutAccept', art.V2_HINTS_WITHOUT_ACCEPT)
    with_restore = hint_group(b, root, 'HintsWithRestore', art.V2_HINTS_WITH_RESTORE)

    ui = b.component(root, 'EventBoardUi', tabSpacing_px_=str(V['tab_spacing']))
    b.field(ui, 'tabPrefab_', tab_guid)
    b.field(ui, 'tabsRoot_', tabs.guid)
    b.field(ui, 'questPagePrefab_', quest_page_guid)
    b.field(ui, 'eventPagePrefab_', event_page_guid)
    b.field(ui, 'noticePagePrefab_', notice_page_guid)
    b.field(ui, 'pagesRoot_', pages.guid)
    b.field(ui, 'hintsWithAccept_', with_accept.guid)
    b.field(ui, 'hintsWithoutAccept_', without_accept.guid)
    b.field(ui, 'restorationPagePrefab_', restoration_page_guid)
    b.field(ui, 'hintsWithRestore_', with_restore.guid)
    b.field(ui, 'veil_', veil.guid)

    presenter = b.component(root, 'EventBoardPresenter')
    b.field(presenter, 'board_', BOARD_DATA)
    b.field(presenter, 'acceptSound_', ACCEPT_SOUND)
    b.field(presenter, 'restoreSound_', RESTORE_SOUND)
    b.field(presenter, 'refuseSound_', REFUSE_SOUND)
    return save_prefab(prefab, PREFAB_DIR, 'EventBoardUI')


def build_all():
    row_guid, _ = build_row()
    quest_row_guid, _ = build_quest_row()
    notice_row_guid, _ = build_notice_row()
    restoration_row_guid, _ = build_restoration_row()
    tab_guid, _ = build_tab()
    event_page_guid, _ = build_event_page(row_guid)
    quest_page_guid, _ = build_quest_page(quest_row_guid)
    notice_page_guid, _ = build_notice_page(notice_row_guid)
    restoration_page_guid, _ = build_restoration_page(restoration_row_guid)
    ui_guid, _ = build_ui(tab_guid, quest_page_guid, event_page_guid, notice_page_guid, restoration_page_guid)
    return ui_guid


# ---------------------------------------------------------------- 3D の掲示板 (Prop)
PROP_DIR = REPO / 'Assets' / 'Prefab' / 'Prop'
MAIN_ISLAND_SCENE = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
ICON_PREFAB = REPO / 'Assets' / 'Prefab' / 'UI' / 'BillBoardNpcChatIcon.prefab'
BOARD_MODEL = asset_guid(REPO / 'Assets/Art/Models/Prop/EventBoard/EventNoticeBoard.mv1.meta')

# 看板の FBX は cm。プレイヤー(Swordman)も cm のモデルを 0.08 倍で置いているので、同じ倍率で実寸比が揃う
MODEL_SCALE = 0.08
BOARD_CM = (220.0, 260.0, 24.0)   # 柱の外側まで / 屋根の頂点まで / 足の奥行き
ICON_HEIGHT = 24.5
# 酒場の仲介人の右手に並べる(仲介人の向きの右方向にこれだけずらし、向きも揃える)
PLACE_NEXT_TO = 'CharacterBrokerNpc'
PLACE_RIGHT_OFFSET = 22.0
RIGIDBODY_DYNAMIC = 2
# 位置X/Z と回転3軸を止める(仲介人と同じ)。全部止めると Static 扱いになり、
# プレイヤーの会話センサー(Kinematic)が拾えない
RIGIDBODY_FREEZE_XZ_ROTATION = 61


def vec3_blob(v):
    return OrderedObj([(f'value{i}', Num.of_float(float(c))) for i, c in enumerate(v)])


def box_collider(size, offset):
    """BoxCollider は ColliderBase を tools.scene が組めないので、エンジンの save() の順に直接組む"""
    cat = catalog_mod.load()
    guid = edits.mint_guid().upper()
    guid_ver = Ver(('type', 'Guid'), 0, OrderedObj([('value_', guid)]))
    component_base = Ver(('type', 'ComponentBase'), 0, OrderedObj([('guid_', guid_ver), ('isEnable_', True)]))
    collider_base = Ver(('type', 'ColliderBase'), 6, OrderedObj([
        ('value0', component_base),
        ('offset_', vec3_blob(offset)),
        ('offsetRotation_', vec3_blob((0.0, 0.0, 0.0))),
        ('layer_', Num.of_int(0)),
        ('isSensor_', False),
        ('friction_', Num.of_float(0.2)),
    ]))
    data = OrderedObj([('value0', collider_base)])
    for i, leaf in enumerate(('IAwakable', 'IBeginPhysics', 'IEndPhysics'), start=1):
        data.append(f'value{i}', edits._empty_base_blob(leaf, (cat.base_info(leaf) or {}).get('version', 0)))
    data.append('size_', vec3_blob(size))
    return model.Component(fqn='NanamiEngine::Module::Component::BoxCollider', class_version=6, data=data)


def build_prop(ui_prefab_guid):
    prefab = new_prefab('EventNoticeBoard')
    b = Builder(prefab)
    root = prefab.root

    board = b.component(root, 'EventNoticeBoard')
    w, h, d = (c * MODEL_SCALE for c in BOARD_CM)
    root.components.append(box_collider((w, h, d), (0.0, h / 2, 0.0)))
    body = b.component(root, 'RigidBody', mass_='1', isGravity_='true', isPartOfParent_='false')
    body.data['motionType_'] = Num.of_int(RIGIDBODY_DYNAMIC)
    body.data['constraints_'] = Num.of_int(RIGIDBODY_FREEZE_XZ_ROTATION)

    model_node = edits.add_gameobject(prefab, parent=root.guid, name='Model', pos=(0.0, 0.0, 0.0),
                                      scale=(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE))
    b.component(model_node, 'ModelRenderer', mv1File_=BOARD_MODEL, useFixedInterpolation_='false')

    icon = edits.instantiate_prefab(prefab, reader.read_prefab_file(ICON_PREFAB), parent=root.guid)
    icon.name = 'ChatIcon'
    icon.transform.local_pos = edits._vec3_from_floats((0.0, ICON_HEIGHT, 0.0))
    icon_comp = next(c for c in icon.components if c.fqn.endswith('::BillBoardNpcChatIcon'))

    b.field(board, 'eventBoardUiPrefab_', ui_prefab_guid)
    b.field(board, 'chatIcon_', guid_of(icon_comp))
    return save_prefab(prefab, PROP_DIR, 'EventNoticeBoard')


def f32(v):
    return struct.unpack('<f', struct.pack('<f', v))[0]


def world_matrix_from_trs(trs):
    x, y, z, w = trs.rot
    r = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
         [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
         [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
    cols = [[r[row][c] * trs.scale[c] for row in range(3)] + [0.0] for c in range(3)]
    cols.append([trs.pos[0], trs.pos[1], trs.pos[2], 1.0])
    obj = OrderedObj()
    for i, col in enumerate(cols):
        obj[f'value{i}'] = OrderedObj((f'value{j}', Num.of_float(f32(v))) for j, v in enumerate(col))
    return obj


def bake_rotated_world_matrices(node, parent=mathutil.IDENTITY):
    world = parent.then(edits._node_local_trs(node))
    node.transform.world_matrix = world_matrix_from_trs(world)
    for child in node.transform.children:
        bake_rotated_world_matrices(child, world)


class StrayVersionStripper(validate._ClassVersionAudit):
    """validate と同じ手順で型ごとの初出を辿り、2回目以降に残っている cereal_class_version だけを消す。
    ファイルから読んだ prefab を別のファイルへ入れると、元のファイルでの初出の版キーがそのまま写るため
    (シーンには既に NPC の吹き出しアイコンなどで同じ型が先に出ている)"""

    def __init__(self, cat, owner_path):
        super().__init__(cat)
        self.owner_path = owner_path
        self.stripped = []

    def _check(self, type_key, node, where):
        if where.startswith(self.owner_path) and type_key in self._first and validate._VER in node:
            node.pop(validate._VER)
            self.stripped.append(where)
            return
        super()._check(type_key, node, where)


def strip_repeat_versions(text, owner_path):
    tree = loads(text)
    if dumps(tree) != text:
        raise SystemExit('cereal_json round trip is not exact - refusing to rewrite')
    stripper = StrayVersionStripper(catalog_mod.load(), owner_path)
    stripper.run(tree, '')
    print(f'  stripped {len(stripper.stripped)} repeat cereal_class_version key(s) inside the new instance')
    return dumps(tree)


def first_versions(text):
    """ファイル中で各型が初めて出た所の cereal_class_version。cereal はこの値で同じ型を全部読む"""
    class Probe(validate._ClassVersionAudit):
        def __init__(self):
            super().__init__(catalog_mod.load())
            self.versions = {}

        def _check(self, type_key, node, where):
            if type_key not in self._first:
                version = node.get(validate._VER)
                self.versions[type_key] = version.value if version is not None else None
            super()._check(type_key, node, where)

    probe = Probe()
    probe.run(loads(text), '')
    return probe.versions


def to_collider_base_v5(node):
    """ColliderBase v5 の並び(mass_ / isGravity_ / emotionType_ / constraints_ を含む)に直す。
    v5 で保存されたシーンでは全ての ColliderBase が v5 として読まれ、v6 の並びだと mass_ が無く読み込みで落ちる"""
    for comp in node.components:
        if not comp.fqn.endswith('::BoxCollider'):
            continue
        slot = comp.data['value0']
        body = slot.body if isinstance(slot, Ver) else slot
        legacy = OrderedObj([
            ('value0', body['value0']),
            ('mass_', Num.of_float(1.0)),
            ('isGravity_', False),
            ('offset_', body['offset_']),
            ('offsetRotation_', body['offsetRotation_']),
            ('emotionType_', Num.of_int(0)),
            ('layer_', body['layer_']),
            ('constraints_', Num.of_int(0)),
            ('isSensor_', body['isSensor_']),
            ('friction_', body['friction_']),
        ])
        comp.data['value0'] = Ver(slot.key, 5, legacy, slot.literal_presence) if isinstance(slot, Ver) else legacy


def place_in_main_island(prop_path):
    """MainIslandScene の酒場の仲介人の右に置く。置き直すときは前の EventNoticeBoard を消してから置く"""
    scene = reader.read_scene_file(MAIN_ISLAND_SCENE)
    scene.roots = [r for r in scene.roots if r.name != 'EventNoticeBoard']
    anchor = next((r for r in scene.roots if r.name == PLACE_NEXT_TO), None)
    if anchor is None:
        raise SystemExit(f'{PLACE_NEXT_TO} not found at the root of {MAIN_ISLAND_SCENE.name}')

    trs = edits._node_local_trs(anchor)
    right = mathutil.quat_rotate_vec(trs.rot, (1.0, 0.0, 0.0))
    pos = tuple(trs.pos[i] + right[i] * PLACE_RIGHT_OFFSET for i in range(3))

    node = edits.instantiate_prefab(scene, reader.read_prefab_file(prop_path), parent=None)
    collider_version = first_versions(MAIN_ISLAND_SCENE.read_text(encoding='utf-8')).get('ColliderBase')
    if collider_version == 5:
        to_collider_base_v5(node)
    elif collider_version not in (None, 6):
        raise SystemExit(f'{MAIN_ISLAND_SCENE.name}: ColliderBase v{collider_version} is not handled')
    node.transform.local_pos = edits._vec3_from_floats(pos)
    node.transform.local_rot = copy.deepcopy(anchor.transform.local_rot)
    bake_rotated_world_matrices(node)

    text = strip_repeat_versions(writer.write_scene(scene), f'/gameObject_{len(scene.roots) - 1}/')
    check(text, validate.validate_scene(scene), MAIN_ISLAND_SCENE.name)
    MAIN_ISLAND_SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(MAIN_ISLAND_SCENE)
    print(f'placed EventNoticeBoard in {MAIN_ISLAND_SCENE.relative_to(REPO)} at '
          f'({pos[0]:.2f}, {pos[1]:.2f}, {pos[2]:.2f}) next to {PLACE_NEXT_TO}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--prop', action='store_true', help='3D の看板 Prop/EventNoticeBoard.prefab も組む')
    ap.add_argument('--place', action='store_true', help='看板を MainIslandScene の酒場の仲介人の右に置く(--prop を含む)')
    args = ap.parse_args()

    ui_guid = build_all()
    if args.prop or args.place:
        _, prop_path = build_prop(ui_guid)
        if args.place:
            place_in_main_island(prop_path)

if __name__ == '__main__':
    main()
