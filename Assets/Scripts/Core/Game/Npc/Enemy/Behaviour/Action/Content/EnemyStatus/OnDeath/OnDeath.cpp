#include "OnDeath.h"

#include "../../../../../../../Game.h"
#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../../../Scene/Main/Content/Title/TitleScene.h"
#include "../../../../../Status/EnemyStatus.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::OnDeath::DoTick(const TickContext& context)
    {
        if (context.EnemyStatus()->Get().Health() > StatusParameter::Health(0))
            return TickStatus::Failure;
        
        context.EnemyGameObject().OnDestroy();
        return TickStatus::Success;
    }

    void Action::OnDeath::DoDrawGui()
    {   
        ImGuiHelper::OnDrawInputField("animatorSetParam_", animatorSetParam_);
    }
}
