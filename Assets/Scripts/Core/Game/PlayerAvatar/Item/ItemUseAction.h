#pragma once
#include <memory>

namespace NanamiEngine::Module::Asset
{
    class ItemData;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::PlayerAvatar
{
    class ItemPouch;
}

namespace GameCore::PlayerAvatar::Item
{
    class IItemEffectTarget;

    // アイテムを使うモーション1回分。アバターの種類を問わず UseItem ステートが持つ
    class ItemUseAction final
    {
    public:
        /** @brief ポーチに預けられたアイテムを受け取る。無ければ何もしないまま IsFinished になる */
        void Begin(ItemPouch& pouch);
        /** @brief 効く時刻を過ぎたら一度だけ使い、演出を出す */
        void Update(float during_secs, ItemPouch& pouch, IItemEffectTarget& target,
                    const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& user);
        void End();

        [[nodiscard]] bool IsFinished(float during_secs) const;

    private:
        std::shared_ptr<NanamiEngine::Module::Asset::ItemData> item_;
        bool hasUsed_ = false;
    };
}
