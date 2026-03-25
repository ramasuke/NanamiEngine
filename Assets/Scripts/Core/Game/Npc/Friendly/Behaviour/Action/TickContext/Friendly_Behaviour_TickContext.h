#pragma once
#include <memory>
#include <string>
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace NanamiEngine::Module::Physics
{
    class ICollider;
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
            const std::weak_ptr<GamePlay::Ui::NpcChatting>& npcChattingUi,
            bool& isChatting,
            const std::unique_ptr<BlackBoard::ParameterGroup>& parameters);
        ~TickContext();

        [[nodiscard]] bool&                                  IsChatting   () const { return isChatting_;          }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> NpcGameObject() const { return ownGameObject_; }
        [[nodiscard]] GameObject::Transform&           NpcTransform () const;
        [[nodiscard]] Component::Animator&             NpcAnimator  () const { return *npcAnimator_.lock(); }
        [[nodiscard]] Physics::ICollider &             NpcCollider  () const { return *npcCollider_.lock(); }
        [[nodiscard]] const std::string              & NpcName      () const { return npcName_;             }
        [[nodiscard]] const GamePlay::Ui::NpcChatting& ChattingUi   () const { return *chattingUi_.lock();  }
        [[nodiscard]] const std::unique_ptr<BlackBoard::ParameterGroup>& Parameter() const { return parameters_; }
        
    private:
        const std::string npcName_;
        const std::weak_ptr<GameObject::IGameObject> ownGameObject_;
        const std::weak_ptr<Component::Animator> npcAnimator_;
        const std::weak_ptr<Physics::ICollider > npcCollider_;
        const std::weak_ptr<GamePlay::Ui::NpcChatting> chattingUi_;
        bool& isChatting_;
        const std::unique_ptr<BlackBoard::ParameterGroup>& parameters_;
    };
}
