#include "NullPlayerAvatarStatus.h"
#include "../../Damage/Game_Damage_IDamage.h"

namespace GameCore::PlayerAvatar
{
    NullPlayerAvatarStatus::NullPlayerAvatarStatus()
        : quest_        (std::make_unique<NullQuestGroup>())
        , completeQuest_(std::make_unique<NullCompleteQuestGroup>())
        , event_        (std::make_unique<NullStatusEvent>())
    {
    }

    NullPlayerAvatarStatus::~NullPlayerAvatarStatus() = default;

    void NullPlayerAvatarStatus::Init()
    {
    }

    void NullPlayerAvatarStatus::OnUpdate()
    {
    }

    IStatusEvent& NullPlayerAvatarStatus::Event() const
    {
        return *event_;
    }

    IQuestGroup& NullPlayerAvatarStatus::Quest() const
    {
        return *quest_;
    }

    Quest::ICompleteQuestGroup& NullPlayerAvatarStatus::CompletedQuest() const
    {
        return *completeQuest_;
    }

    const StatusParameter::Health& NullPlayerAvatarStatus::MaxHealth() const
    {
        return maxHealth_;
    }

    rxcpp::observable<StatusParameter::Health> NullPlayerAvatarStatus::OnChangeHealth() const
    {
        static rxcpp::subjects::subject<StatusParameter::Health> s;
        return s.get_observable();
    }

    StatusParameter::Health NullPlayerAvatarStatus::Health() const
    {
        return StatusParameter::Health(0);
    }

    const EnhancePower& NullPlayerAvatarStatus::MaxEnhancePowerStack() const
    {
        return maxEnhancePowerStack_;
    }

    LibCore::Rx::ReadOnlyReactiveContext<EnhancePower> NullPlayerAvatarStatus::EnhancePowerStack() const
    {
        return enhancePowerStack_.AsReadOnly();
    }

    LibCore::Rx::ReadOnlyReactiveContext<bool> NullPlayerAvatarStatus::IsEnableReinforce() const
    {
        return isReinforceMode_.AsReadOnly();
    }

    StatusParameter::MoveSpeed NullPlayerAvatarStatus::GetWalkSpeed() const
    {
        return StatusParameter::MoveSpeed(0.0f);
    }

    StatusParameter::MoveSpeed NullPlayerAvatarStatus::GetRunSpeed() const
    {
        return StatusParameter::MoveSpeed(0.0f);
    }

    float NullPlayerAvatarStatus::GetMoveRotateSpeed() const
    {
        return 0.0f;
    }

    float NullPlayerAvatarStatus::GetJumpPower() const
    {
        return 0.0f;
    }

    float NullPlayerAvatarStatus::GetJumpCooldown_secs() const
    {
        return 0.0f;
    }

    float NullPlayerAvatarStatus::ReinforceModeDuring_secs() const
    {
        return 0.0f;
    }

    float NullPlayerAvatarStatus::ReinforceModeDuration_secs() const
    {
        return 0.0f;
    }

    bool NullPlayerAvatarStatus::IsOnDisableReinforceMode() const
    {
        return false;
    }

    void NullPlayerAvatarStatus::OnDrawGui()
    {
    }

    void NullPlayerAvatarStatus::AddOnDamageStack(std::unique_ptr<IDamage> damageContext)
    {
    }

    void NullPlayerAvatarStatus::NullQuestGroup::Subscribe(const std::shared_ptr<QuestBase>& addQuest)
    {
    }

    void NullPlayerAvatarStatus::NullCompleteQuestGroup::CompleteQuest(const QuestType& completeQuest)
    {
    }

    bool NullPlayerAvatarStatus::NullCompleteQuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return false;
    }

    rxcpp::observable<StatusParameter::Health> NullPlayerAvatarStatus::NullStatusEvent::OnDamage() const
    {
        static rxcpp::subjects::subject<StatusParameter::Health> s;
        return s.get_observable();
    }

    rxcpp::observable<EnhancePower> NullPlayerAvatarStatus::NullStatusEvent::OnAddEnhancePowerStack() const
    {
        static rxcpp::subjects::subject<EnhancePower> s;
        return s.get_observable();
    }
}
