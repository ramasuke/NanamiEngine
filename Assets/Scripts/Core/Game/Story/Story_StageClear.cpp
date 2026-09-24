#include "Story_StageClear.h"

namespace GameCore::Story
{
    StageClearWatcher::~StageClearWatcher()
    {
        Dispose();
    }

    void StageClearWatcher::Watch(
        const NanamiEngine::R4::Observable<Npc::Enemy::EnemyKind>& onDefeat,
        const StageClearCondition& condition,
        std::function<void(StoryFlag)> onClear)
    {
        subscription_.Set(onDefeat.Subscribe([condition, onClear = std::move(onClear)](const Npc::Enemy::EnemyKind kind)
        {
            if (kind == condition.bossKind)
                onClear(condition.flag);
        }));
    }

    void StageClearWatcher::Dispose()
    {
        subscription_.Dispose();
    }
}
