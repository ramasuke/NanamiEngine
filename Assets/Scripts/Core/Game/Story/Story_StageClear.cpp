#include "Story_StageClear.h"

namespace GameCore::Story
{
    NanamiEngine::R4::Disposable WatchStageClear(
        const NanamiEngine::R4::Observable<Npc::Enemy::EnemyKind>& onDefeat,
        const StageClearCondition& condition,
        std::function<void(StoryFlag)> onClear)
    {
        return onDefeat.Subscribe([condition, onClear = std::move(onClear)](const Npc::Enemy::EnemyKind kind)
        {
            if (kind == condition.bossKind)
                onClear(condition.flag);
        });
    }
}
