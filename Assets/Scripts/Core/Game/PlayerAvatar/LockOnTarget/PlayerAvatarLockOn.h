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
}

// ロックオンの共通処理。剣士と魔術師のステートが使う
namespace GameCore::PlayerAvatar::LockOn
{
    /**
     * @brief LockOn 入力の読み取り・トグル・自動解除をまとめて処理する
     * @param switchDirection ロック中の切り替え。-1 = 左、+1 = 右、0 = なし
     * @return この呼び出しで新しくロックオンしたら true。切り替えは含まない
     * @note ロック中に対象が死亡/索敵範囲外/遮蔽になった場合は自動で解除する
     */
    bool Update(PlayerAvatarCameraGroupBase& cameraGroup,
                const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea,
                const glm::vec3& playerPos,
                bool isLockOnPressed,
                int switchDirection);
    [[nodiscard]] bool IsTargetInRange(const PlayerAvatarCameraGroupBase& cameraGroup,
                                       const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea);
    [[nodiscard]] std::shared_ptr<GameObject::IGameObject> FindNearestTarget(const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea,
                                                                            const glm::vec3& playerPos);
    /**
     * @brief 見えている敵を画面の左右順に並べ、今の狙いの隣へ移る
     * @param direction -1 = 左、+1 = 右。端では反対側の端へ戻る
     */
    void SwitchTarget(PlayerAvatarCameraGroupBase& cameraGroup,
                      const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea,
                      int direction);
    /** @brief カメラから対象まで地形に遮られていないか。敵は遮蔽物に含めない */
    [[nodiscard]] bool HasLineOfSight(const std::shared_ptr<GameObject::IGameObject>& target);
    /** @brief カメラから point まで地形に遮られていないか。target 自身に当たった場合は見えている扱い */
    [[nodiscard]] bool HasLineOfSight(const glm::vec3& point, const GameObject::IGameObject& target);
}
