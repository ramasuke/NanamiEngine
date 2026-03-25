#pragma once
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace GameCore::PlayerAvatar
{
    constexpr auto ENABLE_CURRENT_CAMERA_PRIORITY = 0;
    
    class PlayerAvatarCameraGroupBase : public Component::ComponentBase
    {
    public:
        virtual ~PlayerAvatarCameraGroupBase() override = default;
        [[nodiscard]] std::weak_ptr<CineMachine::CineMachineVirtualCamera> FollowFromBehind() const;
        void ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera);
        virtual void Init(const std::shared_ptr<GameObject::IGameObject>& playerAvatarObject);
        [[nodiscard]] CineMachine::CineMachineVirtualCamera& CurrentCamera() const { return *currentCamera_.lock(); }
        
    private:
        FIELD(CineMachine::CineMachineVirtualCamera) followFromBehindCamera_;

        std::weak_ptr<CineMachine::CineMachineVirtualCamera> currentCamera_;
    
#pragma region Serialization Function
public:
void BasedOnDrawgui() {
    ImGuiHelper::OnDrawInputField("followFromBehindCamera_", followFromBehindCamera_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(CEREAL_NVP(followFromBehindCamera_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 0) archive(CEREAL_NVP(followFromBehindCamera_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase);
#pragma endregion
