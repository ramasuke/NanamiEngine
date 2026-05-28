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
        ImGuiHelper::OnDrawInputField("normalAttackSound_", normalAttackSound_);
        ImGuiHelper::OnDrawInputField("avoidRollingSound_", avoidRollingSound_);
        ImGuiHelper::OnDrawInputField("justAvoidRollingSound_", justAvoidRollingSound_);
        ImGuiHelper::OnDrawInputField("jumpSound_", jumpSound_);
    }
}
