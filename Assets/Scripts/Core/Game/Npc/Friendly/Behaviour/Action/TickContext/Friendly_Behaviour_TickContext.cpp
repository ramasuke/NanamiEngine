#include "Friendly_Behaviour_TickContext.h"

#include "../../../../../Game.h"
#include "../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../GamePlay/Ui/NpcChatting/NpcChatting.h"
#include "../../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../Scene/Sub/Content/ChattingUI/ChattingUIScene.h"
#include "../../../../../Scene/Sub/Group/Sub_GameSceneGroup.h"
#include "../../../../../Scene/Sub/Type/SubSceneType.h"
#include "../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickContext::TickContext(
        std::string npcName,
        const std::weak_ptr<GameObject::IGameObject>& ownGameObject,
        const std::weak_ptr<GamePlay::Ui::BillBoardNpcChatIcon>& ownChatIcon,
        bool& isChatting,
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters)
        : npcName_      (std::move(npcName))
        , ownGameObject_(ownGameObject   )
        , ownChatIcon_  (ownChatIcon)
        , npcAnimator_  (ownGameObject.lock()->Components().Catch<Component::Animator>())
        , npcCollider_  (ownGameObject.lock()->Components().Catch<Component::ColliderBase>())
        , isChatting_   (isChatting        )
        , parameters_   (parameters        )
    {
        
    }

    TickContext::~TickContext() = default;

    GameObject::Transform& TickContext::NpcTransform() const
    {
        return ownGameObject_.lock()->Transform();
    }

    const GamePlay::Ui::NpcChatting& TickContext::ChatUi() const
    {
        const auto& subScenes = Game::Instance().SubScenes();
        const auto& chattingUIScene = subScenes.Catch<Scene::Sub::ChattingUIScene>(Scene::Sub::SceneType::ChattingUI);
        return chattingUIScene->Context().Npc();
    }

    std::shared_ptr<IPlayerAvatar> TickContext::Player() const
    {
        return PlayerAvatar::Owner();
    }

    const PlayerAvatar::IQuestGroup& TickContext::PlayerQuest() const
    {
        return Player()->PlayerStatus().Quest();
    }

    const PlayerAvatar::Quest::ICompleteQuestGroup& TickContext::PlayerCompleteQuest() const
    {
        return Player()->PlayerStatus().CompletedQuest();
    }
}
