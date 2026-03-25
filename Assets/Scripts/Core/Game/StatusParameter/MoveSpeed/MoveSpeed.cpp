#include "MoveSpeed.h"
#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

GameCore::StatusParameter::MoveSpeed::MoveSpeed(const float value)
    : value_(value)
{
    
}

void GameCore::StatusParameter::MoveSpeed::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
}
