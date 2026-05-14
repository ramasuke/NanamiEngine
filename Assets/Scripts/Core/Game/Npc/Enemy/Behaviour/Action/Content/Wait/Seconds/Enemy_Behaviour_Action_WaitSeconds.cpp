#include "Enemy_Behaviour_Action_WaitSeconds.h"

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::WaitSeconds::DoTick(const TickContext& context)
    {
        if (waitSeconds_ <= during_secs_)
        {
            during_secs_ += Time::DeltaTime();
            return TickStatus::Success;
        }
        
        during_secs_ += Time::DeltaTime();
        return TickStatus::Running;
    }

    void Action::WaitSeconds::Reset()
    {
        during_secs_ = 0.0f;
    }

    void Action::WaitSeconds::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("waitSeconds_", waitSeconds_);
    }
}
