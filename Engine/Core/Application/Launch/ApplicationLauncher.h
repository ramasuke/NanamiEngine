#pragma once
#include "Engine/Core/Api/NanamiApi.h"

// exe の WinMain (Main.cpp) から呼ぶ起動処理。Main.cpp は exe に必須のものだけを持つ (docs/HotReload.md §8)
namespace NanamiEngine::Core::Application::Launch
{
    /** @brief -project <フォルダ> があればそこを作業ディレクトリにする。失敗ならダイアログを出して false */
    NANAMI_API bool ApplyProjectArgument();
    /** @brief -game <dll>、無ければ <exe 名>.dll をゲーム DLL として読む (docs/HotReload.md 段階 3)。失敗ならダイアログを出して false */
    NANAMI_API bool LoadGameModule();
    /** @brief APPLICATION_MODE のアプリを Run -> OnExit する。回復できない例外はログ + ダイアログを出して 1 を返す */
    NANAMI_API int RunApplication();
}
