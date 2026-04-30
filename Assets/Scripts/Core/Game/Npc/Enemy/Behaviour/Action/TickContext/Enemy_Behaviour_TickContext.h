#pragma once
#include <memory>
#include <queue>
#include <string>

#include "../../../../../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar
{
    class IQuestGroup;
}

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace GameCore
{
    class IPlayerAvatar;
}

namespace NanamiEngine::Module::Component
{
    class ColliderBase;
}

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace GameCore::Npc::Enemy
{
    class EnemyStatus;
}

namespace GameCore
{
    struct IDamageContext;
}

namespace NanamiEngine::Module::Component
{
    class Animator;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    struct TickContext final
    {
        explicit TickContext(
            const std::weak_ptr<GameObject::IGameObject>& enemyGameObject,
            const std::unique_ptr<EnemyStatus>& enemyStatus,
            const std::unique_ptr<BlackBoard::ParameterGroup>& parameters,
            const std::shared_ptr<std::queue<std::unique_ptr<IDamageContext>>>& onDamagedStack);
        ~TickContext();
        

        [[nodiscard]] GameObject::IGameObject& EnemyGameObject() const { return *enemyGameObject_.lock(); }
        [[nodiscard]] GameObject::Transform  & EnemyTransform () const;
        [[nodiscard]] Component::Animator    & EnemyAnimator  () const { return *enemyAnimator_  .lock(); }
        [[nodiscard]] Component::ColliderBase& EnemyCollider  () const { return *enemyCollider_  .lock(); }
        [[nodiscard]] EnemyStatus& EnemyStatus() const { return *enemyStatus_; }
        [[nodiscard]] const std::unique_ptr<BlackBoard::ParameterGroup>& Parameter() const { return parameters_; }
        [[nodiscard]] const std::shared_ptr<std::queue<std::unique_ptr<IDamageContext>>>& OnDamaged() const { return onDamagedStack_; } 
        [[nodiscard]] bool IsOnDamage() const { return !onDamagedStack_->empty(); }
        [[nodiscard]] std::shared_ptr<IPlayerAvatar> Player() const;
        [[nodiscard]] const PlayerAvatar::IQuestGroup& PlayerQuest() const;

        
        template<typename T>
        [[nodiscard]] T& CatchPrefabObject(const std::string& catchObjectName) const
        {
            for (const auto& child : EnemyTransform().GetAllChildren())
            {
                if (child->Name() != catchObjectName)
                    continue;

                const auto object = child->Components().Catch<T>().lock();
                assert(object, "Object has not T");
                
                return *object;
            }
            throw std::exception(("Object has not (object name:" + catchObjectName + ")").c_str());
        }

    private:
        const std::weak_ptr<GameObject::IGameObject> enemyGameObject_;
        const std::weak_ptr<Component::Animator    > enemyAnimator_;
        const std::weak_ptr<Component::ColliderBase> enemyCollider_;
        const std::unique_ptr<Enemy::EnemyStatus>&   enemyStatus_;
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters_;
        const std::shared_ptr<std::queue<std::unique_ptr<IDamageContext>>> onDamagedStack_;
    };
}