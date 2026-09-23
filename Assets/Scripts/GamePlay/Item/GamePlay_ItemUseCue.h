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

namespace GamePlay::Item
{
    /** @brief 使った人の画面で呼ぶ。使用音と使い手に付いて行く演出を出し、他の画面へも同じものを送る */
    void PlayItemUseCue(const NanamiEngine::Module::Asset::ItemData& item,
                        const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& user);
}
