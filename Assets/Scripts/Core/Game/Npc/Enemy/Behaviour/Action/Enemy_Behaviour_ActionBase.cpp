#include "Enemy_Behaviour_ActionBase.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus ActionBase::Tick(const Action::TickContext& context)
    {
        return DoTick(context);
    }

    void ActionBase::OnDrawGui()
    {
        DoDrawGui();
    }

    void ActionBase::DoDrawGui()
    {
        
    }
}
