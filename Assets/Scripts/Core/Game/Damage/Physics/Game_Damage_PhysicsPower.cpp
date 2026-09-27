#include "Game_Damage_PhysicsPower.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Damage
{
    PhysicsPower::PhysicsPower(const int physicsPower, const Damage::FlinchPower flinchPower)
        : value_(physicsPower)
        , flinchPower_(flinchPower)
    {
    }

    void PhysicsPower::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("value", value_);
        flinchPower_.OnDrawInputField("flinchPower");
    }
}
