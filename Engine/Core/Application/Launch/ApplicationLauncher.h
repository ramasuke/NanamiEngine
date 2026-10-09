#pragma once
#include "Engine/Core/Api/NanamiApi.h"

// NOTE: exe 側の起動処理 (docs/HotReload.md §8)
namespace NanamiEngine::Core::Application::Launch
{
    // NOTE: -project <フォルダ> があればそこを作業ディレクトリにする。失敗ならダイアログを出して false
    NANAMI_API bool ApplyProjectArgument();
    // NOTE: -game <dll>、無ければ <exe 名>.dll をゲーム DLL として読む。失敗ならダイアログを出して false
    NANAMI_API bool LoadGameModule();
    // NOTE: APPLICATION_MODE のアプリを Run -> OnExit する。回復できない例外はログ + ダイアログを出して 1 を返す
    NANAMI_API int RunApplication();
}
