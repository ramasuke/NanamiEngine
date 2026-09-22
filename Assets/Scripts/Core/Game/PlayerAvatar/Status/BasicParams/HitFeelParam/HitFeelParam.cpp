#include "HitFeelParam.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

void GameCore::PlayerAvatar::HitFeelParam::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("shakeIntensity_", shakeIntensity_);
    LibCore::ImGuiHelper::OnDrawInputField("shakeDuration_secs_", shakeDuration_secs_);
    LibCore::ImGuiHelper::OnDrawInputField("particleScale_", particleScale_);
    LibCore::ImGuiHelper::OnDrawInputField("targetShakeAmplitude_", targetShakeAmplitude_);
    LibCore::ImGuiHelper::OnDrawInputField("targetShakeDuration_secs_", targetShakeDuration_secs_);
    LibCore::ImGuiHelper::OnDrawInputField("lungeSpeed_", lungeSpeed_);
}
