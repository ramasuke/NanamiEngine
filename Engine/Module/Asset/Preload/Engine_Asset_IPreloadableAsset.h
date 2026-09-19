#pragma once

namespace NanamiEngine::Module::Asset
{
    /**
     * @brief 起動時には読まず、初めて使われるときかシーンの先読みで DxLib のハンドルを作るアセット。
     *        AssetPreloader が先読みと解放に使う
     */
    class IPreloadableAsset
    {
    public:
        virtual ~IPreloadableAsset() = default;
        /** @brief 未読込なら、そのときの非同期読み込みフラグのまま読み込みを要求する */
        virtual void RequestLoad() const = 0;
        /** @brief 読み込んだハンドルを解放する。次に使われたら読み直す */
        virtual void Unload() = 0;
    };
}
