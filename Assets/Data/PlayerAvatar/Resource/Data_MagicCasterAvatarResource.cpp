#include "Data_MagicCasterAvatarResource.h"

namespace NanamiEngine::Module::Asset
{
    MagicCasterAvatarResource::MagicCasterAvatarResource(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void MagicCasterAvatarResource::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("magicBoltPrefab_", magicBoltPrefab_);
        ImGuiHelper::OnDrawInputField("magicBoltSpeed_", magicBoltSpeed_);
        ImGuiHelper::OnDrawInputField("castFireTime_secs_", castFireTime_secs_);
        ImGuiHelper::OnDrawInputField("castTotalDuration_secs_", castTotalDuration_secs_);
        ImGuiHelper::OnDrawInputField("groundCheckRadius_", groundCheckRadius_);
        ImGuiHelper::OnDrawInputField("groundCheckUpOffset_", groundCheckUpOffset_);
        ImGuiHelper::OnDrawInputField("groundCheckDistance_", groundCheckDistance_);
    }
}
