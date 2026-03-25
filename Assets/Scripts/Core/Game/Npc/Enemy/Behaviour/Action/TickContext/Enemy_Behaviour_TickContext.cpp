#include "Enemy_Behaviour_TickContext.h"

#include "../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../Engine/Module/Component/Collider/ColliderBase.h"
#include "../../../../../DamageContext/IDamageContext.h"
#include "../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../../PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../Status/EnemyStatus.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    TickContext::TickContext(
        const std::weak_ptr<GameObject::IGameObject>& enemyGameObject,
        const std::unique_ptr<Enemy::EnemyStatus>& enemyStatus,
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters,
        const std::shared_ptr<std::queue<std::unique_ptr<IDamageContext>>>& onDamagedStack)
            : enemyGameObject_(enemyGameObject)
            , enemyAnimator_  (enemyGameObject.lock()->Components().Catch<Component::Animator>())
            , enemyCollider_  (enemyGameObject.lock()->Components().Catch<Component::ColliderBase>())
            , enemyStatus_    (enemyStatus   )
            , parameters_     (parameters    )
            , onDamagedStack_ (onDamagedStack)
    {
        
    }

    TickContext::~TickContext() = default;

    GameObject::Transform& TickContext::EnemyTransform() const
    {
        return enemyGameObject_ .lock()->TransformRef();
    }

    IPlayerAvatar& TickContext::Player() const
    {
        //TODO: Network上の自身が操作しているPlayerを取得するように変更必須
        return *IPlayerAvatar::PlayerAvatars().at(0).lock();
    }

    const PlayerAvatar::IQuestGroup& TickContext::PlayerQuest() const
    {
        return Player().PlayerStatus().Quest();
    }
}
