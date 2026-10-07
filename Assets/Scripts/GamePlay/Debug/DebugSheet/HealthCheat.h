#pragma once
#include "Packages/DebugSheet/DebugSheetConfig.h"

#if NANAMI_DEBUG_SHEET_ENABLED
namespace GamePlay::Debug
{
    /**
     * @brief 手元のアバターの体力が減ったら毎フレーム最大まで戻す (疑似無敵)
     * @note  体力を 0 にする一撃は、戻す前に倒れる判定が済むので防げない
     */
    class HealthCheat final
    {
    public:
        /** @brief Game::OnUpdate から呼ぶ */
        static void Update();

        [[nodiscard]] static bool& IsKeepFullHealth() { return isKeepFullHealth_; }

    private:
        static bool isKeepFullHealth_;
    };
}
#endif
