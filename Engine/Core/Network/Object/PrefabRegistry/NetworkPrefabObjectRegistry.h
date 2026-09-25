#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <unordered_map>

#include "../../../../Module/Guid/Guid.h"

namespace NanamiEngine::Module::GameObject
{
    class PrefabGameObject;
}

namespace NanamiEngine::Core::Network
{
    class NANAMI_API PrefabObjectRegistry final
    {
    public:
        void Add(const std::weak_ptr<Module::GameObject::PrefabGameObject>& object);

        [[nodiscard]] std::weak_ptr<Module::GameObject::PrefabGameObject> Catch(const Guid& guid) const;
        /** @brief 期限切れの weak_ptr を全部捨てる (ゲーム DLL を外す前) */
        std::size_t PurgeExpired();

    private:
        std::unordered_map<Guid, std::weak_ptr<Module::GameObject::PrefabGameObject>, GuidHash> assets_;
    };
}
