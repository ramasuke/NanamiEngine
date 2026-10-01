#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Module::Asset
{
    /** @brief 初めて使われるときかシーンの先読みでハンドルを作るアセット */
    class NANAMI_API IPreloadableAsset
    {
    public:
        virtual ~IPreloadableAsset() = default;
        /** @brief 未読込なら、そのときの非同期読み込みフラグのまま読み込みを要求する */
        virtual void RequestLoad() const = 0;
        /** @brief 読み込んだハンドルを解放する。次に使われたら読み直す */
        virtual void Unload() = 0;
    };
}
