#include "Data_ItemData.h"

#include "../../Scripts/Core/Game/PlayerAvatar/Item/Effect/ItemEffectFactory.h"

namespace NanamiEngine::Module::Asset
{
    ItemData::ItemData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void ItemData::ApplyEffects(GameCore::PlayerAvatar::Item::IItemEffectTarget& target) const
    {
        for (const auto& effect : effects_)
        {
            if (effect)
                effect->Apply(target);
        }
    }

    void ItemData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        LibCore::ImGuiHelper::OnDrawInputField("iconSprite_", iconSprite_);
        LibCore::ImGuiHelper::OnDrawInputField("useSound_", useSound_);
        LibCore::ImGuiHelper::OnDrawInputField("maxStack_", maxStack_);
        LibCore::ImGuiHelper::OnDrawInputField("effects_", effects_, [this]
        {
            if (ImGui::Button("Add Effect"))
                ImGui::OpenPopup("AddItemEffect");

            if (ImGui::BeginPopup("AddItemEffect"))
            {
                for (const auto& [effectName, createEffect] : GameCore::PlayerAvatar::Item::ItemEffectFactory::Instance().CreatableEffects())
                {
                    if (ImGui::Selectable(effectName.c_str()))
                        effects_.push_back(createEffect());
                }
                ImGui::EndPopup();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("pickupPrefab_", pickupPrefab_);
        LibCore::ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                descriptionLines_.emplace_back();
            }
        });
    }
}
