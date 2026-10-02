# ストーリー設計

ゲームの筋・登場人物・施設と、それを支えるコード（`StoryProgress` / `RestorationGate`）のまとめ。
台詞・依頼・施設・演出を作るときの **正（source of truth）** はこのファイル。
既存の会話（`Assets/Data/NpcChatText/*.npcChat.meta`）・依頼（`Assets/Data/EventNotice/*.boardQuest.meta`）・
ステージ（`Assets/Data/Stage/*.stageData.meta`）から起こした（2026-09-23）。

---

## 0. このファイルを使って作業するときの約束（AI 向け）

- **【確定】** と書かれた項目は変えない。変えたい場合は、先にユーザーに確認する。
- **【未設計】** と書かれた項目（施設、終章の細部、砂漠の未決定の部分）は、これから作る。
  台詞・依頼・フラグ・施設を勝手に作らず、案を出してユーザーに決めてもらう。
- **【仮】** は、たたき台として使ってよい。決まったら **【確定】** に書き換える。
- 物語に何かを足したら（人物・フラグ・施設・台詞のファイル）、同じ変更でこのファイルも更新する。
- 会話・依頼の中身は `.meta` 側に入っている（本体の `.npcChat` / `.boardQuest` は 0 バイトで正常）。
- 復興の進み具合は **手元の PC にだけ保存し、マルチプレイでは共有しない**。
  ほかのプレイヤーの島を見に行く機能もない。【確定】

## 1. 一行で【確定】（2026-09-25 書き直し）

竜の心臓を奪われて沈みかけた拠点の島を、ハンターの稼ぎで立て直し、
その島ごと最後の竜の巣へ乗り込む話。そこで、島々が竜の心臓で浮いていること、
**ハンターがもとは竜狩りだった** ことを知る。

- お金を稼ぐ理由は **島の復興資金**。
- 狩り場もすべて **空に浮かぶ島**。地上へ降りる話はない（2026-09-25）。
- 各狩り場の島には、その島の人たちの物語と、拠点の島から飛んでいった **心臓** がひとつずつある。
- 島を直すと、施設・住人・行き先が増える。直した島そのものが船になって、最終決戦の舞台になる。
- 狩り場は **2つ**（草原の島・砂の島）、心臓も **2つ**（緑・光）。工数を抑えるため、岩石地帯と火の心臓はやめた（2026-09-24）。

## 2. 世界【確定】（2026-09-25 書き直し）

### 今の世界

- 空に島々が浮かんでいて、足元は一面の **雲海**。雲の下は、落ちた島が行き着く、誰も帰ってこない「底」。
  **地上に住む人はいない**（狩り場も島）。人は飛行船や転移の台座（紫に光る台座 = `IslandPedestial`）で島を渡る。
- 島は、底に埋まった **島の心臓** の力で浮いている。見た目は光る結晶の塊で、正体は竜の心臓（下の「昔の話」）。
  **「心臓」という名前と設定はやめた**（2026-09-25）。台詞・依頼・説明では「島の心臓」「心臓」とだけ言う。
  心臓の正体を知らない人（長老・隊商など）は、見たままに「光る石」と言ってよい。
  コードとアセットの名前（`FloatingStone`、`GreenFloatingStone.prefab`、`StoryFlag::GreenStoneReturned` など）は変えない。
- 拠点の島には **緑と光の2つ** の心臓がある。心臓が2つある島は珍しい（青年の台詞）。
  心臓は砕けたのではなく、**丸ごと抜けて飛んでいった**（欠片・核片という設定はやめた。2026-09-24）。
- **島の心臓は引き合い、ものを引き寄せ、持ち上げる**。物語の不思議は、このひとつのルールで説明する。
  - 獣や魔物を引き寄せる。心臓が落ちた島には群れが集まり、強いものほど心臓の傍に居着く。持ち去れば獣は散る。
    ステージで石が飛び去ると、残っていた獣は石の方を振り向いてから反対へ走り去って消える
    （`Story::IslandHeartDeparture` + BT アクション `Story::FleeFromIslandHeart`。ハイエナ・サソリ・ワームのツリーの先頭）。
  - 【仮】島の底に埋まっている間は土に抑えられて静かで、むき出しになると呼び始める（埋め戻せば、もう呼ばない）。
  - 周りの土を持ち上げる。落ちた心臓の周りでは土くれや岩がゆっくり浮かぶ（在りかが遠くから分かる。去ると浮かなくなる）。
    → `GreenCoreAura` / `LightCoreAura` の `Debris` / `Chunks`（`tools/art/green_core_effect.py`）。
    落ちた心臓が島ごと浮き上がっていく（放っておくと取り戻せない）という設定は **使わない**（急がず稼ぐ遊び方と食い違う）。
  - 心臓どうしが引き合う。抜けた心臓は、近くの島の心臓に引かれてそこへ落ちた。
  - 戻った心臓は、沈んだ島を引き上げる（草原の後に噴水の島が雲海から戻ってくる）。
- **ハンター** は、島々で魔物を狩って報酬を得る稼業。拠点の島は、駆け出しのハンターが集まって鍛える島。
  （表向き。本当の起こりは下の「昔の話」）
- 狩り場の島には属性がある。心臓の色はこの属性に合わせる。

| 狩り場 | 難度・属性 | 状態 |
| --- | --- | --- |
| 草原の島（草原地帯） | 初級・緑 | 【確定】第1章 |
| 砂の島（砂漠地帯） | 中級・光 | 【確定】第2章（2026-09-24 決定。シーンは作りかけ） |

岩石地帯はやめた【確定】（ステージ選択の行と `RockyStage.stageData` は削除済み。`QuestType::RockyTyrant` は int で残るので enum に置いたまま）。

見た目【確定】: 狩り場の外周は山ではなく **崖と雲海** にして、遠景にほかの浮島を置く。到着演出（`StageArrivalMovie`）で島を見下ろすカットを入れる。
→ 実装済み（遠景の浮島だけ。崖と雲海・見下ろすカットは未着手）: 草原の空に 9、砂の島の空に 11 の浮島が浮かぶ（シーンの `SkyIslands`）。
  外周の山の外、山越しに島の底の岩まで見える高さに置き、ゆっくり上下して傾く（`GamePlay::Prop::FloatingDrift`）。脇には岩のかけらが浮かぶ。
  草原の空は拠点の島の周りと同じ丸い島（灰色の岩に草）に、林・廃墟・見張り台。
  砂の島の空は草原と違って見えるように、形も色も変える: 赤い砂岩に砂の島で、細長い尾根・広く薄い岩盤・2つがくっついた島・
  段のある台地・細く垂れる岩の塔（`Assets/Art/Models/isLand/Sky/SkyIsland_*_Sand.mv1`。Blender で作る:
  `tools/art/sky_island_shapes_blender.py` -> `sky_island_models.py`）と丸い島（`SandIsland_Broken.mv1`）に、台地・よその城塞のかけら・
  神殿・門・砂岩の柱・ヤシの泉。どの島かは決めていない（名前も住人もない、ただの遠景）。
  拠点の島 (MainIslandScene) の空にも 12 の浮島がある。草の島の形 (`isLand/Sky/SkyIsland_*_Grass.mv1`) と丸い島を、足元より下から見上げる高さまでばらして置いた。
  島が遠いので、この3つのシーンはカメラの far を 4000、空のドームを 3.5 倍にしている。配置は `python tools/art/sky_islands.py [grass] [desert] [main]`。
  序章 (FirstTouchDownMainIsLandScene) の空にも同じ配置で置いている (襲撃の前の同じ空)。
  拠点の島の地面の小物は `python tools/art/main_island_props.py [prologue] [main]`。序章は襲撃の前なので花・木・旗・焚き火の集まり場でにぎやかに、
  第1章は襲撃のあとなので瓦礫・壊れた柵や荷車・潰れた小屋・倒木と切り株・燻る煙、落ちた島から逃げてきた人の仮住まい (テントと焚き火)、
  島の中心に心臓が抜けた地割れの跡 (`CoreShardCrater`) を置く。第1章の方が栄えて見えないようにする。
  周りの島 (噴水の島・家の島・大砲の島・訓練の島) にも置く。落ちる/戻る島の小物は島の子なので一緒に動く。第1章の噴水の島は
  狩人の一族の野営地 (テント・焚き火・赤い旗)。通り道 (特に訓練の島への橋) はスクリプトが空いているか確かめる。
拠点の島が沈んでいくのは、雲海が迫ってくる見た目で出す。

### 昔の話（物語の真相。プレイヤーには少しずつ明かす）

1. 昔、人も竜も大地に住んでいた。
2. 大地が雲に呑まれた。雲がせり上がり、大地は雲の底へ沈んでいった（今の雲海の下にあるのは、この大地）。なぜ呑まれたかは【未設計】。
3. **竜の心臓は、死んでも光を失わない。** 結晶のように固まり、ものを引き寄せ、持ち上げる力を持ち続ける。
   人は **竜を狩り、心臓を奪って** 自分たちの土地を空へ持ち上げた。これが **ハンター（竜狩り）** の起こり。
   竜の亡骸は、島の岩の奥に眠っている。
4. 長い時が経ち、人は竜を狩り尽くし、そのことを忘れた。「島の心臓」という呼び名と、「ハンター」という稼業の名だけが残った。
   ハンターは今では魔物を狩る稼業で、竜狩りだったことは誰も知らない（言い伝えを知る年寄りがいる程度）。
   - 拠点の島は、昔の **竜狩りの本拠**。だから心臓を2つも持っている（大きな獲物を取った者の島）。
     山頂の大砲は、昔の **竜撃ちの砲**。序章でそれがドラゴンを撃ち落とせたのは、そのための砲だから。
5. **竜は1頭だけ生き残った。それが古竜**（序章のドラゴン）。
   古竜は島々から仲間の心臓を取り戻して回っている。心臓を抜かれた島は、雲海へ落ちる。
   - 目的: **仲間を蘇らせる**。心臓を骨に戻せば竜は起き上がる（砂の島の骸竜がその証拠）。
     巣は、奪った心臓と仲間の骨を積み上げた **竜の墓場**。
   - 拠点の島を襲ったのは、竜狩りの本拠で、心臓が2つあるから。
   - 食べてはいない。「喰らう」「力を増す」という設定はやめた（2026-09-25）。

人は竜の犠牲（竜狩り）の上に暮らしていて、竜は奪われた仲間を取り返したい。**どちらにも言い分がある** のが、この物語の芯。
古竜は悪ではないが、止めなければ島々が落ち、人が死ぬ。

### 明かす順番【確定】

| 章 | プレイヤーが知ること |
| --- | --- |
| 序章 | 謎の竜が島の心臓を奪いに来る。大砲でひるんだ隙に、石が逃げる |
| 第1章 草原 | 長老の昔話「島の心臓は、昔は本当に竜の心臓だったと言う」。ただの言い伝えとして聞き流させる |
| 第2章 砂 | 骨の竜は胸が無い（心臓が落ちた城塞の島の石だった）。光の石が骨に宿って起き上がる。言い伝えは本当だった。クノイチが真相の一部を話す |
| 終章 | 巣は竜の墓場。古竜は仲間を蘇らせようとしている。ハンターがもとは竜狩りで、拠点の島がその本拠だったと分かる |

## 3. 登場人物

台詞を書くときは **口調** の列に合わせる。口調は既存の台詞から拾った。

| 人物 | アセット | 口調（一人称・例） | 役割 |
| --- | --- | --- | --- |
| 主人公 | 剣士 / 魔術師 | 喋らない | 今日の船で着いた新人ハンター。序章でドラゴンを撃ち落とす【確定】 |
| 教官 | `ActionInstructure` / `IdleActionInstructure` | 「俺」。短い命令形。「口より先に手を動かせ。」 | 序章で主人公を鍛える。襲撃で脚を痛めて前線を退き、島の復興をまとめる（復興ボードの担当）【仮】。竜狩りのことは知っていて、古竜を倒した後に明かす【仮】（2026-09-27） |
| 旅のクノイチ | `AirShipKunoichi` | 「私」。くだけた口調。「……気をつけなよ。」 | 故郷の島（今は砂の島に墜ちた城塞）を古竜に落とされ、古竜を追っている。島が落ちたとき、割れた島の底から竜の骨が出てくるのを見ていて、真相の一部を知っている【確定】。章の区切りに現れて情報をくれ、終章で共闘する【仮】 |
| 飛行船の青年 | `AirShipYoungMan` | 「僕」。明るい。「もう胸が躍ってるよ。」 | 空の果てを夢見ている。墜ちた飛行船を直したがっている。造船所の担当にする予定【仮・砂漠と一緒に決める】 |
| 商人 | `Merchant` | 威勢のいい商売口調。「うちで揃えていきな。」 | 島の雑貨屋。復興が進むと品揃えが増える【確定】 |
| 酒場の仲介人 | `CharacterBroker` | ぶっきらぼう。「好きなのを連れていきな。」 | 掲示板の依頼を回す。騒ぎで腕利きが逃げたので仲間は紹介できず、一族の家へ案内する【確定】 |
| 長老 | `CampPeopleElder` | 「わし」。古風。「〜しておる」「〜じゃ」寄り | 草原の狩人一族の長【確定】。「島の心臓は昔は竜の心臓だった」という言い伝えを話す【確定】 |
| 女狩人 | `CampPeopleHuntress` / `ClanHuntress` | 落ち着いた女言葉。「囲まれないで。」 | 狩りの助言役。草原の後、一族と噴水の島に移り住む。一族の家が建つと、復興を手伝いたい一族の者を仲間に出す（キャラ選択）【確定】 |
| 見張り | `CampPeopleLookout` | 小声で緊張した口調。「しっ、静かに。」 | 大顎を見張っている【確定】 |
| 負傷者 | `CampPeopleWounded` | 弱々しい。「ごめんなさい、うまく立てなくて。」 | 大顎に襲われた生き残り【確定】 |
| 古竜 | `FirstEventDragon` | 喋らない | 最後に生き残った竜。仲間の心臓を島々から取り戻し、巣で仲間を蘇らせようとしている【確定】。名前は「古竜」【確定】（2026-09-27） |
| 隊商頭 | `CaravanMaster` | 豪快な商売人。「うちの隊商」「〜だぜ」「ありがてえ」寄り | 砂漠のオアシスで足止めされた隊商の頭。サソリ退治の依頼主【確定】 |
| 水守りの娘 | `CaravanKeeper` | 丁寧で心配性。「私」。「〜です」 | 細っていく泉を守っている。サソリの尾に気をつけろと教える【確定】 |
| 駱駝番の少年 | `CaravanBoy` | 元気な子供。「おれ」。「すげえ！」 | 砂の下の魔物（ワーム）と、東の竜の骨のことを教える【確定】 |
| 隊商の護衛 | `CaravanGuard` | 無骨。「自分」。息が上がっている | 荷車を追って城塞の手前で動けなくなっている。城塞の割れ目を教える【確定】 |
| 骸竜 | `SkeletonDragon`（ボス） | 喋らない | クノイチの故郷の島を浮かせていた竜の骨に、落ちてきた光の心臓（別の竜の心臓）が宿って起き上がったもの。城塞の神殿前の広場に居着く【確定】 |

## 4. 章立て

### 序章 — 襲来（`FirstTouchDownMainIsLandScene`）【確定・大部分は実装済み】

序章でいきなり戦うので、戦う前に **島は浮いている / 浮かせているのは島の底の「島の心臓」/ ハンターは狩り場で魔物を狩って稼ぐ**
の3つだけは伝える【確定】（2026-09-25）。竜が何者か・なぜ心臓を狙うかは伏せる（クノイチが知っていそう、で引っぱる）。
ナレーションは使わず、次の3か所で見せる。

1. 主人公が飛行船で拠点の島に着く。船上でクノイチと青年に会う。（実装済み）
   冒頭は船と島々を外から映すカット（引き+タイトルロゴ → 船腹 → 船首の先の拠点の島）のあと、甲板で船首楼の方へ歩いて伸びをし、
   操作が返ってから船が島へ動き出す（`AboardAirShipMovie`。カットはシーンの `AbordAirShipMovie/OpeningShots` の子で、
   各カメラの子 `End` へ動く。秒数はコンテキストの `openingShotDurations_secs_`。配置は `python tools/art/prologue_opening.py`）。
   青年が窓の外の島を見て、「島の心臓」を話す（`AirShipYoungMan`）。クノイチは「島の心臓の光は、遠くからでも見える。
   ……見えすぎるくらいにね。」と匂わせる（`AirShipKunoichi`）。（台詞は実装済み）
2. 教官の訓練を受ける（走る・跳ぶ・斬る・転がって躱す）。（実装済み）
   教官の自己紹介で、ハンターとは何かを1行で言う（`Introduction ActionInstructure`）。（台詞は実装済み）
   【確定・未実装】島の中心（竜が爪を立てる所。`IslandHeartStones` の真上）に祠を置き、地面の割れ目から緑と金の光を漏らす。
   訓練への道すじをその前に通し、教官に「あの下に島の心臓が眠っている」と言わせる。5. で竜がそこを狙うので、
   「島の心臓が狙いか！」が伝わる。置き場所と道すじはエディタで見て決める。
3. ドラゴンが現れる。教官が「島の者を逃がす時間がいる」と言い、主人公が足止めする。（実装済み）
   着地の前に、竜が島を回り込んで乗ってきた飛行船を撃ち落とす（2026-09-29。`FirstEventDragon` の BT の State0、"Ship ..." ノード）:
   船の正面の空で止まる → 操作ロック・`ShootDownAirShip`（`Prop::AirShip::OnShootDown`: 乗客の NPC を消し、甲板のプレイヤーを
   桟橋の `AirShipEvacuatePoint` へ移す）→ `AirShip Attack Camera`（島の上から船越しに竜を LookAt）→ 咆哮 → 火球がマストに着弾
   （爆発・船に付く炎と黒煙・揺れ・雷）→ `AirShip Fall Camera`（南西から船を横に見る）で、船が船首から傾いて雲の下へ落ちる
   （`FallIsland`）→ 竜が落ちる船の横を急降下し、桟橋の下から上がって着地する。船は雲の下へ消える（残骸は置かない。
   青年の「墜ちた船を直したい」は造船所と一緒に決める）。配置・ルート・BT は `python tools/art/airship_shootdown.py` で組み直す。
   音 (2026-09-29。BT の "Omen ..." ノード、`Sound::FadeBGM`): 竜が呼ばれたら到着時の BGM を下げて嵐の音だけにする → 雲の向こうの
   遠吠え → 島の警鐘（教官の「鐘を鳴らせ！」と対。`ActionInstructure Appear Dragon`）→ 不穏な BGM `Omen_DragonApproach` が上がる →
   火球が船に当たった瞬間に切り、急降下で戻し、着地の前に下げる → 静けさの中で着地 → 咆哮と同時に戦闘 BGM。
   音は `python tools/art/dragon_omen_sfx.py`、BT は `python tools/art/dragon_omen_bt.py`。
4. ドラゴンが空へ上がって島を壊し始める。山頂の大砲で撃ち落とす。（実装済み。BT の最後は `ChangeToMainIslandScene`）
   火球は噴水の島 → 家の島 → 拠点の島の順に落ち、着弾点で `IslandFireImpact`（爆発）・`IslandBurning`（燃え続ける炎と黒煙）・
   `IslandCrumble`（島の底が崩れ落ちる）を出す（BT の "DestroyIslandState 2..4"。`tools/art/island_destruction_effects.py`）。
   教官の台詞は2つに分けてある。火球の前に「奴が空へ上がった！」（`FirstDragon Attack Crisis`）。
   拠点の島に着弾した後に「島がもたん」「大砲で撃ち落とせ」（`FirstDragon Cannon Order`）を流し、そのまま操作を返す。
5. 墜ちたドラゴンが島の中心に爪を突き立て、**2つの心臓が地面から抜け出して**二方の狩り場へ飛んでいく。
   島がぐらりと傾いて暗転する。
   → 実装済み（`FirstEventDragon` の BT の "Heart ..." ノード）: シーンの `Heart Dive Camera`（LookAt でドラゴンを追う）→
   教官の叫び（`StartChat` で待たずに流す。降下・爪・心臓が砕ける瞬間に3行が重なる）→ `ToIslandHeart` のルートで拠点の島の中心へ降下 →
   爪（`Attack1`。ここで島全体を上から映す `LookDestroyIslandCamera` に切り替え、地割れを見せる）→ `IslandHeartBreak`（光の柱・地割れ・突き出す岩）と島の縁の崩落 → `HeartShardScatter`（閃光と衝撃の輪。
   `tools/art/heart_shatter_effect.py`。名前は昔の「欠片が散る」設定のまま）と同時に、緑・金の2つの心臓
   （シーンの `IslandHeartStones` の子。心臓の真下の地中に埋めてある。モデルは `Assets/Art/Models/IslandHeart/`）が
   地面を割ってせり上がり、少し浮いてから、それぞれの色の光の尾（`Green/LightStoneFlight`）を引いて二方の空へ飛んでいく
   （BT アクション `Story::ScatterFloatingStones`。遠景のカメラでも見えるよう、石は高さ約 80）→ 揺れ → 島の底が崩れ落ちる →
   同じカメラのまま、噴水の島（`SecondIsland`）と家の島（`ThirdIsland`）が橋ごと傾いて雲の下へ落ちていく
   （BT アクション `FallIsland`。燃えていた炎と煙は `AttachParticle` で島に付いたまま一緒に落ちる）→ `Heart Dive Camera` のまま巣へ帰るのを見送る。
   NOTE: Sequence は毎フレーム子0から Tick し直すので、待ち（`WaitSeconds` / ルート / 会話）の手前に置く生成・再生・カメラ切替は
   1つずつ `OnceExecute` で包む（包まないと待っている間ずっと毎フレーム出る）。
   この2つの島と3本の橋は、`MainIslandScene` では隠してある（落ちた設定）。噴水の島とその階段だけは草原の後に戻ってくる（第1章）。
   火の心臓（`FireStone`）はシーンから外した。モデル `FireCoreShard.mv1` と `FireStoneFlight` のエフェクトは使われずに残っている。

序章を抜けると `StoryFlag::PrologueCleared` が立つ（実装済み）。

### 第1章 — 残された島（`MainIslandScene`）【確定】

- 島は半壊していて、少しずつ沈みつつある（瓦礫、傾いた建物、消えた灯り）。
- 教官「俺はもう前には出られん。だが、島を立て直す段取りなら付けられる。」
  → 復興ボードが使えるようになる（`StoryFlag::RestorationStarted`）。
- 教官の説明: 島を浮かせ続けるには心臓を戻すしかない。石は近くの島の石に引かれて、よその島へ落ちた。
  むき出しの石は獣も魔物も引き寄せるので、石の周りは群れだらけで、強い奴ほど傍に居座る。まずは一番近い草原の島から。

### 第1章 — 草原：大顎と狩人の一族（`GrassLandScene`）【確定】

- 草原の島は、狩人の一族が住む野生の大きな島（地上ではない。2026-09-25）。
- 長老は一族の言い伝えとして「島の心臓は、昔は本当に竜の心臓だったと言う」と話す【確定】。大顎を倒して石を渡すときに話す（`CampPeopleElder_Cleared`）。
  プレイヤーにはただの昔話に聞こえるように、さらっと言わせる。

- 盆地の村が **大顎**（`Tyrannosaurus`）に潰され、狩人の一族は山の棚の野営地に逃げている。
  → 実装済み（道しるべ）: スポーン地点は野営地の方（東）を向いていて、分かれ道の立て札（赤い布の板 = 野営地、焼けて爪痕の走る板 = 村の跡）から
  北の壁沿いの坂を上る道に、一族の赤い旗竿・石積み・踏み石が続く。焚き火には煙の柱（`CampfireSmoke`）が立つ。野営地から盆地へ
  下りる坂にも旗と石積みがある。赤い布が一族の目印。配置は `python tools/art/grassland_trail.py`（シーンの `Wayfinding`）。
- 大顎が「村の跡から一歩も動かない」のは、**緑の心臓が村の跡に落ちていて、その力に惹かれているから**。
  石が落ちてから獣が村へ寄ってくるようになり、最後に来た大顎が村を潰した。
- 西の林のハイエナの群れ（遠吠えで仲間を呼ぶ）も、石に惹かれて集まってきた。群れを減らし、大顎を倒す。
- クリアすると **緑の心臓** を取り戻す（`StoryFlag::GrassLandCleared`）。
  長老「島を追われる辛さは、わしらが一番知っておる。」一族の何人かが拠点の島へ移り住む。
  石が去ったので、盆地に集まっていた獣も散っていく。
  → 実装済み: 村の跡（柱の輪の中）に着弾跡のクレーターと石（`Assets/Prefab/Prop/Story/GreenFloatingStone.prefab`、
  オーラは `GreenCoreAura`）。大顎を倒すと石が震え、`GreenStoneLiftOff` を出して抜け出し、光の尾（`GreenStoneFlight`）を
  引いて空へ飛び去る（石に付けた `GamePlay::Prop::FloatingStone` の `PlayDepartAsync`。カメラはシーンの `GreenStoneCamera`）。クリア後に来るとクレーターだけ。
- 島の変化: 緑の心臓を島の底へ戻すと島の沈下が止まる。埋め戻した石はもう獣を呼ばない。
  そして石の力で、序章で落ちた **噴水の島**（`SecondIsland`）が雲の下から浮かび上がってきて、拠点の島から上る
  **階段**（`To SecondIsland Bridge`）がひとりでに架かる【確定】。狩人の一族（女狩人・長老・見張り・負傷者）は噴水の島に住む【確定】。
  → 実装済み（石が戻るところまで。沈下が止まるのは見た目に出していない）: 草原クリア後に初めて拠点の島へ戻ると、石が遠くから飛んできて
  島の底の先端にはまる（`PlayReturnAsync`、`GreenStoneDock`）→ `StoryFlag::GreenStoneReturned`。以後は底に付いたまま。
  カメラ・パーティクル・尺は石の `FloatingStone` コンポーネントが持ち、シーンのコンテキストはそれを参照するだけ
  （`GrassLandSceneContext::floatingStone_` / `MainIslandSceneContext::greenStone_`）。
  エフェクトは `python tools/art/green_core_effect.py`。
  → 実装済み: 石がはまったあと、同じ入場で続けて、噴水の島が傾いたまま雲の下からせり上がって水平に戻り、
  階段が手前から1段ずつ下から跳ね上がって架かる（島に付けた `GamePlay::Prop::ReturningIsland` の `PlayReturnAsync`。カメラはシーンの
  `FountainIslandCamera` で、噴水（`MedievalFountainEmpty`）を LookAt で追う）→ `StoryFlag::FountainIslandReturned`。
  以後は最初から出ている。演出の途中でシーンを抜けたら、石と同じくその場で戻ったことにする（始まる前に抜けたら、次に来たときに流す）。
- 噴水の島が戻ると、掲示板の「復興」で **一族の家**（`Facility::ClanHouse`）を建てられる。建てると女狩人が家の前に立ち、
  「私たちの一族に、島の復興を手伝いたいという者たちがいる」と言って **仲間（キャラ選択）を選ばせてくれる**【確定】。
  それまでキャラ選択はできない（仲介人は「今は紹介できる奴がいない」）。

### 第2章 — 砂漠：干上がるオアシスと隊商（`DrySandScene` / `DesertScene.scene`）【確定】（2026-09-24 ユーザー決定）

- 砂漠の外れのオアシスに、行商の **隊商** が足止めされている。数日前、金色に光る石が砂漠の北の
  **砂に沈んだ城塞** に落ちてから、泉が日に日に細っている（石の熱が地下の水脈を干上がらせている）。
- 石に惹かれて **大サソリ**（`DesertScorpion`）の群れが砂漠に集まり、泉の水場にまで出る。西の砂丘には **サンドワーム**（`SandWorm`）が潜む。
- 砂の島も空に浮かぶ島（地上ではない。2026-09-25）。
- 城塞は、この砂の島に **墜ちてきた別の島** の成れの果てで、**クノイチの故郷**。古竜に心臓を奪われて落ちた。
  東の尾根に、落ちたときに割れた島の底から出てきた **竜の骨** がある。その島を浮かせていた竜の亡骸で、胸にあった心臓が、その島の心臓だった。
  両翼だけが残り、頭と胸の骨が無く、砂の跡が城塞へ続いている。
- ボスは **骸竜**（`SkeletonDragon`）= 城塞に落ちた光の心臓（別の竜の心臓）が骨を引き寄せ、宿って起き上がったもの。
  城塞の神殿前の広場で、石の傍に居着く。**心臓を骨に戻すと竜は起き上がる** ことを、プレイヤーはここで見る（終章の伏線）。
- 骸竜を倒した後、クノイチが真相の一部を話す: 島の心臓は竜の心臓だったこと、古竜は仲間の心臓を取り戻して回っていること。
  竜狩りのことは、まだ明かさない。
  倒すと骨は眠り、**光の心臓** を取り戻す（`StoryFlag::DesertCleared`）。石が去ると泉が戻り、サソリも散る。
- 2つの心臓が島に戻ると、島が自力で飛べるようになる。そのまま終章へ【確定】（演出は【仮】: 下の「終章の流れ」）。
  そろった心臓は、古竜の巣に積まれた仲間の心臓に引かれる（心臓が仲間のもとへ帰ろうとする）【確定】。
- **鍛冶場（武器の強化）は作らない**【確定】。実装が重すぎるため。

サブ目標（草原の「一族を助ける」に当たるもの）:

| 目標 | 中身 | 仕組み | 状態 |
| --- | --- | --- | --- |
| 本筋「砂漠の骸竜」 | 城塞の広場の骸竜を倒す。依頼主は教官。緑の心臓が島に戻ってから（`GreenStoneReturned`）貼られる | `DefeatMainStoryQuest`（`DesertSkeletonDragon.boardQuest`、報酬 5000 は仮） | 実装済み |
| 水場のサソリ退治 | オアシスの大サソリを6匹。依頼主は隊商頭。繰り返し受けられる稼ぎ口 | `DefeatRequestQuest`（`DesertScorpionCull.boardQuest`、報酬 600 は仮） | 実装済み |
| はぐれた護衛 | 城塞の東の割れ目の外で座り込んでいる護衛と話すと、城塞への抜け道を教え、`DesertGuardRescued` が立つ | BT `CaravanGuard` | 実装済み（台詞だけ。立てたフラグで変わるのは護衛の台詞だけ） |
| 竜の骨の前のクノイチ | 故郷の島と竜の骨の話。骸竜を倒した後に真相の一部を話す | BT `DesertKunoichi` | 実装済み |
| 砂に埋もれた荷 | 砂丘に散った荷（飛行船の部品）を拾う → 青年に届く → 造船所 | `CollectRequestQuest`（`QuestType::DesertLostCargo`） | 【未実装】荷のアイテムと置き場所、造船所の施設が要る。壊れた荷車だけ砂丘に3台置いてある |

案（決定ではない）: 荷の中身は飛行船の部品で、青年が造船所を任される。

### 終章 — 嵐の巣【骨格のみ確定】（2026-09-25 書き直し）

- **なぜ巣へ向かうか【確定】（2026-09-25）**: 2つの心臓が戻った瞬間から、島は巣に積まれた仲間の心臓に引かれて
  **勝手に流されはじめ、止められない**。逃げ場は無いので、島ごと乗り込んで決着をつけるしかない
  （序章の「島が沈む」と対になる「島が引きずられていく」危機）。
- 引かれるまま、嵐（既存の `SetStorm` / `Lightning`）を抜けて古竜の巣へ向かう。
- 序章の山頂の大砲（昔の竜撃ちの砲）で、島の仲間が援護射撃する（序章と対になる場面）。
- 巣は **竜の墓場**。奪われた心臓が仲間の骨に戻され、起き上がりかけた骸がいくつもある。
  ここで、ハンターがもとは竜狩りで、拠点の島がその本拠だったと分かる（誰が明かすかは【未設計】）。
- クノイチと共闘して古竜を倒す。
- 結末【確定】（2026-09-25）: 古竜が倒れると、巣に積まれていた大量の心臓が空へ散り散りに飛んでいく。
  散った心臓は、それぞれ雲海に沈んでいた島（落とされた島や、昔の大地のかけら）を引き上げ、**空に大量の島が浮かび上がる**
  （「戻った心臓は沈んだ島を引き上げる」の、いちばん大きな形）。
  - 【仮】古竜の心臓も、どれかの島の心臓になり、どれかの島を浮かせる。クノイチの故郷（砂の島に墜ちた城塞）も浮かび上がるか。
  - 人は今度こそ、竜の犠牲の上に暮らしていることを忘れずに受け継ぐ。
- エピローグ: ほかの島から人が集まり始める。教官は「駆け出しの島」を再開する。
  ハンターの新しい仕事は、浮かび上がった島々へ渡って拓くこと（→ §4.1 運営）。

ステージの作り方は専用シーン（古竜の巣）【仮】。骸たちは戦わせない【仮】。

#### 終章の流れ【仮】（2026-09-27。「一旦全部決めて実装してよい」とのユーザー指示で決めた。実装済み・要確認）

1. **光の心臓が戻る**: 砂漠のクリア後に拠点の島へ戻ると、緑の心臓と同じ演出で光の心臓が飛んできて、島の底の先端の緑の心臓の隣に
   はまる（`StoryFlag::LightStoneReturned`。シーンの `IslandHeart/LightFloatingStone`、`MainIslandScene::ApplyStageRewards`）。
2. **教官に驚きアイコン**（`DesertCleared` かつ `NestVoyageStarted` がまだ）。話すと `Instructor_DesertReport`:
   島が揺れ続けている → 北へ流されていて止まらない → 北の空には古竜が帰っていった嵐がある → 心臓が仲間の心臓に引かれている、
   あの嵐の奥が古竜の巣 → 逃げ場は無いので島ごと乗り込む → 山頂の大砲（昔の竜撃ちの砲）は教官たちが受け持つ → 支度ができたら声をかけろ。
   → `NestVoyageStarted` を立てる。
3. **出発**: その後は話しかけるたびに `Instructor_ToNest`（「行くぞ、何かに掴まっていろ！」）→ BT アクション `Story::DepartForNest` →
   `MainIslandScene::BeginNestDeparture`: 島の南の外から島と北の空を映すカメラ（`NestDepartureCamera`）に切り替わり、地鳴りとともに
   揺れがだんだん強くなって（6 秒）、古竜の巣（`SceneType::DragonNest`）へ移る。巣へは島ごと着いた扱いで、見た目はポータルの到着のまま（【未対応】）。
4. **古竜戦**（`AncientDragon`、`EnemyKind::AncientDragon` = 7。体力 2400、名前「古竜」。序章のドラゴンの姿と戦い方）:
   - 心臓の山の上で羽ばたいて待つ。プレイヤーが坂を下りて 420 まで近づくと登場: 竜のカメラ → クノイチの声（`AncientDragon_Intro`。
     追いついた、骸の胸に心臓が戻されている＝仲間を起こすつもり、上から援護する）→ 嵐が強まり、雷とともに降り立って咆哮 → ボス曲。
   - **大砲の援護**: 体力が 60% と 25% を切ると、教官の声（`AncientDragon_Cannon1/2`「大砲、撃てえ！」）と一緒に山頂の大砲の弾が
     古竜に降り注ぎ（序章の火球の着弾 `IslandFireImpact`）、古竜がのけぞって 3 秒ほど攻め込める（序章と対になる場面）。
   - クノイチは声だけの共闘（一緒に戦う NPC は【未対応】）。
   - 最期: 止まって最後の咆哮 → クノイチの声（`AncientDragon_Defeat`）→ 嵐が晴れ、光になって消える。
5. **結末**: 古竜が倒れると `AncientDragonDefeated` が立ち、心臓の山を映すカメラ（`NestEndingCamera`）で、巣に積まれていた心臓
   （`Nest/Hearts` と `Nest/Floating` の心臓）が次々に浮き上がり、それぞれの色の光の尾を引いて四方の空へ飛んでいく
   （`DragonNestScene` の `PlayHeartScatterAsync`。任意のボタンで飛ばせる）→ 拠点の島へ帰る。
6. **エピローグ**: 教官に驚きアイコン（`AncientDragonDefeated` かつ `EpilogueHeard` がまだ）。`Instructor_Epilogue`:
   雲の下から島がいくつも浮かび上がってきている → 散った心臓が沈んでいた島を引き上げた → 巣に刺さっていた銛は俺たちハンターの銛だ
   → 昔のハンターは竜を狩る者で、島の心臓はその竜から奪ったもの → この島は竜狩りの本拠で、大砲が竜撃ちの砲なのもそのせい
   → 竜の犠牲の上に暮らしていることを今度こそ忘れずに伝えていく → 浮かび上がった島々は新しい狩り場、この島も駆け出しの島に戻る。
   → `EpilogueHeard`。以後は `Instructor_AfterEnding`（新しい島々への渡り方は探らせている）。
   **竜狩りの真相を明かすのは教官**【仮】。浮かび上がった島々を見せる演出は無い（台詞だけ。【未対応】）。

作り直すときは（どれもアセットの GUID は保つ）:

```
python tools/art/story_npcs.py --only island --only nest   # 教官の会話と BT、古竜の BT が流す会話
python tools/art/main_island_finale.py                      # 拠点の島の光の心臓・NestDepartureCamera と MainIslandSceneContext (版 6)
python tools/art/nest_heightgrid_bake.py                    # 古竜の経路探索用の Nest.heightGridMap
python tools/art/ancient_dragon.py                          # 古竜の BT・プレハブ・EnemyFactory・巣の湧き地点と NestEndingCamera
python tools/art/nest_context.py                            # DragonNestSceneContext (クリア条件と結末の参照。版 1)
```

- 古竜の BT は序章のドラゴンの BT の写し（戦いの枝はそのまま、序章だけの枝を巣の枝に替える）。台の数値・位置は
  `ancient_dragon.py` の先頭（`APPROACH_DISTANCE` / `CANNON_RATES` / `SPAWN_POS` など）。
- `nest_scene.py` で巣を作り直したら、`ancient_dragon.py place` → `nest_context.py` を必ず流す。
- 要確認（ビルドとエディタで）: 古竜が降り立つ高さと向き、竜のカメラ（序章のプレハブの子のカメラを流用）の画、大砲の弾の位置、
  結末のカメラの画と心臓の飛び方、出発のカメラの向き、光の心臓の置き場所。

#### 巣のシーン【仮】（2026-09-27。見た目と入場だけ。入口はデバッグメニューのみ）

`Assets/Scene/DragonNestScene.scene`。巣そのものの見た目を、上の骨格から読み取って作った。

- **嵐の目に浮かぶ黒い岩の島**。周りを嵐の壁（雲の渦）がゆっくり回り、見上げると雲の天井に開いた目の奥にだけ空が見える。
  島の底は逆さの円錐の岩で、雲の渦の底へ垂れている。
- 外輪には **岩の爪** が内へ反って並び、島そのものが巣を抱える爪か肋骨に見える。南の切れ目が着き場からの坂道で、
  坂の上では大きな爪が左右から覆いかぶさって門になる。着き場（南の縁の岩棚）はいまはポータルで着く所で、
  拠点の島が横付けする場所にする想定（島ごと乗り込むのは未設計）。坂の下に巨大な竜の頭骨。
- 巣の底の中央に **心臓の山**。拠点の島と同じ緑・光の結晶に、よその島から奪った火・潮（青）・宵（紫）の心臓が積まれている。
  「心臓は引き合い、ものを引き寄せ、持ち上げる」ので、山の上では岩と、心臓ごと持ち去られた島の瓦礫、心臓のいくつかが宙に浮いて漂う。
- 山を囲んで **心臓を胸に戻された竜の骸** が5体、山の方を向いて座り込む（起き上がりかけ。北の1体がいちばん起きていて、
  西と東は半分埋もれている）。心臓の無い骨（`DragonBones`）は伏せたまま。
- 骸の周りと坂道の脇には **古い竜撃ちの銛** が刺さったまま残る。ハンターがもとは竜狩りだったことの物証
  （誰が明かすかは【未設計】のまま。銛をきっかけに話させられる）。
- 敵の湧き地点は古竜 1 体（`AncientDragon`、`AncientDragonDefeated` の後は湧かない）。戦いは上の「終章の流れ」。島が着く演出はまだ無い。
- `SceneType::DragonNest`（5）/ `DragonNestScene` / `DragonNestSceneContext`（GameManage.scene）で、ほかの狩り場と同じく入れる
  （部屋のキーは `"DragonNest"`。到着はポータルの演出、BGM は砂漠のまま、クリア条件と空撮は無し）。
  入口は教官（砂漠の後、`Story::DepartForNest`）と DebugSheet（F1）の Stage Entry / Scene。ステージ選択の欄・ロード画面の航路（`routes_`）は無い（航路が無いと「移動中…」）。
- 嵐は序章の `WeatherService` 一式（空のドームの下・閃光・突風・雷の音）を `nest_scene.py` が写して置き、
  `initialStormIntensity_` 0.6 で始まる（遠雷が鳴る）。嵐の壁の回転も `wallRotator_` で嵐の強さに連動する。
  古竜の BT が登場で `SetStorm` 0.85 → 1.0 へ上げ、倒れるときに序章と同じ "Storm Clear" で下げる。雲の壁と天井は `ModelRenderer` なので嵐で暗くならない（【未対応】）。

作り直すときは上から順に流す（アセットの GUID は保つ。シーンの GameObject の GUID は毎回新しい）:

```
sh <work>/rebuild_all.sh      # nest_terrain.py (高さ data/nest_terrain.npz とテクスチャ) -> Blender で地形・岩の爪・嵐の壁・銛・骨・頭骨
                              # -> .mv1 (Assets/Art/Models/Nest)。名前を並べるとそのモデルの変換だけ
python tools/art/nest_prefabs.py                         # Assets/Prefab/Prop/Nest
python tools/art/nest_scene.py                           # DragonNestScene.scene (草原のシーンの複製が土台)
python tools/art/nest_context.py                         # GameManage.scene の DragonNestSceneContext (シーンの GUID を指し直す)
```

`<work>` は `%USERPROFILE%\NanamiAssetsWork\Nest`（`rebuild_all.sh` と `process_nest.py`、元の `.glb` がある）。
配置の意図と座標は `nest_terrain.py` の先頭、骸・心臓・銛の並びは `nest_scene.py` の `SKELETONS` / `HEART_COLORS` など。
潮・宵の心臓（`Nest/TideCoreShard` / `DuskCoreShard`）は `FireCoreShard.mv1` のテクスチャ名を差し替え、色相を回した画像にしたもの。

### 4.1 運営：浮かび上がった島々【骨格のみ確定】（2026-09-25）

このゲームは運営型。本編（終章）の後は、結末で浮かび上がった島々へ渡る **イベント** で遊び続ける。

- 本編の結末が、そのまま運営の入口になる。島はいくらでも増やせる（どの島も「散った心臓が引き上げた島」で説明が付く）。
- イベントの島ごとに、狩り場・ボス・住人・依頼を足す（本編の草原の島・砂の島と同じ作り）。
  - 落とされていた島: 昔、古竜に心臓を奪われて沈んだ島。住人の生き残りや、その島の物語がある。
  - 昔の大地のかけら: 雲に呑まれる前の大地。竜狩りの時代の遺跡、見たことのない魔物。世界の謎（大地が雲に呑まれた理由）を少しずつ明かす場所。
- 拠点の島は、島々へ渡る船着き場・ハブになる（転移の台座・飛行船）。
- 毎週のイベント【確定】（2026-09-29）: 金曜 12:00〜翌金曜 04:59。報酬は **島の飾り**（手元の PC にだけ保存、
  マルチプレイでは共有しない）とお金。**本編の進み具合に関係なく誰でも参加できる**。型のローテーションと作り方は `docs/LiveOps.md`。
  - 最初の4週（10/09〜11/06）【仮】（2026-10-02）: 草原だけで回す（砂漠は物語で解放されるため）。群狼討伐（酒場の仲介人）→
    薬草あつめ（商人）→ 怒れる大顎（見張り。期間限定のイベント用ステージで戦い、本編の大顎は変えない）→ 群狼討伐 第2回。依頼文・飾りは `tools/art/weekly_events.py` の `WEEKS` が正。
- 島ごとの大型イベント（新しい島を足す）の期間・報酬は【未設計】。

## 5. 施設（`GameCore::Story::Facility`）

お金を払って建てる（掲示板の「復興」。§7）。建てると `RestorationGate` が島の見た目を切り替える。

| ID | 名前 | 条件 | 建てると | 状態 |
| --- | --- | --- | --- | --- |
| `ClanHouse` | 一族の家（2000） | `FountainIslandReturned` | 噴水の島に女狩人が住み、キャラ選択ができるようになる | 【確定】 |

仮置きだった4つ（船着き場・雑貨屋の修繕・狩人小屋・畑）は、データ・prefab・シーンの建つ場所ごと削除した（2026-09-24）。
`Facility` の enum の `Dock`..`Field` は中身の無い番号で、再利用しない（エディタの選択肢 `FACILITIES` からは外した）。新しい施設は末尾に足す。

- 草原のご褒美は、噴水の島と階段が戻ってくること（§4 第1章）。一族の家はその島に建てる。
- 造船所・灯台・山頂の大砲 などは砂漠と一緒に決める。
- 見た目だけ変わる施設（家屋・噴水など）を混ぜると、お金の使い道が途切れにくい。

## 6. 報酬の流れ【確定】

- **狩り場の初回クリア**: 心臓と、まとまった復興資金（ストーリークエスト = `MainStoryQuestBase`）。
  草原は掲示板の「草原の大顎」（依頼主: 教官、`GrassLandTyrant.boardQuest`、`RestorationStarted` で解放）。中身は
  `DefeatMainStoryQuest`（`enemyKind_` を1体倒したら達成。受注前に `clearedFlag_` が立っていれば受注した時点で達成）。
  掲示板の依頼は今これ1件だけ。報酬 3000 は仮。
- **掲示板の依頼**: 繰り返し受けられる稼ぎ口（`RequestQuestBase`）。依頼主と文面はその土地の人に合わせる。
- **お金の使い道**: 施設の復興と、店での消耗品。武器の強化（鍛冶場）はやらない。

## 7. コード

### StoryProgress（実装済み）

`Assets/Scripts/Core/Game/Story/`

| ファイル | 中身 |
| --- | --- |
| `Story_StoryFlag.h` | `enum class StoryFlag`（物語の節目）と `ToString` / `STORY_FLAGS` |
| `Story_Facility.h` | `enum class Facility`（直せる施設）と `ToString` / `FACILITIES` |
| `Story_StoryProgress.h/.cpp` | シングルトン `StoryProgress` |

```cpp
auto& story = GameCore::Story::StoryProgress::Instance();
story.IsSet(StoryFlag::GrassLandCleared);
story.Set(StoryFlag::GrassLandCleared);      // 初めてなら true。その場で保存する
story.IsRestored(Facility::Dock);
story.Restore(Facility::Dock);               // 初めてなら true。その場で保存する
story.OnChanged().Subscribe(...).AddTo(this); // フラグか施設が変わると流れる
```

- 保存先は `LocalPrefs/GameProgression/StoryProgress.json`。職業をまたいで1つ。
- 保存がまだ無く、古い `GameProgresion` が序章より先なら、`PrologueCleared` を立てて引き継ぐ。
- デバッグ用: Game コンポーネントの Inspector と、ツールバー > LocalPrefs の両方にチェックボックスがある。
  Inspector 側で切り替えると、その場で保存され、`RestorationGate` の見た目もすぐ変わる。
- **enum の値は必ず末尾に足す**（セーブとシーンに int で残る）。足したら `ToString` と配列にも足す。
- `GameProgresion`（`MainProgression.h`）はタイトルからの開始シーン選びにまだ使っている。いずれ `StoryProgress` に寄せる。

### 狩り場に着いたときの空撮（実装済み）

草原・砂漠に **初めて** 着いたときだけ、ポータルの演出の前に島を見せる（`StageArrivalMovie::PlayOverviewAsync`）。
島を見下ろして島の名前（「草 原 の 島」）→ 見どころを1か所ずつ切り替えで映して地名と一言 → 最後の見どころからポータルへ降りる。
何かキーを押すとスキップ（ポータルも飛ばして操作を返す）。見たら（スキップしても）`GrassLandOverviewSeen` / `DesertOverviewSeen` /
`DragonNestOverviewSeen` が立ち、2回目からはポータルの演出だけ。途中でシーンを抜けたら次にもう一度流す。

- ショットと字幕の文言はシーンのコンテキスト（GameManage.scene の `arrivalOverview*` / `arrivalIslandTitle_` / `arrivalTourShots_`）。
  見どころはポータルに近いものを最後に並べる（そこから降りる）。巣は空撮を作っていない（`arrivalOverview_msecs_` 0）。
- 地名と一言はこの文書の正史から取った（草原: 村の跡 / 野営地 / 西の林、砂漠: 城塞 / 竜の骨 / オアシス）。
- 字幕は `GamePlay::Ui::StageArrivalCaption`（`Assets/Prefab/UI/StageArrival/StageArrivalCaption.prefab`、
  `python tools/art/stage_arrival_caption.py --emit` → `stage_arrival_caption_prefab.py`）。

### RestorationGate（実装済み）

`Assets/Scripts/GamePlay/Prop/RestorationGate/Prop_RestorationGate.h/.cpp`。Add Component > Prop にある。

- `facility_` が直っていれば `restoredObject_` を、直っていなければ `brokenObject_` を有効にする。
  `StoryProgress` が変わると、その場で切り替わる。
- 置き方: 空の親 GameObject に `RestorationGate` を付け、その子に「建った姿」（`restoredObject_`）を置いて参照させる。
  施設は壊れた物を直すのではなく**新しく建てる**ので、建てる前は何も置かない（`brokenObject_` は空でよい）。
  **切り替える子の GameObject 自身には付けない**（自分が無効化されると購読が止まる）。

### 復興ボード（実装済み）

拠点の掲示板（`EventNoticeBoard`）の4枚目の木札「復 興」。左に普請の札、右下に小さな見積の札（費用・所持金・残り）。
この頁だけは暗幕を敷かない。施設を選ぶと、その施設の `RestorationGate` が **下見**をする: カメラが島のその場所へ寄り
（`previewCamera_` の優先度を 100 に上げる）、建てる前でも建った姿を出す。頁を離れるか閉じると元に戻る。
A でその場でお金を払い、`StoryProgress::Restore` する（所持金は `SaveStatus` で保存）。足りなければ断りの音だけ鳴る。

- 施設ごとのデータは `Assets/Data/Restoration/*.restorationFacility`（`Asset::RestorationFacility`: 施設・名前・説明・費用・
  前提の `requiredStoryFlag_` / `requiredFacility_`（-1 = なし）・前提の文言）。今は `ClanHouse` だけ。
  掲示板に貼る並びは `MainIslandEventBoard.eventBoard` の `facilities_`。
  **正は `tools/art/restoration_facilities.py` の `FACILITIES`**。直して再実行すると、アセットと `facilities_` を書き直す（GUID は保つ）。
- 建つ場所と姿は MainIslandScene の `RestorationSite_<施設>`（`Assets/Prefab/Prop/Restoration/`）。root に `RestorationGate`、
  子に Restored / PreviewCamera（→ PreviewTarget を見る）。建てる前は空き地。**正は `tools/art/restoration_sites.py` の `SITES`**
  （置き場所・カメラの向き・借りているモデル）。再実行すると prefab を組み直し、シーンの `RestorationSite_*` を置き直す。
  建った姿のモデルは仮（Settlement の prefab を借りている）。
  建った姿は最初はコンポーネントを切って隠してある（エディタでは空き地に見える。`isActive_` は読み込みで効かないため）。
- 掲示板は `RestorationGate::Find(facility)`（シーンにある門の一覧）で門を探す。門が無い施設と、前提がまだ（未開）の施設は下見しない
  （噴水の島が戻る前に一族の家を下見すると、島の無い空に家が浮くため）。
- 一族の家: 建つ場所は `RestorationSite_ClanHouse`（噴水の島の芝生 (-205, 87, 432)、入口は階段の方 yaw -20°。`restoration_sites.py` の
  `CLAN_HOUSE_POS` / `CLAN_HOUSE_YAW`）。建った姿は `ClanHouse.prefab`（`python tools/art/clan_house.py`）: 草原の野営地と同じ
  Settlement の獣皮のテント・焚き火・干し棚・トーテムと、女狩人 `ClanHuntress`。NPC は Dynamic の RigidBody なので、シーンに置いて
  隠すと島が戻る前に落ちていく。だから建てたときに生成する `restoredPrefab_` に入れてある。
  キャラ選択の展示台 `CharacterPodium` は、MainIslandScene の `FirstIsland` の子のまま、家の前 (家の local (0, 0, -40)) に移した。
  見えるのは選んでいる間だけで、カメラは家の方を向く。女狩人の BT は展示台を GUID で指すが、prefab から生成した NPC でも
  `OpenCharacterSelect` はシーンの展示台を探し直すので動く。
  置き場所・下見と展示台のカメラ・芝生の高さは、AutoMCP で見て決めた（2026-09-24）。
- 教官から任される（`RestorationStarted`）までは空の頁で「まだ普請の段取りは付いていない」と出る。
- 状態は 直せる / お金が足りない / 未開（前提がまだ）/ 竣工。`RestorationBoardModel`（`Ui/EventBoard/Model/`）が決める。
- 見た目は `tools/art/event_board.py`（`V2` の `rrow_*` / `r_*`、モックは `mock_v2(tab=3)`。`--shot` には下見中の実画面を渡す）
  → `--emit` → `tools/art/event_board_prefab.py`。

### 草原のご褒美: 石と島が戻る（実装済み）

`MainIslandScene::ApplyGrassLandReward`（`Assets/Scripts/Core/Game/Scene/Main/Content/MainIslandScene/`）が入場のたびに決める。
シーンのコンテキスト `MainIslandSceneContext`（GameManage.scene）は `greenStone_`（石の `FloatingStone`）と
`fountainIsland_`（`SecondIsland` の `ReturningIsland`）だけを持つ。カメラ・パーティクル・尺はそれぞれのコンポーネントが持つ:
`ReturningIsland` の `stairs_`（`To SecondIsland Bridge`。子が1段ずつ）/ `camera_`（`FountainIslandCamera`）/
`focus_`（`MedievalFountainEmpty`。カメラが見る所）。

- `FountainIslandReturned` が立っていれば島と階段を出す（シーンではコンポーネントを切って隠してある）。
- 立っていなければ `ReturningIsland::Sink` で隠し、島と階段を雲の下（y -3000）へ下ろしてコライダーを作り直す。
  **隠すだけではコライダーが当たり続ける**（物理は Component の有効・無効を見ない）ため、見えない階段を歩けてしまう。
- `GrassLandCleared` の後、まだなら 石が戻る → 島が戻る の順に流す（`Prop_FloatingStone.h` / `Prop_ReturningIsland.h`）。演出は石・島が破棄される（シーンを抜ける）と止まる。どちらも任意のボタンで飛ばせる。
- Static のコライダーは Transform に付いてこないので、動かした後は `BodyAssembler::MarkDirty` で作り直す（演出の最後に一度）。
- カメラ `FountainIslandCamera` の位置 (60, 110, 200) は仮置き。エディタで見て直す。

### NPC の会話と BT（実装済み）

**台詞の正は `tools/art/story_npcs.py` の `CHATS`。** `.npcChat` や BT を手で直さず、スクリプトを直して再実行する。

```
python tools/art/story_npcs.py                 # 全部作り直す（アセットの GUID は保つ）
python tools/art/story_npcs.py --only camp     # prologue / dragon / island / newcomers / camp
```

- BT の分岐には、フレンドリー BT アクション `Story::IsStoryFlag` / `Story::IsRestored` / `Story::SetStoryFlag` を使う
  （`flag_` / `facility_` は tools で設定できるよう `int`）。その場かぎりの「もう話した」は blackboard に持つ。
- `Chat` はプレイヤーが話しかけたときにしか始まらない。続けて話させるときは、2つ目から `ImplementChat` にする。
- ステージのクリア条件は、そのシーンのコンテキストが持つ（`GrassLandSceneContext::clearEnemyKind_` / `clearStoryFlag_`、
  GameManage.scene。草原は大顎 = `Tyrannosaurus` → `GrassLandCleared`）。`GrassLandScene` が `Init` で
  `Story::StageClearWatcher`（`Story_StageClear.h`、GameObject に依らないクラス）で記録帳の `OnDefeat` を見張り、`DoExit` で外す。
  記録帳と同じく、協力プレイでも各ピアで立つ。**ロジックで済むものはコンポーネントにしない。**
- ボスは倒したあとも、ステージに入るたびに湧く（`EnemySpawnPoint` は物語の進み具合を見ない。イベントの「大物出現」「レイド」で
  本編クリア済みのホストでも湧くように）。倒し直しでは `StoryProgress::Set` が false を返すので、クリアの演出は流れない。

| 場所 | NPC（BT） | 物語の状態 → 会話 |
| --- | --- | --- |
| 序章 | クノイチ（`Adventure`） | `AirShipKunoichi`（探し物の旅、「あの日と同じ匂い」） |
| 序章 | 青年（船上） | `AirShipYoungMan`（浮かぶ島・島の心臓の説明） |
| 序章 | 教官 | `Introduction ActionInstructure`（自己紹介とハンターの説明）。訓練中の台詞は既存のまま |
| 序章 | ドラゴン | 撃墜後に `FirstDragon Heart Shatter`（教官が叫ぶ。「心臓が抜かれた」） → 巣へ帰る |
| 拠点の島 | 教官（`IdleActionInstructure`） | `RestorationStarted` 前: 驚きアイコン → `Instructor_RestorationStart` + 台座を映して `Instructor_PortalGuide` → `RestorationStarted` を立てる／草原前: `Instructor_BeforeGrassLand`／草原後: `Instructor_GrassLandReport`（1回。噴水の島が戻ったこと、狩人の一族がそこに住むこと）→ `Instructor_AfterGrassLand`／砂漠後: 驚きアイコン → `Instructor_DesertReport` → `NestVoyageStarted`／以後: `Instructor_ToNest` → 巣へ出発／古竜後: 驚きアイコン → `Instructor_Epilogue` → `EpilogueHeard` → `Instructor_AfterEnding` |
| 拠点の島 | 商人（`Merchant`） | `Merchant_First`（1回）／ふだん: `Merchant` → 店を開く |
| 拠点の島 | 仲介人（`CharacterBroker`） | `CharacterBroker_First`（1回）／一族の家の後: `CharacterBroker_ClanHouse`（一族を訪ねろ）／ふだん: `CharacterBroker`（紹介できる奴がいない）。キャラ選択はしない |
| 噴水の島 | 女狩人（`ClanHuntress`、一族の家の prefab の中） | `ClanHuntress_First`（1回。一族に手伝いたい者がいる）／ふだん: `ClanHuntress` → キャラ選択 |
| 拠点の島 | クノイチ・青年（`IslandKunoichi` / `IslandYoungMan`、GameObject "StoryNpcs" の下） | 草原の前: `_First`（1回）→ `_Again`／後: `_Cleared`（1回）→ `_ClearedAgain` |
| 草原 | 野営地の4人（`CampPeople*`） | 大顎を倒す前: `_First`（1回）→ `_Again`／倒した後: `_Cleared`（1回）→ `_ClearedAgain` |
| 砂漠 | 隊商の3人（`CaravanMaster` / `Keeper` / `Boy`）、竜の骨の前のクノイチ（`DesertKunoichi`） | 骸竜を倒す前: `_First`（1回）→ `_Again`／倒した後: `_Cleared`（1回）→ `_ClearedAgain`（`story_npcs.py --only desert`） |
| 古竜の巣 | 古竜（`AncientDragon` の BT、`StartChat`） | 登場: `AncientDragon_Intro`（クノイチ）／体力 60%・25%: `AncientDragon_Cannon1/2`（教官）／最期: `AncientDragon_Defeat`（クノイチ）（`story_npcs.py --only nest`、BT は `ancient_dragon.py`） |
| 砂漠 | 護衛（`CaravanGuard`） | `CaravanGuard_Lost`（`DesertGuardRescued` を立てる）→ `_Back`／骸竜の後: `_Cleared` |

### 砂漠のステージの作り方（実装済み・一部は要確認）

作り直すときは、上から順に流す（どれもアセットの GUID は保つ）。元のモデルは Sketchfab / Mixamo（出典は `docs/ThirdPartyAssets.md`）で、
加工スクリプトと元ファイルはリポジトリの外の `%USERPROFILE%\NanamiAssetsWork\Desert\` にある。

```
python tools/art/desert_terrain.py <work>            # 地形の高さ (data/desert_terrain.npz) と砂・泥・石畳のテクスチャ
blender -b --python tools/art/desert_terrain_blender.py -- <work>   # 地形の FBX -> tools.model で DesertTerrain.mv1
python tools/art/desert_prefabs.py                   # 岩・城塞・神殿・竜の骨・サボテン・ヤシのプレハブ
python tools/art/desert_scene.py                     # DesertScene.scene (草原のシーンの複製が土台。GameObject の GUID は毎回新しい)
python tools/art/desert_context.py                   # GameManage.scene の DrySandSceneContext を、今のシーンの GUID に合わせて足し直す
python tools/art/story_npcs.py --only desert         # 隊商・護衛・クノイチの会話と BT
python tools/art/desert_caravan.py place             # 隊商の4人とクノイチを Caravan ルートに置く (desert_scene.py の後は必ず)
python tools/art/desert_enemies.py                   # 大サソリ・ワーム・骸竜の AnimTree / BT / プレハブと EnemyFactory
python tools/art/desert_heightgrid_bake.py           # 敵の経路探索用の Desert.heightGridMap
python tools/art/desert_quests.py                    # 掲示板の依頼
python tools/art/desert_sandstorm.py prefabs|place  # 砂嵐と転がる枝玉 (place は desert_scene.py の後に必ず)
python tools/art/desert_boss_heart.py                # 光の心臓の当たり判定 (HeartHitBox + StormHeart) を prefab とシーンに付ける
python tools/art/desert_enemies.py --storm-only      # 作り直さずに、骸竜の AnimTree / BT へ砂嵐のギミックだけ足す
python tools/art/sky_islands.py desert               # 遠景の浮島、カメラの far、空のドーム (desert_scene.py の後に必ず)
```

- シーンのコンテキストは `DrySandSceneContext`（中身は草原と同じ）。到着演出は草原と共通の `StageArrivalMovie<TContext>`。
  クリア条件は骸竜（EnemyKind 6）→ `DesertCleared`。光の心臓は `LightFloatingStone.prefab`（`green_core_effect.py` の `LightCoreAura` / `LightStoneLiftOff`）。
- 砂嵐（`GamePlay::Weather::Sandstorm`、ルート "Sandstorm"）は凪（50〜90 秒）と砂嵐（25〜40 秒）を繰り返し、入場後 40 秒は凪。
  砂嵐の間は砂色の霧と `tktk2/sandStorm.efkefc` を出し、Dynamic な RigidBody を WindZone の向きへ押す（自分のアバターと、同期していない物だけ）。
  天候はピアごとに独立で同期しない。枝玉（`GamePlay::Prop::Tumbleweed`、ルート "Tumbleweeds" に 16 個）は砂嵐の間だけ風下へ転がって跳ね、
  置き場所から 450 離れたら戻る。
- 敵の AnimTree の State 番号はハイエナと同じ（0 移動 / 11111 待機 / 7 / 8 / 15 / 20 死ぬ / 23-25 攻撃）なので、BT はハイエナの写し。
  骸竜もいまはハイエナと同じ動き（ボスらしい攻撃の組み立ては【未設計】）。
- 骸竜のギミック「嵐を呼ぶ骸竜」（2026-09-27 ユーザー決定。#186）: 体力が 70% と 35% を切ると、骸竜が止まって
  `cast_1`（State 30）を再生し、砂嵐を呼ぶ（`Sandstorm::BeginSummoned`。周期を止めて砂嵐にし、最長 60 秒）。
  砂嵐の間だけ、広場の光の心臓を叩けるようになる（`GamePlay::Prop::StormHeart`。Stone の子の `HeartHitBox`。砂嵐の外では地面の下へ退ける）。
  3回叩くと心臓が揺らぎ、砂嵐が止んで、骸竜は倒れ込み（`stunstart` 31）→ 伏せ（`stunidle` 32、6 秒）→ 起き上がる（`stunover` 33）。
  その間が大きく攻め込む時間。心臓が骨を動かしていることを、プレイヤーが手で確かめる場面（終章の伏線）。
  骸竜の砂嵐はホストの BT が決めて `BossSandstormRpc` で全員へ送る（周期の砂嵐はピアごとのまま）。
  クライアントで揺らいだ心臓は `StormHeartShakenRpc` でホストへ知らせ、気絶させるのはホストの BT（`SkeletonDragon::HeartStun`）。
  BT の枝は `desert_enemies.py` の `add_storm_gimmick`、State は `STORM_STATES`。
  エフェクトと音は仮（叩く: `ItemPickupSparkle` / `HolyGlass`、揺らぐ: `LightStoneLiftOff` / `RockWall_Crumble`）。
- BGM: フィールドは「砂の街」（`DrySandSceneContext.bgm_`）。骸竜戦は「星の胎動」で、骸竜の BT の先頭の枝が
  プレイヤーが 220 まで近づくか、攻撃されて警戒したら（Alert 1）1回だけ `PlayBGM` する（`desert_enemies.py` の `add_boss_bgm`）。
  出典は `docs/ThirdPartyAssets.md`。
- ロード画面の地図は、もと「未開放」だった島を「砂漠地帯」にした（`loading_map.py`、航路は `loading_map_prefab.py --routes-only`）。
- 要確認（ビルドとエディタで）: モデルの大きさと向き、.mv1 の中のクリップの並び（名前順と見ている）、NPC と小物の立ち位置、
  骸竜の当たり判定と攻撃範囲、泉（泥のくぼ地）と水の見た目、骸竜戦の BGM が切り替わる距離。

### これから作るもの

| 物語の要素 | 仕組み | 状態 |
| --- | --- | --- |
| 施設 | 仮置きの4つは削除した。何を建てるか・金額・見た目を決める | 未決定 |
| 狩人の一族の引っ越し | 長老・見張り・負傷者も噴水の島に移す【確定】（2026-09-25）。置き方（一族の家の prefab に入れて建てた後だけ出すか）・移った後の台詞・草原の野営地に残すかは未決定 | 未着手 |
| 一族の家の動作確認 | ビルド後に: 建てる → 女狩人が出る → 話す → キャラ選択。prefab から生成した FriendlyNpc が会話センサーに拾われるか | 要確認 |
| 島が戻る演出のカメラ | `FountainIslandCamera` の位置は仮。エディタで確かめて直す | 要確認 |
| 狩り場のクリア状態 | 今は `StageData::isCleared_`（アセット側）。`StoryFlag` に寄せる | 未着手 |
| クノイチ・青年の立ち位置 | `story_npcs.py` の `NEWCOMERS` で仮置き（地面の高さを測れないので、少し上から落として着地させている）。エディタで確かめて直す | 要確認 |

## 8. 台詞を書くときの決まり

- 1つの `text_` は **2行まで**（改行は `\n`）。1行は **22 字まで**。CP932 に無い字（「〜」など）は使えない。
- `_First`（初めて話しかけたとき）は3〜5ページ、`_Again`（2回目以降）は1ページで要点だけ。
- 間は「……」で表す（「...」は使わない）。
- 説明は台詞の中でする。ナレーションは使わない。
- ゲームの操作は、NPC が世界の言葉で言う（例:「正面に立つな、横へ跳べ。」）。ボタン名は出さない。

## 9. AI への頼み方の例

- 「`docs/Story.md` に沿って、一族の家の女狩人に2回目以降の会話を足して」
- 「一族の家に長老も置いて。建てた後だけ出す形で（`clan_house.py`）」
- 「砂に埋もれた荷の収集依頼を作って。荷のアイテムと、砂丘の置き場所も」

## 10. 未決定事項

- 固有名: 島・世界・クノイチ・教官・青年の名前。
- 大地が雲に呑まれた理由（運営のイベントで少しずつ明かす候補）。（教官が竜狩りを知っていて、古竜の後に明かす形で【仮】実装した。変えるなら `Instructor_Epilogue`）
- 砂漠の施設（造船所など）と、砂に埋もれた荷の依頼。骸竜のボスらしい攻撃（ギミックの「嵐を呼ぶ骸竜」は決定・実装済み。攻撃の組み立ては未設計）。
- 施設の費用と、依頼報酬の相場（常設の依頼 600、毎週のイベントは 1,200〜3,000 で【仮】。`docs/LiveOps.md` §2）。
- 終章: 島が巣に横付けする演出、クノイチと一緒に戦う NPC、浮かび上がった島々を見せる演出（いまは台詞だけ）。
