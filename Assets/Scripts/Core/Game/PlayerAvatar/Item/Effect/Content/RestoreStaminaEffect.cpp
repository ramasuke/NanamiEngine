#include "RestoreStaminaEffect.h"

#include "../IItemEffectTarget.h"
#include "../../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::PlayerAvatar::Item
{
    void RestoreStaminaEffect::Apply(IItemEffectTarget& target) const
    {
        target.RestoreStamina(amount_);
    }

    void RestoreStaminaEffect::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("amount_", amount_);
    }
}
