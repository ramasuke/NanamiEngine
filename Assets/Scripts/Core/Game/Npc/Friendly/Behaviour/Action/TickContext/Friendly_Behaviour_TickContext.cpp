#include "Friendly_Behaviour_TickContext.h"

#include "../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../Engine/Module/Component/Collider/ICollider.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickContext::TickContext(
        std::string npcName,
        const std::weak_ptr<GameObject::IGameObject>& ownGameObject,
        const std::weak_ptr<GamePlay::Ui::NpcChatting>& npcChattingUi,
        bool& isChatting,
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters)
        : npcName_      (std::move(npcName))
        , ownGameObject_(ownGameObject  )
        , npcAnimator_  (ownGameObject.lock()->Components().Catch<Component::Animator>())
        , npcCollider_  (ownGameObject.lock()->Components().Catch<Physics::ICollider >())
        , chattingUi_   (npcChattingUi  )
        , isChatting_   (isChatting        )
        , parameters_   (parameters        )
    {
        
    }

    TickContext::~TickContext() = default;

    GameObject::Transform& TickContext::NpcTransform() const
    {
        return ownGameObject_.lock()->TransformRef();
    }
}
