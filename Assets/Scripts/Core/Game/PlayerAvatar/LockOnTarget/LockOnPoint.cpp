#include "LockOnPoint.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar
{
    void LockOnPoint::OnDrawGui()
    {
        ImGui::TextUnformatted("Lock On Point");
        ImGui::Separator();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::PlayerAvatar::LockOnPoint);
#pragma endregion
