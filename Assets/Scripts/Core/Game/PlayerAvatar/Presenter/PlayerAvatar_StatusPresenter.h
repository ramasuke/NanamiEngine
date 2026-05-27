#pragma once

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

namespace GameCore::PlayerAvatar::SwordMan
{
    class StatusPresenter final
    {
    public:
        explicit StatusPresenter(
            const GamePlay::Ui::PlayerStatus& playerStatusView,
            const SwordManAvatarStatus& playerStatusModel);

    private:
        void SubscribeModelEventToView(
            const GamePlay::Ui::PlayerStatus& view,
            const SwordManAvatarStatus& model);
    };
}
