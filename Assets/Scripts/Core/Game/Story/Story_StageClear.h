#pragma once
#include <functional>

#include "Story_StoryFlag.h"
#include "../Npc/Enemy/Type/EnemyKind.h"
#include "Packages/R4/R4.h"

namespace GameCore::Story
{
    /** @brief ステージのクリア条件。bossKind の敵を倒したら flag を立てる */
    struct StageClearCondition
    {
        Npc::Enemy::EnemyKind bossKind;
        StoryFlag             flag;
    };

    /** @brief onDefeat に condition.bossKind が流れたら onClear(condition.flag) を呼ぶ。購読は自分が持ち、破棄か Dispose で外す */
    class StageClearWatcher final
    {
    public:
        StageClearWatcher() = default;
        ~StageClearWatcher();
        StageClearWatcher(const StageClearWatcher&) = delete;
        StageClearWatcher& operator=(const StageClearWatcher&) = delete;

        /** @note 見張り中なら前の購読を外してから繋ぎ直す */
        void Watch(
            const NanamiEngine::R4::Observable<Npc::Enemy::EnemyKind>& onDefeat,
            const StageClearCondition& condition,
            std::function<void(StoryFlag)> onClear);
        void Dispose();

    private:
        NanamiEngine::R4::SerialDisposable subscription_;
    };
}
