#pragma once
#include "Engine/Core/Api/NanamiApi.h"

// NOTE: DxLib の非同期読み込みフラグの扱い (SetUseASyncLoadFlag / CheckHandleASyncLoad の DxLib を出さない入口)
namespace NanamiEngine::Platform::AsyncLoad
{
    // NOTE: 生きている間だけ非同期読み込みを切る。読み込み中のハンドルになると困るものを同期で作るときに使う
    class NANAMI_API SyncLoadScope final
    {
    public:
        SyncLoadScope();
        ~SyncLoadScope();
        SyncLoadScope(const SyncLoadScope&)            = delete;
        SyncLoadScope& operator=(const SyncLoadScope&) = delete;

    private:
        bool wasAsync_;
    };

    [[nodiscard]] NANAMI_API bool IsEnabled();
    // NOTE: ハンドルがまだ読み込み中か
    [[nodiscard]] NANAMI_API bool IsHandleLoading(int handle);
}
