#pragma once
#include <memory>
#include <string>

#include "vec3.hpp"
#include "Engine/Module/Guid/Guid.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Npc::Enemy
{
    /**
     * @brief 敵の攻撃予兆の見た目と音。どのボーンに出すかは攻撃ごとに呼ぶ側が決める
     * @note 他のピアは WarningGuid() で同じ provider を ObjectRegistry から引いて PlayWarning する
     */
    class IEnemyWarningEffectProvider
    {
    public:
        virtual ~IEnemyWarningEffectProvider() = default;

        /** 攻撃が当たる何秒前に出すか */
        [[nodiscard]] virtual float WarningLead_secs() const = 0;

        [[nodiscard]] virtual const Guid& WarningGuid() const = 0;

        /** enemyのボーン boneNameに予兆を出す。*/
        virtual void PlayWarning(
            const std::shared_ptr<GameObject::IGameObject>& enemy,
            const std::string& boneName,
            const glm::vec3& boneOffset) const = 0;
    };
}
