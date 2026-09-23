#include "Grassable.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void Grassable::OnDrawGui()
    {
        ImGui::TextDisabled("GrassField の配置モードで草を生やせます");
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::Grassable);
#pragma endregion
