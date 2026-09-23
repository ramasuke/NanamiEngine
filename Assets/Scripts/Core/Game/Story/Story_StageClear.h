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

    /**
     * @brief onDefeat に condition.bossKind が流れたら onClear(condition.flag) を呼ぶ。
     * @return 購読。持ち主が Dispose する
     */
    [[nodiscard]] NanamiEngine::R4::Disposable WatchStageClear(
        const NanamiEngine::R4::Observable<Npc::Enemy::EnemyKind>& onDefeat,
        const StageClearCondition& condition,
        std::function<void(StoryFlag)> onClear);
}
