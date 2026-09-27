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
        void MoveForward  (const glm::vec3& inputVelocity, float rotateSpeed) const;
        void RotateTowards(const glm::vec3& direction    , float rotateSpeed) const;
        /** @brief direction の水平方向へ即座に向ける */
        void FaceTowards  (const glm::vec3& direction                       ) const;
        void Jump         (const glm::vec3& direction                       ) const;
        /** @brief 入力(x: 右, y: 前)をカメラ基準の水平方向へ変換する。長さは入力のまま */
        [[nodiscard]] glm::vec3 CameraRelativeDirection(const glm::vec2& input) const;
        /** @brief 進行方向にある、登れる最大角度より急な面は速度を消す */
        [[nodiscard]] glm::vec3 LimitToWalkableSlope(const glm::vec3& horizontalVelocity) const;

    private:
        const std::shared_ptr<IPlayerAvatarStateContext> stateContext_;
    };
}
