#pragma once
#include "../../CameraGroup/PlayerAvatarCameraGroupBase.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarCameraGroup final : public PlayerAvatarCameraGroupBase,
                                            public LifeCycleCallback::IAwakable
    {
    public:
        
    private:
        void OnAwake() override;
        
        
        
#pragma region Serialization Function
        public:
        void OnDrawGui() override;
    
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<PlayerAvatarCameraGroupBase>(this));
        }
    
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<PlayerAvatarCameraGroupBase>(this));
            [[serialize(1)]] std::string idleCameraName_;
            [[serialize(1)]] FIELD(CineMachine::CineMachineVirtualCamera) idleCamera_;
            if (version == 1) archive(CEREAL_NVP(idleCameraName_));
            if (version == 1) archive(CEREAL_NVP(idleCamera_));
        }
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase, GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup);
#pragma endregion
