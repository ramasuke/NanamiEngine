#pragma once
#include <memory>

#include "vec3.hpp"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GamePlay::PlayerAvatar
{
    class LockOnDetectionArea;
}

namespace GameCore::PlayerAvatar
{
    class PlayerAvatarCameraGroupBase;

    // NOTE: ロックオンの共通処理。カメラグループと索敵範囲を借りるだけの値オブジェクトで、使う度に作る
    class LockOnController final
    {
    public:
        LockOnController(PlayerAvatarCameraGroupBase& cameraGroup,
                         const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea);

        // NOTE: 入力に応じたロックオンのトグルと自動解除。新しくロックオンしたら true (切り替えは含まない)
        // NOTE: switchDirection はロック中の切り替え (-1 = 左、+1 = 右、0 = なし)
        bool Update(const glm::vec3& playerPos, bool isLockOnPressed, int switchDirection) const;
        [[nodiscard]] bool IsTargetInRange() const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> FindNearestTarget(const glm::vec3& playerPos) const;
        // NOTE: 見えている敵を画面の左右順に並べ、今の狙いの隣へ移る
        // NOTE: direction は -1 = 左、+1 = 右。端では反対側の端へ戻る
        void SwitchTarget(int direction) const;

    private:
        // NOTE: カメラから対象まで地形に遮られていないか。敵は遮蔽物に含めない
        [[nodiscard]] static bool HasLineOfSight(const std::shared_ptr<GameObject::IGameObject>& target);
        // NOTE: カメラから point まで地形に遮られていないか。target 自身に当たった場合は見えている扱い
        [[nodiscard]] static bool HasLineOfSight(const glm::vec3& point, const GameObject::IGameObject& target);

        PlayerAvatarCameraGroupBase& cameraGroup_;
        const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea_;
    };
}
