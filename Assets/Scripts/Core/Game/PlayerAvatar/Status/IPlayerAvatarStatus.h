#pragma once
#include <cstdint>

#include "../../../../../../Libs/LibCore/Rx/ReadOnlyReactiveContext/ReadOnlyReactiveContext.h"
#include "../../../../../../Libs/LibCore/Rx/SerializableSubject/unit/unit.h"
#include "../../StatusParameter/Health/Health.h"
#include "../../StatusParameter/Stamina/Stamina.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

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
    struct IDamage;
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
        [[nodiscard]] virtual Quest::ICompleteQuestGroup& CompletedQuest() const = 0;
        [[nodiscard]] virtual const StatusParameter::Health&                                MaxHealth() const = 0;
        [[nodiscard]] virtual rxcpp::observable<StatusParameter::Health> OnChangeHealth() const = 0;
        [[nodiscard]] virtual StatusParameter::Health                    Health() const = 0;
        [[nodiscard]] virtual const StatusParameter::Stamina&                                MaxStamina() const = 0;
        [[nodiscard]] virtual LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Stamina> Stamina() const = 0;
        [[nodiscard]] virtual bool CanRun() const = 0;
        [[nodiscard]] virtual StatusParameter::MoveSpeed GetWalkSpeed() const = 0;
        [[nodiscard]] virtual StatusParameter::MoveSpeed GetRunSpeed () const = 0;
        [[nodiscard]] virtual float GetMoveRotateSpeed  () const = 0;
        [[nodiscard]] virtual float GetJumpPower        () const = 0;
        [[nodiscard]] virtual float GetJumpCooldown_secs() const = 0;
        [[nodiscard]] virtual bool  IsInjured() const { return false; }
        [[nodiscard]] virtual bool  IsDowned () const { return false; }
        [[nodiscard]] virtual bool  IsDeath  () const { return false; }
        virtual void Revive() {}
        [[nodiscard]] virtual rxcpp::observable<LibCore::Rx::unit> OnBecomeInjured() const
        {
            static rxcpp::subjects::subject<LibCore::Rx::unit> s;
            return s.get_observable();
        }
        [[nodiscard]] virtual rxcpp::observable<LibCore::Rx::unit> OnRecoverFromInjured() const
        {
            static rxcpp::subjects::subject<LibCore::Rx::unit> s;
            return s.get_observable();
        }
        virtual void OnDrawGui() = 0;
        virtual void AddOnDamageStack(std::unique_ptr<IDamage> damageContext) = 0;

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
