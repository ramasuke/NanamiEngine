#include "PlayerAvatar_ControlLock.h"

#include <cstdint>
#include <string>

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Packages/ControlLock/ControlLock.h"

namespace GameCore::PlayerAvatar
{
    namespace
    {
        std::string MakeControlLockKey(const NanamiEngine::Module::GameObject::IGameObject& owner, const std::string_view tag)
        {
            return std::string(tag) + "/" + std::to_string(reinterpret_cast<std::uintptr_t>(&owner));
        }
    }

    void LockControlBy(NanamiEngine::Module::GameObject::IGameObject& owner, const std::string_view tag)
    {
        auto& service = NanamiEngine::ControlLock::Service::Instance();
        const std::string key = MakeControlLockKey(owner, tag);
        if (service.IsKeyHeld(key))
            return;

        // NOTE: Unlock まで届かずに owner が消えても返るよう、owner の Component の寿命に結ぶ
        const auto lifetime = owner.Components().Catch<NanamiEngine::Module::Component::ComponentBase>().lock();
        if (!lifetime)
            return;

        service.AcquireKeyed(key).AddTo(*lifetime);
    }

    void UnlockControlBy(NanamiEngine::Module::GameObject::IGameObject& owner, const std::string_view tag)
    {
        NanamiEngine::ControlLock::Service::Instance().ReleaseKeyed(MakeControlLockKey(owner, tag));
    }
}
