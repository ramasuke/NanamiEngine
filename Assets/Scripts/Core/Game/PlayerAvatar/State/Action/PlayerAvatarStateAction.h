#pragma once
#include <memory>

#include "../Context/IPlayerAvatarStateContext.h"

namespace GameCore::PlayerAvatar::State
{
    struct PlayerAvatarStateAction final
    {
        explicit PlayerAvatarStateAction(const std::shared_ptr<IPlayerAvatarStateContext>& stateContext);

        // NOTE: カメラ基準で inputVelocity の方向へ進み、進行方向へ向く。deltaTime は掛けずに渡す
        void MoveForward  (const glm::vec3& inputVelocity, float rotateSpeed) const;
        void RotateTowards(const glm::vec3& direction    , float rotateSpeed) const;
        // NOTE: direction の水平方向へ即座に向ける
        void FaceTowards  (const glm::vec3& direction                       ) const;
        void Jump         (const glm::vec3& direction                       ) const;
        // NOTE: 入力(x: 右, y: 前)をカメラ基準の水平方向へ変換する。長さは入力のまま
        [[nodiscard]] glm::vec3 CameraRelativeDirection(const glm::vec2& input) const;
        // NOTE: 進行方向に登れないほど急な面があれば、面へ向かう成分を消す
        [[nodiscard]] glm::vec3 LimitToWalkableSlope(const glm::vec3& horizontalVelocity) const;

    private:
        const std::shared_ptr<IPlayerAvatarStateContext> stateContext_;
    };
}
