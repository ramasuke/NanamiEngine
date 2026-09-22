#pragma once
#include <cstdint>
#include <memory>

#include "../glm/vec3.hpp"
#include "StateMachine/EventScene/IPlayerAvatarEventSceneStateMachine.h"


namespace NanamiEngine::Module::Component
{
    class RigidBody;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerInteractable;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStatus;
}

namespace GameCore::PlayerAvatar
{
    enum class PlayerAvatarType;
}

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace GamePlay::PlayerAvatar
{
    class InteractableArea;
}

namespace GamePlay::PlayerAvatar
{
    class WakeUpArea;
}

    namespace GamePlay::Ui
{
    class NpcChatting;
}

namespace GameCore
{
    /**
     * @brief 全てのPlayerAvatarに必要な処理を実装するインターフェース
     */
    class IPlayerAvatar
    {
    public:
        virtual ~IPlayerAvatar() = default;

        /** @brief EventScene用のStateMachine */
        [[nodiscard]] virtual PlayerAvatar::IPlayerAvatarEventSceneStateMachine& GetEventSceneStateMachine() const = 0;
        [[nodiscard]] virtual NanamiEngine::Module::Component::RigidBody       & RigidBody       () const = 0;
        [[nodiscard]] virtual GamePlay::PlayerAvatar::InteractableArea            & InteractableArea   () const = 0;
        [[nodiscard]] virtual GamePlay::PlayerAvatar::WakeUpArea               & WakeUpArea      () const = 0;
        [[nodiscard]] virtual GamePlay::Ui::NpcChatting                        & NpcChattingUi   () const = 0;
        [[nodiscard]] virtual const glm::vec3&                                   FeatStepPosition() const = 0;
        [[nodiscard]] virtual NanamiEngine::Module::GameObject::Transform      & PlayerTransform () const = 0;
        [[nodiscard]] virtual PlayerAvatar::IPlayerAvatarStatus                & PlayerStatus    () const = 0;
        [[nodiscard]] virtual PlayerAvatar::PlayerAvatarType                     Type            () const = 0;
        virtual void SaveStatus() = 0;
        static const std::vector<std::weak_ptr<IPlayerAvatar>>& PlayerAvatars();
        virtual void EnableStateMachiine() = 0;
        virtual void DisableStateMachine() = 0;
        virtual void ApplySyncState(uint8_t stateValue) = 0;
        
    protected:
        static std::vector<std::weak_ptr<IPlayerAvatar>>& PlayerAvatars_();
    };
}
