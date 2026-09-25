#include "Data_MagicSpellData.h"

#include "../../Scripts/Core/Game/Magic/IMagicCaster.h"
#include "../../Scripts/Core/Game/Magic/MagicSpellEffectFactory.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    MagicSpellData::MagicSpellData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    GameCore::Magic::MagicCastTarget MagicSpellData::Aim(const GameCore::Magic::IMagicCaster& caster) const
    {
        if (effect_)
            return effect_->Aim(caster);

        GameCore::Magic::MagicCastTarget target;
        target.origin    = caster.CastOrigin();
        target.rotation  = caster.CastRotation();
        target.targetPos = caster.CastOrigin();
        return target;
    }

    void MagicSpellData::Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const
    {
        if (effect_)
            effect_->Execute(caster, target);
    }

    void MagicSpellData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        LibCore::ImGuiHelper::OnDrawInputField("iconSprite_", iconSprite_);
        LibCore::ImGuiHelper::OnDrawInputField("castSound_", castSound_);
        LibCore::ImGuiHelper::OnDrawInputField("manaCost_", manaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("cooldown_secs_", cooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("castFireTime_secs_", castFireTime_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("castTotalDuration_secs_", castTotalDuration_secs_);
        LibCore::ImGuiHelper::OnDrawEnumField("castMotion_", castMotion_, GameCore::Magic::MAGIC_CAST_MOTIONS, GameCore::Magic::ToString);
        LibCore::ImGuiHelper::OnDrawInputField("castEffectPrefab_", castEffectPrefab_);

        if (ImGui::Button("Change Effect"))
            ImGui::OpenPopup("ChangeMagicSpellEffect");

        if (ImGui::BeginPopup("ChangeMagicSpellEffect"))
        {
            for (const auto& [effectName, createEffect] : GameCore::Magic::MagicSpellEffectFactory::Instance().CreatableEffects())
            {
                if (ImGui::Selectable(effectName.c_str()))
                    effect_ = createEffect();
            }
            ImGui::EndPopup();
        }

        LibCore::ImGuiHelper::OnDrawInputField("effect_", effect_);
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(MagicSpellData, MAGIC_SPELL_DATA_EXTENSION_LABEL, "Player::MagicCaster")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::MagicSpellData, NanamiEngine::Module::ScriptableObject);
#pragma endregion
