#include "MagicCasterAvatarInputAction.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    std::optional<int> MagicCasterAvatarInputAction::PressedLoadoutSlot() const
    {
        for (int i = 0; i < SPELL_SLOTS_PER_PAGE; ++i)
        {
            if (slots_[static_cast<size_t>(i)]->IsPressed())
                return IsSecondPage() ? i + SPELL_SLOTS_PER_PAGE : i;
        }
        return std::nullopt;
    }

    void MagicCasterAvatarInputAction::OnDrawGui()
    {
    }
}
