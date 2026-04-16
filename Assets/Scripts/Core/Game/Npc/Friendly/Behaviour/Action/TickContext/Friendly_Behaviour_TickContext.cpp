#include "Friendly_Behaviour_TickContext.h"

#include "../../../../../Game.h"
#include "../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../Engine/Module/Component/Collider/ICollider.h"
#include "../../../../../../../GamePlay/Ui/NpcChatting/NpcChatting.h"
#include "../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../Scene/Sub/Content/ChattingUI/ChattingUIScene.h"
#include "../../../../../Scene/Sub/Group/Sub_GameSceneGroup.h"
#include "../../../../../Scene/Sub/Type/SubSceneType.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickContext::TickContext(
        std::string npcName,
        const std::weak_ptr<GameObject::IGameObject>& ownGameObject,
        bool& isChatting,
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters)
        : npcName_      (std::move(npcName))
        , ownGameObject_(ownGameObject   )
        , npcAnimator_  (ownGameObject.lock()->Components().Catch<Component::Animator>())
        , npcCollider_  (ownGameObject.lock()->Components().Catch<Physics::ICollider >())
        , isChatting_   (isChatting        )
        , parameters_   (parameters        )
    {
        
    }

    TickContext::~TickContext() = default;

    GameObject::Transform& TickContext::NpcTransform() const
    {
        return ownGameObject_.lock()->TransformRef();
    }

    const GamePlay::Ui::NpcChatting& TickContext::ChattingUi() const
    {
        const auto& subScenes = Game::Instance().SubScenes();
        const auto& chattingUIScene = subScenes.Catch<Scene::Sub::ChattingUIScene>(Scene::Sub::SceneType::ChattingUI);
        return chattingUIScene->Context().Npc();
    }
}
