#include "Enemy_Behaviour_Action_OnDamage.h"

#include "../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../DamageContext/IDamageContext.h"
#include "../../../../Status/EnemyStatus.h"

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
            context.EnemyStatus()->OnDamage(onDamaged->DamageValue());
            damageStacks.pop();
        }

        if (damageEffectPrefab_.get())
        {
            Scene::GameObject::Instantiate(*damageEffectPrefab_.get(), context.EnemyTransform().GetWorldPos() + damageEffectOffset_);
        }
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animatorSetParam_);
        return TickStatus::Success;
    }

    void Action::OnDamage::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("damageEffect_", damageEffectPrefab_);
        ImGuiHelper::OnDrawInputField("damageEffectOffset_", damageEffectOffset_);
        ImGuiHelper::OnDrawInputField("animatorSetParam_", animatorSetParam_);
    }
}
