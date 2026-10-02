#pragma once
#include <chrono>

#include "Packages/DebugSheet/DebugSheetConfig.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include "Packages/R4/R4.h"
#endif

namespace GameCore::Condition::Clock
{
    /** @brief 開催判定に使う今の時刻(UTC)。system_clock を直接読まずにこれを使う */
    [[nodiscard]] std::chrono::sys_seconds Now();

#if NANAMI_DEBUG_SHEET_ENABLED
    /** @brief Now() が target になるようにずらす。ずらした後も時計は進む */
    void SetDebugNow(std::chrono::sys_seconds target);
    void ShiftDebugNow(std::chrono::seconds delta);
    void ResetDebugNow();
    [[nodiscard]] bool IsDebugNowActive();
    /** @brief ずらす前の時刻 */
    [[nodiscard]] std::chrono::sys_seconds RealNow();
    /** @brief ずらし方を変えたとき */
    [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnDebugNowChanged();
#endif
}
