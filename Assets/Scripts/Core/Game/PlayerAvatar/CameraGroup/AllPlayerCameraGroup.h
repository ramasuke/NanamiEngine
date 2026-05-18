#pragma once
#include <memory>

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarCameraGroup;
}

namespace GameCore::PlayerAvatar
{
    struct AllPlayerCameraGroup final
    {
    public:
        explicit AllPlayerCameraGroup(
            std::weak_ptr<SwordMan::SwordManAvatarCameraGroup> swordman);

        [[nodiscard]] std::shared_ptr<SwordMan::SwordManAvatarCameraGroup> Swordman() const { return swordman_.lock(); }

    private:
        std::weak_ptr<SwordMan::SwordManAvatarCameraGroup> swordman_;
    };
}
