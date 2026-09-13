#pragma once
#include "../../../Status/Presenter/PlayerAvatar_StatusPresenterBase.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStatus;
}

namespace GamePlay::Ui
{
    class PlayerStatus;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStatus;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    class StatusPresenter final : public StatusPresenterBase
    {
    public:
        void Initialize(
            const Ui::PlayerStatus& playerStatusView,
            const GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus& playerStatusModel);
        
    private:
        [[serialize(1)]] std::string lowHealthScreenEffectName_ = "LowHealthScreenEffect";

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(lowHealthScreenEffectName_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(lowHealthScreenEffectName_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::SwordMan::StatusPresenter, 1)