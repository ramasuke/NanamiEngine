#pragma once
#include <memory>

#include "IPlayerAvatarStatus.h"
#include "Event/PlayerAvatar_IStatusEvent.h"
#include "../Quest/PlayerAvatar_IQuestGroup.h"
#include "../Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../Wallet/PlayerAvatar_Wallet.h"
#include "../Item/ItemPouch.h"
#include "../../StatusParameter/Health/Health.h"
#include "../../StatusParameter/MoveSpeed/MoveSpeed.h"
#include "Packages/R4/R4.h"

namespace GameCore::PlayerAvatar
{
    /**
     * @brief StatusのNull Object。
     */
    class NullPlayerAvatarStatus final : public IPlayerAvatarStatus
    {
    public:
        NullPlayerAvatarStatus();
        ~NullPlayerAvatarStatus() override;

        void Init    () override;
        void OnUpdate() override;

        [[nodiscard]] IStatusEvent              & Event         () const override;
        [[nodiscard]] IQuestGroup                & Quest         () const override;
        [[nodiscard]] Quest::ICompleteQuestGroup & CompletedQuest() const override;
        [[nodiscard]] PlayerAvatar::Wallet         & Wallet        () const override;
        [[nodiscard]] ItemPouch                    & Pouch         ()       override { return pouch_; }
        [[nodiscard]] const ItemPouch              & Pouch         () const override { return pouch_; }

        [[nodiscard]] const StatusParameter::Health&                     MaxHealth() const override;
        [[nodiscard]] NanamiEngine::R4::Observable<StatusParameter::Health> OnChangeHealth() const override;
        [[nodiscard]] StatusParameter::Health                            Health() const override;

        [[nodiscard]] const StatusParameter::Stamina&                                MaxStamina() const override;
        [[nodiscard]] NanamiEngine::R4::ReadOnlyReactiveProperty<StatusParameter::Stamina> Stamina   () const override;
        [[nodiscard]] bool                                                           CanRun    () const override;

        [[nodiscard]] StatusParameter::MoveSpeed GetWalkSpeed          () const override;
        [[nodiscard]] StatusParameter::MoveSpeed GetRunSpeed           () const override;
        [[nodiscard]] float                      GetMoveRotateSpeed    () const override;
        [[nodiscard]] float                      GetJumpPower          () const override;
        [[nodiscard]] float                      GetJumpStateDuration_secs() const override;

        void OnDrawGui() override;
        void AddOnDamageStack(std::unique_ptr<IDamage> damageContext) override;

    private:
        // Event()/Quest()/CompletedQuest()が返す参照を満たすためだけのダミー実装。
        // NullPlayerAvatarStatusは実際のAvatarには使われないため、これらが実際に呼ばれることは想定していない。
        class NullQuestGroup final : public IQuestGroup
        {
        public:
            void Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest) override;
            [[nodiscard]] bool IsTaking(const QuestType& quest) const override;
        };

        class NullCompleteQuestGroup final : public Quest::ICompleteQuestGroup
        {
        public:
            void CompleteQuest(const QuestType& completeQuest) override;
            [[nodiscard]] bool CheckCompleted(const QuestType& quest) const override;
        };

        class NullStatusEvent final : public IStatusEvent
        {
        public:
            [[nodiscard]] NanamiEngine::R4::Observable<StatusParameter::Health> OnDamage() const override;
        };

        std::unique_ptr<NullQuestGroup        > quest_        ;
        std::unique_ptr<NullCompleteQuestGroup> completeQuest_;
        std::unique_ptr<NullStatusEvent       > event_        ;
        std::unique_ptr<PlayerAvatar::Wallet  > wallet_       ;
        ItemPouch                               pouch_        ;

        StatusParameter::Health  maxHealth_;
        StatusParameter::Stamina maxStamina_;

        NanamiEngine::R4::SerializableReactiveProperty<StatusParameter::Stamina> stamina_;
    };
}
