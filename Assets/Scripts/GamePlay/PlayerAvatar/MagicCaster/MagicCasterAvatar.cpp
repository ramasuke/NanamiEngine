#include "MagicCasterAvatar.h"

#include "../../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    PlayerAvatarType MagicCasterAvatar::Type() const
    {
        return PlayerAvatarType::MagicCaster;
    }

    glm::vec3 MagicCasterAvatar::CastOrigin() const
    {
        const auto castPoint = castPoint_.get();
        return castPoint ? castPoint->Transform().GetWorldPos() : Transform().GetWorldPos();
    }

    std::weak_ptr<GameObject::IGameObject> MagicCasterAvatar::AimTarget() const
    {
        const auto cameraGroup = AvatarCameraGroup().lock();
        if (!cameraGroup || !cameraGroup->IsLockedOn())
            return {};
        return cameraGroup->LockOnAim();
    }

    std::shared_ptr<Asset::PrefabGameObjectFile> MagicCasterAvatar::DealDamageTextPrefab() const
    {
        const auto resources = resources_.get();
        return resources ? resources->DealDamageTextBillBoardPrefab() : nullptr;
    }

    void MagicCasterAvatar::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("resources_", resources_);
        ImGuiHelper::OnDrawInputField("castPoint_", castPoint_);
        ImGuiHelper::OnDrawInputField("lockOnDetectionArea_", lockOnDetectionArea_);
        ImGuiHelper::OnDrawInputField("successAvoidRollingParticle_", successAvoidRollingParticle_);
    }
}

#pragma region SerializationMacro
REGISTER_PLAYER_AVATAR_BASE(MagicCaster::MagicCasterAvatarTraits)
CEREAL_REGISTER_TYPE(GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarTraits>, GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar);
#pragma endregion
