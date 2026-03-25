#include "PhysicsPower.h"

#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Damage
{
    PhysicsPower::PhysicsPower(const int physicsPower)
        : value_(physicsPower)
    {
    }

    void PhysicsPower::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("value", value_);
    }
}
