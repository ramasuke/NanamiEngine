#pragma once
#include "../../CameraGroup/PlayerAvatarCameraGroupBase.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarCameraGroup final : public PlayerAvatarCameraGroupBase
    {
    public:
        
    private:
        
        
        
#pragma region Serialization Function
public:
void OnDrawGui() {
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<PlayerAvatarCameraGroupBase>(this));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<PlayerAvatarCameraGroupBase>(this));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase, GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup);
#pragma endregion
