#pragma once
#include <cstdint>

#include "../../../../../../Libs/LibCore/Rx/ReadOnlyReactiveContext/ReadOnlyReactiveContext.h"
#include "../../../../../../Libs/LibCore/Rx/SerializableSubject/unit/unit.h"
#include "../../StatusParameter/Health/Health.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "EnahancePower/EnhancePower.h"

namespace GameCore::PlayerAvatar
{
    class IQuestGroup;
}

namespace GameCore::PlayerAvatar
{
    class IStatusEvent;
}

namespace GameCore
{
    struct IDamageContext;
}

namespace GameCore::StatusParameter
{
    struct MoveSpeed;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStatus
    {
    public:
        virtual ~IPlayerAvatarStatus() = default;
        virtual void Init    () = 0;
        virtual void OnUpdate() = 0;
        [[nodiscard]] virtual IStatusEvent& Event() const = 0;
        [[nodiscard]] virtual IQuestGroup & Quest() const = 0;
        [[nodiscard]] virtual const StatusParameter::Health&                                MaxHealth() const = 0;
        [[nodiscard]] virtual LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Health> Health   () const = 0;
        [[nodiscard]] virtual const EnhancePower&                                MaxEnhancePowerStack() const = 0;
        [[nodiscard]] virtual LibCore::Rx::ReadOnlyReactiveContext<EnhancePower> EnhancePowerStack() const = 0;
        [[nodiscard]] virtual LibCore::Rx::ReadOnlyReactiveContext<bool> IsEnableReinforce() const = 0;
        [[nodiscard]] virtual StatusParameter::MoveSpeed GetWalkSpeed() const = 0;
        [[nodiscard]] virtual StatusParameter::MoveSpeed GetRunSpeed () const = 0;
        [[nodiscard]] virtual float GetMoveRotateSpeed() const = 0;
        [[nodiscard]] virtual float GetJumpPower      () const = 0;
        [[nodiscard]] virtual float GetJumpCooldown_secs() const = 0;
        [[nodiscard]] virtual float ReinforceModeDuring_secs() const = 0;
        [[nodiscard]] virtual float ReinforceModeDuration_secs() const = 0;
        [[nodiscard]] virtual bool  IsOnDisableReinforceMode() const = 0;
        virtual void OnDrawGui() = 0;
        virtual void AddOnDamageStack(std::unique_ptr<IDamageContext> damageContext) = 0;

        template <class Archive>
        void save(Archive& archive, const uint32_t version) const
        {
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
        }
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::IPlayerAvatarStatus, 0);
#pragma endregion
