"""docs/Story.md の筋に沿って、NPC の会話 (.npcChat) と BT を作り直す。台詞の正はこのファイル。

    python tools/art/story_npcs.py                # 全部
    python tools/art/story_npcs.py --only camp    # camp / island / newcomers / clan / prologue / dragon / desert のどれか

- 会話は camp_people.write_npc_chat で書く (本体は 0 バイト、中身は .meta。GUID は保つ)。
  1ページ2行・1行 22 字まで・CP932 に無い字は不可 (write_npc_chat が弾く)。
- BT は new-tree --force で作り直し、前の GUID を .meta に戻す (シーンからの参照を切らない)。
  シーン内の物 (驚きアイコン・カメラ・屋台・台座) の参照は、作り直す前の BT から読んで引き継ぐ。
- 物語の分岐は Story::IsStoryFlag / SetStoryFlag (施設なら IsRestored)。その場限りの「もう話した」は blackboard。
- Chat はプレイヤーが話しかけたときにしか始まらず、終わると会話中を下ろすので、続けて話させるときは
  2つ目から ImplementChat にする。IsChat で始まる Sequence の中だと次のフレームで IsChat が落ちて
  打ち切られるので、続きは blackboard で IsChat の外の枝に渡す。
- 野営地の4人のモデル・AnimTree・配置は camp_people.py。
"""
import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.bt import model, reader  # noqa: E402
from tools.common import meta_base  # noqa: E402

from camp_people import BT_DIR, apply_ops, asset_guid, recreate, run, write_npc_chat  # noqa: E402

# Story_StoryFlag.h / Story_Facility.h の値 (末尾に足す約束なので固定でよい)
PROLOGUE_CLEARED, RESTORATION_STARTED, GRASSLAND_CLEARED = 0, 1, 2
DESERT_CLEARED, DESERT_GUARD_RESCUED = 5, 6
FLAG_NAMES = {PROLOGUE_CLEARED: 'PrologueCleared', RESTORATION_STARTED: 'RestorationStarted',
              GRASSLAND_CLEARED: 'GrassLandCleared', DESERT_CLEARED: 'DesertCleared',
              DESERT_GUARD_RESCUED: 'DesertGuardRescued'}
FACILITY_CLAN_HOUSE = 4

# キャラ選択の画面と、MainIslandScene の展示台 (一族の家の前に置いてある)
CHARACTER_SELECT_UI = REPO / 'Assets' / 'Prefab' / 'UI' / 'CharacterSelect' / 'CharacterSelectUI.prefab.meta'

# SetEnableShowChatIcon の (表示, 話しかけられる, 会話中, 驚き)
ICON_CHATTABLE = (True, True, False, False)
ICON_CHATTING = (True, False, True, False)
ICON_HIDDEN = (False, False, False, False)

GUID_RE = re.compile(r'[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}')


# ---------------------------------------------------------------- 台詞
CHATS = {
    # --- 序章 (FirstTouchDownMainIsLandScene)
    'AirShipKunoichi': [
        '……着いたね。\nここが駆け出しハンターの島か。',
        '私は探し物の旅の途中。\nしばらくこの辺りにいるつもり。',
        'ただ、今日は風が妙にざわついてる。\n……あの日と、同じ匂いがする。',
        '浮遊石の光は、遠くからでも見える。\n……見えすぎるくらいにね。',
        '気をつけなよ。\n空から来るものは、待ってくれない。',
    ],
    # 船上で世界の仕組み (浮かぶ島・浮遊石・島の心臓) を話す案内役
    'AirShipYoungMan': [
        'やあ、君もこの島は初めてかい？',
        '見てごらん、島が雲の上に浮いてる。\n何度見ても不思議だよね。',
        '島の底には浮遊石って石が埋まってて、\nその力で浮いてるんだって。',
        '雲の下には昔、大地があったんだって。\n島はそこから持ち上げられたらしいよ。',
        '石が二つもある島は珍しいらしいよ。\nみんな「島の心臓」って呼んでる。',
        '僕は空の果てまで旅するのが夢でね。\nこの船旅だけで、もう胸が躍ってるよ。',
        'ハンターになるなら、船を降りてすぐの\n教官に声をかけるといいよ。',
    ],
    'Introduction ActionInstructure': [
        'お前が今日の船で着いた新入りか。\n話は聞いている。',
        '俺はこの島で、駆け出しのハンターに\n戦い方を叩き込んでいる教官だ。',
        'よその島の狩り場には、魔物がうろつく。\nそれを狩って稼ぐのがハンターだ。',
        'ここらの空じゃ、魔物は待ってくれん。\n着いたその日から訓練を始める。',
        'ついて来い。\nまずは体の動かし方からだ。',
    ],
    # ドラゴンを撃ち落とした直後。話すのは教官 (displayName_)
    'FirstDragon Heart Shatter': [
        'やったか……！？\n――待て、奴の様子がおかしい！',
        '島の真ん中に爪を……！\nまずい、浮遊石が狙いか！',
        '浮遊石が抜かれた……！\n島が傾くぞ、何かに掴まれ！',
    ],

    # --- 拠点の島 (MainIslandScene)
    'Instructor_RestorationStart': [
        '……目が覚めたか。\n丸二日、眠っていたぞ。',
        '奴は撃ち落とした。だが墜ちる間際、\n島の真ん中に爪を突き立てていった。',
        'この島は、底に埋まった\n二つの浮遊石の力で浮いている。',
        '奴の爪で、石は二つとも抜け出し、\nよその島へ飛んでいった。',
        'このままじゃ、島は少しずつ沈む。\n……俺の脚も、この有様だ。',
        '俺はもう前には出られん。だが、\n島を立て直す段取りなら付けられる。',
        '浮遊石を取り戻せ。それと、金だ。\n島を直すには、とにかく金が要る。',
        '稼ぎ口は掲示板の依頼だ。\n酒場の仲介人に話を通してある。',
    ],
    'Instructor_PortalGuide': [
        '緑の浮遊石は、草原の島の石に引かれて\n落ちた。あの紫の台座から渡れる。',
        'むき出しの浮遊石は、獣も魔物も\n引き寄せる。今ごろ石の周りは群れだらけだ。',
        '強い奴ほど、石の傍に居座る。\n……無理はするなよ。',
    ],
    'Instructor_BeforeGrassLand': [
        '草原へは、紫の台座から渡れる。\n石に寄ってきた魔物には気をつけろ。',
    ],
    'Instructor_GrassLandReport': [
        '緑の浮遊石、確かに受け取った。\n島の揺れが、嘘みたいに収まった。',
        '見たか。石の力で、落ちた噴水の島まで\n浮かび上がってきた。',
        '階段もひとりでに架かった。\nあれで向こうへ渡れる。',
        '草原の狩人たちには、\nあの島に住んでもらうつもりだ。',
        '一族の家を建ててやってくれ。\n掲示板の「復興」から頼める。',
        '残る浮遊石は、あとひとつ。\n行方は今、探らせている。',
    ],
    'Instructor_AfterGrassLand': [
        '最後の浮遊石は、砂漠の城塞跡に落ちた。\n掲示板に依頼を出しておいたぞ。',
    ],
    'Merchant_First': [
        'いらっしゃい！……と言いたいが、\n店もこの有様でね。',
        '棚も屋根も、ドラゴンの風で\nすっかり吹き飛ばされちまった。',
        'あるもんで良けりゃ売るよ。\n島を直すにも、まずは先立つもんだ。',
    ],
    'Merchant': [
        'いらっしゃい！\n旅の支度なら、うちで揃えていきな。',
    ],
    'CharacterBroker_First': [
        'よう、新入り。ドラゴンを撃ち落とした\nってのは、あんたか。',
        'あの騒ぎで、腕利きはみんな\n島を逃げ出しちまった。',
        '依頼を受けるなら、そこの掲示板だ。\n稼ぎは島の立て直しに回るからな。',
        '仲間が欲しけりゃ、まずはこの島に\n人が住めるようにすることだ。',
    ],
    'CharacterBroker': [
        '今は紹介できる奴がいなくてな。\n人が戻るまでは、掲示板で稼ぎな。',
    ],
    'CharacterBroker_ClanHouse': [
        '仲間なら、噴水の島の一族を訪ねな。\n腕の立つのが揃ってるぞ。',
    ],

    # 一族の家 (Facility::ClanHouse) を建てると、噴水の島に女狩人が住み、仲間を出してくれる
    'ClanHuntress_First': [
        '家を建ててくれたのね。\n……一族みんなで喜んでる。',
        '住む場所をもらった恩は、\n狩りの腕で返すつもりよ。',
        '私たちの一族に、島の復興を\n手伝いたいという者たちがいるの。',
        '腕は確かよ。狩りに出るなら、\n好きな者を連れていって。',
    ],
    'ClanHuntress': [
        '一緒に行く者を選んで。\n一族の誰でも、力になるわ。',
    ],

    # 序章の船に乗っていた2人。墜落の後は拠点の島にいる (StoryNpcs)
    'IslandKunoichi_First': [
        '……生きてたんだ。\nあの竜を撃ち落とすなんて、やるじゃん。',
        'あの竜は、前にも見たことがある。\n……その話は、また今度ね。',
        '飛んでいく浮遊石、私も見てた。\nひとつは草原の方へ落ちていったよ。',
        '石が落ちた所には、獣が集まる。\n……私の故郷でも、そうだった。',
    ],
    'IslandKunoichi_Again': [
        '浮遊石は獣を呼ぶ。竜も、きっと同じ。\n石を追えば、あいつに辿り着く。',
    ],
    'IslandKunoichi_Cleared': [
        '草原の浮遊石、取り戻したんだって？\n島の風が、少し落ち着いた。',
        '次の浮遊石の行方は、私も探ってみる。\n……借りを返すだけだから。',
    ],
    'IslandKunoichi_ClearedAgain': [
        '竜の巣は、嵐の向こう。\n……いつか、案内するよ。',
    ],
    'IslandYoungMan_First': [
        'やあ、無事だったんだね！\n僕も、なんとか生きてるよ。',
        '乗ってきた船、見たかい？\nあの竜のせいで、墜ちちゃったんだ。',
        'でも僕は諦めないよ。\nいつか必ず、あの船を直してみせる。',
        '部品さえ手に入ればなあ……\nどこかで見かけたら教えてよ。',
    ],
    'IslandYoungMan_Again': [
        '空の果てまで行く夢は、\nまだ終わってないからね。',
    ],
    'IslandYoungMan_Cleared': [
        '島の揺れが収まったね！\n君のおかげだって、みんな言ってるよ。',
        '船を直す部品は、まだ見つからない。\nでも、希望が見えてきた気がする。',
    ],
    'IslandYoungMan_ClearedAgain': [
        '空の果てまで行く夢は、\nまだ終わってないからね。',
    ],

    # --- 草原 (GrassLandScene) の野営地
    'CampPeopleElder_First': [
        '……よそ者か。\nこんな山の上まで、よく来たな。',
        'わしらは下の盆地に村を構えていた\n狩人の一族じゃ。',
        '数日前の晩、空から緑に光る石が\n村の真ん中に落ちてきてな。',
        'それからじゃ。獣という獣が村へ寄り、\n最後に大顎が来て、家も柵も潰した。',
        'あやつは今も、あの石の傍を離れん。\n石に呼ばれて、居着いておるのじゃ。',
        '腕の立つハンターと見込んで頼む。\nどうか、あやつを村から追い払ってくれ。',
    ],
    'CampPeopleElder_Again': [
        '大顎は今も村の跡に居座っておる。\nわしらの帰る場所は、あそこしかない。',
    ],
    'CampPeopleElder_Cleared': [
        '……終わったのか。\n本当に、あの大顎を。',
        'あの光る石、あんたの島のものじゃな。\n持っていけ。石が去れば獣も散る。',
        '一族の言い伝えでは、島の心臓は\n昔、本物の竜の心臓だったそうじゃ。',
        '……年寄りの昔話じゃ。\n聞き流してくれ。',
        '村を建て直すには時がかかる。\nその間、若い者を何人か預けたい。',
        '島を追われる辛さは、\nわしらが一番知っておる。',
        'あんたの島の立て直し、手伝わせてくれ。\n受けた恩は、狩人の流儀で返す。',
    ],
    'CampPeopleElder_ClearedAgain': [
        '若い者たちの支度が済み次第、\nあんたの島へ向かわせよう。',
    ],
    'CampPeopleLookout_First': [
        'しっ、静かに。……盆地の真ん中、\nあの影が見えるか？',
        'あれが村を潰した大顎だ。\n昼も夜も、ずっとあそこにいる。',
        '村に光る石が落ちてから、獣が\n寄ってくるようになった。あいつもだ。',
        '石から離れたくないんだろうな。\n踏み込んだら最後、吠えて突っ込んでくる。',
        '突進は正面に立つな、横へ跳べ。\n……ここから見てるからな。',
    ],
    'CampPeopleLookout_Again': [
        '大顎は相変わらずだ。\n石の傍から一歩も動かない。',
    ],
    'CampPeopleLookout_Cleared': [
        '見てたぞ！\n大顎が倒れるところ！',
        '……村に、帰れるんだな。\n本当に、ありがとう。',
    ],
    'CampPeopleLookout_ClearedAgain': [
        '石が無くなって、獣も散っていった。\n盆地が静かだ。こんな夜は久しぶりだよ。',
    ],
    'CampPeopleHuntress_First': [
        '肉を干してるところ。\n狩り場を追われて、これが最後の蓄えなの。',
        '石が落ちてから、西の林には\nハイエナの群れが集まってきてる。',
        '三頭ひと組で動くから、\n囲まれないで。',
        '遠吠えには気をつけて。仲間を呼ぶ合図よ。\n放っておくと、周りの群れまで集まってくる。',
    ],
    'CampPeopleHuntress_Again': [
        '遠吠えが聞こえたら、先に黙らせること。\nそれが群れと戦うコツよ。',
    ],
    'CampPeopleHuntress_Cleared': [
        '大顎を倒したのね。\n……ありがとう。',
        '長から聞いたわ。\nあなたの島へ行くのは、私たち。',
        '向こうの島に住まわせてもらうの。\n狩りの腕なら、役に立てるから。',
    ],
    'CampPeopleHuntress_ClearedAgain': [
        '干し肉の作り方くらいは教えてあげる。\n……向こうに着いたらね。',
    ],
    'CampPeopleWounded_First': [
        '……あ、ハンターさん？\nごめんなさい、うまく立てなくて。',
        'あの晩、村から逃げる途中で\n大顎に追いつかれたの。',
        '真横に逃げれば大丈夫だと思ったのに、\nあいつ、首を振って横まで噛みついてきた。',
        'それに、あの頭突き……\nまともに受けちゃだめよ。',
    ],
    'CampPeopleWounded_Again': [
        '……私のことはいいの。\n村を、取り戻して。',
    ],
    'CampPeopleWounded_Cleared': [
        '……聞いたよ。\n大顎、倒したんだね。',
        '私も、早く歩けるようにならなきゃ。\n村に帰る日のために。',
    ],
    'CampPeopleWounded_ClearedAgain': [
        'ありがとう、ハンターさん。\nこの恩は、きっと忘れない。',
    ],

    # --- 砂漠 (DrySandScene) のオアシスの隊商と、竜の骨の前のクノイチ (docs/Story.md 第2章)
    'CaravanMaster_First': [
        'おう、ハンターか！\nこんな砂の果てまで、よく来たな。',
        'うちは島から島へ荷を運ぶ隊商だ。\nこの泉で、足止めを食らってる。',
        '数日前、金色に光る石が\n北の城塞跡に落ちてきてな。',
        'それから泉は日に日に細るし、\nサソリどもが水場まで出やがる。',
        '砂嵐で荷車も二台なくした。\n……頼む、力を貸してくれ。',
    ],
    'CaravanMaster_Again': [
        'サソリの群れを散らしてくれ。\nそれと、砂に埋もれた荷もな。',
    ],
    'CaravanMaster_Cleared': [
        '泉の水が……戻ってきた！\nあんたがやってくれたんだな。',
        'あの光る石、あんたの島のもの\nだったのか。道理で、ただ事じゃねえ。',
        '拾った荷の中身は、飛行船の部品だ。\n島に要るなら、持っていきな。',
        '隊商は恩を忘れねえ。\nあんたの島にも、荷を回すぜ。',
    ],
    'CaravanMaster_ClearedAgain': [
        '泉が戻りゃ、商いも戻る。\n島に寄ったら、よろしくな。',
    ],
    'CaravanKeeper_First': [
        'ようこそ、泉へ。……と言っても、\nもう水はほとんど残っていません。',
        '泉は北の城塞の下から\n湧いているんです。',
        '光る石が落ちてから、地の底が\n熱を帯びたみたいで……。',
        'このままでは、あと幾日も\nもちません。どうか、急いで。',
    ],
    'CaravanKeeper_Again': [
        'サソリは尾を振り上げてから刺します。\n尾が上がったら、下がって。',
    ],
    'CaravanKeeper_Cleared': [
        '見てください、泉が……！\n水の音が、戻ってきました。',
        '本当に、ありがとうございます。\nこの水は、あなたのおかげです。',
    ],
    'CaravanKeeper_ClearedAgain': [
        '泉の水、少し飲んでいきますか？\n……冷たくて、おいしいですよ。',
    ],
    'CaravanBoy_First': [
        'わっ、ハンターだ！\nねえねえ、どこから来たの？',
        '空の島から？すげえ！\nおれ、駱駝の世話係なんだ。',
        'この砂漠、砂の下に何かいるんだよ。\n西の砂丘で、砂が盛り上がるの。',
        '砂が動いたら、すぐ離れて！\n下から、でっかい口が出てくるから。',
    ],
    'CaravanBoy_Again': [
        '東の砂丘には、でっかい竜の骨が\nあるんだ。見に行ってみなよ！',
    ],
    'CaravanBoy_Cleared': [
        'やったー！泉が戻ってきた！\n駱駝たちも、よろこんでるよ！',
        'おれも大きくなったら、\nハンターになるんだ！',
    ],
    'CaravanBoy_ClearedAgain': [
        'また来てね！\n次は駱駝に乗せてあげる！',
    ],
    'CaravanGuard_Lost': [
        '……っ、誰だ。\nハンター……か。助かった。',
        '自分は隊商の護衛だ。荷車を追って、\nここまで来て……しくじった。',
        '城塞の奥に、骨の竜がいる。\n光る石の傍から、動こうとしない。',
        '崩れた壁の割れ目から、\n中へ入れる。……気をつけろ。',
        '少し休んだら、泉へ戻る。\n頭に、無事だと伝えてくれ。',
    ],
    'CaravanGuard_Back': [
        '借りができたな。\n……骨の竜の翼には、気をつけろ。',
    ],
    'CaravanGuard_Cleared': [
        '骨の竜を倒したのか……！\n自分には、到底できなかった。',
    ],
    'DesertKunoichi_First': [
        '……来たんだ。\nこの骨、見せたかったんだよね。',
        '北の城塞は、私の故郷の島。\nあの古竜に、浮遊石を奪われて落ちた。',
        '落ちたとき、割れた島の底から\nこの骨が出てきた。……竜の骨だよ。',
        'でも見て。頭と胸の骨が無い。\n……砂の跡が、城塞へ続いてる。',
        '光る石が、この骨を呼んだんだ。\n……眠らせてあげて。お願い。',
    ],
    'DesertKunoichi_Again': [
        'あの骨は、ずっと私の島を\n支えてくれてた。……休ませてあげたい。',
    ],
    'DesertKunoichi_Cleared': [
        '……眠れたんだね。\nありがとう。あんたに頼んでよかった。',
        '見たでしょ。石が骨に宿って、\n竜が起き上がった。',
        '島の心臓は、竜の心臓なんだよ。\n私の島は、あの竜の心臓で浮いてた。',
        'あの古竜は、仲間の心臓を\n取り返して回ってるんだと思う。',
        '私があの古竜を追うのは、仇だから。\n……でも、それだけじゃなくなった。',
        'あんたの島を、この城塞みたいには\nさせない。……私も、手を貸すよ。',
    ],
    'DesertKunoichi_ClearedAgain': [
        '古竜の巣は、嵐の向こう。\n……その時は、私も行く。',
    ],
}

# どの BT にも使われなくなった会話。作り直したら消す
RETIRED_CHATS = ['Idle ActionInstructure', 'Merchant_Restored']


# ---------------------------------------------------------------- BT の部品
class Ops:
    """tools.bt apply に渡す op の列を組み立てる"""

    def __init__(self):
        self.ops = []

    def node(self, parent, kind):
        guid = meta_base.mint_guid()
        self.ops.append({'op': 'add-node', 'parent': parent, 'kind': kind, 'guid': guid})
        return guid

    def action(self, parent, label, kind, **params):
        guid = meta_base.mint_guid()
        self.ops.append({'op': 'add-node', 'parent': parent, 'kind': 'action', 'name': label, 'type': kind,
                         'guid': guid})
        if params:
            values = {k: (str(v).lower() if isinstance(v, bool) else str(v)) for k, v in params.items()}
            self.ops.append({'op': 'set-params', 'node': guid, 'set': values})
        return guid

    def bb(self, name):
        self.ops.append({'op': 'add-bb-param', 'name': name, 'value': 0})

    # --- よく使う形
    def icon(self, parent, label, icon):
        return self.action(parent, label, 'SetEnableShowChatIcon',
                           value1=icon[0], value2=icon[1], value3=icon[2], value4=icon[3])

    def once_icon(self, parent, icon=ICON_CHATTABLE):
        # NOTE: 吹き出しは最初の1回だけ出す。毎フレーム出すと、近づいたときのアイコンの切り替えを上書きする
        self.icon(self.node(parent, 'once-exec'), 'Show Chat Icon', icon)

    def chat(self, parent, label, name):
        return self.action(parent, label, 'Chat', value1=chat_guids[name])

    def implement_chat(self, parent, label, name):
        return self.action(parent, label, 'ImplementChat', value1=chat_guids[name])

    def flag(self, parent, flag, expected=True):
        return self.action(parent, f'{FLAG_NAMES[flag]} == {expected}', 'IsStoryFlag', flag_=flag, expected_=expected)

    def read_bb(self, parent, key, value=0):
        return self.action(parent, f'{key} == {value}', 'ReadBlackBoard', keyName_=key, equalValue_=value)

    def write_bb(self, parent, key, value=1):
        return self.action(parent, f'{key} = {value}', 'WriteBlackBoard', keyName_=key, value_=value)


chat_guids = {}


def write_chats(names):
    for name in names:
        chat_guids[name] = write_npc_chat(name, CHATS[name])


def rebuild(name, build):
    """BT を作り直して build(ops) の op を当てる。GUID は保つ"""
    data = BT_DIR / f'{name}.friendBehaviourData'
    recreate('tools.bt', name, data, BT_DIR / f'{name}.friendBehaviourData.meta', '--npc-kind', 'friendly')
    ops = Ops()
    build(ops)
    path = f'Assets/Data/FriendlyNpcBehviour/{name}.friendBehaviourData'
    apply_ops('tools.bt', path, ops.ops)
    run('tools.bt', 'layout', path)
    run('tools.bt', 'validate', path)


def walk(node):
    if node is None:
        return
    yield node
    for child in getattr(node, 'children', None) or []:
        yield from walk(child)
    yield from walk(getattr(node, 'child', None))


def refs(tree_name, *action_names):
    """作り直す前の BT から、名前の付いたアクションが参照している GUID を拾う (見つかった順)"""
    tree = reader.read_tree_file(BT_DIR / f'{tree_name}.friendBehaviourData')
    found = {}
    for node in walk(tree.entry):
        if isinstance(node, model.Action) and node.name in action_names and node.name not in found:
            found[node.name] = GUID_RE.findall(repr(node.params))
    missing = [n for n in action_names if n not in found]
    if missing:
        raise SystemExit(f'{tree_name}: no action named {missing} (already rebuilt? see REFS)')
    return found


# 作り直した後の BT でも同じ名前のアクションを残すので、2回目以降もここから拾える
INSTRUCTOR_REFS = ('Enable SurpriseIcon', 'Enable PortalGuideCamera')
MERCHANT_REFS = ('Open Shop',)


# ---------------------------------------------------------------- 拠点の島
def instructor():
    r = refs('IdleActionInstructure', *INSTRUCTOR_REFS)
    surprise, = r['Enable SurpriseIcon']
    camera, = r['Enable PortalGuideCamera']

    def build(o):
        o.bb('ReportTalked')
        o.bb('GuideStep')
        root = o.node('entry', 'selector')

        # 最初の会話の続き。Chat が会話中を下ろすので IsChat の外で続ける
        guide = o.node(root, 'sequence')
        o.read_bb(guide, 'GuideStep', 1)
        o.action(guide, 'Enable PortalGuideCamera', 'PurposeCamera', purposeCamera_=camera, onPurposeCameraEnable_=True)
        o.implement_chat(guide, 'Portal Guide', 'Instructor_PortalGuide')
        o.action(guide, 'Disable PortalGuideCamera', 'PurposeCamera', purposeCamera_=camera, onPurposeCameraEnable_=False)
        o.action(guide, 'Set RestorationStarted', 'SetStoryFlag', flag_=RESTORATION_STARTED)
        o.write_bb(guide, 'GuideStep', 2)

        talk = o.node(root, 'sequence')
        o.action(talk, 'IsChat', 'IsChat')
        o.action(talk, 'Disable SurpriseIcon', 'GameObjectSetEnable', enableGameObject_=surprise, isEnable_=False)
        o.icon(talk, 'Show ChattingIcon', ICON_CHATTING)
        pick = o.node(talk, 'selector')

        # 目覚めて最初の会話。島の状況を話し、草原への台座を見せる
        start = o.node(pick, 'sequence')
        o.flag(start, RESTORATION_STARTED, False)
        o.chat(start, 'Restoration Start', 'Instructor_RestorationStart')
        o.write_bb(start, 'GuideStep', 1)

        report = o.node(pick, 'sequence')
        o.flag(report, GRASSLAND_CLEARED)
        o.read_bb(report, 'ReportTalked')
        o.chat(report, 'GrassLand Report', 'Instructor_GrassLandReport')
        o.write_bb(report, 'ReportTalked')

        after = o.node(pick, 'sequence')
        o.flag(after, GRASSLAND_CLEARED)
        o.chat(after, 'After GrassLand', 'Instructor_AfterGrassLand')

        before = o.node(pick, 'sequence')
        o.action(before, 'Enable PortalGuideCamera', 'PurposeCamera', purposeCamera_=camera, onPurposeCameraEnable_=True)
        o.chat(before, 'Before GrassLand', 'Instructor_BeforeGrassLand')
        o.action(before, 'Disable PortalGuideCamera', 'PurposeCamera', purposeCamera_=camera, onPurposeCameraEnable_=False)

        # 話しかけられる前。物語が始まるまでは驚きアイコンで呼ぶ
        waiting = o.node(root, 'sequence')
        o.flag(waiting, RESTORATION_STARTED, False)
        o.icon(waiting, 'Hide ChatIcon', ICON_HIDDEN)
        o.action(waiting, 'Enable SurpriseIcon', 'GameObjectSetEnable', enableGameObject_=surprise, isEnable_=True)

        idle = o.node(o.node(root, 'once-exec'), 'sequence')
        o.action(idle, 'Disable SurpriseIcon', 'GameObjectSetEnable', enableGameObject_=surprise, isEnable_=False)
        o.icon(idle, 'Show Chat Icon', ICON_CHATTABLE)

    rebuild('IdleActionInstructure', build)


def merchant():
    prefab, stall = refs('Merchant', *MERCHANT_REFS)['Open Shop']

    def build(o):
        o.bb('Talked')
        root = o.node('entry', 'sequence')
        o.once_icon(root)
        select = o.node(root, 'selector')
        talk = o.node(select, 'sequence')
        o.action(talk, 'IsChat', 'IsChat')
        o.action(talk, 'Talk Animation', 'PlayAnimation', animatorSetParamNumber_=1)
        pick = o.node(talk, 'selector')

        first = o.node(pick, 'sequence')
        o.read_bb(first, 'Talked')
        o.chat(first, 'First', 'Merchant_First')
        o.write_bb(first, 'Talked')

        o.chat(pick, 'Greet', 'Merchant')
        o.action(talk, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)
        o.action(talk, 'Open Shop', 'OpenShop', prefab_=prefab, stall_=stall)
        o.action(select, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)

    rebuild('Merchant', build)


def broker():
    """キャラ選択は一族の家の女狩人に移した。仲介人は一族の家へ案内するだけ"""

    def build(o):
        o.bb('Talked')
        root = o.node('entry', 'sequence')
        o.once_icon(root)
        select = o.node(root, 'selector')
        talk = o.node(select, 'sequence')
        o.action(talk, 'IsChat', 'IsChat')
        o.action(talk, 'Talk Animation', 'PlayAnimation', animatorSetParamNumber_=1)
        pick = o.node(talk, 'selector')
        first = o.node(pick, 'sequence')
        o.read_bb(first, 'Talked')
        o.chat(first, 'First', 'CharacterBroker_First')
        o.write_bb(first, 'Talked')
        clan = o.node(pick, 'sequence')
        o.action(clan, 'ClanHouse Restored', 'IsRestored', facility_=FACILITY_CLAN_HOUSE, expected_=True)
        o.chat(clan, 'ClanHouse', 'CharacterBroker_ClanHouse')
        o.chat(pick, 'Greet', 'CharacterBroker')
        o.action(talk, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)
        o.action(select, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)

    rebuild('CharacterBroker', build)


def island():
    write_chats(['Instructor_RestorationStart', 'Instructor_PortalGuide', 'Instructor_BeforeGrassLand',
                 'Instructor_GrassLandReport', 'Instructor_AfterGrassLand',
                 'Merchant_First', 'Merchant',
                 'CharacterBroker_First', 'CharacterBroker', 'CharacterBroker_ClanHouse'])
    instructor()
    merchant()
    broker()
    for name in RETIRED_CHATS:
        for path in (BT_DIR.parent / 'NpcChatText' / f'{name}.npcChat', BT_DIR.parent / 'NpcChatText' / f'{name}.npcChat.meta'):
            if path.exists():
                path.unlink()
                print(f'  removed {path.relative_to(REPO)}')


# ---------------------------------------------------------------- 草原の野営地
CAMP_ROLES = ['Elder', 'Lookout', 'Huntress', 'Wounded']


def talker(name, cleared_flag=GRASSLAND_CLEARED):
    """話しかけると、cleared_flag (既定は草原) が立つ前は <name>_First (1回) → _Again、後は _Cleared (1回) → _ClearedAgain"""

    def build(o):
        o.bb('Talked')
        o.bb('ClearedTalked')
        root = o.node('entry', 'sequence')
        o.once_icon(root)
        select = o.node(root, 'selector')
        talk = o.node(select, 'sequence')
        o.action(talk, 'IsChat', 'IsChat')
        o.action(talk, 'Talk Animation', 'PlayAnimation', animatorSetParamNumber_=1)
        pick = o.node(talk, 'selector')

        cleared = o.node(pick, 'sequence')
        o.flag(cleared, cleared_flag)
        o.read_bb(cleared, 'ClearedTalked')
        o.chat(cleared, 'Cleared', f'{name}_Cleared')
        o.write_bb(cleared, 'ClearedTalked')

        cleared_again = o.node(pick, 'sequence')
        o.flag(cleared_again, cleared_flag)
        o.chat(cleared_again, 'Cleared Again', f'{name}_ClearedAgain')

        first = o.node(pick, 'sequence')
        o.read_bb(first, 'Talked')
        o.chat(first, 'First Talk', f'{name}_First')
        o.write_bb(first, 'Talked')

        o.chat(pick, 'Talk Again', f'{name}_Again')
        o.action(talk, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)
        o.action(select, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)

    rebuild(name, build)


TALK_STATES = ('First', 'Again', 'Cleared', 'ClearedAgain')


def camp():
    for role in CAMP_ROLES:
        write_chats([f'CampPeople{role}_{s}' for s in TALK_STATES])
        talker(f'CampPeople{role}')


# ---------------------------------------------------------------- 拠点の島に移ってきた2人
MAIN_ISLAND = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
PROLOGUE_SCENE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
NEWCOMER_ROOT = 'StoryNpcs'
TEMPLATE_NPC = 'CharacterBrokerNpc'   # 拠点の島で動いている NPC。版キーがこのシーンに合っている
PLAYER_SPAWN_XZ = (-10.5, -18.5)      # "PlayerSpawn Pos"。2人ともこちらを向く

# (BT 名, 序章シーンの GameObject 名, 置く位置 (world))。
# 位置はエディタで足元を見て決めた (2026-09-23)。石の広場の床は y 34.58 (教官と同じ)、階段下の芝生は y 22.5
NEWCOMERS = [
    ('IslandKunoichi', 'Kunoichi-Adventure', (-18.0, 34.58, -24.0)),   # 広場の奥。空を眺めている
    ('IslandYoungMan', 'WildYoungMan', (-8.0, 22.5, 24.0)),            # 階段を下りた芝生
]


def field_guid(blob):
    return GUID_RE.findall(repr(blob))[0]


def place_newcomers(bt_guids):
    import copy
    import math

    from camp_people import ICON_WORLD_SCALE, ICON_X, VersionFixer, set_vec3, yaw_facing
    from game_over_prefab import check, let_writer_place_versions
    from grassland_nature_scatter import bake_world_matrices, first_versions, quat_axis
    from grassland_nature_scatter import walk as scene_walk
    from tools.common.cereal_json import Num, dumps, loads, read_text, to_file_bytes
    from tools.scene import catalog as catalog_mod, edits, validate, writer
    from tools.scene import reader as scene_reader

    def find(scene, name):
        node = next((n for r in scene.roots for n in scene_walk(r) if n.name == name), None)
        if node is None:
            raise SystemExit(f'{name} not found')
        return node

    def world_scale(scene, name):
        # NOTE: 序章の2人は縮小された飛行船 (AirShip) の子なので、親の scale も掛ける
        def rec(node, scale):
            scale *= float(node.transform.local_scale.x.value)
            if node.name == name:
                return scale
            for child in node.transform.children:
                found = rec(child, scale)
                if found is not None:
                    return found
            return None
        return next(s for s in (rec(r, 1.0) for r in scene.roots) if s is not None)

    scene = scene_reader.read_scene_file(MAIN_ISLAND)
    scene.roots = [r for r in scene.roots if r.name != NEWCOMER_ROOT]
    template = find(scene, TEMPLATE_NPC)
    prologue_scene = scene_reader.read_scene_file(PROLOGUE_SCENE)

    root = edits.add_gameobject(scene, parent=None, name=NEWCOMER_ROOT)
    for key, source_name, pos in NEWCOMERS:
        source = find(prologue_scene, source_name)
        src = {c.fqn.rsplit('::', 1)[-1]: c.data for c in source.components}
        src_icon = next(c for c in source.transform.children if c.name == 'BillBoardNpcChatIcon')
        scale = world_scale(prologue_scene, source_name)

        node = copy.deepcopy(template)
        remap = {}
        edits._remint_guids(node, remap)
        edits._remap_guid_references(node, remap)
        node.name = key
        node.transform.children = [c for c in node.transform.children if c.name == 'BillBoardNpcChatIcon']
        yaw, _ = yaw_facing((pos[0], pos[2]), PLAYER_SPAWN_XZ)
        node.transform.local_pos = edits._vec3_from_floats(pos)
        node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), yaw))
        node.transform.local_scale = edits._vec3_from_floats((scale,) * 3)

        for comp in node.components:
            leaf = comp.fqn.rsplit('::', 1)[-1]
            if leaf == 'FriendlyNpc':
                comp.data['name_'] = src['FriendlyNpc']['name_']
                edits._set_field_guid(comp.data['friendlyNpcBehaviourFile_'], bt_guids[key])
            elif leaf == 'ModelRenderer':
                edits._set_field_guid(comp.data['mv1File_'], field_guid(src['ModelRenderer']['mv1File_']))
            elif leaf == 'Animator':
                edits._set_field_guid(comp.data['animationTreeFile_'], field_guid(src['Animator']['animationTreeFile_']))
            elif leaf == 'CapsuleCollider':
                base = comp.data['value0']
                base = base.body if hasattr(base, 'body') else base
                src_base = src['CapsuleCollider']['value0']
                src_base = src_base.body if hasattr(src_base, 'body') else src_base
                offset = [float(src_base['offset_'][k].value) for k in ('value0', 'value1', 'value2')]
                set_vec3(base['offset_'], offset)
                comp.data['radius_'] = Num.of_float(float(src['CapsuleCollider']['radius_'].value))
                comp.data['height_'] = Num.of_float(float(src['CapsuleCollider']['height_'].value))
            elif leaf == 'RigidBody':
                if int(comp.data['motionType_'].value) != 2 or int(comp.data['constraints_'].value) != 61:
                    raise SystemExit('template RigidBody is no longer Dynamic + constraints 61')

        icon = node.transform.children[0]
        icon.transform.local_pos = edits._vec3_from_floats((ICON_X, float(src_icon.transform.local_pos.y.value), 0.0))
        icon.transform.local_scale = edits._vec3_from_floats((ICON_WORLD_SCALE / scale,) * 3)
        root.transform.children.append(node)
        print(f'  {key:16s} {pos}  yaw {math.degrees(yaw):6.1f}  scale {scale}')

    for node in scene_walk(root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root)

    source_versions = first_versions(read_text(MAIN_ISLAND))
    text = writer.write_scene(scene)
    tree = loads(text)
    fixer = VersionFixer(catalog_mod.load(), f'/gameObject_{len(scene.roots) - 1}/', source_versions)
    fixer.run(tree, '')
    text = dumps(tree)
    print(f'  versions: added {sorted(set(fixer.added))}, stripped {fixer.stripped} repeat key(s)')
    check(text, validate.validate_scene(scene), MAIN_ISLAND.name)
    MAIN_ISLAND.write_bytes(to_file_bytes(text))
    scene_reader.read_scene_file(MAIN_ISLAND)
    print(f'  wrote {MAIN_ISLAND.relative_to(REPO)}')


def newcomers():
    bt_guids = {}
    for key, _source, _pos in NEWCOMERS:
        write_chats([f'{key}_{s}' for s in TALK_STATES])
        talker(key)
        bt_guids[key] = asset_guid(BT_DIR / f'{key}.friendBehaviourData.meta')
    place_newcomers(bt_guids)


# ---------------------------------------------------------------- 一族の家
def podium_component_guid():
    """MainIslandScene の展示台 (CharacterPodium コンポーネント) の GUID"""
    from game_over_prefab import guid_of
    from grassland_nature_scatter import walk as scene_walk
    from tools.scene import reader as scene_reader

    scene = scene_reader.read_scene_file(MAIN_ISLAND)
    for root in scene.roots:
        for node in scene_walk(root):
            for comp in node.components:
                if comp.fqn.endswith('::CharacterPodium'):
                    return guid_of(comp)
    raise SystemExit('CharacterPodium not found in MainIslandScene')


def clan():
    """一族の家 (ClanHouse.prefab) に立つ女狩人。話しかけると仲間を選ばせる。prefab は clan_house.py"""
    write_chats(['ClanHuntress_First', 'ClanHuntress'])
    prefab = asset_guid(CHARACTER_SELECT_UI)
    podium = podium_component_guid()

    def build(o):
        o.bb('Talked')
        root = o.node('entry', 'sequence')
        o.once_icon(root)
        select = o.node(root, 'selector')
        talk = o.node(select, 'sequence')
        o.action(talk, 'IsChat', 'IsChat')
        o.action(talk, 'Talk Animation', 'PlayAnimation', animatorSetParamNumber_=1)
        pick = o.node(talk, 'selector')
        first = o.node(pick, 'sequence')
        o.read_bb(first, 'Talked')
        o.chat(first, 'First', 'ClanHuntress_First')
        o.write_bb(first, 'Talked')
        o.chat(pick, 'Greet', 'ClanHuntress')
        o.action(talk, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)
        o.action(talk, 'Open Character Select', 'OpenCharacterSelect', prefab_=prefab, podium_=podium)
        o.action(select, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)

    rebuild('ClanHuntress', build)


# ---------------------------------------------------------------- 砂漠のオアシスの隊商と、竜の骨の前のクノイチ
CARAVAN_TALKERS = ['CaravanMaster', 'CaravanKeeper', 'CaravanBoy', 'DesertKunoichi']


def caravan_guard():
    """城塞の東の割れ目の外で座り込んでいる護衛。最初に話すと抜け道を教えて DesertGuardRescued を立てる。
    その後は _Back、骸竜を倒した後は _Cleared"""
    write_chats(['CaravanGuard_Lost', 'CaravanGuard_Back', 'CaravanGuard_Cleared'])

    def build(o):
        root = o.node('entry', 'sequence')
        o.once_icon(root)
        select = o.node(root, 'selector')
        talk = o.node(select, 'sequence')
        o.action(talk, 'IsChat', 'IsChat')
        o.action(talk, 'Talk Animation', 'PlayAnimation', animatorSetParamNumber_=1)
        pick = o.node(talk, 'selector')
        cleared = o.node(pick, 'sequence')
        o.flag(cleared, DESERT_CLEARED)
        o.chat(cleared, 'Cleared', 'CaravanGuard_Cleared')
        back = o.node(pick, 'sequence')
        o.flag(back, DESERT_GUARD_RESCUED)
        o.chat(back, 'Back', 'CaravanGuard_Back')
        lost = o.node(pick, 'sequence')
        o.chat(lost, 'Lost', 'CaravanGuard_Lost')
        o.action(lost, 'Set DesertGuardRescued', 'SetStoryFlag', flag_=DESERT_GUARD_RESCUED)
        o.action(talk, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)
        o.action(select, 'Idle Animation', 'PlayAnimation', animatorSetParamNumber_=0)

    rebuild('CaravanGuard', build)


def desert():
    """会話と BT だけ作る。砂漠のシーンへの配置は tools/art/desert_caravan.py place"""
    for name in CARAVAN_TALKERS:
        write_chats([f'{name}_{s}' for s in TALK_STATES])
        talker(name, DESERT_CLEARED)
    caravan_guard()


# ---------------------------------------------------------------- 序章
def prologue():
    # 船上の2人と教官の訓練は BT をそのまま使い、台詞だけ差し替える
    write_chats(['AirShipKunoichi', 'AirShipYoungMan', 'Introduction ActionInstructure'])


DRAGON_TREE = 'Assets/Data/EnemyBehaviour/FirstEventDragon.enemyBehaviourData'


DRAGON_ROUTES = REPO / 'Assets' / 'Data' / 'EventNpcWalkingRoute' / 'FirstEventDragon'
HEART_PARTICLE = REPO / 'Assets' / 'Prefab' / 'Particle' / 'HeartShardScatter.prefab.meta'   # heart_shatter_effect.py
PRODUCTION_CAMERA = 'FirstTouchDownIsland ProductionCamera'
DRAGON_CLAW_STATE, DRAGON_FLYING_IDLE_STATE = 10, 4    # FirstEventDragon.animTree の Attack1 / FlyingIdle


def dragon():
    """撃ち落とした後、巣へ帰る前に島の浮遊石を抜く (docs/Story.md 序章 5)。
    島へ降りて爪を突き立て、2つの浮遊石が二方へ飛んでいき、教官が叫ぶ。前に入れた Heart ノードは作り直す"""
    write_chats(['FirstDragon Heart Shatter'])
    tree = reader.read_tree_file(REPO / DRAGON_TREE)
    nodes = list(walk(tree.entry))
    parent = next(n for n in nodes if any(isinstance(c, model.Action) and c.name == 'Return To Residence'
                                         for c in getattr(n, 'children', None) or []))
    ops = [{'op': 'remove-node', 'node': c.guid} for c in parent.children
           if isinstance(c, model.Action) and c.name.startswith('Heart ')]
    index = [c for c in parent.children if not (isinstance(c, model.Action) and c.name.startswith('Heart '))]
    index = next(i for i, c in enumerate(index) if getattr(c, 'name', '') == 'Return To Residence')

    def add(label, kind, **params):
        nonlocal index
        guid = meta_base.mint_guid()
        ops.append({'op': 'add-node', 'parent': parent.guid, 'kind': 'action', 'name': label, 'type': kind,
                    'guid': guid, 'index': index})
        if params:
            ops.append({'op': 'set-params', 'node': guid,
                        'set': {k: (str(v).lower() if isinstance(v, bool) else str(v)) for k, v in params.items()}})
        index += 1

    add('Heart Camera On', 'PurposeCamera', prefabPurposeCamera_=PRODUCTION_CAMERA, priority_=100)
    add('Heart Dive', 'MoveEventRoute', moveRoute_=asset_guid(DRAGON_ROUTES / 'ToDestroyIsland.eventNpcWalkingRoute.meta'),
        isOnceExecute_=True, isRotateToMoveDir_=True, rotateSpeedDeg_=180.0)
    add('Heart Claw Animation', 'PlayAnimation', animatorSetParamNumber_=DRAGON_CLAW_STATE)
    add('Heart Claw Wait', 'WaitSeconds', waitSeconds_=0.6)
    add('Heart Shard Scatter', 'GenerateParticle', particlePrefab_=asset_guid(HEART_PARTICLE), lifeTime_=3.3)
    add('Heart Shatter Shake', 'ShakeCamera', intensity_=0.8, duration_=2.5)
    add('Heart Shatter Chat', 'Chat', displayName_='教官', chatData_=chat_guids['FirstDragon Heart Shatter'])
    add('Heart Camera Off', 'PurposeCamera', prefabPurposeCamera_=PRODUCTION_CAMERA, priority_=-1)
    add('Heart Flying Idle', 'PlayAnimation', animatorSetParamNumber_=DRAGON_FLYING_IDLE_STATE)
    apply_ops('tools.bt', DRAGON_TREE, ops)
    run('tools.bt', 'validate', DRAGON_TREE)


STEPS = {'prologue': prologue, 'dragon': dragon, 'island': island, 'newcomers': newcomers, 'camp': camp,
         'clan': clan, 'desert': desert}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--only', choices=sorted(STEPS), action='append')
    args = parser.parse_args()
    for key in args.only or STEPS:
        print(f'[{key}]')
        STEPS[key]()


if __name__ == '__main__':
    main()
