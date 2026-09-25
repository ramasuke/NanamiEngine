#include "Data_MagicCasterAvatarResource.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    MagicCasterAvatarResource::MagicCasterAvatarResource(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    std::shared_ptr<const GameCore::Magic::IMagicSpell> MagicCasterAvatarResource::LoadoutSpell(const int slot) const
    {
        if (slot < 0 || slot >= LOADOUT_SLOT_COUNT)
            return nullptr;
        return loadout_[static_cast<size_t>(slot)].get();
    }

    void MagicCasterAvatarResource::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("basicSpell_", basicSpell_);
        if (ImGui::TreeNode("loadout_"))
        {
            for (size_t i = 0; i < loadout_.size(); ++i)
                ImGuiHelper::OnDrawInputField("loadout_" + std::to_string(i), loadout_[i]);
            ImGui::TreePop();
        }
        ImGuiHelper::OnDrawInputField("groundCheckRadius_", groundCheckRadius_);
        ImGuiHelper::OnDrawInputField("groundCheckUpOffset_", groundCheckUpOffset_);
        ImGuiHelper::OnDrawInputField("groundCheckDistance_", groundCheckDistance_);
        ImGuiHelper::OnDrawInputField("maxWalkableSlope_deg_", maxWalkableSlope_deg_);
        ImGuiHelper::OnDrawInputField("slopeCheckRadius_", slopeCheckRadius_);
        ImGuiHelper::OnDrawInputField("slopeCheckUpOffset_", slopeCheckUpOffset_);
        ImGuiHelper::OnDrawInputField("slopeCheckDistance_", slopeCheckDistance_);
        ImGuiHelper::OnDrawInputField("dealDamageTextBillBoardPrefab_", dealDamageTextBillBoardPrefab_);
        ImGuiHelper::OnDrawInputField("avoidRollingSound_", avoidRollingSound_);
        ImGuiHelper::OnDrawInputField("justAvoidRollingSound_", justAvoidRollingSound_);
        ImGuiHelper::OnDrawInputField("footstepParticlePrefab_", footstepParticlePrefab_);
        ImGuiHelper::OnDrawInputField("footstepContactHeight_", footstepContactHeight_);
        ImGuiHelper::OnDrawInputField("footstepBoneNames_", footstepBoneNames_, [this]
        {
            if (ImGui::Button("Add Footstep Bone"))
                footstepBoneNames_.emplace_back();
        });
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(MagicCasterAvatarResource, MAGIC_CASTER_RESOURCE_EXTENSION_LABEL, "Player::MagicCaster")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::MagicCasterAvatarResource, NanamiEngine::Module::ScriptableObject);
#pragma endregion
