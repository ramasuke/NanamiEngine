#include "Engine_Physics_PhysicsLayer.h"

#include "ImGuiHelper.h"

namespace NanamiEngine::Module::Physics
{
    bool DrawChoiceLayerGui(const char* label, Layer& layer)
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

    bool DrawLayerMaskGui(const char* label, LayerMask& mask)
    {
        bool changed = false;

        ImGui::TextUnformatted(label);
        ImGui::PushID(label);
        for (int i = 0; i < static_cast<int>(Layer::Count); ++i)
        {
            const Layer layer = ToLayer(i);

            bool enabled = HasLayer(mask, layer);
            if (ImGui::Checkbox(ToName(layer), &enabled))
            {
                if (enabled)
                    AddLayer(mask, layer);
                else
                    RemoveLayer(mask, layer);

                changed = true;
            }
        }
        ImGui::PopID();

        return changed;
    }
}
