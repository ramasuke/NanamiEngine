#pragma once
#include "../PlayerAvatarBase.h"
#include "../../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "Traits/SwordManAvatarTraits.h"

namespace NanamiEngine::Module::Component
{
    class ParticleSystem;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    constexpr auto NORMAL_ATTACK_AREA_NAME             = "NormalAttackArea";
    constexpr auto DASH_ATTACK_AREA_NAME               = "DashAttackArea";
    constexpr auto ON_REINFORCE_PARTICLE_NAME          = "OnReinforceParticle";
    constexpr auto REINFORCING_PARTICLE_NAME           = "ReinforcingParticle";
    constexpr auto SUCCESS_AVOID_ROLLING_PARTICLE_NAME = "SuccessAvoidRollingParticle";
    constexpr auto HIT_NORMAL_ATTACK_PARTICLE_NAME     = "HitNormalAttackParticle";
    constexpr auto REINFORCE_PARTICLE_NAME             = "ReinforceParticle";
    
    class SwordManAvatar final : public PlayerAvatarBase<GameCore::PlayerAvatar::SwordMan::SwordManAvatarTraits>
    {
    public:
        [[nodiscard]] std::weak_ptr<PlayerAttackArea         > CatchNormalAttackArea() const; 
        [[nodiscard]] std::weak_ptr<PlayerAttackArea         > CatchDashAttackArea  () const;
        [[nodiscard]] std::weak_ptr<Component::ParticleSystem> OnReinforceParticle  () const;
        [[nodiscard]] std::weak_ptr<Component::ParticleSystem> ReinforcingParticle  () const;
        [[nodiscard]] std::weak_ptr<Asset::SoundFile         > NormalAttackSound    () const;
        [[nodiscard]] std::weak_ptr<Asset::SoundFile         > AvoidRollingSound    () const;
        [[nodiscard]] PlayerAvatarType Type() const override;

    private:
        [[serialize(1)]] FIELD(Asset::SoundFile) normalAttackSound_;
        [[serialize(2)]] FIELD(Asset::SoundFile) avoidRollingSound_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            archive(CEREAL_NVP(normalAttackSound_));
            archive(CEREAL_NVP(avoidRollingSound_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            if (version >= 1) archive(CEREAL_NVP(normalAttackSound_));
            if (version >= 2) archive(CEREAL_NVP(avoidRollingSound_));
        }
#pragma endregion
    };
}

REGISTER_PLAYER_AVATAR_BASE(SwordMan::SwordManAvatarTraits)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar, 2);
CEREAL_REGISTER_TYPE(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::SwordMan::SwordManAvatarTraits>, GamePlay::PlayerAvatar::SwordMan::SwordManAvatar);
#pragma endregion
