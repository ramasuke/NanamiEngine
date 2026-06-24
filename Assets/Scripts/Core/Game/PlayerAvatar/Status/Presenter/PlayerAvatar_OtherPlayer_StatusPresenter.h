#pragma once
#include "PlayerAvatar_StatusPresenterBase.h"

namespace GamePlay::PlayerAvatar::OtherPlayer
{
    class StatusPresenter final : public StatusPresenterBase
    {
    public:
        ~StatusPresenter() override = default;
        
        void Initialize(
            const Ui::PlayerStatus& playerStatusView,
            const GameCore::PlayerAvatar::IPlayerAvatarStatus& playerStatusModel);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::OtherPlayer::StatusPresenter, 0)