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
    // 魔法を撃つ側。魔法の実装はこれだけを見るので、撃ち手がどのアバターかを知らない
    class IMagicCaster
    {
    public:
        virtual ~IMagicCaster() = default;
        [[nodiscard]] virtual std::shared_ptr<GameObject::IGameObject> CasterObject() const = 0;
        [[nodiscard]] virtual glm::vec3 CastOrigin() const = 0;
        [[nodiscard]] virtual glm::quat CastRotation() const = 0;
        /** @brief ロックオン中に狙っている先(部位を選んでいればその部位)。していなければ空 */
        [[nodiscard]] virtual std::weak_ptr<GameObject::IGameObject> AimTarget() const = 0;
        [[nodiscard]] virtual float SpellPowerRate() const = 0;
        [[nodiscard]] virtual Core::Network::NetworkObjectId CasterNetworkObjectId() const = 0;
        /** @brief 魔法が当たったときに撃ち手の画面に出すダメージ表記。出さないなら nullptr */
        [[nodiscard]] virtual std::shared_ptr<Asset::PrefabGameObjectFile> DealDamageTextPrefab() const = 0;
    };
}
