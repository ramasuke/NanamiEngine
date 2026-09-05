#pragma once
#include <memory>

#include "IPlayerAvatarStatus.h"
#include "Event/PlayerAvatar_IStatusEvent.h"
#include "../Quest/PlayerAvatar_IQuestGroup.h"
#include "../Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../StatusParameter/Health/Health.h"
#include "../../StatusParameter/MoveSpeed/MoveSpeed.h"
#include "../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"

namespace GameCore::PlayerAvatar
{
    /**
     * @brief Statusの「上書きなし」を表すNull Object。
     * PlayerAvatarFactory::LoadInitedPlayerAvatarに、LocalPrefsからの通常ロードにフォールバック
     * させたいことを伝えるセンチネル値として渡される想定で、実際のAvatarには紐付かない。
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

        [[nodiscard]] const StatusParameter::Health&                     MaxHealth() const override;
        [[nodiscard]] rxcpp::observable<StatusParameter::Health> OnChangeHealth() const override;
        [[nodiscard]] StatusParameter::Health                            Health() const override;

        [[nodiscard]] const StatusParameter::Stamina&                                MaxStamina() const override;
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Stamina> Stamina   () const override;
        [[nodiscard]] bool                                                           CanRun    () const override;

        [[nodiscard]] StatusParameter::MoveSpeed GetWalkSpeed          () const override;
        [[nodiscard]] StatusParameter::MoveSpeed GetRunSpeed           () const override;
        [[nodiscard]] float                      GetMoveRotateSpeed    () const override;
        [[nodiscard]] float                      GetJumpPower          () const override;
        [[nodiscard]] float                      GetJumpCooldown_secs  () const override;

        void OnDrawGui() override;
        void AddOnDamageStack(std::unique_ptr<IDamage> damageContext) override;

    private:
        // Event()/Quest()/CompletedQuest()が返す参照を満たすためだけのダミー実装。
        // NullPlayerAvatarStatusは実際のAvatarには使われないため、これらが実際に呼ばれることは想定していない。
        class NullQuestGroup final : public IQuestGroup
        {
        public:
            void Subscribe(const std::shared_ptr<QuestBase>& addQuest) override;
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
            [[nodiscard]] rxcpp::observable<StatusParameter::Health> OnDamage() const override;
        };

        std::unique_ptr<NullQuestGroup        > quest_        ;
        std::unique_ptr<NullCompleteQuestGroup> completeQuest_;
        std::unique_ptr<NullStatusEvent       > event_        ;

        StatusParameter::Health  maxHealth_;
        StatusParameter::Stamina maxStamina_;

        LibCore::Rx::SerializableSubject<StatusParameter::Stamina> stamina_;
    };
}
