#include "Enemy_Behaviour_Action_Flinch.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::Flinch::DoTick(const TickContext& context)
    {
        std::optional<Damage::FlinchPower>& pendingFlinchPower = context.PendingFlinchPower();
        const bool isFlinchHit = pendingFlinchPower && flinchResistance_.IsFlinchedBy(*pendingFlinchPower);
        pendingFlinchPower.reset();

        if (isFlinchHit)
        {
            isFlinching_ = true;
            during_secs_ = 0.0f;
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animatorSetParam_);

            if (isStopHorizontalMove_)
            {
                auto& rigidBody = context.EnemyRigidBody();
                rigidBody.SetLinearVelocity(glm::vec3(0.0f, rigidBody.LinearVelocity().y, 0.0f));
            }
        }

        if (!isFlinching_)
            return TickStatus::Failure;

        during_secs_ += Time::DeltaTime();
        if (during_secs_ < flinch_secs_)
            return TickStatus::Running;

        // NOTE: 明けた Tick は Failure で後ろの枝へ譲り、そのまま次の行動を始めさせる
        DoReset();
        return TickStatus::Failure;
    }

    void Action::Flinch::DoReset()
    {
        isFlinching_ = false;
        during_secs_ = 0.0f;
    }

    void Action::Flinch::DoDrawGui()
    {
        flinchResistance_.OnDrawInputField("flinchResistance_");
        ImGuiHelper::OnDrawInputField("animatorSetParam_", animatorSetParam_);
        ImGuiHelper::OnDrawInputField("flinch_secs_", flinch_secs_);
        ImGuiHelper::OnDrawInputField("isStopHorizontalMove_", isStopHorizontalMove_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::Flinch, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
