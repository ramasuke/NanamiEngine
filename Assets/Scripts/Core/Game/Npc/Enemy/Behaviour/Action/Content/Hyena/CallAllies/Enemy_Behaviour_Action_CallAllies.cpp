#include "Enemy_Behaviour_Action_CallAllies.h"

#include <algorithm>
#include <random>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../../../../../../../../../GamePlay/Npc/Enemy/Hyena/GamePlay_Enemy_Hyena.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../Enemy_BehaviourTree.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::CallAllies::DoTick(const TickContext& context)
    {
        const float now = Time::CurrentTime();
        const bool isNewSight = lastTickedTime_secs_ < 0.0f
                             || now - lastTickedTime_secs_ > reDecideAfterSeconds_;
        lastTickedTime_secs_ = now;

        if (isNewSight)
        {
            isHowling_        = RollHowl();
            howlElapsed_secs_ = 0.0f;

            if (isHowling_)
                BeginHowl(context);
        }

        if (!isHowling_)
            return TickStatus::Success;

        howlElapsed_secs_ += Time::DeltaTime();
        if (howlElapsed_secs_ < howlSeconds_)
        {
            const glm::vec3 velocity = context.EnemyRigidBody().LinearVelocity();
            context.EnemyRigidBody().SetLinearVelocity({ 0.0f, velocity.y, 0.0f });
            return TickStatus::Running;
        }

        isHowling_ = false;
        return TickStatus::Success;
    }

    bool Action::CallAllies::RollHowl() const
    {
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_int_distribution dist(0, 99);

        return dist(rng) < std::clamp(successRate_, 0, 100);
    }

    void Action::CallAllies::BeginHowl(const TickContext& context) const
    {
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animatorSetParamNumber_);

        if (howlSound_)
        {
            const glm::vec3 position = context.EnemyTransform().GetWorldPos();
            GamePlay::Sound::SoundPlayer::PlaySe(*howlSound_.get(), position);

            // 権威側限定Tickなら、Tickしていない他ピアにも同じSEを鳴らさせる
            if (context.IsNetworkAuthority())
            {
                GameCore::Network::PlaySeRpc::Send(
                    context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, howlSound_->GetGuid(), position);
            }
        }

        AlertNearbyAllies(context);
    }

    void Action::CallAllies::AlertNearbyAllies(const TickContext& context) const
    {
        const auto factory = enemyFactory_.get();
        if (!factory)
            return;

        const auto      self    = context.EnemyGameObject().Components().Catch<GamePlay::Npc::Enemy::Hyena>().lock();
        const glm::vec3 selfPos = context.EnemyTransform().GetWorldPos();

        for (const auto& ally : factory->Hyenas().All())
        {
            if (ally == self)
                continue;

            const auto allyObject = ally->Entity().lock();
            if (!allyObject)
                continue;

            const glm::vec3 diff = allyObject->Transform().GetWorldPos() - selfPos;
            if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z > callRadius_ * callRadius_)
                continue;

            const auto tree = ally->BehaviourTree();
            if (!tree)
                continue;

            if (const auto alert = tree->Parameters().Catch<int>(alertKeyName_))
                alert->Set(alertValue_);
        }
    }

    void Action::CallAllies::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("successRate_", successRate_);
        ImGuiHelper::OnDrawInputField("callRadius_", callRadius_);
        ImGuiHelper::OnDrawInputField("animatorSetParamNumber_", animatorSetParamNumber_);
        ImGuiHelper::OnDrawInputField("howlSeconds_", howlSeconds_);
        ImGuiHelper::OnDrawInputField("alertKeyName_", alertKeyName_);
        ImGuiHelper::OnDrawInputField("alertValue_", alertValue_);
        ImGuiHelper::OnDrawInputField("reDecideAfterSeconds_", reDecideAfterSeconds_);
        ImGuiHelper::OnDrawInputField("enemyFactory_", enemyFactory_);
        ImGuiHelper::OnDrawInputField("howlSound_", howlSound_);
    }
}
