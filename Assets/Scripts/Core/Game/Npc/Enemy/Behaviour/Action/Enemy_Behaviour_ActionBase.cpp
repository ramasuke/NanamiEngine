#include "Enemy_Behaviour_ActionBase.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus ActionBase::Tick(const Action::TickContext& context)
    {
        // NOTE: Running のまま前回の Tick で呼ばれなかった = 上位の枝に割り込まれたので、途中から再開させない
        if (wasRunning_ && lastTickIndex_ + 1 != context.TickIndex())
            Reset();

        const TickStatus status = DoTick(context);
        lastTickIndex_ = context.TickIndex();
        wasRunning_    = status == TickStatus::Running;
        return status;
    }

    void ActionBase::Reset()
    {
        wasRunning_ = false;
        DoReset();
    }

    void ActionBase::OnDrawGui()
    {
        DoDrawGui();
    }

    void ActionBase::DoReset()
    {

    }

    void ActionBase::DoDrawGui()
    {
        
    }
}
