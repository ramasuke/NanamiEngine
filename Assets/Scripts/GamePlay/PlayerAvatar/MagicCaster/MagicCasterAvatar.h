#pragma once
#include "../PlayerAvatarBase.h"
#include "../../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"
#include "../../../Core/Game/Magic/IMagicCaster.h"
#include "../../../Core/Game/PlayerAvatar/MagicCaster/Traits/MagicCasterAvatarTraits.h"
#include "../LockOnDetectionArea/LockOnDetectionArea.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatar final : public PlayerAvatarBase<GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarTraits>,
                                    public GameCore::Magic::IMagicCaster
    {
    public:
        [[nodiscard]] std::weak_ptr<Asset::MagicCasterAvatarResource> Resources() const { return resources_.get(); }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> CastPoint() const { return castPoint_.get(); }
        [[nodiscard]] std::weak_ptr<LockOnDetectionArea> CatchLockOnDetectionArea() const { return lockOnDetectionArea_.get(); }
        [[nodiscard]] PlayerAvatarType Type() const override;

        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> CasterObject() const override { return Entity().lock(); }
        [[nodiscard]] glm::vec3 CastOrigin() const override;
        [[nodiscard]] glm::quat CastRotation() const override { return Transform().GetWorldRot(); }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> AimTarget() const override;
        [[nodiscard]] float SpellPowerRate() const override { return PlayerStatus().AttackPowerRate(); }
        [[nodiscard]] Core::Network::NetworkObjectId CasterNetworkObjectId() const override { return GetNetworkObjectId(); }
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> DealDamageTextPrefab() const override;

    private:
        [[serialize(0)]] FIELD(Asset::MagicCasterAvatarResource) resources_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) castPoint_;
        [[serialize(1)]] FIELD(LockOnDetectionArea) lockOnDetectionArea_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            archive(CEREAL_NVP(resources_));
            archive(CEREAL_NVP(castPoint_));
            archive(CEREAL_NVP(lockOnDetectionArea_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            archive(CEREAL_NVP(resources_));
            archive(CEREAL_NVP(castPoint_));
            if (version >= 1) archive(CEREAL_NVP(lockOnDetectionArea_));
        }
#pragma endregion
    };
}

REGISTER_PLAYER_AVATAR_BASE(MagicCaster::MagicCasterAvatarTraits)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar, 1);
CEREAL_REGISTER_TYPE(GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarTraits>, GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar);
#pragma endregion
