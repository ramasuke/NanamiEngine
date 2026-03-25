#pragma once
#include <memory>

#include "../Context/IPlayerAvatarStateContext.h"

namespace GameCore::PlayerAvatar::State
{
    struct PlayerAvatarStateAction final
    {
        explicit PlayerAvatarStateAction(const std::shared_ptr<IPlayerAvatarStateContext>& stateContext);

        /**
         * @brief camera位置を基準に、inputVelocity方向に移動する
         * @note inputVelocityにdeltaTimeは不要
         * @note 移動方向に回転
         */
        void ForwardMove  (const glm::vec3& inputVelocity, float rotateSpeed) const;
        void RotateTowards(const glm::vec3& direction    , float rotateSpeed) const;
        void Jump         (const glm::vec3& direction                       ) const;
        
    private:
        const std::shared_ptr<IPlayerAvatarStateContext> stateContext_;
    };
}
