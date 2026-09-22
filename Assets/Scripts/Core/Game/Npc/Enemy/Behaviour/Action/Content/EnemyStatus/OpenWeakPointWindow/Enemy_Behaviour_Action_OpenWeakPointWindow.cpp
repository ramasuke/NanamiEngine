#include "Enemy_Behaviour_Action_OpenWeakPointWindow.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../../../GamePlay/Npc/Enemy/BodyPart/GamePlay_Enemy_BodyPartWeakPoint.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::OpenWeakPointWindow::DoTick(const TickContext& context)
    {
        // 閉じるノードは置かない。スタンで枝が中断されても部位側の時限で必ず閉じる
        const auto weakPointObject = context.EnemyTransform().CatchChild(weakPointObjectName_);
        if (!weakPointObject)
            return TickStatus::Success;

        if (const auto weakPoint = weakPointObject->Components().Catch<GamePlay::Npc::Enemy::BodyPartWeakPoint>().lock())
            weakPoint->OpenWeakWindow(duration_secs_);

        return TickStatus::Success;
    }

    void Action::OpenWeakPointWindow::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("weakPointObjectName_", weakPointObjectName_);
        ImGuiHelper::OnDrawInputField("duration_secs_", duration_secs_);
    }
}
