#pragma once
#include "Story_StoryFlag.h"
#include "../Npc/Enemy/Type/EnemyKind.h"

namespace GameCore::Story
{
    // NOTE: ステージのクリア条件。bossKind の敵を倒したら flag を立てる
    struct StageClearCondition
    {
        Npc::Enemy::EnemyKind bossKind;
        StoryFlag             flag;
    };
}
