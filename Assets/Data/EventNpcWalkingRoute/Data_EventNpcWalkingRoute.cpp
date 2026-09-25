#include "Data_EventNpcWalkingRoute.h"

#include "Engine/Module/3DRender/Shapes/Shapes.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    EventNpcWalkingRoute::EventNpcWalkingRoute(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void EventNpcWalkingRoute::OnDrawRouteGizmo() const
    {
        if (route_.size() < 2)
            return;

        const Color32 cyan(0, 255, 255);
        for (size_t i = 0; i + 1 < route_.size(); ++i)
        {
            const auto& a = route_[i    ].Position();
            const auto& b = route_[i + 1].Position();

            Render3D::Shapes::DrawLine3D(a, b, cyan);
        }
    }

    void EventNpcWalkingRoute::OnDrawGui()
    {
        if (!ImGui::CollapsingHeader("Event Walking Route", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        OnDrawRouteGizmo();

        if (ImGui::Button("Add Point"))
        {
            route_.emplace_back(Core::Application::ApplicationBase::GameWindow()->GetCameraPosition(), 1.0f );
        }


        ImGui::Separator();

        for (int i = 0; i < static_cast<int>(route_.size()); ++i)
        {
            ImGui::PushID(i);

            auto& point = route_[i];

            if (ImGui::TreeNode(("Point " + std::to_string(i)).c_str()))
            {
                point.OnDrawGui();

                if (ImGui::Button("Remove"))
                {
                    ImGui::TreePop();
                    ImGui::PopID();
                    route_.erase(route_.begin() + i);
                    break;
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(EventNpcWalkingRoute, EVENT_NPC_WALKING_ROUTE_EXTENSION_LABEL, "Npc::Friendly")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::EventNpcWalkingRoute, NanamiEngine::Module::Asset::AssetBase);
#pragma endregion
