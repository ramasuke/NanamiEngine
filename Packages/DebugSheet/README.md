# DebugSheet — UnityDebugSheet 風のデバッグメニュー

`namespace NanamiEngine::DebugSheet`。使う側は `Packages/DebugSheet/DebugSheet.h` だけ include する。

ページを `"セーブ/全初期化"` のようなパスで登録し、カテゴリ一覧 → 子ページとページスタックで辿る（`<` で戻る）。
画面右端の固定パネルに、専用のスタイル（濃紺 + オレンジ、文字 1.3 倍、リストセル）で描くので、エディタの ImGui とは
見た目で区別がつく。スタイルは描画の間だけ Push/Pop するので、エディタ側のスタイルは変わらない。

## 有効な場所

`NANAMI_DEBUG_SHEET_ENABLED`（`DebugSheetConfig.h`）はエディタと **Debug** 構成のゲームビルドで 1、Release の
ゲームビルドで 0。F1 で開閉する。

エンジンからは呼ばない。常駐するゲーム側のコンポーネントから回す。このプロジェクトでは `GameCore::Game` が
`IUserInterfaceRenderable`（描画順は最大）を実装して呼んでいる:

```cpp
void Game::OnUserInterfaceRender()
{
#if NANAMI_DEBUG_SHEET_ENABLED
    auto& debugSheet = NanamiEngine::DebugSheet::Sheet::Instance();
    debugSheet.Update();   // F1
    debugSheet.Render();
#endif
}
```

- UI 描画はエディタの非プレイ中も回るので、編集モードでも開ける。ゲームが動いていないと使えないページは、その旨を出して return する。
- エディタでは `Render()` はエディタの ImGui フレームに描くだけ。
- ゲームビルドは ImGui を初期化していないので、初めて開いたときに `Render()` が ImGui を作り、以降は毎フレーム自前で
  `NewFrame` / `EndFrame` / `RenderVertex` / 描画を回す（閉じていても回す。回さないと入力がキューに溜まる）。開いている間だけ OS のカーソルを出す。

## ゲーム側のコードを変えない

デバッグのためにゲーム側の既存コードを変えない。ページは既存の公開 API だけで作る（例: ストーリーは LocalPrefs の
保存ファイルを書いて `StoryProgress::Reload()`）。どうしても口が要るときは、追加するもの（include・基底クラス・
メンバ・定義・呼び出し）を全部 `#if NANAMI_DEBUG_SHEET_ENABLED` で囲う。今あるのは `Game` の
`OnUserInterfaceRender` / `GetRenderOrder` と `OnUpdate` 内の `SaveDataReset::Update()`、`RecordBook::Reload()` だけ。

## ページの追加

ゲーム側の `.cpp`（ヘッダには書かない）で、Release から外すためにファイル全体を `#if NANAMI_DEBUG_SHEET_ENABLED` で囲み、
グローバルスコープで登録する:

```cpp
#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
namespace GamePlay::Debug
{
    namespace
    {
        void DrawGodMode()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            static bool isOn = false;
            Widgets::Toggle("無敵", isOn);
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(GodMode, "チート/無敵", 40, GamePlay::Debug::DrawGodMode)
#endif
```

- `ID` は翻訳単位内で一意。`PATH` は `/` で何段でも掘れる。`ORDER` は同じ階層内の並び順（小さい方が上）。カテゴリは子ページの最小の order の位置に並び、同じ order は名前順。
- 部品は `Widgets`（`Header` / `Note` / `Button` / `NavigationCell` / `Toggle` / `Label` / `ButtonRow` / `ConfirmButton` / `InputInt`）を使うとシートのデザインに揃う。素の `ImGui::` もシートのスタイルで描かれる。
- 取り返しのつかない操作は `ConfirmButton`（3 秒以内にもう一度押すと実行）にする。

## このプロジェクトのページ

`Assets/Scripts/GamePlay/Debug/DebugSheet/`:

- セーブ/全初期化（`SaveDataReset`。`LocalPrefs/` の `.json` から `Display/` `Network/` `Settings/` を除いて消す）
- ストーリー/フラグ
- シーン/移動（プレイ中のみ）
- チート/所持金・アイテム（手元のアバターは `GameCore::PlayerAvatar::Owner()`）
