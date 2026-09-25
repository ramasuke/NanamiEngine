#pragma once

// DxLib の非同期読み込みフラグの扱い (SetUseASyncLoadFlag / CheckHandleASyncLoad の DxLib を出さない入口)
namespace NanamiEngine::Platform::AsyncLoad
{
    /** 生きている間だけ非同期読み込みを切る。読み込み中のハンドルになると困る (定数バッファなど) ものを同期で作るときに使う */
    class SyncLoadScope final
    {
    public:
        SyncLoadScope();
        ~SyncLoadScope();
        SyncLoadScope(const SyncLoadScope&)            = delete;
        SyncLoadScope& operator=(const SyncLoadScope&) = delete;

    private:
        bool wasAsync_;
    };

    [[nodiscard]] bool IsEnabled();
    /** @brief ハンドルがまだ読み込み中か */
    [[nodiscard]] bool IsHandleLoading(int handle);
}
