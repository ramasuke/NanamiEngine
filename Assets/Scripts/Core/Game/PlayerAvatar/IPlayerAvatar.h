#pragma once
#include <memory>

#include "vec3.hpp"
#include "../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "StateMachine/EventScene/IPlayerAvatarEventSceneStateMachine.h"


namespace NanamiEngine::Module::Component
{
    class ColliderBase;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerChattable;
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
    class ChattableArea;
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
        [[nodiscard]] virtual NanamiEngine::Module::Component::ColliderBase    & Collider        () const = 0;
        [[nodiscard]] virtual GamePlay::PlayerAvatar::ChattableArea   &          ChattableArea   () const = 0;
        [[nodiscard]] virtual GamePlay::Ui::NpcChatting               &          NpcChattingUi   () const = 0;
        [[nodiscard]] virtual const glm::vec3&                                   FeatStepPosition() const = 0;
        [[nodiscard]] virtual const NanamiEngine::Module::GameObject::Transform& PlayerTransform () const = 0;
        [[nodiscard]] virtual PlayerAvatar::IPlayerAvatarStatus                & PlayerStatus    () const = 0;
        [[nodiscard]] virtual PlayerAvatar::PlayerAvatarType                     Type            () const = 0;
        virtual void SaveStatus() = 0;
        static const std::vector<std::weak_ptr<IPlayerAvatar>>& PlayerAvatars();
        virtual void EnableStateMachiine() = 0;
        virtual void DisableStateMachine() = 0;
        
    protected:
        static std::vector<std::weak_ptr<IPlayerAvatar>>& PlayerAvatars_();
    };
}
