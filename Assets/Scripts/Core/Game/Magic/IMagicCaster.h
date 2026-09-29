#pragma once
#include <memory>

#include "vec3.hpp"
#include "gtc/quaternion.hpp"
#include "Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace GameCore::Magic
{
    class IMagicCaster
    {
    public:
        virtual ~IMagicCaster() = default;
        [[nodiscard]] virtual std::shared_ptr<GameObject::IGameObject> CasterObject() const = 0;
        [[nodiscard]] virtual glm::vec3 CastOrigin() const = 0;
        [[nodiscard]] virtual glm::quat CastRotation() const = 0;
        [[nodiscard]] virtual std::weak_ptr<GameObject::IGameObject> AimTarget() const = 0;
        [[nodiscard]] virtual float SpellPowerRate() const = 0;
        [[nodiscard]] virtual Core::Network::NetworkObjectId CasterNetworkObjectId() const = 0;
        [[nodiscard]] virtual std::shared_ptr<Asset::PrefabGameObjectFile> DealDamageTextPrefab() const = 0;
    };
}
