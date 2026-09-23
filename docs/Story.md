# ストーリー設計

ゲームの筋・登場人物・施設と、それを支えるコード（`StoryProgress` / `RestorationGate`）のまとめ。
台詞・依頼・施設・演出を作るときの **正（source of truth）** はこのファイル。
既存の会話（`Assets/Data/NpcChatText/*.npcChat.meta`）・依頼（`Assets/Data/EventNotice/*.boardQuest.meta`）・
ステージ（`Assets/Data/Stage/*.stageData.meta`）から起こした（2026-09-23）。

---

## 0. このファイルを使って作業するときの約束（AI 向け）

- **【確定】** と書かれた項目は変えない。変えたい場合は、先にユーザーに確認する。
- **【未設計】** と書かれた項目（砂漠・岩石地帯とその施設、終章の細部）は、これから作る。
  台詞・依頼・フラグ・施設を勝手に作らず、案を出してユーザーに決めてもらう。
- **【仮】** は、たたき台として使ってよい。決まったら **【確定】** に書き換える。
- 物語に何かを足したら（人物・フラグ・施設・台詞のファイル）、同じ変更でこのファイルも更新する。
- 会話・依頼の中身は `.meta` 側に入っている（本体の `.npcChat` / `.boardQuest` は 0 バイトで正常）。
- 復興の進み具合は **手元の PC にだけ保存し、マルチプレイでは共有しない**。
  ほかのプレイヤーの島を見に行く機能もない。【確定】

## 1. 一行で【確定】

ドラゴンに「島の心臓」を砕かれて沈みかけた拠点の島を、ハンターの稼ぎで立て直し、
その島ごとドラゴンの巣へ乗り込んで倒す話。

- お金を稼ぐ理由は **島の復興資金**。
- 各狩り場には、その土地の人たちの物語と、島の心臓の欠片（**核片**）がある。
- 島を直すと、施設・住人・行き先が増える。直した島そのものが最終決戦の舞台になる。

## 2. 世界【確定】

- 空に島々が浮かんでいて、人は飛行船や転移の台座（紫に光る台座 = `IslandPedestial`）で島を渡る。
  地上には、魔物がうろつく **狩り場** がある。
- 島は、中心に埋まった **浮遊石（島の心臓）** の力で浮いている。
- **ハンター** は、狩り場で魔物を狩って報酬を得る稼業。拠点の島は、駆け出しのハンターが集まって鍛える島。
- 狩り場には属性がある。核片の色はこの属性に合わせる。

| 狩り場 | 難度・属性 | 状態 |
| --- | --- | --- |
| 草原地帯 | 初級・緑 | 【確定】第1章 |
| 砂漠地帯 | 中級・光 | 【未設計】シーンもこれから作る |
| 岩石地帯 | 上級・火 | 【未設計】シーンもこれから作る |

## 3. 登場人物

台詞を書くときは **口調** の列に合わせる。口調は既存の台詞から拾った。

| 人物 | アセット | 口調（一人称・例） | 役割 |
| --- | --- | --- | --- |
| 主人公 | 剣士 / 魔術師 | 喋らない | 今日の船で着いた新人ハンター。序章でドラゴンを撃ち落とす【確定】 |
| 教官 | `ActionInstructure` / `IdleActionInstructure` | 「俺」。短い命令形。「口より先に手を動かせ。」 | 序章で主人公を鍛える。襲撃で脚を痛めて前線を退き、島の復興をまとめる（復興ボードの担当）【仮】 |
| 旅のクノイチ | `AirShipKunoichi` | 「私」。くだけた口調。「……気をつけなよ。」 | 風の変化でドラゴンの襲来を予感していた。ドラゴンを追っている（故郷の島を落とされた）。章の区切りに現れて情報をくれ、終章で共闘する【仮】 |
| 飛行船の青年 | `AirShipYoungMan` | 「僕」。明るい。「もう胸が躍ってるよ。」 | 空の果てを夢見ている。墜ちた飛行船を直したがっている。造船所の担当にする予定【仮・砂漠と一緒に決める】 |
| 商人 | `Merchant` | 威勢のいい商売口調。「うちで揃えていきな。」 | 島の雑貨屋。復興が進むと品揃えが増える【確定】 |
| 酒場の仲介人 | `CharacterBroker` | ぶっきらぼう。「好きなのを連れていきな。」 | 掲示板の依頼と、仲間の斡旋をする【確定】 |
| 長老 | `CampPeopleElder` | 「わし」。古風。「〜しておる」「〜じゃ」寄り | 草原の狩人一族の長【確定】 |
| 女狩人 | `CampPeopleHuntress` | 落ち着いた女言葉。「囲まれないで。」 | 狩りの助言役。島に移ると狩人小屋を営む【仮】 |
| 見張り | `CampPeopleLookout` | 小声で緊張した口調。「しっ、静かに。」 | 大顎を見張っている【確定】 |
| 負傷者 | `CampPeopleWounded` | 弱々しい。「ごめんなさい、うまく立てなくて。」 | 大顎に襲われた生き残り【確定】 |
| ドラゴン | `FirstEventDragon` | 喋らない | 島の心臓を喰らって力を増す古竜【確定】。名前は【未設計】 |

## 4. 章立て

### 序章 — 襲来（`FirstTouchDownMainIsLandScene`）【確定・大部分は実装済み】

1. 主人公が飛行船で拠点の島に着く。船上でクノイチと青年に会う。（実装済み）
2. 教官の訓練を受ける（走る・跳ぶ・斬る・転がって躱す）。（実装済み）
3. ドラゴンが現れる。教官が「島の者を逃がす時間がいる」と言い、主人公が足止めする。（実装済み）
4. ドラゴンが空へ上がって島を壊し始める。山頂の大砲で撃ち落とす。（実装済み。BT の最後は `ChangeToMainIslandScene`）
   火球は噴水の島 → 家の島 → 拠点の島の順に落ち、着弾点で `IslandFireImpact`（爆発）・`IslandBurning`（燃え続ける炎と黒煙）・
   `IslandCrumble`（島の底が崩れ落ちる）を出す（BT の "DestroyIslandState 2..4"。`tools/art/island_destruction_effects.py`）。
5. 墜ちたドラゴンが島の中心に爪を突き立て、**島の心臓を砕いて**飛び去る。
   欠片は三方の狩り場へ散り、島がぐらりと傾いて暗転する。
   → 実装済み（`FirstEventDragon` の BT の "Heart ..." ノード）: 演出カメラ → `ToIslandHeart` のルートで拠点の島の中心へ降下 →
   爪（`Attack1`）→ `IslandHeartBreak`（光の柱・地割れ・突き出す岩）と島の縁の崩落 → `HeartShardScatter`（緑・金・赤の光が
   三方へ散る。`tools/art/heart_shatter_effect.py`）→ 揺れ → 島の底が崩れ落ちる →
   島全体を映すカメラで、噴水の島（`SecondIsland`）と家の島（`ThirdIsland`）が橋ごと傾いて雲の下へ落ちていく
   （BT アクション `FallIsland`。燃えていた炎と煙は `AttachParticle` で島に付いたまま一緒に落ちる）→ 教官の叫び → 巣へ帰る。
   この2つの島と3本の橋は `MainIslandScene` には無い（落ちた設定）。

序章を抜けると `StoryFlag::PrologueCleared` が立つ（実装済み）。

### 第1章 — 残された島（`MainIslandScene`）【確定】

- 島は半壊していて、少しずつ沈みつつある（瓦礫、傾いた建物、消えた灯り）。
- 教官「俺はもう前には出られん。だが、島を立て直す段取りなら付けられる。」
  → 復興ボードが使えるようになる（`StoryFlag::RestorationStarted`）。
- 教官の説明: 島を浮かせ続けるには心臓を戻すしかない。欠片はまだ力を放っていて、周りの魔物が荒れている。
  まずは一番近い草原から。
- 最初の復興は **船着き場**（`Facility::Dock`）。チュートリアルを兼ねる。

### 第1章 — 草原：大顎と狩人の一族（`GrassLandScene`）【確定】

- 盆地の村が **大顎**（`Tyrannosaurus`）に潰され、狩人の一族は山の棚の野営地に逃げている。
- 大顎が「村の跡から一歩も動かない」のは、**緑の核片が村の跡に落ちていて、その力に惹かれているから**。
- 西の林のハイエナの群れ（遠吠えで仲間を呼ぶ）を減らし、大顎を倒す。
- クリアすると **緑の核片** を手に入れる（`StoryFlag::GrassLandCleared`）。
  長老「島を追われる辛さは、わしらが一番知っておる。」一族の何人かが拠点の島へ移り住む。
- 島の変化: 緑の核片をはめると島の沈下が止まり、草木が戻る。
- 解放: 狩人小屋（`Facility::HunterLodge`）、畑（`Facility::Field`）。

### 第2章・第3章 — 砂漠地帯 / 岩石地帯【未設計】

ステージを作るときに一緒に決める。決まっているのは、次の条件だけ。

- それぞれの狩り場に **光の核片 / 火の核片** がある。
- 既存の依頼「砂に埋もれた荷」（依頼主: 行商人）と「岩場の暴君」（依頼主: 岩石地帯の見張り）とつなげる。
  なお、「岩場の暴君」は今 `Tyrannosaurus` を討伐対象にしていて、草原の大顎と同じ魔物になっている。
- 3つの核片がそろうと、島の心臓が戻り、島が自力で飛べるようになる。

案（決定ではない）: 砂漠の荷の中身は飛行船の部品で、青年が造船所を任される。
クノイチはこのあたりでドラゴンを追う理由を明かす。
**鍛冶場（武器の強化）は作らない**【確定】。実装が重すぎるため。

### 終章 — 嵐の巣【骨格のみ確定】

- 復興した島そのものを船にして、嵐（既存の `SetStorm` / `Lightning`）を抜け、ドラゴンの巣へ向かう。
- 序章の山頂の大砲で、島の仲間が援護射撃する（序章と対になる場面）。
- クノイチと共闘する。ドラゴンを倒すと、喰われていたほかの島々の心臓の光が空へ還っていく。
- エピローグ: ほかの島から人が集まり始める。教官は「駆け出しの島」を再開する。

ステージの作り方（専用シーンか、ロード画面の演出か）は【未設計】。

## 5. 施設（`GameCore::Story::Facility`）

お金を払って直す（掲示板の「復興」。§7）。直すと `RestorationGate` が島の見た目を切り替える。金額はまだ決めていない（今は仮の値）。

| ID | 名前 | 条件 | 直すと | 状態 |
| --- | --- | --- | --- | --- |
| `Dock` | 船着き場 | 第1章開始 | 定期船が来る。チュートリアル | 【確定】 |
| `GeneralStore` | 雑貨屋の修繕 | — | 商店の品揃えが1段増える | 【確定】 |
| `HunterLodge` | 狩人小屋 | `GrassLandCleared` | 食料と回復アイテムの店 | 【確定】 |
| `Field` | 畑 | `HunterLodge` | 薬草などが定期的に手に入る（`HerbPatch`） | 【確定】 |
| — | 造船所・灯台・山頂の大砲 など | 砂漠・岩石のクリア | — | 【未設計】enum にはまだ足さない |

見た目だけ変わる施設（家屋・噴水など）を混ぜると、お金の使い道が途切れにくい。

## 6. 報酬の流れ【確定】

- **狩り場の初回クリア**: 核片と、まとまった復興資金（ストーリークエスト = `MainStoryQuestBase`）。
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
  前提の `requiredStoryFlag_` / `requiredFacility_`（-1 = なし）・前提の文言）。
  掲示板に貼る並びは `MainIslandEventBoard.eventBoard` の `facilities_`。
  **正は `tools/art/restoration_facilities.py` の `FACILITIES`**。直して再実行すると、アセットと `facilities_` を書き直す（GUID は保つ）。
- 建つ場所と姿は MainIslandScene の `RestorationSite_<施設>`（`Assets/Prefab/Prop/Restoration/`）。root に `RestorationGate`、
  子に Restored / PreviewCamera（→ PreviewTarget を見る）。建てる前は空き地。**正は `tools/art/restoration_sites.py` の `SITES`**
  （置き場所・カメラの向き・借りているモデル）。再実行すると prefab を組み直し、シーンの `RestorationSite_*` を置き直す。
  建った姿のモデルは仮（Settlement の prefab を借りている）。
  建った姿は最初はコンポーネントを切って隠してある（エディタでは空き地に見える。`isActive_` は読み込みで効かないため）。
- 掲示板は `RestorationGate::Find(facility)`（シーンにある門の一覧）で門を探す。門が無い施設は下見しない（札は出る）。
- 教官から任される（`RestorationStarted`）までは空の頁で「まだ普請の段取りは付いていない」と出る。
- 状態は 直せる / お金が足りない / 未開（前提がまだ）/ 竣工。`RestorationBoardModel`（`Ui/EventBoard/Model/`）が決める。
- 見た目は `tools/art/event_board.py`（`V2` の `rrow_*` / `r_*`、モックは `mock_v2(tab=3)`。`--shot` には下見中の実画面を渡す）
  → `--emit` → `tools/art/event_board_prefab.py`。

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
  `Story::WatchStageClear`（`Story_StageClear.h`、GameObject に依らない関数）を記録帳の `OnDefeat` に繋ぎ、`DoDispose` で外す。
  記録帳と同じく、協力プレイでも各ピアで立つ。**ロジックで済むものはコンポーネントにしない。**
- `EnemySpawnPoint::skipIfStoryFlag_`（-1 = 常に湧く）にフラグを入れると、そのフラグが立った後は湧かない。
  大顎のスポーン地点は `GrassLandCleared`（2）。敵を湧かせるのはホストなので、判断もホストの進み具合。

| 場所 | NPC（BT） | 物語の状態 → 会話 |
| --- | --- | --- |
| 序章 | クノイチ（`Adventure`） | `AirShipKunoichi`（探し物の旅、「あの日と同じ匂い」） |
| 序章 | 教官・青年 | 既存のまま |
| 序章 | ドラゴン | 撃墜後に `FirstDragon Heart Shatter`（教官が叫ぶ） → 巣へ帰る |
| 拠点の島 | 教官（`IdleActionInstructure`） | `RestorationStarted` 前: 驚きアイコン → `Instructor_RestorationStart` + 台座を映して `Instructor_PortalGuide` → `RestorationStarted` を立てる／草原前: `Instructor_BeforeGrassLand`／草原後: `Instructor_GrassLandReport`（1回）→ `Instructor_AfterGrassLand` |
| 拠点の島 | 商人（`Merchant`） | 雑貨屋が直る前: `Merchant_First`（1回）／直った後: `Merchant_Restored`（1回）／ふだん: `Merchant` → 店を開く |
| 拠点の島 | 仲介人（`CharacterBroker`） | `CharacterBroker_First`（1回）／ふだん: `CharacterBroker` → キャラ選択 |
| 拠点の島 | クノイチ・青年（`IslandKunoichi` / `IslandYoungMan`、GameObject "StoryNpcs" の下） | 草原の前: `_First`（1回）→ `_Again`／後: `_Cleared`（1回）→ `_ClearedAgain` |
| 草原 | 野営地の4人（`CampPeople*`） | 大顎を倒す前: `_First`（1回）→ `_Again`／倒した後: `_Cleared`（1回）→ `_ClearedAgain` |

### これから作るもの

| 物語の要素 | 仕組み | 状態 |
| --- | --- | --- |
| 施設の金額 | 今は仮（`tools/art/restoration_facilities.py`）。報酬の額と合わせて決める | 未決定 |
| 施設の見た目 | 置き場所と建った姿のモデルは仮（`tools/art/restoration_sites.py`）。施設ごとに作る | 仮 |
| 狩り場のクリア状態 | 今は `StageData::isCleared_`（アセット側）。`StoryFlag` に寄せる | 未着手 |
| クノイチ・青年の立ち位置 | `story_npcs.py` の `NEWCOMERS` で仮置き（地面の高さを測れないので、少し上から落として着地させている）。エディタで確かめて直す | 要確認 |

## 8. 台詞を書くときの決まり

- 1つの `text_` は **2行まで**（改行は `\n`）。1行は **22 字まで**。CP932 に無い字（「〜」など）は使えない。
- 会話は短く。`_First`（初めて話しかけたとき）は2〜3ページ、`_Again`（2回目以降）は1ページで要点だけ。
  1行は16字くらいを目安にし、筋とゲームのヒントに要らない前置き・言い換えは削る。
- 間は「……」で表す（「...」は使わない）。
- 説明は台詞の中でする。ナレーションは使わない。
- ゲームの操作は、NPC が世界の言葉で言う（例:「正面に立つな、横へ跳べ。」）。ボタン名は出さない。

## 9. AI への頼み方の例

- 「`docs/Story.md` に沿って、狩人小屋が直った後の女狩人の会話を `story_npcs.py` に足して」
- 「`Facility::Dock` の船着き場を、MainIslandScene に RestorationGate で置いて。建った姿のモデル案も出して」
- 「砂漠地帯のストーリー案を3つ出して。§0 の約束に従って、【確定】と矛盾しないように」

## 10. 未決定事項

- 固有名: 島・世界・ドラゴン・クノイチ・教官・青年の名前。
- 砂漠・岩石地帯のボス、物語、施設（ステージを作るときに決める）。
- 施設の費用と、依頼報酬の相場。
- 終章のステージの作り方。
