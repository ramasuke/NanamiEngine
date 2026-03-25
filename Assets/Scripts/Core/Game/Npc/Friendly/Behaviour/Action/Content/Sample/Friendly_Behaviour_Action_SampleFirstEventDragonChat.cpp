#include "Friendly_Behaviour_Action_SampleFirstEventDragonChat.h"

#include "../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus SampleFirstEventDragonChat::DoTick(const TickContext& context)
    {
        if (context.IsChatting())
        {
            context.IsChatting() = false;
            AppearFirstEventDragon(context);
            return TickStatus::Success; 
        }
        return TickStatus::Failure;
    }

    void SampleFirstEventDragonChat::AppearFirstEventDragon(const TickContext& context)
    {
        const auto dragon = Scene::GameObject::Instantiate(firstEventDragonPrefab_.get(), appearFirstEventDragonPosition_);
        //dragon.lock()->TransformRef().SetParent(context.NpcGameObject(), true);
        //dragon.lock()->TransformRef().SetParent(std::weak_ptr<GameObject::IGameObject>(), true);
    }

    void SampleFirstEventDragonChat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("firstEventDragonPrefab_", firstEventDragonPrefab_);
        ImGuiHelper::OnDrawInputField("AppearFirstEventDragonPosition_", appearFirstEventDragonPosition_);
    }
}
