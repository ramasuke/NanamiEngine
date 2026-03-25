#include "Friendly_Behaviour_ActionBase.h"

#include "../../../../PlayerAvatar/IPlayerAvatar.h"

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
        return IPlayerAvatar::PlayerAvatars().at(0).lock();
    }
}
