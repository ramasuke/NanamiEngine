#include "SwordManAvatarCameraGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    void SwordManAvatarCameraGroup::OnAwake()
    {
    }

    void SwordManAvatarCameraGroup::OnDrawGui()
    {
        
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase, GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup);
#pragma endregion
