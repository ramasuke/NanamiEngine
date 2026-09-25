#include "Data_NpcWalkingRoute.h"

#include "Engine/Module/3DRender/Shapes/Shapes.h"

#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    NpcWalkingRoute::NpcWalkingRoute(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void NpcWalkingRoute::OnDrawRouteGizmo()
    {
        if (walkingRoute_.size() >= 2)
        {
            const Color32 yellow(255, 255, 0);

            for (size_t i = 0; i + 1 < walkingRoute_.size(); ++i)
            {
                const glm::vec3& a = walkingRoute_[i];
                const glm::vec3& b = walkingRoute_[i + 1];

                Render3D::Shapes::DrawLine3D(a, b, yellow);
            }
        }
    }

    void NpcWalkingRoute::OnDrawGui()
    {
        if (!ImGui::CollapsingHeader("Walking Route", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        OnDrawRouteGizmo();
        
        // 追加ボタン
        if (ImGui::Button("Add Point"))
        {
            walkingRoute_.push_back(Core::Application::ApplicationBase::GameWindow()->GetCameraPosition());
        }

        ImGui::Separator();

        // 各ルートポイント
        for (int i = 0; i < static_cast<int>(walkingRoute_.size()); ++i)
        {
            ImGui::PushID(i);

            glm::vec3& p = walkingRoute_[i];

            if (ImGui::TreeNode(("Point " + std::to_string(i)).c_str()))
            {
                ImGui::DragFloat3("Position", &p.x, 0.1f);

                if (ImGui::Button("Remove"))
                {
                    ImGui::TreePop();
                    ImGui::PopID();
                    walkingRoute_.erase(walkingRoute_.begin() + i);
                    break;
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(NpcWalkingRoute, FRIENDLY_NPC_WALKING_ROUTE_EXTENSION_LABEL, "Npc::Friendly")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::NpcWalkingRoute, NanamiEngine::Module::Asset::AssetBase);
#pragma endregion
