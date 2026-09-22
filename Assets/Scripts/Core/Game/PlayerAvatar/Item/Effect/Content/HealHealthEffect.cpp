#include "HealHealthEffect.h"

#include "../IItemEffectTarget.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::PlayerAvatar::Item
{
    void HealHealthEffect::Apply(IItemEffectTarget& target, const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>&) const
    {
        target.Heal(StatusParameter::Health(amount_));
    }

    void HealHealthEffect::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("amount_", amount_);
    }
}
