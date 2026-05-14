#pragma once
#include <memory>
#include <string>
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::Component
{
    class ColliderBase;
}

namespace GamePlay::Ui
{
    class BillBoardNpcChatIcon;
}

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::PlayerAvatar
{
    class IQuestGroup;
}

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace NanamiEngine::Module::Component
{
    class Animator;
}

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace GamePlay::Ui
{
    class NpcChatting;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    struct TickContext final
    {
        explicit TickContext(
            std::string npcName,
            const std::weak_ptr<GameObject::IGameObject>& ownGameObject,
            const std::weak_ptr<GamePlay::Ui::BillBoardNpcChatIcon>& ownChatIcon,
            bool& isChatting,
            const std::unique_ptr<BlackBoard::ParameterGroup>& parameters);
        ~TickContext();

        [[nodiscard]] bool&                                  IsChatting   () const { return isChatting_;      }
        [[nodiscard]] GamePlay::Ui::BillBoardNpcChatIcon&    OwnChatIcon  () const { return *ownChatIcon_  .lock(); }
        [[nodiscard]] GameObject::IGameObject&         NpcGameObject() const { return *ownGameObject_.lock(); }
        [[nodiscard]] GameObject::Transform&           NpcTransform () const;
        [[nodiscard]] Component::Animator&             NpcAnimator  () const { return *npcAnimator_.lock(); }
        [[nodiscard]] Component::ColliderBase&         NpcCollider  () const { return *npcCollider_.lock(); }
        [[nodiscard]] const std::string              & NpcName      () const { return npcName_;             }
        [[nodiscard]] const GamePlay::Ui::NpcChatting& ChatUi   () const;
        [[nodiscard]] const std::unique_ptr<BlackBoard::ParameterGroup>& Parameter() const { return parameters_; }
        [[nodiscard]] std::shared_ptr<IPlayerAvatar> Player() const;
        [[nodiscard]] const PlayerAvatar::IQuestGroup& PlayerQuest() const;
        
    private:
        const std::string npcName_;
        const std::weak_ptr<GameObject::IGameObject> ownGameObject_;
        const std::weak_ptr<GamePlay::Ui::BillBoardNpcChatIcon> ownChatIcon_;
        const std::weak_ptr<Component::Animator> npcAnimator_;
        const std::weak_ptr<Component::ColliderBase> npcCollider_;
        bool& isChatting_;
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters_;
    };
}
