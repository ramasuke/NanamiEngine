#pragma once
#include "Packages/DebugSheet/DebugSheetConfig.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <string>
#include <vector>

namespace GamePlay::Debug
{
    // NOTE: LocalPrefs の進行系セーブをまとめて消し、新規プレイの状態に戻す。表示モード・接続先などの設定は残す
    // NOTE: アバターが居ると遷移時の SaveStatus() が古い状態を書き戻すので、プレイ中は Title に戻ってから消す
    class SaveDataReset final
    {
    public:
        // NOTE: 今すぐ消せるなら消し、プレイ中なら Title へ戻してから消す
        static void Request();
        // NOTE: Title への遷移待ちを進める。毎フレーム呼ぶ
        static void Update();
        [[nodiscard]] static bool IsPending() { return isPending_; }

        // NOTE: 消す対象。LocalPrefs 以下の .json から設定のフォルダを除いたもの
        [[nodiscard]] static std::vector<std::string> TargetFilePaths();

    private:
        static void ResetNow();

        static bool isPending_;
    };
}
#endif
