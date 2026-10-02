# 運営：毎週のイベント

毎週のイベントの型・開催時間・報酬と、その作り方。物語の上での位置づけは `docs/Story.md` §4.1。
決めたこと（2026-09-29、ユーザー決定）:

- 報酬は **島の飾り**（見た目だけ。持っているかは手元の PC にだけ保存し、マルチプレイでは共有しない）とお金。
- **誰でも参加できる**。イベントの依頼には、期間のほかに物語の進み具合の条件を付けない。
- 毎週の運営は **データだけ** で回す（exe の更新なし。エディタの *Asset Dist* で R2 に上げれば届く）。

## 1. 開催時間

金曜 12:00 開始 → 翌金曜 04:59 終了（日本時間。前例の「草原の群狼 討伐週間」と同じ）。
お知らせ（`.announcement`）は開始の 2 日前（水曜）に出す。

## 2. ローテーション

4 つの型を順に回し、一巡したら敵・ステージ・飾りを入れ替えて再開催する（「第2回 群狼討伐」）。

| 週 | 型 | 例 | 中身 |
| --- | --- | --- | --- |
| 1 | 討伐週間 | 草原の群狼 / 水場の大サソリ / 砂の下の大ワーム | `DefeatRequestQuest`。対象の敵を N 体 |
| 2 | 納品・採集 | 隊商の荷あつめ / 島の資材集め | `CollectRequestQuest`。拾う物をステージのシーンに置く |
| 3 | 大物出現 | 怒れる大顎 / 砂嵐の骸竜 | 既存ボスの prefab を複製して強くし、**イベント用のステージ**で湧かせる（本編のステージは変えない） |
| 4 | 共闘レイド | 嵐を呼ぶ竜 | 強敵。マルチ向けだがソロでも倒せる難度にする |

- 新しく始めた人も遊べるよう、草原の敵の週を多めにする。砂漠の週・レイドは告知文に難しいことを書く。
- 新しい島（Story §4.1「浮かび上がった島々」）は、月 1〜季節の大型更新として別枠。そこで増えた敵がローテーションの弾になる。

### 予定（2026-10-02 決定。草原だけで4週）

砂漠のステージは物語で解放されるので「誰でも参加」に合わない。最初の一巡は草原だけで回す。
データは `python tools/art/weekly_events.py` が `WEEKS` から書く（告知・依頼・お知らせ・飾り・掲示板・島の置き場所）。
中身を変えるときは `WEEKS` を直して再実行する（guid は保たれる）。ファイルは `Assets/Data/EventNotice/Weekly/` と `Assets/Data/Decoration/`。
告知のバナーは週ごとに `Assets/Art/UI/EventBoard/Banner/<asset>.png`（964x400。草原の実画面を AutoMCP で撮って色を付けたもの）。新しい週はこの名前で置いてから再実行する。

| 週 | 期間 | 告知 | 依頼主 | 中身 | 報酬 | 飾り（見た目） |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 10/09〜10/16 | 草原の群狼 討伐週間 | 酒場の仲介人 | ハイエナ ×15 | 1,500 G ＋ 群狼の旗 | `Settlement/TrailBanner` |
| 2 | 10/16〜10/23 | 草原の薬草あつめ | 商人 | 薬草 ×10（受注後に摘んだ分） | 1,200 G ＋ 薬草のかご | `Settlement/SupplyPile` |
| 3 | 10/23〜10/30 | 怒れる大顎 | 見張り | イベント用のステージで強い大顎 ×1 | 3,000 G ＋ 大顎のトーテム | `Settlement/Totem` |
| 4 | 10/30〜11/06 | 群狼討伐 第2回 | 酒場の仲介人 | ハイエナ ×20 | 1,500 G ＋ 毛皮の干し台 | `Settlement/DryingRack` |

- 報酬の相場: 草原1周 ≒ 555 G、一族の家 2,000 G、常設の依頼 600 G。イベントは1度きりなので常設の2〜5倍にした。
- 依頼の `questType_` はナビが見ていない `Kill10Slimes`(1) を使い回す。
- 飾りは MainIslandScene の `IslandDecorations`（掲示板の周り）に、週ごとの `ConditionalObject` で置いてある。
- 第3週は **イベント用のステージ「怒れる大顎」**（`SceneType::GrassLandEvent` = 6、`GrasslandEventStage.stageData`）で戦う。
  本編の草原地帯の大顎は変えない（新しく始めた人が強い個体で詰まらないように）。
  - 草原と同じ `GrassLandScene.scene` / `GrassLandScene` を使い、`GameManage.scene` の2つ目の `GrassLandSceneContext`
    （`sceneType_` 6）が大顎を `TyrannosaurusEnraged.prefab`（HP 2400、元は 1200）に差し替える（`enemyOverrideKind_` /
    `enemyOverridePrefab_`）。物語のクリア・初回の空撮・ナビの目標・浮遊石は持ち込まない。マルチの部屋も本編と分かれる。
  - 強い大顎は討伐を `EnemyKind::EnragedTyrannosaurus`(8) で記録する（`Tyrannosaurus::recordKind_`）。本編の大顎では
    イベントの依頼が、イベントの大顎では本編の依頼が進まない。
  - ステージ選択の3行目。`unlockConditions_` の期間（`WEEKS` の `stage='event'` の週）の外は `hideWhenLocked_` で行ごと隠れる。
  - 用意は `python tools/art/grassland_event_stage.py`（context・ステージ選択の行・ロード画面の航路 2->6 / 6->6 / 6->2・
    強い大顎の記録）。ステージのデータと期間は `weekly_events.py` が書く。
- 「嵐を呼ぶ竜」（`StormDragonRaid.eventNotice`）は古竜のステージができるまで掲示板から外してある（データは残す）。

## 3. 仕組み

| もの | 場所 | 役割 |
| --- | --- | --- |
| `EventNotice` | `Assets/Data/EventNotice/*.eventNotice` | 掲示板のイベント告知。期間 (`startAt_` / `endAt_`) と `unlockConditions_` |
| `BoardQuest` | `Assets/Data/EventNotice/*.boardQuest` | 依頼。`event_` を付けると、その期間だけ掲示板に出る。受注中・達成済みは **BoardQuest の guid** で見分ける |
| `EnemySpawnPoint` | ステージのシーン | 敵の湧き地点。`prefab_` を指定するとその prefab を湧かせる（空なら `kind_` の prefab） |
| 報酬 `IReward` | `Assets/Scripts/Core/Game/Reward/` | 依頼の `rewards_`（一覧）。`MoneyReward` / `DecorationReward`。それぞれに `conditions_` を付けられる |
| `DecorationData` | `Assets/Data/Decoration/*.decoration` | 島の飾り 1 つ（名前・説明・アイコン） |
| `DecorationCollection` | `Assets/Scripts/Core/Game/Decoration/` | 持っている飾り（`LocalPrefs/GameProgression/Decorations`） |
| `DecorationOwnedCondition` | `Assets/Scripts/Core/Game/Condition/` | 飾りを持っていれば満たす条件 |
| `ConditionalObject` | `Assets/Scripts/GamePlay/Prop/ConditionalObject/` | シーンに置く。`conditions_` を満たす間だけ `prefab_` を子に出す / `target_` を有効にする |

- 依頼の `questType_` は週ごとに使い回してよい（掲示板から受けた依頼は guid で見分ける。`QuestType` を見るのはナビの
  `QuestTakingCondition` くらい）。受注中・達成済みの記録は `LocalPrefs/TakingQuests` / `LocalPrefs/CompletedBoardQuests`。
- イベントの依頼は `quest_` の `repeatable_` を **false** にする。達成すると掲示板で「達成済み」になり、報酬は1回だけ。
  `true`（既定）は常設の汎用依頼用で、達成のたびに報酬が出て、また受けられる。
- 湧き地点に期間の条件はない。イベントの湧き地点は、終わったらデータの更新で外す。ボスは本編で倒したあとも毎回湧く。

条件は汎用の `GameCore::Condition`（`PeriodCondition` / `StoryFlagCondition` などの `StoryConditions` / `QuestCompletedCondition` /
`DecorationOwnedCondition` / `AnyOfCondition` / `NotCondition`）。依頼・告知・ステージ・報酬・シーンの飾りで同じものを使う。

開催の判定に使う今の時刻は、`system_clock` を直接読まずに `GameCore::Condition::Clock::Now()`（`Condition_Clock.h`）から取る。
DebugSheet の **時間/開催日時** でこの時刻をずらせる（直接指定・±1時間/日・告知ごとの「開始1分前/開催中/終了1分前/終了後」）。
ずらしはメモリに持つだけで、起動し直すと戻る。手元の PC にだけ効く。Release のゲームビルドではコードごと消える。

- 報酬の種類を増やすとき: `IReward` を継承したクラスを 1 つ足し、`.cpp` の末尾で `REGISTER_REWARD` と
  `NANAMI_REGISTER_TYPE(T, GameCore::Reward::IReward)`。これだけは exe の更新が要る。
- 報酬の条件の例: 「初回だけ飾り」= `DecorationReward` に `NotCondition(DecorationOwnedCondition(同じ飾り))`。
  「期間中だけのおまけ」= `MoneyReward` に `PeriodCondition`。一覧の条件は、付与の前にまとめて判定する。
- 報酬がお金だけだったころのデータ（`ITakeableQuest` version 0 の `rewardMoney_`）は、読むときに `MoneyReward` 1 つへ読み替える。
  エディタで保存し直すと version 1（`rewards_`）で書かれる。`tools/art/desert_quests.py` は version 0 の形を書き換えるので、
  version 1 で保存した依頼には使えない。

## 4. 毎週の作り方

1. **飾り**: エディタで `Assets/Data/Decoration/` に `.decoration` を作り、名前・説明・アイコンを入れる。
2. **島に置く**: `MainIslandScene` に置き場所の GameObject を足し、`ConditionalObject` を付ける。
   `conditions_` に `DecorationOwnedCondition`（1 の飾り）、`prefab_` に飾りの見た目。
3. **告知**: `.eventNotice`（題名・タグ・期間・説明・バナー）を作り、`MainIslandEventBoard.eventBoard` の `notices_` に足す。
4. **依頼**: `.boardQuest` を作る（先週のものを使い回さず、毎週新しく作る = guid が変わる）。`event_` に 3 の告知、
   `quest_` に依頼の中身（`repeatable_` は false）、`rewards_` にお金と 1 の飾り。
   依頼主と文面は `docs/Story.md` §3 の口調に合わせる。掲示板の `quests_` に足す。
   大物出現・レイドは、既存ボスの prefab を複製して強くし、イベント用のステージで湧かせる（§2 の第3週）。
   本編のステージの湧き地点は変えない。
5. **お知らせ**: `.announcement` を作り、掲示板の `announcements_` に足す。
6. **確認**: エディタで掲示板を開き、期間内だけ依頼が出ること・報酬欄が「1,500 G ＋ 飾りの名前」になることを見る。
   DebugSheet の「ストーリー/島の飾り」で飾りを付け外しして、島の見た目が切り替わることを見る。
7. **配信**: ツールバーの *Asset Dist* で Version を上げて *Build Manifest* → *Diff vs Live* で差分がイベントのデータだけか確かめる →
   *Upload (Dry Run)* → *Upload (Release)*（`Packages/AssetUpdater/README.md`）。
