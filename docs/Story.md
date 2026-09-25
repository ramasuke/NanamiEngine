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

見た目【確定・未実装】: 狩り場の外周は山ではなく **崖と雲海** にして、遠景にほかの浮島を置く。到着演出（`StageArrivalMovie`）で島を見下ろすカットを入れる。
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
| 教官 | `ActionInstructure` / `IdleActionInstructure` | 「俺」。短い命令形。「口より先に手を動かせ。」 | 序章で主人公を鍛える。襲撃で脚を痛めて前線を退き、島の復興をまとめる（復興ボードの担当）【仮】。竜狩りのことを知っているかは【未設計】 |
| 旅のクノイチ | `AirShipKunoichi` | 「私」。くだけた口調。「……気をつけなよ。」 | 故郷の島（今は砂の島に墜ちた城塞）を古竜に落とされ、古竜を追っている。島が落ちたとき、割れた島の底から竜の骨が出てくるのを見ていて、真相の一部を知っている【確定】。章の区切りに現れて情報をくれ、終章で共闘する【仮】 |
| 飛行船の青年 | `AirShipYoungMan` | 「僕」。明るい。「もう胸が躍ってるよ。」 | 空の果てを夢見ている。墜ちた飛行船を直したがっている。造船所の担当にする予定【仮・砂漠と一緒に決める】 |
| 商人 | `Merchant` | 威勢のいい商売口調。「うちで揃えていきな。」 | 島の雑貨屋。復興が進むと品揃えが増える【確定】 |
| 酒場の仲介人 | `CharacterBroker` | ぶっきらぼう。「好きなのを連れていきな。」 | 掲示板の依頼を回す。騒ぎで腕利きが逃げたので仲間は紹介できず、一族の家へ案内する【確定】 |
| 長老 | `CampPeopleElder` | 「わし」。古風。「〜しておる」「〜じゃ」寄り | 草原の狩人一族の長【確定】。「島の心臓は昔は竜の心臓だった」という言い伝えを話す【確定】 |
| 女狩人 | `CampPeopleHuntress` / `ClanHuntress` | 落ち着いた女言葉。「囲まれないで。」 | 狩りの助言役。草原の後、一族と噴水の島に移り住む。一族の家が建つと、復興を手伝いたい一族の者を仲間に出す（キャラ選択）【確定】 |
| 見張り | `CampPeopleLookout` | 小声で緊張した口調。「しっ、静かに。」 | 大顎を見張っている【確定】 |
| 負傷者 | `CampPeopleWounded` | 弱々しい。「ごめんなさい、うまく立てなくて。」 | 大顎に襲われた生き残り【確定】 |
| 古竜 | `FirstEventDragon` | 喋らない | 最後に生き残った竜。仲間の心臓を島々から取り戻し、巣で仲間を蘇らせようとしている【確定】。名前は【未設計】 |
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
   青年が窓の外の島を見て、「島の心臓」を話す（`AirShipYoungMan`）。クノイチは「島の心臓の光は、遠くからでも見える。
   ……見えすぎるくらいにね。」と匂わせる（`AirShipKunoichi`）。（台詞は実装済み）
2. 教官の訓練を受ける（走る・跳ぶ・斬る・転がって躱す）。（実装済み）
   教官の自己紹介で、ハンターとは何かを1行で言う（`Introduction ActionInstructure`）。（台詞は実装済み）
   【確定・未実装】島の中心（竜が爪を立てる所。`IslandHeartStones` の真上）に祠を置き、地面の割れ目から緑と金の光を漏らす。
   訓練への道すじをその前に通し、教官に「あの下に島の心臓が眠っている」と言わせる。5. で竜がそこを狙うので、
   「島の心臓が狙いか！」が伝わる。置き場所と道すじはエディタで見て決める。
3. ドラゴンが現れる。教官が「島の者を逃がす時間がいる」と言い、主人公が足止めする。（実装済み）
4. ドラゴンが空へ上がって島を壊し始める。山頂の大砲で撃ち落とす。（実装済み。BT の最後は `ChangeToMainIslandScene`）
   火球は噴水の島 → 家の島 → 拠点の島の順に落ち、着弾点で `IslandFireImpact`（爆発）・`IslandBurning`（燃え続ける炎と黒煙）・
   `IslandCrumble`（島の底が崩れ落ちる）を出す（BT の "DestroyIslandState 2..4"。`tools/art/island_destruction_effects.py`）。
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
- 2つの心臓が島に戻ると、島が自力で飛べるようになる。そのまま終章へ【確定】（演出は【未設計】）。
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

ステージの作り方（専用シーンか、ロード画面の演出か）と、骸たちを戦わせるかは【未設計】。

### 4.1 運営：浮かび上がった島々【骨格のみ確定】（2026-09-25）

このゲームは運営型。本編（終章）の後は、結末で浮かび上がった島々へ渡る **イベント** で遊び続ける。

- 本編の結末が、そのまま運営の入口になる。島はいくらでも増やせる（どの島も「散った心臓が引き上げた島」で説明が付く）。
- イベントの島ごとに、狩り場・ボス・住人・依頼を足す（本編の草原の島・砂の島と同じ作り）。
  - 落とされていた島: 昔、古竜に心臓を奪われて沈んだ島。住人の生き残りや、その島の物語がある。
  - 昔の大地のかけら: 雲に呑まれる前の大地。竜狩りの時代の遺跡、見たことのない魔物。世界の謎（大地が雲に呑まれた理由）を少しずつ明かす場所。
- 拠点の島は、島々へ渡る船着き場・ハブになる（転移の台座・飛行船）。
- イベントの期間・報酬・マルチプレイとの関わり、本編をクリアしていない人の扱いは【未設計】。

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
  `Story::StageClearWatcher`（`Story_StageClear.h`、GameObject に依らないクラス）で記録帳の `OnDefeat` を見張り、`DoDispose` で外す。
  記録帳と同じく、協力プレイでも各ピアで立つ。**ロジックで済むものはコンポーネントにしない。**
- `EnemySpawnPoint::skipIfStoryFlag_`（-1 = 常に湧く）にフラグを入れると、そのフラグが立った後は湧かない。
  大顎のスポーン地点は `GrassLandCleared`（2）。敵を湧かせるのはホストなので、判断もホストの進み具合。

| 場所 | NPC（BT） | 物語の状態 → 会話 |
| --- | --- | --- |
| 序章 | クノイチ（`Adventure`） | `AirShipKunoichi`（探し物の旅、「あの日と同じ匂い」） |
| 序章 | 青年（船上） | `AirShipYoungMan`（浮かぶ島・島の心臓の説明） |
| 序章 | 教官 | `Introduction ActionInstructure`（自己紹介とハンターの説明）。訓練中の台詞は既存のまま |
| 序章 | ドラゴン | 撃墜後に `FirstDragon Heart Shatter`（教官が叫ぶ。「心臓が抜かれた」） → 巣へ帰る |
| 拠点の島 | 教官（`IdleActionInstructure`） | `RestorationStarted` 前: 驚きアイコン → `Instructor_RestorationStart` + 台座を映して `Instructor_PortalGuide` → `RestorationStarted` を立てる／草原前: `Instructor_BeforeGrassLand`／草原後: `Instructor_GrassLandReport`（1回。噴水の島が戻ったこと、狩人の一族がそこに住むこと）→ `Instructor_AfterGrassLand` |
| 拠点の島 | 商人（`Merchant`） | `Merchant_First`（1回）／ふだん: `Merchant` → 店を開く |
| 拠点の島 | 仲介人（`CharacterBroker`） | `CharacterBroker_First`（1回）／一族の家の後: `CharacterBroker_ClanHouse`（一族を訪ねろ）／ふだん: `CharacterBroker`（紹介できる奴がいない）。キャラ選択はしない |
| 噴水の島 | 女狩人（`ClanHuntress`、一族の家の prefab の中） | `ClanHuntress_First`（1回。一族に手伝いたい者がいる）／ふだん: `ClanHuntress` → キャラ選択 |
| 拠点の島 | クノイチ・青年（`IslandKunoichi` / `IslandYoungMan`、GameObject "StoryNpcs" の下） | 草原の前: `_First`（1回）→ `_Again`／後: `_Cleared`（1回）→ `_ClearedAgain` |
| 草原 | 野営地の4人（`CampPeople*`） | 大顎を倒す前: `_First`（1回）→ `_Again`／倒した後: `_Cleared`（1回）→ `_ClearedAgain` |
| 砂漠 | 隊商の3人（`CaravanMaster` / `Keeper` / `Boy`）、竜の骨の前のクノイチ（`DesertKunoichi`） | 骸竜を倒す前: `_First`（1回）→ `_Again`／倒した後: `_Cleared`（1回）→ `_ClearedAgain`（`story_npcs.py --only desert`） |
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
```

- シーンのコンテキストは `DrySandSceneContext`（中身は草原と同じ）。到着演出は草原と共通の `StageArrivalMovie<TContext>`。
  クリア条件は骸竜（EnemyKind 6）→ `DesertCleared`。光の心臓は `LightFloatingStone.prefab`（`green_core_effect.py` の `LightCoreAura` / `LightStoneLiftOff`）。
- 敵の AnimTree の State 番号はハイエナと同じ（0 移動 / 11111 待機 / 7 / 8 / 15 / 20 死ぬ / 23-25 攻撃）なので、BT はハイエナの写し。
  骸竜もいまはハイエナと同じ動き（ボスらしい攻撃の組み立ては【未設計】）。
- ロード画面の地図は、もと「未開放」だった島を「砂漠地帯」にした（`loading_map.py`、航路は `loading_map_prefab.py --routes-only`）。
- 要確認（ビルドとエディタで）: モデルの大きさと向き、.mv1 の中のクリップの並び（名前順と見ている）、NPC と小物の立ち位置、
  骸竜の当たり判定と攻撃範囲、泉（泥のくぼ地）と水の見た目、BGM（今は草原と同じ）。

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

- 固有名: 島・世界・古竜・クノイチ・教官・青年の名前。
- 大地が雲に呑まれた理由（運営のイベントで少しずつ明かす候補）。教官が竜狩りのことを知っているか。終章で竜狩りの真相を誰が明かすか。
- 砂漠の施設（造船所など）と、砂に埋もれた荷の依頼。骸竜のボスらしい攻撃。
- 施設の費用と、依頼報酬の相場。
- 終章のステージの作り方。
