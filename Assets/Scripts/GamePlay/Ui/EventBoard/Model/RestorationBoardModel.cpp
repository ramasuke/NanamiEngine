#include "RestorationBoardModel.h"

#include "../../Format/Ui_MoneyFormat.h"
#include "../../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "../../../../Core/Game/Story/Story_StoryProgress.h"

namespace GamePlay::Ui
{
    namespace
    {
        RestorationBoardState ResolveRestorationBoardState(const Asset::RestorationFacility& facility)
        {
            const auto& story = GameCore::Story::StoryProgress::Instance();
            if (story.IsRestored(facility.Facility()))
                return RestorationBoardState::Restored;

            const auto flag = facility.RequiredStoryFlag();
            if (flag && !story.IsSet(*flag))
                return RestorationBoardState::Locked;

            const auto required = facility.RequiredFacility();
            if (required && !story.IsRestored(*required))
                return RestorationBoardState::Locked;

            return RestorationBoardState::Open;
        }

        std::vector<RestorationBoardEntry> BuildRestorationBoardEntries(
            const std::vector<std::shared_ptr<Asset::RestorationFacility>>& facilities)
        {
            if (!GameCore::Story::StoryProgress::Instance().IsSet(GameCore::Story::StoryFlag::RestorationStarted))
                return {};

            std::vector<RestorationBoardEntry> entries;
            for (const auto& facility : facilities)
            {
                RestorationBoardEntry entry;
                entry.facility = facility;
                entry.costText = FormatMoney(facility->Cost());
                entries.push_back(std::move(entry));
            }
            return entries;
        }
    }

    RestorationBoardModel::RestorationBoardModel(
        const std::vector<std::shared_ptr<Asset::RestorationFacility>>& facilities,
        GameCore::PlayerAvatar::Wallet* wallet,
        const size_t visibleRowCount)
        : entries_(BuildRestorationBoardEntries(facilities))
        , wallet_(wallet)
        , cursor_(entries_.size(), visibleRowCount)
    {
        Reevaluate();
    }

    const RestorationBoardEntry* RestorationBoardModel::Selected() const
    {
        if (cursor_.SelectedIndex() >= entries_.size())
            return nullptr;

        return &entries_[cursor_.SelectedIndex()];
    }

    int RestorationBoardModel::Balance() const
    {
        return wallet_ ? wallet_->Balance().Value() : 0;
    }

    bool RestorationBoardModel::RestoreSelected()
    {
        const auto entry = Selected();
        if (!entry || !wallet_ || entry->state != RestorationBoardState::Open)
            return false;

        if (!wallet_->TrySpend(GameCore::StatusParameter::Money(entry->facility->Cost())))
            return false;

        GameCore::Story::StoryProgress::Instance().Restore(entry->facility->Facility());
        Reevaluate();
        return true;
    }

    void RestorationBoardModel::Reevaluate()
    {
        const int balance = Balance();
        for (auto& entry : entries_)
        {
            entry.state        = ResolveRestorationBoardState(*entry.facility);
            entry.isAffordable = wallet_ && balance >= entry.facility->Cost();

            switch (entry.state)
            {
            case RestorationBoardState::Open:
                entry.stateText = entry.isAffordable ? "直せる" : "お金が足りない";
                entry.noteText  = entry.stateText;
                break;
            case RestorationBoardState::Locked:
                entry.stateText = "未開";
                entry.noteText  = entry.facility->ConditionText();
                break;
            case RestorationBoardState::Restored:
                entry.stateText = "竣工";
                entry.noteText  = "直した";
                break;
            }
        }
    }
}
