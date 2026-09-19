#include "Enemy_Behaviour_Action_OnDamage.h"

#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../../../../../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../../../../../../../../../GamePlay/Npc/Enemy/BodyPart/GamePlay_Enemy_BodyPartWeakPoint.h"
#include "../../../../../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../../../Damage/Game_Damage_IDamage.h"
#include "../../../../../Status/EnemyStatus.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::OnDamage::DoTick(const TickContext& context)
    {
        if (!context.IsOnDamage())
            return TickStatus::Failure;

        const bool isStunned = IsStunned(context);
        bool       isStunTriggered = false;

        auto& damageStacks = *context.OnDamaged();
        while (!damageStacks.empty())
        {
            const auto& onDamaged = damageStacks.front();
            const int   rawDamage = onDamaged->DamageValue();
            const int   appliedDamage = isStunned
                ? static_cast<int>(static_cast<float>(rawDamage) * stunnedDamageScale_)
                : rawDamage;

            context.EnemyStatus()->Get().OnDamage(appliedDamage);

            if (TryTriggerStun(context, *onDamaged, rawDamage))
                isStunTriggered = true;

            // ダウン中のボスは滑らない
            if (!isStunned && knockbackForcePerDamage_ > 0.0f)
            {
                const glm::vec3 knockbackDirection = onDamaged->DamageDirection();
                const float knockbackSpeed = static_cast<float>(appliedDamage) * knockbackForcePerDamage_;
                glm::vec3 knockbackVelocity = knockbackDirection * knockbackSpeed;
                knockbackVelocity.y = context.EnemyRigidBody().LinearVelocity().y;
                context.EnemyRigidBody().SetLinearVelocity(knockbackVelocity);
            }

            damageStacks.pop();
        }

        if (isStunTriggered)
        {
            if (const auto stunState = context.Parameter()->Catch<int>(stunStateKeyName_))
                stunState->Set(1);
        }

        if (damageEffectPrefab_.get())
        {
            const glm::vec3 position = context.EnemyTransform().GetWorldPos() + damageEffectOffset_;
            GamePlay::Spawn::SpawnPrefab(*damageEffectPrefab_.get(), position, 0.0f);

            if (context.IsNetworkAuthority())
            {
                GameCore::Network::SpawnPrefabRpc::Send(
                    context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable,
                    damageEffectPrefab_->GetGuid(), position, 0.0f);
            }
        }
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animatorSetParam_);
        return isOnDamagedReturnBehaviour_ ? TickStatus::Success : TickStatus::Failure;
    }

    bool Action::OnDamage::IsStunned(const TickContext& context) const
    {
        if (stunStateKeyName_.empty())
            return false;

        const auto stunState = context.Parameter()->Catch<int>(stunStateKeyName_);
        return stunState && stunState->Get() != 0;
    }

    bool Action::OnDamage::TryTriggerStun(const TickContext& context, const IDamage& damage, const int rawDamage) const
    {
        const auto hitPart = damage.HitPart().lock();
        if (!hitPart)
            return false;

        const auto weakPoint = GamePlay::Npc::Enemy::BodyPartWeakPoint::FindFrom(hitPart, context.EnemyGameObject());
        if (!weakPoint)
            return false;

        const bool isChargeCounter = weakPoint->IsChargeCounter(damage.IsChargedAttack());
        const bool isJustBroken    = weakPoint->AccumulateDamage(rawDamage);

        if (stunStateKeyName_.empty())
            return false;

        return isChargeCounter || isJustBroken;
    }

    void Action::OnDamage::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("damageEffect_", damageEffectPrefab_);
        ImGuiHelper::OnDrawInputField("damageEffectOffset_", damageEffectOffset_);
        ImGuiHelper::OnDrawInputField("animatorSetParam_", animatorSetParam_);
        ImGuiHelper::OnDrawInputField("isOnDamagedReturnBehaviour_", isOnDamagedReturnBehaviour_);
        ImGuiHelper::OnDrawInputField("knockbackForcePerDamage_", knockbackForcePerDamage_);
        ImGuiHelper::OnDrawInputField("stunStateKeyName_", stunStateKeyName_);
        ImGuiHelper::OnDrawInputField("stunnedDamageScale_", stunnedDamageScale_);
    }
}
