#include "Condition_Clock.h"

namespace GameCore::Condition::Clock
{
    namespace
    {
        std::chrono::sys_seconds SystemNow()
        {
            return std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        }

#if NANAMI_DEBUG_SHEET_ENABLED
        // NOTE: 起動ごとに戻す。保存しない
        std::chrono::seconds debugOffset{ 0 };

        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>& DebugNowChanged()
        {
            static NanamiEngine::R4::Subject<NanamiEngine::R4::Unit> subject;
            return subject;
        }

        void SetDebugOffset(const std::chrono::seconds offset)
        {
            debugOffset = offset;
            DebugNowChanged().OnNext(NanamiEngine::R4::Unit{});
        }
#endif
    }

    std::chrono::sys_seconds Now()
    {
#if NANAMI_DEBUG_SHEET_ENABLED
        return SystemNow() + debugOffset;
#else
        return SystemNow();
#endif
    }

#if NANAMI_DEBUG_SHEET_ENABLED
    void SetDebugNow(const std::chrono::sys_seconds target)
    {
        SetDebugOffset(target - SystemNow());
    }

    void ShiftDebugNow(const std::chrono::seconds delta)
    {
        SetDebugOffset(debugOffset + delta);
    }

    void ResetDebugNow()
    {
        SetDebugOffset(std::chrono::seconds{ 0 });
    }

    bool IsDebugNowActive()
    {
        return debugOffset != std::chrono::seconds{ 0 };
    }

    std::chrono::sys_seconds RealNow()
    {
        return SystemNow();
    }

    NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnDebugNowChanged()
    {
        return DebugNowChanged().AsObservable();
    }
#endif
}
