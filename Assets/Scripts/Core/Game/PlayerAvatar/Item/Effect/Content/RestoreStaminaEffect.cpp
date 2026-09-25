#include "RestoreStaminaEffect.h"

#include "../IItemEffectTarget.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Item
{
    void RestoreStaminaEffect::Apply(IItemEffectTarget& target, const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>&) const
    {
        target.RestoreStamina(amount_);
    }

    void RestoreStaminaEffect::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("amount_", amount_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::Item::RestoreStaminaEffect, GameCore::PlayerAvatar::Item::IItemEffect);
#pragma endregion
