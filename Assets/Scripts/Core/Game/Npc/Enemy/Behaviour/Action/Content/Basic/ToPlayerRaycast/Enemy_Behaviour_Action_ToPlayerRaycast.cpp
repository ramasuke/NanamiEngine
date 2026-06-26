#include "Enemy_Behaviour_Action_ToPlayerRaycast.h"

#include <string>

#include "../../../../../../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ToPlayerRaycast::DoTick(const TickContext& context)
    {
        const glm::vec3 selfPos = context.EnemyTransform().GetWorldPos() + glm::vec3(0.0f, offsetY_, 0.0f);

        Physics::LayerMask mask = Physics::CreateLayerMask();
        for (const auto layer : layers_)
            Physics::AddLayer(mask, layer);

        for (const auto& player : context.AllPlayer())
        {
            const glm::vec3 playerPos = player.lock()->PlayerTransform().GetWorldPos() + glm::vec3(0.0f, offsetY_, 0.0f);
            const glm::vec3 direction = playerPos - selfPos;

            const auto hit = Physics::Raycast(selfPos, direction, maxDistance_, mask);
            if (!hit.Hit())
                continue;

            const auto hitPlayer = hit.HitObject().Components().Catch<IPlayerAvatar>().lock();
            if (hitPlayer)
                return TickStatus::Success;
        }
        return TickStatus::Failure;
    }

    void Action::ToPlayerRaycast::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("maxDistance_", maxDistance_);
        ImGuiHelper::OnDrawInputField("offsetY_", offsetY_);
        for (int i = 0; i < static_cast<int>(layers_.size()); ++i)
        {
            const std::string label    = "layers_[" + std::to_string(i) + "]";
            const std::string removeId = "-##layers_" + std::to_string(i);
            Physics::DrawChoiceLayerGui(label.c_str(), layers_[i]);
            ImGui::SameLine();
            if (ImGui::Button(removeId.c_str()))
            {
                layers_.erase(layers_.begin() + i);
                --i;
            }
        }
        if (ImGui::Button("+##layers_add"))
            layers_.push_back(Physics::Layer::Default);
    }
}
