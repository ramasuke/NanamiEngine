# UiFlow — メニュー画面の開閉と入力

`namespace NanamiEngine::UiFlow`。使う側は `Packages/UiFlow/UiFlow.h` だけ include する。

メニュー画面（画面を覆って操作を受け付けるもの）に共通の処理をまとめたもの。見た目の部品は
`Engine/Module/NanamiUI/` のままで、ここは扱わない。

| 部品 | 役割 |
|---|---|
| `UiScreen` | 画面 1 枚。開閉、最前面かどうか、開いている間の操作ロック |
| `ScreenStack` | 開いている画面の重なり |
| `UiInputReader` / `UiActionMap` | 決定・キャンセルなどの論理アクションを読む |
| `InputDevice` | いま使われている入力機器 |
| `DeviceHint` | 操作ヒントの札を入力機器に合わせて差し替える |

## 画面を作る

Presenter と同じ GameObject に `UiScreen` を付ける（prefab / scene に入れておく）。

| フィールド | 意味 |
|---|---|
| `screenId_` | 画面の名前。同じ名前の画面は同時に 1 枚しか開かない |
| `locksPlayerControl_` | 開いている間、プレイヤーの操作を止める（`Packages/ControlLock`） |
| `destroysOnClose_` | 閉じたら GameObject ごと破棄する。prefab を生成して開く画面は true、常駐する画面は false |
| `repeatDelay_secs_` / `repeatInterval_secs_` | 押し続けたときのリピート |

```cpp
void ShopPresenter::OnStart()
{
    screen_ = RequireComponent<UiFlow::UiScreen>();
    if (!screen_->Open())           // 既に開いている
    {
        Entity().lock()->OnDestroy();
        return;
    }
    ...
}

void ShopPresenter::OnUpdate()
{
    using UiFlow::UiAction;
    auto& input = screen_->Input();
    if (input.IsPressed(UiAction::Up))      model_->Cursor().Move(-1);
    if (input.IsRepeated(UiAction::Right))  ChangeQuantity(1);
    if (input.IsPressed(UiAction::Submit))  Purchase();
    if (input.IsPressed(UiAction::Cancel))  screen_->Close();
}
```

- 閉じずに破棄されても（シーンの切り替えなど）、`UiScreen` がスタックから外れてロックを返す。
- 開閉の音は Presenter が鳴らす。
- 常駐する画面は、閉じている間に開くキーを読むための `UiInputReader` を Presenter が別に持つ（`StageReturnPresenter`）。

## 画面の重なり

後から開いた画面が最前面になる。入力を受けるのは最前面の画面だけ。

| 通知 | いつ |
|---|---|
| `OnOpened()` / `OnClosed()` | 開いた / 閉じた |
| `OnCovered()` | 上に別の画面が開いた |
| `OnRevealed()` | 上の画面が閉じて、最前面に戻った |

下の画面は操作ロックを持ったままなので、上の画面を閉じてもプレイヤーは動かない。上の画面を閉じるのに使った入力は、
下の画面では一度離すまで無視される。

```cpp
screen_->OnCovered ().Subscribe([this](R4::Unit) { view_->Hide(); }).AddTo(this);
screen_->OnRevealed().Subscribe([this](R4::Unit) { view_->Open(); }).AddTo(this);
```

購読は必ず `AddTo(this)` する。

## 入力

`UiActionMap::Default()` の割り当て:

| アクション | キーボード | パッド |
|---|---|---|
| `Up` / `Down` / `Left` / `Right` | 矢印、W / S / A / D | 十字、左スティック（しきい値 12000） |
| `Submit` | Enter | A |
| `Cancel` | Esc | B |
| `TabPrev` / `TabNext` | Q / E | LB / RB |
| `Menu` | Esc | Start |
| `Erase` | Back | X |
| `ValueUp` / `ValueDown` | なし | なし |

`ValueUp` / `ValueDown` は、上下の移動と別に値を上げ下げする 2 組目の上下。使う画面が割り当てる。

画面ごとの違いは、開いた後に足す:

```cpp
screen_->Input().Map()
    .AddKey   (UiAction::Submit, Key::Space)
    .AddButton(UiAction::Cancel, GamepadButton::Start)
    .AddStick (UiAction::Up,     StickDirection::RightStickUp)
    .Set      (UiAction::Left,   { {}, { GamepadButton::DPadLeft }, { StickDirection::Left } });
```

スティックは `StickDirection` の `Up` / `Down` / `Left` / `Right`（左）と `RightStickUp` など（右）。1 つのアクションに複数付けられる。
画面の中で割り当てを変えるときは `Set` で置き換える（`StageSelectPresenter::ApplyInputMap`）。

| 問い合わせ | 返すもの |
|---|---|
| `IsPressed` | 押した瞬間 |
| `IsHeld` | 押している間 |
| `IsRepeated` | 押した瞬間と、押し続けて delay を過ぎてからは interval ごと |
| `IsAnyPressed` | キー・マウス・パッドのどれかを押した瞬間（「ボタンを押してください」） |
| `PressedDigit` | 押した瞬間の数字キー 0〜9。無ければ -1 |

- 問い合わせたときに 1 フレームに 1 回だけ読み直す。`Update` を呼ぶ必要はない。
- **`OnUpdate` から使う。** `OnFixedUpdate` は 1 フレームに 0 回のことも複数回のこともある。
- 開いた直後・下の画面へ戻った直後・2 フレーム以上読まれなかった後は、押されたままの入力を一度離すまで無視する。
- ウィンドウが非アクティブの間は何も押されていない扱い。

## マウス

`NanamiUi::Button` は、`UiScreen` の下（同じ GameObject か子孫）にあるとき、その画面が最前面の間だけ反応する。
`UiScreen` の下に無い Button は今までどおり常に反応する。
`ScreenStack::WantsCursor()` は最前面の画面の Button が動いている間だけ true で、ゲームのカーソル (`GameCursor`) はこれで出し入れする（画面外の HUD の Button では出ない）。

## 入力機器とヒント

`InputDevice::Current()` が、最後に触られた機器（`KeyboardMouse` / `Gamepad`）を返す。

`DeviceHint` は札の Renderer（`ImageRenderer` / `BlendImageRenderer`）と同じ GameObject に付け、`keyboardSprite_` /
`gamepadSprite_` を指す。その機器の Sprite が未設定なら、いまの絵のままにする。

2 枚の絵は同じ大きさにする。Renderer は絵の中心を Transform の位置に合わせるので、大きさが違うと札とラベルの間が変わる。
このプロジェクトの札の絵と取り付けは `tools/art/device_hint.py`（`docs/UIDesign.md` §5）。

## 描画順

`renderOrder` は今までどおり画面ごとに数値で決める。いま使っている帯:

| 帯 | 使っているもの |
|---|---|
| -900〜1100 | HUD |
| 3000 台 | タイトル |
| 7000〜7051 | 店 / 掲示板 / 会話 |
| 7600 台 | ステージから帰る貼り紙 |
| 7700 台 | 設定 |
| 8000 台 | ゲームオーバー |
| 9000 台 | アセット更新 / ロード画面 |
| 20000 | カーソル |

後から開く画面ほど大きい帯にする。

## 移行した画面

| 画面 | `screenId_` | ロック | 閉じたら破棄 | 開く / 閉じる |
|---|---|---|---|---|
| タイトル | Title | しない | しない | `OnStart` で開いたまま。設定・荷札が上に重なる |
| アセット更新 | AssetUpdate | しない | しない | 荷札が出ている間だけ開く（`SyncScreen`） |
| ゲームオーバー | GameOver | しない | しない | 全員倒れたら開き、遷移を頼んだら閉じる |
| ステージ選択 | StageSelect | する | する | 台座が prefab を生成して開く |
| 店 / 掲示板 / 設定 / キャラ選択 | Shop など | する | する | prefab を生成して開く |
| ステージから帰る | StageReturn | する | しない | 常駐。閉じている間は別の `UiInputReader` で開くキーを読む |

## ホットリロード

`GameModule::Reload` がシーンを破棄した後に `ScreenStack::Clear()` を呼ぶ。スタックは `UiScreen` を生ポインタで持ち、
`UiScreen` が破棄時に自分を外す。
