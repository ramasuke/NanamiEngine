#pragma once
#include <cstdint>

#include "Packages/R4/R4.h"
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

namespace GameCore::PlayerAvatar
{
    class Wallet;
    class ItemPouch;
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
        [[nodiscard]] virtual PlayerAvatar::Wallet& Wallet() const = 0;
        [[nodiscard]] virtual ItemPouch&       Pouch()       = 0;
        [[nodiscard]] virtual const ItemPouch& Pouch() const = 0;
        [[nodiscard]] virtual const StatusParameter::Health&                                MaxHealth() const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<StatusParameter::Health> OnChangeHealth() const = 0;
        [[nodiscard]] virtual StatusParameter::Health                    Health() const = 0;
        [[nodiscard]] virtual const StatusParameter::Stamina&                                MaxStamina() const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::ReadOnlyReactiveProperty<StatusParameter::Stamina> Stamina() const = 0;
        [[nodiscard]] virtual bool CanRun() const = 0;
        [[nodiscard]] virtual StatusParameter::MoveSpeed GetWalkSpeed() const = 0;
        [[nodiscard]] virtual StatusParameter::MoveSpeed GetRunSpeed () const = 0;
        [[nodiscard]] virtual float GetMoveRotateSpeed  () const = 0;
        [[nodiscard]] virtual float GetJumpPower        () const = 0;
        [[nodiscard]] virtual float GetJumpStateDuration_secs() const = 0;
        [[nodiscard]] virtual bool  IsInjured() const { return false; }
        [[nodiscard]] virtual bool  IsDowned () const { return false; }
        [[nodiscard]] virtual bool  IsDeath  () const { return false; }
        virtual void Revive() {}
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnBecomeInjured() const
        {
            return NanamiEngine::R4::Observable<NanamiEngine::R4::Unit>::Never();
        }
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnRecoverFromInjured() const
        {
            return NanamiEngine::R4::Observable<NanamiEngine::R4::Unit>::Never();
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
