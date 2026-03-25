#include "Data_Event_Npc_WalkingRoute_RoutePoint.h"

#include "ImGuiHelper.h"

Data::EventNpcWalkingRoute::RoutePoint::RoutePoint(const glm::vec3& pos, const float duration_secs)
        : position_(pos)
        , duration_secs_(std::max(duration_secs, 0.001f))
{
}

void Data::EventNpcWalkingRoute::RoutePoint::OnDrawGui()
{
    ImGui::DragFloat3("Position", &position_.x, 0.1f);
    ImGui::DragFloat("Duration (secs)", &duration_secs_, 0.1f, 0.01f, 1000.0f);
}