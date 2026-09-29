#pragma once
#include <string_view>

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::PlayerAvatar
{
    /**
     * @brief Lock と Unlock が別々に呼ばれる相手 (BT のノード、RPC) 用。owner が UnlockControlBy を呼ぶか、破棄されるまで操作を止める
     * @param tag 何のロックか。同じ owner でも tag が違えば別のロックになる
     * @note  同じ owner と tag で重ねて呼んでも、ロックは 1 つ
     */
    void LockControlBy(NanamiEngine::Module::GameObject::IGameObject& owner, std::string_view tag);
    void UnlockControlBy(NanamiEngine::Module::GameObject::IGameObject& owner, std::string_view tag);
}
