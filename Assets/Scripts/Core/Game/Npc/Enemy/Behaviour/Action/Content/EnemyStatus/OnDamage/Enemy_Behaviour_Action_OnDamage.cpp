#include "Enemy_Behaviour_Action_OnDamage.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../../../../../../../../../GamePlay/Npc/Enemy/BodyPart/GamePlay_Enemy_BodyPartWeakPoint.h"
#include "../../../../../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../../../Damage/Game_Damage_IDamage.h"
#include "../../../../../Status/EnemyStatus.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::OnDamage::DoTick(const TickContext& context)
    {
        if (!context.IsOnDamage())
            return TickStatus::Failure;

        const bool isStunned = IsStunned(context);
        int        stunStateValue = 0;

        auto& damageStacks = *context.OnDamaged();
        while (!damageStacks.empty())
        {
            const auto& onDamaged = damageStacks.front();
            const int   rawDamage = onDamaged->DamageValue();
            const int   appliedDamage = isStunned
                ? static_cast<int>(static_cast<float>(rawDamage) * stunnedDamageScale_)
                : rawDamage;

            context.EnemyStatus()->Get().OnDamage(appliedDamage);

            if (const int value = ResolveStunStateValue(context, *onDamaged, rawDamage); value != 0 && stunStateValue == 0)
                stunStateValue = value;

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

        // NOTE: 気絶中に別のスタンで上書きすると、進行中の Stun の経過時間が別の枝に持ち越されるので重ねない
        if (stunStateValue != 0 && !isStunned)
        {
            if (const auto stunState = context.Parameter()->Catch<int>(stunStateKeyName_))
                stunState->Set(stunStateValue);
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

    int Action::OnDamage::ResolveStunStateValue(const TickContext& context, const IDamage& damage, const int rawDamage) const
    {
        const auto hitPart = damage.HitPart().lock();
        if (!hitPart)
            return 0;

        const auto weakPoint = GamePlay::Npc::Enemy::BodyPartWeakPoint::FindFrom(hitPart, context.EnemyGameObject());
        if (!weakPoint)
            return 0;

        const bool isChargeCounter = weakPoint->IsChargeCounter(damage.IsChargedAttack());
        const bool isJustBroken    = weakPoint->AccumulateDamage(rawDamage);

        if (stunStateKeyName_.empty())
            return 0;

        if (isChargeCounter)
            return chargeCounterStunStateValue_;

        if (isJustBroken && weakPoint->IsStunOnBreak())
            return breakStunStateValue_;

        return 0;
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
        ImGuiHelper::OnDrawInputField("chargeCounterStunStateValue_", chargeCounterStunStateValue_);
        ImGuiHelper::OnDrawInputField("breakStunStateValue_", breakStunStateValue_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::OnDamage)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::OnDamage)
#pragma endregion
