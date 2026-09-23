#include "Enemy_Behaviour_Action_MoveToPlayerPos.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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
        moveVelocity.y = context.EnemyRigidBody().LinearVelocity().y; 
        context.EnemyRigidBody().SetLinearVelocity(moveVelocity);
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

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::MoveToPlayerPos)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::MoveToPlayerPos
)
#pragma endregion
