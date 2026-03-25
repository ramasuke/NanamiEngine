#include "EnhancePower.h"

#include "../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

void GameCore::PlayerAvatar::EnhancePower::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
}