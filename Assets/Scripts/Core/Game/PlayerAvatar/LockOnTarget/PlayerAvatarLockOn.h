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

    /**
     * @brief ロックオンの共通処理。剣士と魔術師のステートが使う
     * @note カメラグループと索敵範囲を借りるだけの値オブジェクト。使う度に作る（PlayerAvatarStateBase::LockOnControl）
     */
    class LockOnController final
    {
    public:
        LockOnController(PlayerAvatarCameraGroupBase& cameraGroup,
                         const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea);

        /**
         * @brief LockOn 入力の読み取り・トグル・自動解除をまとめて処理する
         * @param switchDirection ロック中の切り替え。-1 = 左、+1 = 右、0 = なし
         * @return この呼び出しで新しくロックオンしたら true。切り替えは含まない
         * @note ロック中に対象が死亡/索敵範囲外/遮蔽になった場合は自動で解除する
         */
        bool Update(const glm::vec3& playerPos, bool isLockOnPressed, int switchDirection) const;
        [[nodiscard]] bool IsTargetInRange() const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> FindNearestTarget(const glm::vec3& playerPos) const;
        /**
         * @brief 見えている敵を画面の左右順に並べ、今の狙いの隣へ移る
         * @param direction -1 = 左、+1 = 右。端では反対側の端へ戻る
         */
        void SwitchTarget(int direction) const;

    private:
        /** @brief カメラから対象まで地形に遮られていないか。敵は遮蔽物に含めない */
        [[nodiscard]] static bool HasLineOfSight(const std::shared_ptr<GameObject::IGameObject>& target);
        /** @brief カメラから point まで地形に遮られていないか。target 自身に当たった場合は見えている扱い */
        [[nodiscard]] static bool HasLineOfSight(const glm::vec3& point, const GameObject::IGameObject& target);

        PlayerAvatarCameraGroupBase& cameraGroup_;
        const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea_;
    };
}
