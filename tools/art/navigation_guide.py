"""Build Assets/Data/Navigation/Main.navGuide(.meta) — the ordered "what to do next" steps shown by
GamePlay::Ui::NavigationPresenter / NavigationBanner (docs: the plan in the navigation PR).

    python tools/art/navigation_guide.py [--dry-run]

STEPS below is the source of truth: edit it and re-run instead of editing the .meta by hand
(an existing .meta keeps its GUID). The first step whose activeConditions_ all hold and whose
doneConditions_ do not all hold is shown (empty doneConditions_ = never done), so order matters:
story steps first, then quest steps. A step's targets are tried in order; the first targetId the current
scene's context knows (SceneContextBase::navigationTargets_) is used, else only the step title is shown.

cereal writes a class version and a polymorphic type name only the first time it meets that type in an
archive, and numbers every shared_ptr, so this script emits the JSON with those rules.
"""
import argparse
import json
import uuid
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
OUT = REPO_ROOT / 'Assets' / 'Data' / 'Navigation' / 'Main.navGuide'

# GameCore::Story::StoryFlag
PROLOGUE_CLEARED, RESTORATION_STARTED, GRASSLAND_CLEARED, GREEN_STONE_RETURNED, FOUNTAIN_ISLAND_RETURNED, \
    DESERT_CLEARED, DESERT_GUARD_RESCUED, LIGHT_STONE_RETURNED, NEST_VOYAGE_STARTED, ANCIENT_DRAGON_DEFEATED, \
    EPILOGUE_HEARD = range(11)
# GameCore::Story::Facility
CLAN_HOUSE = 4
# GameCore::PlayerAvatar::QuestType
GRASSLAND_HYENA_CULL, HYENA_HUNT_WEEK1 = 3, 4
GRASSLAND_TYRANT, DESERT_SKELETON_DRAGON, DESERT_SCORPION_CULL = 7, 8, 9


def flag(value):
    return ('GameCore::Condition::StoryFlagCondition', 'storyFlag_', value)


def facility(value):
    return ('GameCore::Condition::FacilityRestoredCondition', 'facility_', value)


def taking(value):
    return ('GameCore::Condition::QuestTakingCondition', 'questType_', value)


def scene_objective(objective_id):
    """今のシーンだけの一時的な目標(SceneContextBase::SetNavigationObjective)"""
    return ('GameCore::Condition::SceneObjectiveCondition', 'objectiveId_', objective_id)


def target(target_id, title, label):
    return (target_id, title, label)


INSTRUCTOR = 'Instructor'
PORTAL = 'Portal'
BOARD = 'Board'

# (id, title, activeConditions, doneConditions, targets)
STEPS = [
    # 竜の体力が減ると UnlockPlayerCannon が立て、竜が倒れると LockPlayerCannon が下ろす。訓練の段より先に置く
    ('FireCannon', '山頂の大砲で竜を撃ち落とす', [scene_objective('PlayerCannon')], [],
     [target('Cannon', '山頂の大砲で竜を撃ち落とす', '大砲')]),
    ('Prologue', '教官の訓練を受ける', [], [flag(PROLOGUE_CLEARED)],
     [target(INSTRUCTOR, '教官の訓練を受ける', '教官')]),
    ('HearInstructor', '教官に話を聞く', [flag(PROLOGUE_CLEARED)], [flag(RESTORATION_STARTED)],
     [target(INSTRUCTOR, '教官に話を聞く', '教官')]),
    # 受注中の段を、受ける段より先に置く(先頭から当てはまるものを出すので)
    ('DefeatTyrant', '草原の大顎を倒す', [taking(GRASSLAND_TYRANT)], [flag(GRASSLAND_CLEARED)],
     [target('GrassLandTyrant', '村の跡の大顎を倒す', '大顎'),
      target(PORTAL, '転移台から草原へ渡る', '転移台')]),
    ('TakeTyrant', '掲示板で「草原の大顎」を受ける', [flag(RESTORATION_STARTED)], [flag(GRASSLAND_CLEARED)],
     [target(BOARD, '掲示板で「草原の大顎」を受ける', '掲示板')]),
    ('ReturnWithGreenStone', '拠点の島へ戻る', [flag(GRASSLAND_CLEARED)], [flag(FOUNTAIN_ISLAND_RETURNED)], []),
    ('BuildClanHouse', '掲示板の「復興」で一族の家を建てる', [flag(FOUNTAIN_ISLAND_RETURNED)], [facility(CLAN_HOUSE)],
     [target(BOARD, '掲示板の「復興」で一族の家を建てる', '掲示板')]),
    ('DefeatSkeletonDragon', '砂漠の骸竜を倒す', [taking(DESERT_SKELETON_DRAGON)], [flag(DESERT_CLEARED)],
     [target('DesertSkeletonDragon', '城塞の広場の骸竜を倒す', '骸竜'),
      target(PORTAL, '転移台から砂漠へ渡る', '転移台')]),
    ('TakeSkeletonDragon', '掲示板で「砂漠の骸竜」を受ける', [flag(GREEN_STONE_RETURNED)], [flag(DESERT_CLEARED)],
     [target(BOARD, '掲示板で「砂漠の骸竜」を受ける', '掲示板')]),
    ('ReturnWithLightStone', '拠点の島へ戻る', [flag(DESERT_CLEARED)], [flag(LIGHT_STONE_RETURNED)], []),
    ('HearDesertReport', '教官に話を聞く', [flag(DESERT_CLEARED)], [flag(NEST_VOYAGE_STARTED)],
     [target(INSTRUCTOR, '教官に話を聞く', '教官')]),
    ('DefeatAncientDragon', '古竜を倒す', [flag(NEST_VOYAGE_STARTED)], [flag(ANCIENT_DRAGON_DEFEATED)],
     [target('AncientDragon', '古竜を倒す', '古竜'),
      target(INSTRUCTOR, '教官に声をかけて出発する', '教官')]),
    ('HearEpilogue', '拠点の島で教官に話を聞く', [flag(ANCIENT_DRAGON_DEFEATED)], [flag(EPILOGUE_HEARD)],
     [target(INSTRUCTOR, '教官に話を聞く', '教官')]),
    # 受注中の依頼。何度も受けられる依頼は達成しても「達成済み」が残らないので、受注が消えたら出なくなる
    ('HyenaCull', '西の林のハイエナを減らす', [taking(GRASSLAND_HYENA_CULL)], [],
     [target('GrassLandHyenaWoods', '西の林のハイエナを減らす', '西の林'),
      target(PORTAL, '転移台から草原へ渡る', '転移台')]),
    ('HyenaHuntWeek', 'ハイエナ狩りの週', [taking(HYENA_HUNT_WEEK1)], [],
     [target('GrassLandHyenaWoods', '西の林でハイエナを狩る', '西の林'),
      target(PORTAL, '転移台から草原へ渡る', '転移台')]),
    ('ScorpionCull', 'オアシスの大サソリを退治する', [taking(DESERT_SCORPION_CULL)], [],
     [target('DesertOasis', 'オアシスの大サソリを退治する', 'オアシス'),
      target(PORTAL, '転移台から砂漠へ渡る', '転移台')]),
]


class Archive:
    """cereal の JSON の書き方(型ごとの初回だけ版と型名、shared_ptr の通し番号)を真似る"""

    def __init__(self):
        self.versioned = set()
        self.poly_ids = {}
        self.next_ptr = 1

    def version(self, obj, cls, value=0):
        if cls not in self.versioned:
            self.versioned.add(cls)
            obj['cereal_class_version'] = value
        return obj

    def condition(self, spec):
        cls, field, value = spec
        entry = {}
        if cls in self.poly_ids:
            entry['polymorphic_id'] = self.poly_ids[cls]
        else:
            self.poly_ids[cls] = len(self.poly_ids) + 1
            entry['polymorphic_id'] = 0x80000000 | self.poly_ids[cls]
            entry['polymorphic_name'] = cls
        data = self.version({}, cls)
        data['value0'] = self.version({}, 'GameCore::Condition::ICondition')
        data[field] = value
        entry['ptr_wrapper'] = {'id': 0x80000000 | self.next_ptr, 'data': data}
        self.next_ptr += 1
        return entry


def build(guid):
    ar = Archive()
    ar.poly_ids['NanamiEngine::Module::Asset::NavigationGuide'] = 1
    ar.next_ptr = 2
    content_path = 'Assets\\Data\\Navigation/Main.navGuide'
    steps = []
    for step_id, title, active, done, targets in STEPS:
        step = ar.version({}, 'NavigationStep')
        step['id_'] = step_id
        step['title_'] = title
        step['activeConditions_'] = [ar.condition(c) for c in active]
        step['doneConditions_'] = [ar.condition(c) for c in done]
        step['targets_'] = []
        for target_id, target_title, label in targets:
            option = ar.version({}, 'NavigationTargetOption')
            option.update({'targetId_': target_id, 'title_': target_title, 'label_': label})
            step['targets_'].append(option)
        steps.append(step)

    return {'value0': {
        'polymorphic_id': 0x80000001,
        'polymorphic_name': 'NanamiEngine::Module::Asset::NavigationGuide',
        'ptr_wrapper': {'id': 0x80000001, 'data': {
            'cereal_class_version': 0,
            'value0': {
                'cereal_class_version': 0,
                'value0': {'cereal_class_version': 0},
                'contentPath_': content_path,
                'guid_': {'cereal_class_version': 0, 'value_': guid},
            },
            'steps_': steps,
        }},
    }}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    meta = OUT.with_name(OUT.name + '.meta')
    if meta.exists():
        guid = json.loads(meta.read_text(encoding='utf-8'))['value0']['ptr_wrapper']['data']['value0']['guid_']['value_']
    else:
        guid = str(uuid.uuid4()).upper()

    text = json.dumps(build(guid), ensure_ascii=False, indent=4).replace('\n', '\r\n')
    if args.dry_run:
        print(text)
        return
    OUT.parent.mkdir(parents=True, exist_ok=True)
    if not OUT.exists():
        OUT.write_bytes(b'')
    meta.write_bytes(text.encode('utf-8'))
    print(f'{meta.relative_to(REPO_ROOT)}: {len(STEPS)} steps, guid={guid}')


if __name__ == '__main__':
    main()
