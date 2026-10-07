#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <string_view>

#include "SaveDataReset.h"
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"

namespace GamePlay::Debug
{
    namespace
    {
        void DrawResetAll()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            Widgets::Note("LocalPrefs の進行系セーブ (ストーリー・進行度・クエスト・記録帳・お知らせと依頼の既読・職業ステータス) を消して、新規プレイの状態に戻す。表示モード・リレーサーバー・ゲーム設定は残す。");
            Widgets::Note("プレイ中は Title に戻ってから消す。");

            if (SaveDataReset::IsPending())
                Widgets::Note("Title への移動待ち...");
            else if (Widgets::ConfirmButton("セーブを全部初期化", "ResetAll"))
                SaveDataReset::Request();

            Widgets::Header("消すファイル");
            const auto paths = SaveDataReset::TargetFilePaths();
            if (paths.empty())
                Widgets::Note("なし");

            const std::string_view prefix = NanamiEngine::Module::LocalPrefs::LOCAL_PREFS_DATA_FOLDER_PATH;
            for (const auto& path : paths)
            {
                const std::string_view view(path);
                Widgets::Note(view.starts_with(prefix) ? view.substr(prefix.size()) : view);
            }
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(SaveResetAll, "セーブ/全初期化", 0, GamePlay::Debug::DrawResetAll)
#endif
