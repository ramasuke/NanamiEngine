#include "Data_SwordManAvatarResource.h"

namespace NanamiEngine::Module::Asset
{
    SwordManAvatarResource::SwordManAvatarResource(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void SwordManAvatarResource::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("normalAttackParticlePrefab_",normalAttackParticlePrefab_);
        ImGuiHelper::OnDrawInputField("dealDamageTextBillBoardPrefab_",dealDamageTextBillBoardPrefab_);
        ImGuiHelper::OnDrawInputField("normalAttackSound_", normalAttackSound_);
        ImGuiHelper::OnDrawInputField("avoidRollingSound_", avoidRollingSound_);
        ImGuiHelper::OnDrawInputField("justAvoidRollingSound_", justAvoidRollingSound_);
        ImGuiHelper::OnDrawInputField("jumpSound_", jumpSound_);
        ImGuiHelper::OnDrawInputField("footstepParticlePrefab_", footstepParticlePrefab_);
        ImGuiHelper::OnDrawInputField("walkFootstepContactPhases_", walkFootstepContactPhases_, [this]
        {
            if (ImGui::Button("Add Walk Phase"))
            {
                walkFootstepContactPhases_.push_back(0.5f);
            }
        });
        ImGuiHelper::OnDrawInputField("runFootstepContactPhases_", runFootstepContactPhases_, [this]
        {
            if (ImGui::Button("Add Run Phase"))
            {
                runFootstepContactPhases_.push_back(0.5f);
            }
        });
        ImGuiHelper::OnDrawInputField("walkFootstepSounds_", walkFootstepSounds_, [this]
        {
            if (ImGui::Button("Add Walk Footstep Sound"))
                walkFootstepSounds_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("runFootstepSounds_", runFootstepSounds_, [this]
        {
            if (ImGui::Button("Add Run Footstep Sound"))
                runFootstepSounds_.emplace_back();
        });
    }
}
