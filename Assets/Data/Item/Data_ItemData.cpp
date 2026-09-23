#include "Data_ItemData.h"

#include "../../Scripts/Core/Game/PlayerAvatar/Item/Effect/ItemEffectFactory.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    ItemData::ItemData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void ItemData::ApplyEffects(GameCore::PlayerAvatar::Item::IItemEffectTarget& target, const std::shared_ptr<GameObject::IGameObject>& user) const
    {
        for (const auto& effect : effects_)
        {
            if (effect)
                effect->Apply(target, user);
        }
    }

    void ItemData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        LibCore::ImGuiHelper::OnDrawInputField("iconSprite_", iconSprite_);
        LibCore::ImGuiHelper::OnDrawInputField("useSound_", useSound_);
        LibCore::ImGuiHelper::OnDrawInputField("useParticle_", useParticle_);
        LibCore::ImGuiHelper::OnDrawEnumField("useMotion_", useMotion_, GameCore::PlayerAvatar::Item::ITEM_USE_MOTIONS, GameCore::PlayerAvatar::Item::ToString);
        LibCore::ImGuiHelper::OnDrawInputField("useEffectTime_secs_", useEffectTime_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("useTotalDuration_secs_", useTotalDuration_secs_);
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

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(ItemData, ITEM_DATA_EXTENSION_LABEL, "Item")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::ItemData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::ItemData);
#pragma endregion
