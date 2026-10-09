#include "Enemy_Behaviour_Action_HeartStun.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../../../../../../../../GamePlay/Prop/StormHeart/GamePlay_StormHeart.h"
#include "../../../../../../../../../GamePlay/Weather/Sandstorm.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::HeartStun::DoTick(const TickContext& context)
    {
        if (!isStunned_)
        {
            if (!GamePlay::Prop::StormHeart::ConsumeShaken())
                return TickStatus::Failure;

            isStunned_    = true;
            elapsed_secs_ = 0.0f;
            GamePlay::Weather::Sandstorm::EndSummoned();
            GamePlay::Prop::StormHeart::PlayShakenBurst();
            if (context.IsNetworkAuthority())
            {
                GameCore::Network::BossSandstormRpc::Send(
                    context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, false, true);
            }
        }

        elapsed_secs_ += Time::DeltaTime();

        auto& rigidBody = context.EnemyRigidBody();
        rigidBody.SetLinearVelocity(glm::vec3(0.0f, rigidBody.LinearVelocity().y, 0.0f));

        // NOTE: 親ノードから毎フレーム Tick し直されるので、倒れる・伏せる・起きるの切り替えはこのノードの中で進める
        const int state = elapsed_secs_ < start_secs_              ? stunStartState_
                        : elapsed_secs_ < start_secs_ + idle_secs_ ? stunIdleState_
                        :                                            stunOverState_;
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(state);

        if (elapsed_secs_ < start_secs_ + idle_secs_ + over_secs_)
            return TickStatus::Running;

        isStunned_ = false;
        return TickStatus::Success;
    }

    void Action::HeartStun::DoReset()
    {
        isStunned_ = false;
    }

    void Action::HeartStun::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stunStartState_", stunStartState_);
        ImGuiHelper::OnDrawInputField("stunIdleState_",  stunIdleState_ );
        ImGuiHelper::OnDrawInputField("stunOverState_",  stunOverState_ );
        ImGuiHelper::OnDrawInputField("start_secs_",     start_secs_    );
        ImGuiHelper::OnDrawInputField("idle_secs_",      idle_secs_     );
        ImGuiHelper::OnDrawInputField("over_secs_",      over_secs_     );
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::HeartStun, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
