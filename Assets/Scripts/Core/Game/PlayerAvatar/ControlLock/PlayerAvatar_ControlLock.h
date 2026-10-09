#pragma once
#include <string_view>

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::PlayerAvatar
{
    // NOTE: Lock と Unlock を別々に呼ぶ相手用。owner が UnlockControlBy を呼ぶか破棄されるまで操作を止める
    // NOTE: tag が違えば同じ owner でも別のロック。同じ owner と tag で重ねて呼んでもロックは 1 つ
    void LockControlBy(NanamiEngine::Module::GameObject::IGameObject& owner, std::string_view tag);
    void UnlockControlBy(NanamiEngine::Module::GameObject::IGameObject& owner, std::string_view tag);
}
