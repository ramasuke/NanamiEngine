#include "Friendly_Behaviour_Action_GameObjectInstantaite.h"

#include "../../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../TickStatus/Friendly_Behaviour_TickStatus.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::GameObjectInstantiate::DoTick(const TickContext& context)
    {
        const auto npcPos = context.NpcTransform().GetWorldPos();
        const auto npcRot = context.NpcTransform().GetWorldRot();

        const glm::vec3 rotatedOffset = npcRot * offset_;

        const glm::vec3 spawnPos =
            useAbsolutePosition_
            ? offset_
            : npcPos + rotatedOffset;

        const auto instance = Scene::GameObject::Instantiate(
            prefab_.get(),
            spawnPos
        ).lock();

        // 回転設定
        if (instance)
        {
            if (inheritRotation_)
            {
                instance->TransformRef().SetWorldRot(npcRot);
            }
        }

        return TickStatus::Success;
    }

    void Action::GameObjectInstantiate::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("offset_", offset_);
        ImGuiHelper::OnDrawInputField("useAbsolutePosition_", useAbsolutePosition_);
        ImGuiHelper::OnDrawInputField("inheritRotation_", inheritRotation_);
        ImGuiHelper::OnDrawInputField("prefab_", prefab_);
    }

}
