#include "Engine_Physics_PhysicsLayer.h"

#include "ImGuiHelper.h"

bool NanamiEngine::Module::Physics::DrawChoiceLayerGui(const char* label, Layer& layer)
{
    int current = ToIndex(layer);
    bool changed = false;

    if (ImGui::Combo(label, &current, LAYER_NAMES, static_cast<int>(Layer::Count)))
    {
        layer = ToLayer(current);
        changed = true;
    }

    return changed;
}
