#pragma once
#include "../Condition/Condition_ConditionContext.h"

namespace GameCore::PlayerAvatar
{
    class Wallet;
}

namespace GameCore::Decoration
{
    class DecorationCollection;
}

namespace GameCore::Reward
{
    // NOTE: 報酬の渡し先。どれも空のことがあり、空の渡し先の報酬は出さない
    struct RewardContext
    {
        PlayerAvatar::Wallet*             wallet      = nullptr;
        Decoration::DecorationCollection* decorations = nullptr;
        // NOTE: 報酬ごとの conditions_ を判定するときに見る
        Condition::ConditionContext       conditions;
    };
}
