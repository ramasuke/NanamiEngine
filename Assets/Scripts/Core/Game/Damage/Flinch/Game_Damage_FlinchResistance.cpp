#include "Game_Damage_FlinchResistance.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Damage
{
    void FlinchResistance::OnDrawInputField(const std::string& label)
    {
        LibCore::ImGuiHelper::OnDrawInputField(label, value_);
    }
}
