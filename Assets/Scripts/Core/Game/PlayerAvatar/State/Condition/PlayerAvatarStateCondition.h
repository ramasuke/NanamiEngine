#pragma once
#include "../Context/IPlayerAvatarStateContext.h"

namespace GameCore::PlayerAvatar::State
{
    struct PlayerAvatarStateCondition
    {
        explicit PlayerAvatarStateCondition(const std::shared_ptr<IPlayerAvatarStateContext>& stateContext);
        
        [[nodiscard]] bool IsGround   () const;
        [[nodiscard]] bool IsChattable() const;
        
    private:
        const std::shared_ptr<IPlayerAvatarStateContext> stateContext_;
    };
}
