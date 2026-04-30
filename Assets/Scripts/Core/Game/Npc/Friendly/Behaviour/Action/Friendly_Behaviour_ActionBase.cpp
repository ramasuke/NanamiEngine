#include "Friendly_Behaviour_ActionBase.h"

#include "../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"

namespace GameCore::Npc::Friendly::Behaviour
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

    std::shared_ptr<IPlayerAvatar> ActionBase::GetPlayerAvatar() const
    {
        return PlayerAvatar::Owner();
    }
}
