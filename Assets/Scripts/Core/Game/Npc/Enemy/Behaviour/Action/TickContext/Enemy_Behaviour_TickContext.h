#pragma once
#include <memory>
#include <queue>
#include <string>

#include "Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "Engine/Core/Network/Object/Creator/NetworkParamCreator.h"
#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

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
    class RigidBody;
}

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace GameCore::Npc::Enemy
{
    class EnemyStatus;
    class IShowHealthGaugeProvider;
}

namespace GameCore
{
    struct IDamage;
}

namespace NanamiEngine::Module::Component
{
    class Animator;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GamePlay::Ui
{
    class NpcChatting;
}

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    struct TickContext final
    {
        explicit TickContext(
            const std::weak_ptr<GameObject::IGameObject>& enemyGameObject,
            SyncParam<EnemyStatus>& enemyStatus,
            const std::unique_ptr<BlackBoard::ParameterGroup>& parameters,
            const std::shared_ptr<std::queue<std::unique_ptr<IDamage>>>& onDamagedStack,
            IShowHealthGaugeProvider* showHealthGaugeProvider,
            Core::Network::NetworkObjectId networkObjectId,
            bool isNetworkAuthority);
        ~TickContext();
        

        [[nodiscard]] GameObject::IGameObject& EnemyGameObject() const { return *enemyGameObject_.lock(); }
        [[nodiscard]] GameObject::Transform  & EnemyTransform () const;
        [[nodiscard]] Component::Animator    & EnemyAnimator  () const { return *enemyAnimator_  .lock(); }
        [[nodiscard]] Component::RigidBody   & EnemyRigidBody () const { return *enemyRigidBody_ .lock(); }
        [[nodiscard]] SyncParam<EnemyStatus> & EnemyStatus    () const { return enemyStatus_; }
        [[nodiscard]] const std::unique_ptr<BlackBoard::ParameterGroup>& Parameter() const { return parameters_; }
        [[nodiscard]] const std::shared_ptr<std::queue<std::unique_ptr<IDamage>>>& OnDamaged() const { return onDamagedStack_; } 
        [[nodiscard]] bool IsOnDamage() const { return !onDamagedStack_->empty(); }
        // ボスHPゲージを持たない敵は nullptr
        [[nodiscard]] IShowHealthGaugeProvider* ShowHealthGaugeProvider() const { return showHealthGaugeProvider_; }
        [[nodiscard]] std::shared_ptr<IPlayerAvatar> Player() const;
        [[nodiscard]] static const std::vector<std::weak_ptr<IPlayerAvatar>>& AllPlayer();
        [[nodiscard]] const PlayerAvatar::IQuestGroup& PlayerQuest() const;
        [[nodiscard]] const PlayerAvatar::Quest::ICompleteQuestGroup& PlayerCompleteQuest() const;
        [[nodiscard]] const GamePlay::Ui::NpcChatting& ChatUi() const;

        // この敵の NetworkObjectId。ネットワーク生成されていない個体は Invalid()
        [[nodiscard]] Core::Network::NetworkObjectId NetworkObjectId() const { return networkObjectId_; }
        // このTickが権威側限定(他ピアはTickしていない)なら true。
        // 一回限りの副作用(SE/エフェクト/攻撃発火等)は、この時だけ RPC で他ピアへ複製する。
        [[nodiscard]] bool IsNetworkAuthority() const { return isNetworkAuthority_; }


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
        const std::weak_ptr<Component::RigidBody   > enemyRigidBody_;
        SyncParam<Enemy::EnemyStatus>&   enemyStatus_;
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters_;
        const std::shared_ptr<std::queue<std::unique_ptr<IDamage>>> onDamagedStack_;
        IShowHealthGaugeProvider* const showHealthGaugeProvider_;
        const Core::Network::NetworkObjectId networkObjectId_;
        const bool isNetworkAuthority_;
    };
}