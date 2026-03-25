#include "OnDeath.h"

#include "../../../../../../Game.h"
#include "../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../Scene/Group/Main/GameMainSceneGroup.h"
#include "../../../../../../Scene/Title/TitleScene.h"
#include "../../../../Status/EnemyStatus.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::OnDeath::DoTick(const TickContext& context)
    {
        if (context.EnemyStatus()->Health().Value() > StatusParameter::Health(0))
            return TickStatus::Failure;
        
        context.EnemyGameObject().OnDestroy();
        Game::Instance().MainScene().RequestChangeScene<Scene::TitleScene>();
        return TickStatus::Success;
    }

    void Action::OnDeath::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animatorSetParam_", animatorSetParam_);
    }
}
