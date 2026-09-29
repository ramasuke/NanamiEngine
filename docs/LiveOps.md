# 運営：毎週のイベント

毎週のイベントの型・開催時間・報酬と、その作り方。物語の上での位置づけは `docs/Story.md` §4.1。
決めたこと（2026-09-29、ユーザー決定）:

- 報酬は **島の飾り**（見た目だけ。持っているかは手元の PC にだけ保存し、マルチプレイでは共有しない）とお金。
- **誰でも参加できる**。イベントの依頼には、期間のほかに物語の進み具合の条件を付けない。
- 毎週の運営は **データだけ** で回す（exe の更新なし。`tools.dist` で R2 に上げれば届く）。

## 1. 開催時間

金曜 12:00 開始 → 翌金曜 04:59 終了（日本時間。前例の「草原の群狼 討伐週間」と同じ）。
お知らせ（`.announcement`）は開始の 2 日前（水曜）に出す。

## 2. ローテーション

4 つの型を順に回し、一巡したら敵・ステージ・飾りを入れ替えて再開催する（「第2回 群狼討伐」）。

| 週 | 型 | 例 | 中身 |
| --- | --- | --- | --- |
| 1 | 討伐週間 | 草原の群狼 / 水場の大サソリ / 砂の下の大ワーム | `DefeatRequestQuest`。対象の敵を N 体 |
| 2 | 納品・採集 | 隊商の荷あつめ / 島の資材集め | `CollectRequestQuest`。拾う物をステージのシーンに置く |
| 3 | 大物出現 | 怒れる大顎 / 砂嵐の骸竜 | 既存ボスの prefab を複製し、HP・攻撃・色を上げた強個体をステージに置く |
| 4 | 共闘レイド | 嵐を呼ぶ竜 | 強敵。マルチ向けだがソロでも倒せる難度にする |

- 新しく始めた人も遊べるよう、草原の敵の週を多めにする。砂漠の週・レイドは告知文に難しいことを書く。
- 新しい島（Story §4.1「浮かび上がった島々」）は、月 1〜季節の大型更新として別枠。そこで増えた敵がローテーションの弾になる。

## 3. 仕組み

| もの | 場所 | 役割 |
| --- | --- | --- |
| `EventNotice` | `Assets/Data/EventNotice/*.eventNotice` | 掲示板のイベント告知。期間 (`startAt_` / `endAt_`) と `unlockConditions_` |
| `BoardQuest` | `Assets/Data/EventNotice/*.boardQuest` | 依頼。`event_` を付けると、その期間だけ掲示板に出る |
| 報酬 `IReward` | `Assets/Scripts/Core/Game/Reward/` | 依頼の `rewards_`（一覧）。`MoneyReward` / `DecorationReward`。それぞれに `conditions_` を付けられる |
| `DecorationData` | `Assets/Data/Decoration/*.decoration` | 島の飾り 1 つ（名前・説明・アイコン） |
| `DecorationCollection` | `Assets/Scripts/Core/Game/Decoration/` | 持っている飾り（`LocalPrefs/GameProgression/Decorations`） |
| `DecorationOwnedCondition` | `Assets/Scripts/Core/Game/Condition/` | 飾りを持っていれば満たす条件 |
| `ConditionalObject` | `Assets/Scripts/GamePlay/Prop/ConditionalObject/` | シーンに置く。`conditions_` を満たす間だけ `prefab_` を子に出す / `target_` を有効にする |

条件は汎用の `GameCore::Condition`（`PeriodCondition` / `StoryFlagCondition` などの `StoryConditions` / `QuestCompletedCondition` /
`DecorationOwnedCondition` / `AnyOfCondition` / `NotCondition`）。依頼・告知・ステージ・報酬・シーンの飾りで同じものを使う。

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
4. **依頼**: `.boardQuest` を作る。`event_` に 3 の告知、`quest_` に依頼の中身、`rewards_` にお金と 1 の飾り。
   依頼主と文面は `docs/Story.md` §3 の口調に合わせる。掲示板の `quests_` に足す。
5. **お知らせ**: `.announcement` を作り、掲示板の `announcements_` に足す。
6. **確認**: エディタで掲示板を開き、期間内だけ依頼が出ること・報酬欄が「1,500 G ＋ 飾りの名前」になることを見る。
   DebugSheet の「ストーリー/島の飾り」で飾りを付け外しして、島の見た目が切り替わることを見る。
7. **配信**: `python -m tools.dist build --version <v>` → `python -m tools.dist diff` で差分がイベントのデータだけか確かめる →
   `python -m tools.dist upload`（CLAUDE.md「Asset distribution」）。
