#include "MagicCasterAvatar.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    PlayerAvatarType MagicCasterAvatar::Type() const
    {
        return PlayerAvatarType::MagicCaster;
    }

    void MagicCasterAvatar::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("resources_", resources_);
        ImGuiHelper::OnDrawInputField("castPoint_", castPoint_);
    }
}
