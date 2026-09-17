#pragma once
#include "../PlayerAvatarBase.h"
#include "../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"
#include "../../../Core/Game/PlayerAvatar/MagicCaster/Traits/MagicCasterAvatarTraits.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatar final : public PlayerAvatarBase<GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarTraits>
    {
    public:
        [[nodiscard]] std::weak_ptr<Asset::MagicCasterAvatarResource> Resources() const { return resources_.get(); }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> CastPoint() const { return castPoint_.get(); }
        [[nodiscard]] PlayerAvatarType Type() const override;

    private:
        [[serialize(0)]] FIELD(Asset::MagicCasterAvatarResource) resources_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) castPoint_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            archive(CEREAL_NVP(resources_));
            archive(CEREAL_NVP(castPoint_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<PlayerAvatarBase>(this));
            archive(CEREAL_NVP(resources_));
            archive(CEREAL_NVP(castPoint_));
        }
#pragma endregion
    };
}

REGISTER_PLAYER_AVATAR_BASE(MagicCaster::MagicCasterAvatarTraits)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar, 0);
CEREAL_REGISTER_TYPE(GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarTraits>, GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar);
#pragma endregion
