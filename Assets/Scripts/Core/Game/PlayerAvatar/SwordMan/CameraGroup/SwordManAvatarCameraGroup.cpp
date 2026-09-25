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
NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup, GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase);
#pragma endregion
