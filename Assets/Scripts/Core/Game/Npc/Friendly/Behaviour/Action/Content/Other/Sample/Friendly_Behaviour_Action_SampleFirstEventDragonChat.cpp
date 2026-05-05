#include "Friendly_Behaviour_Action_SampleFirstEventDragonChat.h"

#include "../../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus SampleFirstEventDragonChat::DoTick(const TickContext& context)
    {
        AppearFirstEventDragon(context);
        return TickStatus::Success; 
    }

    void SampleFirstEventDragonChat::AppearFirstEventDragon(const TickContext& context)
    {
        Scene::GameObject::Instantiate(firstEventDragonPrefab_.get(), appearFirstEventDragonPosition_);
    }

    void SampleFirstEventDragonChat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("firstEventDragonPrefab_", firstEventDragonPrefab_);
        ImGuiHelper::OnDrawInputField("AppearFirstEventDragonPosition_", appearFirstEventDragonPosition_);
    }
}
