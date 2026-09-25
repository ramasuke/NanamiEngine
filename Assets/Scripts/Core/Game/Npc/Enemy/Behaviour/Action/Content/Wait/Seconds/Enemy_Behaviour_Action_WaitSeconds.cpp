#include "Enemy_Behaviour_Action_WaitSeconds.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

    void Action::WaitSeconds::DoReset()
    {
        during_secs_ = 0.0f;
    }

    void Action::WaitSeconds::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("waitSeconds_", waitSeconds_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::WaitSeconds, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
