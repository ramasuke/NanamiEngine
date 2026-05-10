#include "Engine_Physics_Constraints.h"

#include "ImGuiHelper.h"

void NanamiEngine::Module::Physics::DrawConstraintCheckBoxsGui(Constraints& constraints)
{
    auto drawSingle = [&](const char* label, Constraints flag)
    {
        bool enabled = HasConstraint(constraints, flag);
        if (ImGui::Checkbox(label, &enabled))
        {
            if (enabled)
                constraints |= flag;
            else
                constraints = static_cast<Constraints>(
                    static_cast<uint8_t>(constraints) &
                    ~static_cast<uint8_t>(flag)
                );
        }
    };

    ImGui::Text("Freeze Position");
    drawSingle("Freeze Pos X", Constraints::FreezePosX);
    drawSingle("Freeze Pos Y", Constraints::FreezePosY);
    drawSingle("Freeze Pos Z", Constraints::FreezePosZ);

    ImGui::Separator();
    ImGui::Text("Freeze Rotation");
    drawSingle("Freeze Rot X", Constraints::FreezeRotX);
    drawSingle("Freeze Rot Y", Constraints::FreezeRotY);
    drawSingle("Freeze Rot Z", Constraints::FreezeRotZ);
}
