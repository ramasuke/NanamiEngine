#include "Game_Damage_FlinchPower.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Damage
{
    void FlinchPower::OnDrawInputField(const std::string& label)
    {
        LibCore::ImGuiHelper::OnDrawInputField(label, value_);
    }
}
