#pragma once
#include "../PlayerAvatarBase.h"
#include "../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "../../../Core/Game/PlayerAvatar/SwordMan/Traits/SwordManAvatarTraits.h"
#include "../LockOnDetectionArea/LockOnDetectionArea.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar final : public PlayerAvatarBase<GameCore::PlayerAvatar::SwordMan::SwordManAvatarTraits>
    {
    public:
        [[nodiscard]] std::weak_ptr<Asset::SwordManAvatarResource> Resources() const { return resources_.get(); }
        [[nodiscard]] std::weak_ptr<PlayerAttackArea> CatchNormalAttackArea      () const;
        [[nodiscard]] std::weak_ptr<PlayerAttackArea> CatchDashAttackArea        () const;
        [[nodiscard]] std::weak_ptr<LockOnDetectionArea> CatchLockOnDetectionArea   () const;
        [[nodiscard]] std::weak_ptr<Component::ParticleSystem> CatchSuccessAvoidRollingParticle() const;
        [[nodiscard]] PlayerAvatarType Type() const override;

    private:
        [[serialize(3)]] FIELD(Asset::SwordManAvatarResource) resources_;
        [[serialize(5)]] FIELD(PlayerAttackArea) normalAttackArea_;
        [[serialize(5)]] FIELD(PlayerAttackArea) dashAttackArea_;
        [[serialize(5)]] FIELD(LockOnDetectionArea) lockOnDetectionArea_;
        [[serialize(5)]] FIELD(Component::ParticleSystem) successAvoidRollingParticle_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            [[serialize(1)]] FIELD(Asset::SoundFile) normalAttackSound_;
            [[serialize(2)]] FIELD(Asset::SoundFile) avoidRollingSound_;
            if(version <= 3) archive(CEREAL_NVP(normalAttackSound_));
            if(version <= 3) archive(CEREAL_NVP(avoidRollingSound_));
            archive(CEREAL_NVP(resources_));
            archive(CEREAL_NVP(normalAttackArea_));
            archive(CEREAL_NVP(dashAttackArea_));
            archive(CEREAL_NVP(lockOnDetectionArea_));
            archive(CEREAL_NVP(successAvoidRollingParticle_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            [[serialize(1)]] FIELD(Asset::SoundFile) normalAttackSound_;
            [[serialize(2)]] FIELD(Asset::SoundFile) avoidRollingSound_;
            if (version <= 3) archive(CEREAL_NVP(normalAttackSound_));
            if (version <= 3) archive(CEREAL_NVP(avoidRollingSound_));
            if (version >= 3) archive(CEREAL_NVP(resources_));
            if (version >= 5) archive(CEREAL_NVP(normalAttackArea_));
            if (version >= 5) archive(CEREAL_NVP(dashAttackArea_));
            if (version >= 5) archive(CEREAL_NVP(lockOnDetectionArea_));
            if (version >= 5) archive(CEREAL_NVP(successAvoidRollingParticle_));
        }
#pragma endregion
    };
}

REGISTER_PLAYER_AVATAR_BASE(SwordMan::SwordManAvatarTraits)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar, 5);
CEREAL_REGISTER_TYPE(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::SwordMan::SwordManAvatarTraits>, GamePlay::PlayerAvatar::SwordMan::SwordManAvatar);
#pragma endregion
