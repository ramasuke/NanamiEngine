#pragma once
#include "Packages/DebugSheet/DebugSheetConfig.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <string>
#include <vector>

namespace GamePlay::Debug
{
    /**
     * @brief LocalPrefs の進行系セーブをまとめて消し、新規プレイの状態に戻す。表示モード・接続先などの設定は残す
     * @note  アバターが居ると遷移時の SaveStatus() が古い状態を書き戻すので、プレイ中は Title に戻ってから消す
     */
    class SaveDataReset final
    {
    public:
        /** @brief 今すぐ消せるなら消し、プレイ中なら Title へ戻してから消す */
        static void Request();
        /** @brief Title への遷移待ちを進める。Game::OnUpdate から呼ぶ */
        static void Update();
        [[nodiscard]] static bool IsPending() { return isPending_; }

        /** @brief 消す対象。LocalPrefs 以下の .json から設定のフォルダを除いたもの */
        [[nodiscard]] static std::vector<std::string> TargetFilePaths();

    private:
        static void ResetNow();

        static bool isPending_;
    };
}
#endif
