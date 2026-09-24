"""島の復興施設アセットを書き出し、メイン島の掲示板に貼る。

    python tools/art/restoration_facilities.py

Assets/Data/Restoration/<Name>.restorationFacility(.meta) を FACILITIES から書き(本体は 0 バイト、中身は .meta)、
Assets/Data/EventNotice/MainIslandEventBoard.eventBoard.meta の facilities_ をこの並びで置き換える。
.meta の guid は既存があれば保つので、書き直しても参照は切れない。
施設の一覧・前提は docs/Story.md §5。金額はまだ決まっていない(仮)。建つ場所と姿はシーンの RestorationGate が持つ。
"""
import copy
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from tools.common import meta_base  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, read_text, to_file_bytes  # noqa: E402

DATA_DIR = REPO / 'Assets' / 'Data' / 'Restoration'
BOARD_META = REPO / 'Assets' / 'Data' / 'EventNotice' / 'MainIslandEventBoard.eventBoard.meta'
# FIELD(...) の並びをこのファイルから借りる(stage_ が最初の FIELD)
FIELD_TEMPLATE = REPO / 'Assets' / 'Data' / 'EventNotice' / 'GrassLandTyrant.boardQuest.meta'
FQN = 'NanamiEngine::Module::Asset::RestorationFacility'
FIRST_ID = 2147483649
NONE = -1

# GameCore::Story::Facility / StoryFlag の値 (Story_Facility.h / Story_StoryFlag.h)
CLAN_HOUSE = 4
GRASS_LAND_CLEARED, FOUNTAIN_ISLAND_RETURNED = 2, 4

# NOTE: 仮置きだった4つ (船着き場・雑貨屋・狩人小屋・畑) は 2026-09-24 に削除した。施設を決めたらここに足す。
#   dict(asset='Name', facility=<Facility の値>, name='表示名', cost=500, lines=['説明1', '説明2'],
#        flag=GRASS_LAND_CLEARED, condition='前提の文言', required=<前提の Facility>)
FACILITIES = [
    # 建てると、女狩人が仲間 (キャラ選択) を出してくれる。草原の報酬 (3000) で建てられる額
    dict(asset='ClanHouse', facility=CLAN_HOUSE, name='一族の家', cost=2000,
         flag=FOUNTAIN_ISLAND_RETURNED, condition='噴水の島が戻ってから',
         lines=['草原の狩人の一族が住む家。', '復興を手伝う仲間が集まる。']),
]


def asset_guid(meta_path):
    m = re.search(r'"guid_"\s*:\s*\{[^{}]*?"value_"\s*:\s*"([0-9A-Fa-f-]{36})"', read_text(meta_path))
    if not m:
        raise SystemExit(f'no guid_ in {meta_path}')
    return m.group(1).upper()


def field_blob(guid, blob_id, first):
    """FIELD(T) 1つ。cereal は型ごとに初出だけ版を書く"""
    template = loads(read_text(FIELD_TEMPLATE))['value0']['ptr_wrapper']['data']['stage_']
    blob = copy.deepcopy(template)
    wrapper = blob['value0']['ptr_wrapper']
    wrapper['id'] = Num.of_int(blob_id)
    wrapper['data']['value0']['value_'] = guid
    if not first:
        blob.pop('cereal_class_version')
        wrapper['data'].pop('cereal_class_version')
    return blob


def write_facility(f):
    meta_path = DATA_DIR / f"{f['asset']}.restorationFacility.meta"
    guid = asset_guid(meta_path) if meta_path.exists() else meta_base.mint_guid()

    base = OrderedObj([
        ('cereal_class_version', Num.of_int(0)),
        ('value0', OrderedObj([('cereal_class_version', Num.of_int(0))])),
        ('contentPath_', f"Assets\\Data\\Restoration/{f['asset']}.restorationFacility"),
        ('guid_', OrderedObj([('cereal_class_version', Num.of_int(0)), ('value_', guid)])),
    ])
    data = OrderedObj([
        ('cereal_class_version', Num.of_int(0)),
        ('value0', base),
        ('facility_', Num.of_int(f['facility'])),
        ('name_', f['name']),
        ('descriptionLines_', list(f['lines'])),
        ('cost_', Num.of_int(f['cost'])),
        ('requiredStoryFlag_', Num.of_int(f.get('flag', NONE))),
        ('requiredFacility_', Num.of_int(f.get('required', NONE))),
        ('conditionText_', f.get('condition', '')),
    ])
    root = OrderedObj([('value0', OrderedObj([
        ('polymorphic_id', Num.of_int(FIRST_ID)),
        ('polymorphic_name', FQN),
        ('ptr_wrapper', OrderedObj([('id', Num.of_int(FIRST_ID)), ('data', data)])),
    ]))])

    DATA_DIR.mkdir(parents=True, exist_ok=True)
    meta_path.write_bytes(to_file_bytes(dumps(root)))
    (DATA_DIR / f"{f['asset']}.restorationFacility").write_bytes(b'')
    print(f"  {f['asset']}.restorationFacility  guid={guid}")
    return guid


def pin_on_board(guids):
    tree = loads(read_text(BOARD_META))
    data = tree['value0']['ptr_wrapper']['data']
    data['cereal_class_version'] = Num.of_int(max(2, data['cereal_class_version'].value))

    # NOTE: shared_ptr の id はファイルの中で通し番号。既存の FIELD の続きから振る
    ids = [item['value0']['ptr_wrapper']['id'].value
           for key in ('notices_', 'quests_', 'announcements_') for item in data[key]]
    next_id = max(ids, default=FIRST_ID) + 1
    facilities = [field_blob(guid, next_id + i, first=i == 0) for i, guid in enumerate(guids)]
    if 'facilities_' in data.keys():
        data['facilities_'] = facilities
    else:
        data.append('facilities_', facilities)
    BOARD_META.write_bytes(to_file_bytes(dumps(tree)))
    print(f'  {BOARD_META.name}: facilities_ = {len(guids)}')


def main():
    guids = [write_facility(f) for f in FACILITIES]
    pin_on_board(guids)


if __name__ == '__main__':
    main()
