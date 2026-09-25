#include "Enemy_Behaviour_Action_OnDamage.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
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

        auto& damageStacks = *context.OnDamaged();
        while (!damageStacks.empty())
        {
            const auto& onDamaged = damageStacks.front();
            const int   damage = onDamaged->DamageValue();

            context.EnemyStatus()->Get().OnDamage(damage);

            if (knockbackForcePerDamage_ > 0.0f)
            {
                const glm::vec3 knockbackDirection = onDamaged->DamageDirection();
                const float knockbackSpeed = static_cast<float>(damage) * knockbackForcePerDamage_;
                glm::vec3 knockbackVelocity = knockbackDirection * knockbackSpeed;
                knockbackVelocity.y = context.EnemyRigidBody().LinearVelocity().y;
                context.EnemyRigidBody().SetLinearVelocity(knockbackVelocity);
            }

            damageStacks.pop();
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

    void Action::OnDamage::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("damageEffect_", damageEffectPrefab_);
        ImGuiHelper::OnDrawInputField("damageEffectOffset_", damageEffectOffset_);
        ImGuiHelper::OnDrawInputField("animatorSetParam_", animatorSetParam_);
        ImGuiHelper::OnDrawInputField("isOnDamagedReturnBehaviour_", isOnDamagedReturnBehaviour_);
        ImGuiHelper::OnDrawInputField("knockbackForcePerDamage_", knockbackForcePerDamage_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::OnDamage, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
