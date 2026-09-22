# UI デザインの約束ごと

新しいゲーム内 UI を作るときに、既存の画面と見た目・手触りを揃えるためのまとめ。
2026-09-22 時点の `Assets/Prefab/UI/*.prefab`、`Assets/Art/UI/*`、`tools/art/*.py`、
`Assets/Scripts/GamePlay/Ui/*` を調べて書いた。数値は実際の prefab / コードから拾ったもの。

---

## 1. 系統は2つ。新しい「画面」は「手で触れる物」系で作る

| 系統 | 使っている UI | 見た目 |
| --- | --- | --- |
| **A. 手で触れる物（酒場・冒険者の道具）** — 現行の主流 | キャラ選択（酒場の貼り紙）、イベント掲示板、店（品書きの黒板）、ポーズメニュー（冒険者の手帳）、ゲームオーバー（刻まれた石版）、タイトルのアセット更新（早馬の荷札） | 羊皮紙・木の板・釘・蝋封・朱の判子・真鍮・鉄板・麻紐・チョーク。インクは焦げ茶 |
| **B. HUD（戦闘中に常時出るもの）** | 操作ガイド（細い青緑の帯）、チュートリアル札、アイテム袋、魔法陣パレット、ボスの水晶ゲージ、ロックオン | 暗い青緑〜紫の半透明、白〜淡色の文字、光るアクセント |

- 画面を覆って操作を受け付けるもの（メニュー・確認・結果）は **A**。HUD は **B**。
- ステージ選択（青と金の既製品風）とロード画面（青いカード枠）、タイトルのボタン（ドット絵の木札）は
  A/B どちらの語彙にも乗っていない古い・仮の画面。**真似しない**。
- 全 UI に共通の禁止事項（`tools/art/*.py` の docstring で毎回確認されている方針）:
  **「平らな角丸パネルに細い発光枠」という既製品っぽい見た目にしない。**

## 2. 系統 A の素材

素材はすべて `tools/art/character_select.py` の関数を fbm ノイズで焼いて作る（手描きや外部素材ではない）。
新しい UI もここから import して使う（`shop.py` / `event_board.py` / `pause_menu.py` がそうしている）。

| 素材 | 関数 | 使いどころ（既存例） |
| --- | --- | --- |
| 羊皮紙 | `parchment(w, h, seed, aged=0..1, ragged=13)` | 貼り紙・勘定書き・手帳の頁・掲示の紙。`aged` を上げると古び・無効の表現（`Bill_Locked` は 0.72） |
| 酒場の板 | `wood_board(w, h, seed, plank=138)` | 掲示板・キャラ選択の背板。`plank=9999` で継ぎ目なしの一枚板（看板・ヒント札の地） |
| 釘 | `nail(size=22)` | 紙を板に留める。紙の上端中央や四隅 |
| 蝋封 | `wax_seal(size=82)` | 選択中の印、封書 |
| 判子（朱） | `stamp_sprite(label, px=46, angle=-8)` | 状態の表示：「毎度」「受注中」「募集前」「更新」など。斜めに押す |
| 鉄板 | `iron_plate(w, h, notch=26)` + `engrave()` | 両端の絞れた鉄札。文字は彫り込み |
| 真鍮の環 | `brass_ring(size, thickness)` | 既存 HUD のポートレート枠に合わせた環 |
| 麻紐・縄 | `hanging_rope(w, h)`、`Shop_Twine.png` | 看板や勘定書きを吊るす |
| 黒板とチョーク | `shop.py`（`Shop_Board`、`ChalkGrain` を文字の上に重ねてかすれを出す） | 品書き。チョーク文字は TextRenderer で描く |
| 案内の木札 | `hint_tag(w, h, label)` | 操作ヒント（§5） |
| 看板 | `tavern_sign(w, h, label)` | 画面の見出し（「酒 場 — 仲 間 を 誘 う」） |

作り方の癖:

- 縁は `ragged` / fbm でぎざぎざに削る。直線の矩形にしない。
- 紙や板は **±1〜3° 傾ける**（`paste_tilted`、キャラ選択の貼り紙は -2.4° / 1.9° / -1.2°）。機械的な整列を崩す。
- ただし **文字は必ず水平**。エンジンの `TextRenderer` は文字を回転できないので、傾けるのは紙と飾りだけ。
  判子の文字だけはスプライトに焼き込むので傾けてよい。
- 部品の下には `drop_shadow(blur_r=10, offset=(6, 9), alpha=0.62)` 相当の影。
- 背景のゲーム画面は暗くする（`tavern_grade`：暗く＋ろうそく色の光、`pause_menu.dim`：暗く＋ぼかし＋周辺減光＋暖色）。
  ゲーム内では `Assets/Art/UI/BlackMask.png` や `PauseMenu/Backdrop.png` を ImageRenderer で敷く。

## 3. 色

系統 A の文字色（prefab の `TextRenderer::textColor_` を集計、多い順）:

| 役割 | 色 | 定数（`character_select.py`） | 主な用途 |
| --- | --- | --- | --- |
| 墨（本文・見出し） | `#301e14` (48,30,20) | `INK` | 紙の上の文字全般。最も多い（49箇所） |
| 薄墨（補足・ラベル） | `#684e36` (104,78,54) | `INK_FADE` | 「期 間」「体 力」「所持金」などのラベル、空の案内文、罫線 |
| 朱（判子・警告） | `#92261e` (146,38,30) | `STAMP_RED` | 判子、「お金が足りない」などの断り |
| 濃い墨（キャラ選択） | `#3a2416` / `#503824` | — | 貼り紙の名前・読み |
| 暗い地の上の生成り | `#f0e2ca` (240,226,202) | `HINT_COLOR`（shop.py） | 操作ヒントのラベル（12箇所）。`#eee0c6` `#e2d6c0` も同系 |
| チョーク | `#f0eee4` / 淡 `#c8d0c4` / 黄 `#fae296` / 赤 `#d67868` | `CHALK*`（shop.py） | 黒板の上だけ |
| 見出しの金茶 | `#96541a` | — | 依頼の報酬など、強調を1箇所だけ |

素材の基準色（0..1、`character_select.py`）: 羊皮紙 `PARCH (0.76,0.68,0.52)` / 古び `PARCH_OLD (0.62,0.54,0.40)`、
蝋 `WAX (0.46,0.09,0.08)`、真鍮 `BRASS (0.62,0.45,0.17)`、鉄 `STEEL (0.17,0.18,0.20)`、ろうそく光 `CANDLE (1.0,0.74,0.38)`。

系統 B（HUD）は別の色: 操作ガイドの地 `#06141a` に文字 `#fffff7`、青緑アクセント `#38d6c4`、進行中の金 `#ffce68`、
魔法パレットの紫 `#cebaff` / `#d6c0ff`、数値の白 `#ffffff` / 淡 `#d2e4df`。

## 4. フォント

`Assets/Art/Font/` の3つ（アセットの `size_` はどれも 60。見た目の大きさは Transform の scale で決める）。

| フォント | guid | 使い方 |
| --- | --- | --- |
| **Zen Old Mincho Bold** (`ZenOldMincho-Bold.ttf`) | `02951627-F120-4EDC-B4F7-C27985C7F643` | 系統 A の **本文・ラベル・操作ヒント**。一番多い |
| **怨霊** (`onryou.ttf`、縁取り 3px `#06141a`) | `30487603-1A70-4739-978D-5CF0105A60D9` | **手書き風の見出し・名前・数値**（「冒 険 者 の 手 帳」「剣士」、HP の数字、ダメージ数値、ロード画面） |
| IPA明朝 (`ipam.ttf`) | `C48F5FF6-C374-4289-A3BF-3BAF6C9C24EC` | 手帳の本文、ゲームオーバー、系統 B の HUD、古い画面 |

- 見出しは **全角スペースで字間を空ける**（「冒 険 者 の 手 帳」「目 次」「期 間」「体 力」）。
- モックを描く `text()` は `BODY_FONT` = Zen Old Mincho、`BRUSH_FONT` = 怨霊。

## 5. 操作ヒント

- 画面の **右下** に横一列。並びは「選ぶ → 決める → やめる / 閉じる」（`character_select.hint_strip`）。
- 札は木の小札 `hint_tag`（1文字なら 46×42、`◀▶` / `▲▼` なら 68×42）、中の文字は Zen Old 22px `#fae4ba`。
- 札の右にラベル（Zen Old 26px、`#f0e2ca`、影 (2,2) 黒 210/255）。札とラベルの間 16px、組と組の間 30px。
- マウスでも答えられる画面（タイトルなど）は、ヒントの組ごとに `NanamiUi::Button`（`eventAreaSize_` は中心からの半分の幅・高さ）を
  重ねてクリックも受ける（`AssetUpdateUI.prefab` の `Hints/Confirm/Click`）。
- 既存のヒント札: `CharacterSelect/HintTag_{Move,Confirm,Cancel}.png`、`EventBoard/HintTag_{LB,RB,LBRB,UpDown}.png`、
  `PauseMenu/Hint_{At,Enter,UpDown}.png`。同じ用途なら作り直さず使い回す（店もこれらを参照している）。
- ラベルの語彙: **選ぶ / 決める / やめる / 閉じる / 切り替え**、画面ごとの動詞（買う・受注する・個数）。
  ゲームオーバーだけは鉄の語彙なので「選ぶ / 決める」を `#c6ced4` で出している。

## 6. 文言

- 画面の中の人（酒場の主人・運営係・行商人）が書いたように書く。システム語を避ける。
  例: 「お勘定」「単価 / 個数 / 合計」「毎度」「入荷待ち」「受注中」「募集前」「いま貼り出されている依頼はありません」
  「— 酒場 運営係」「力尽きた」「もう一度挑む」「まだ雇えない。」
- 英字は読み仮名的な添え（「SWORDMAN」）だけ。ボタン名を英語にしない。
- 断りや失敗は短く、朱で（「これ以上持てない」「お金が足りない」）。

## 7. 動き

大きく派手に動かさない。短い時間で「置く・押す・揺れる」程度。数値はすべて `[[serialize(0)]]` のフィールドにして
インスペクタから調整できるようにする（命名: 秒は `_secs_`、ピクセルは `_px_`、割合は `Rate_`、倍率は `Scale_`）。

| 動き | 既存の値 |
| --- | --- |
| 選択中の拡大 | `selectedScale_` 1.05〜1.08、選ばれた札が少し下がる `selectedDrop_px_` 8 |
| 行のフェード・頁の切り替え | `rowFadeDuration_secs_` 0.15、`pageSwapDuration_secs_` 0.15、`fadeDuration_secs_` 0.2〜0.25 |
| 判子を押す | `stampDuration_secs_` 0.6、`stampStartScale_` 1.6 → 1.0。押し込み 20% → 残る → 最後の 30% で薄れる（`ShopReceipt::OnUpdate`） |
| 選択の脈動 | `selectPulseDuration_secs_` 0.3、`focusPulsePeriod_secs_` 0.9 |
| 無効の表現 | ImageRenderer の `SetBlendRate`（`markInactiveBlendRate_`）、羊皮紙の `aged` |
| 進み具合の表示 | `progressFollowRate_` 6.0 で指数的に追従し、後戻りさせない（`LoadingScreenUi::UpdateProgress`） |

## 8. 実装の型

```
<Name>Presenter  入力・開閉・Model との橋渡し（ComponentBase + IStartable/IUpdatable）
<Name>Ui         見た目のまとめ役。FIELD で ImageRenderer / TextRenderer / 子 prefab を参照し、Bind(model) で書く
Model/           状態とカーソル（BoardListCursor など）。描画を知らない
Row/ Page/ ...   繰り返す部品。自分の prefab を持ち、Ui が表示窓の分だけ生やす
```

- 入力はキーボードと XInput を両方読む（`ReadKeys()`、スティックのしきい値 12000）。前フレームと比べて押した瞬間だけ拾い、
  **開いた直後は押しっぱなしを拾わない**（`previousKeys_ = ReadKeys()` を OnStart で）。
- 開いている間はプレイヤーの `DisableStateMachine()`。閉じるのは次のフレームにして、閉じた B をジャンプとして拾わせない。
- 二重に開かないよう static の `isOpen_` で弾く（`ShopPresenter`）。
- 効果音は `FIELD(Asset::SoundFile)`（`cursorSound_` / `acceptSound_` / `refuseSound_` …）。合成は `tools/art/*_sfx.py`。

## 9. 作る手順（既存 UI はすべてこの流れ）

1. `tools/art/<ui>.py` に部品の生成と `--shot <実画面.png> --out-dir <dir>` のモック合成を書く。
   **実際のゲーム画面に合成した 1920×1080** で、2〜3案を並べてユーザーに選んでもらう
   （ポーズメニューは 手帳 / 真鍮の輪 / 鉄の見出し の3案から「冒険者の手帳」、ゲームオーバーは案A「刻まれた石版」、
   アセット更新は 吊り荷札 / 送り状と荷札 / 隅の小荷札 の3案から「吊り荷札」）。
   決定後は `--preview` のように、**prefab と同じ `LAYOUT` からモックを描き直す**と、見た目と実装がずれない
   （`tools/art/asset_update.py`。エンジンは背景をぼかせないので、プレビューも `Backdrop` / `BlackMask` を敷くだけにする）。
2. 決まったら `--emit` で `Assets/Art/UI/<Ui>/` に書き出す。`write_sprite()` は既存 `.meta` の GUID を保つので、
   作り直しても参照は切れない。
3. `tools/art/<ui>_prefab.py` が同じ `LAYOUT` / `layout()` を読んで prefab を組む。モックと同じ座標がそのままゲームに出る。
4. 新しい .cpp / .h は `.vcxproj` に手で足す。Component を足したら `python -m tools.scene regen-catalog`。
