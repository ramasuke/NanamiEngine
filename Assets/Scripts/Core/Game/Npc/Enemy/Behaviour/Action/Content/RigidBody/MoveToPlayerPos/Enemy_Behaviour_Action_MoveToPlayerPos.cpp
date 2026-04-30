#include "Enemy_Behaviour_Action_MoveToPlayerPos.h"

#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../../Engine/Module/Component/Collider/ColliderBase.h"
#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Physics_.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::MoveToPlayerPos::DoTick(const TickContext& context)
    {
        const glm::vec3 selfPos   = context.EnemyTransform().GetWorldPos();
        const glm::vec3 playerPos = context.Player()->PlayerTransform().GetWorldPos();

        glm::vec3 moveDirection = playerPos - selfPos;
        moveDirection.y = 0.0f;

        const float distSq = glm::length2(moveDirection);
        if (const float startDistSq = moveStartDistance_ * moveStartDistance_; distSq <= startDistSq)
            return TickStatus::Failure;

        moveDirection = glm::normalize(moveDirection);
        glm::vec3 moveVelocity = moveDirection * moveSpeed_;
        moveVelocity.y = Physics::GetLinearVelocity(context.EnemyCollider().BodyId()).y; 
        Physics::SetLinearVelocity(context.EnemyCollider().BodyId(), moveVelocity);
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationNumber_);

        return TickStatus::Running;
    }

    void Action::MoveToPlayerPos::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
        ImGuiHelper::OnDrawInputField("moveStartDistance_", moveStartDistance_);
        ImGuiHelper::OnDrawInputField("animationNumber_", animationNumber_);
    }
}
