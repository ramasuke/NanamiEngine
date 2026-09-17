#include "Grassable.h"

namespace GamePlay::Prop
{
    void Grassable::OnDrawGui()
    {
        ImGui::TextDisabled("GrassField の配置モードで草を生やせます");
    }
}
