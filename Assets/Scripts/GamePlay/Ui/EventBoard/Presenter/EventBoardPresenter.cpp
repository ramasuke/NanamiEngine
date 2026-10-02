#include "EventBoardPresenter.h"

#include <algorithm>

#include "../../../../Core/Game/Condition/Condition_Clock.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../Prop/RestorationGate/Prop_RestorationGate.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_ITakeableQuest.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "../../../../Core/Game/Decoration/Decoration_DecorationCollection.h"
#include "../../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void EventBoardPresenter::OnStart()
    {
        screen_ = RequireComponent<UiFlow::UiScreen>();
        // 調べるたびに二重に生えるのを防ぐ
        if (!screen_->Open())
        {
            isClosed_ = true;
            Entity().lock()->OnDestroy();
            return;
        }
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);

        view_ = RequireComponent<EventBoardUi>();
        view_->Build();

        const auto owner = GameCore::PlayerAvatar::Owner();
        suspendedAvatar_ = owner;

        const auto board = board_.get();
        const auto now   = GameCore::Condition::Clock::Now();
        const auto questPage  = view_->QuestPage();
        const auto eventPage  = view_->EventPage();
        const auto noticePage = view_->NoticePage();
        const auto restorationPage = view_->RestorationPage();

        questModel_ = std::make_unique<QuestBoardModel>(
            board ? board->Quests() : std::vector<std::shared_ptr<Asset::BoardQuest>>{},
            now,
            owner ? &owner->PlayerStatus().Quest() : nullptr,
            owner ? &owner->PlayerStatus().CompletedQuest() : nullptr,
            GameCore::Story::StoryProgress::Instance(),
            questPage ? questPage->MaxVisibleRows() : 0);
        eventModel_ = std::make_unique<EventBoardModel>(
            board ? board->Notices() : std::vector<std::shared_ptr<Asset::EventNotice>>{},
            now,
            GameCore::Condition::ConditionContext{
                GameCore::Story::StoryProgress::Instance(),
                owner ? &owner->PlayerStatus().CompletedQuest() : nullptr,
                now,
                GameCore::Decoration::DecorationCollection::Instance() },
            eventPage ? eventPage->MaxVisibleRows() : 0);
        noticeModel_ = std::make_unique<NoticeBoardModel>(
            board ? board->Announcements() : std::vector<std::shared_ptr<Asset::Announcement>>{},
            now,
            noticePage ? noticePage->MaxVisibleRows() : 0);
        restorationModel_ = std::make_unique<RestorationBoardModel>(
            board ? board->Facilities() : std::vector<std::shared_ptr<Asset::RestorationFacility>>{},
            owner ? &owner->PlayerStatus().Wallet() : nullptr,
            restorationPage ? restorationPage->MaxVisibleRows() : 0);

        if (questPage)
        {
            questPage->BuildRows(std::min(questModel_->Entries().size(), questModel_->Cursor().VisibleRowCount()));
            questPage->SubscribeOnClickRow([this](const size_t row)
            {
                questModel_->Cursor().Select(questModel_->Cursor().FirstVisibleIndex() + row);
            });
        }
        if (eventPage)
        {
            eventPage->BuildRows(std::min(eventModel_->Entries().size(), eventModel_->Cursor().VisibleRowCount()));
            eventPage->SubscribeOnClickRow([this](const size_t row)
            {
                eventModel_->Cursor().Select(eventModel_->Cursor().FirstVisibleIndex() + row);
            });
        }
        if (noticePage)
        {
            noticePage->BuildRows(std::min(noticeModel_->Entries().size(), noticeModel_->Cursor().VisibleRowCount()));
            noticePage->SubscribeOnClickRow([this](const size_t row)
            {
                noticeModel_->Cursor().Select(noticeModel_->Cursor().FirstVisibleIndex() + row);
            });
        }
        if (restorationPage)
        {
            restorationPage->BuildRows(std::min(restorationModel_->Entries().size(), restorationModel_->Cursor().VisibleRowCount()));
            restorationPage->SubscribeOnClickRow([this](const size_t row)
            {
                restorationModel_->Cursor().Select(restorationModel_->Cursor().FirstVisibleIndex() + row);
            });
        }

        for (size_t i = 0; i < EVENT_BOARD_TAB_COUNT; ++i)
        {
            const auto type = static_cast<EventBoardTabType>(i);
            if (const auto tab = view_->Tab(type))
            {
                tab->SetBadgeCount(0);
                tab->SubscribeOnClick([this, type]
                {
                    SelectTab(type);
                });
            }
        }

        const auto onSelectionChanged = [this](size_t)
        {
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
            Refresh();
        };
        questModel_ ->Cursor().OnSelectionChanged().Subscribe(onSelectionChanged).AddTo(this);
        eventModel_ ->Cursor().OnSelectionChanged().Subscribe(onSelectionChanged).AddTo(this);
        noticeModel_->Cursor().OnSelectionChanged().Subscribe(onSelectionChanged).AddTo(this);
        restorationModel_->Cursor().OnSelectionChanged().Subscribe(onSelectionChanged).AddTo(this);

        view_->ShowTab(currentTab_);
        Refresh();
    }

    void EventBoardPresenter::OnUpdate()
    {
        if (isClosed_ || !view_)
            return;

        using UiFlow::UiAction;
        auto& input = screen_->Input();

        if (input.IsPressed(UiAction::Up))
            CurrentCursor().Move(-1);
        if (input.IsPressed(UiAction::Down))
            CurrentCursor().Move(1);
        // 頁は Q / E (LB / RB) のほか、左右でも切り替わる
        if (input.IsPressed(UiAction::TabPrev) || input.IsPressed(UiAction::Left))
            SwitchTab(-1);
        if (input.IsPressed(UiAction::TabNext) || input.IsPressed(UiAction::Right))
            SwitchTab(1);
        if (input.IsPressed(UiAction::Submit))
            Confirm();
        if (input.IsPressed(UiAction::Cancel))
            Close();
    }

    BoardListCursor& EventBoardPresenter::CurrentCursor() const
    {
        switch (currentTab_)
        {
        case EventBoardTabType::Quest:  return questModel_->Cursor();
        case EventBoardTabType::Event:  return eventModel_->Cursor();
        case EventBoardTabType::Notice: return noticeModel_->Cursor();
        case EventBoardTabType::Restoration: return restorationModel_->Cursor();
        }
        return questModel_->Cursor();
    }

    bool EventBoardPresenter::CanAcceptSelected() const
    {
        if (currentTab_ != EventBoardTabType::Quest || suspendedAvatar_.expired())
            return false;

        const auto entry = questModel_->Selected();
        return entry && entry->state == QuestBoardState::Open;
    }

    bool EventBoardPresenter::CanRestoreSelected() const
    {
        if (currentTab_ != EventBoardTabType::Restoration || suspendedAvatar_.expired())
            return false;

        // NOTE: お金が足りなくても A は出す。押すと断りの音で足りないと分かる(店と同じ)
        const auto entry = restorationModel_->Selected();
        return entry && entry->state == RestorationBoardState::Open;
    }

    EventBoardConfirmHint EventBoardPresenter::ConfirmHint() const
    {
        if (CanAcceptSelected())
            return EventBoardConfirmHint::Accept;
        if (CanRestoreSelected())
            return EventBoardConfirmHint::Restore;
        return EventBoardConfirmHint::None;
    }

    void EventBoardPresenter::SwitchTab(const int delta)
    {
        const int count = static_cast<int>(EVENT_BOARD_TAB_COUNT);
        const int next  = (static_cast<int>(currentTab_) + delta % count + count) % count;
        SelectTab(static_cast<EventBoardTabType>(next));
    }

    void EventBoardPresenter::SelectTab(const EventBoardTabType type)
    {
        if (type == currentTab_)
            return;

        currentTab_ = type;
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Tab);
        view_->ShowTab(currentTab_);
        Refresh();
    }

    void EventBoardPresenter::Confirm()
    {
        switch (currentTab_)
        {
        case EventBoardTabType::Quest:
            AcceptQuest();
            break;
        case EventBoardTabType::Restoration:
            RestoreFacility();
            break;
        case EventBoardTabType::Event:
        case EventBoardTabType::Notice:
            break;
        }
    }

    void EventBoardPresenter::AcceptQuest()
    {
        if (!CanAcceptSelected())
        {
            if (questModel_->Selected())
                PlaySe(refuseSound_);
            return;
        }

        const auto owner = suspendedAvatar_.lock();
        const auto& boardQuest = questModel_->Selected()->quest;
        const auto& source = boardQuest->Quest();
        const auto quest = source ? source->Clone() : nullptr;
        if (!owner || !quest)
            return;
        // 受注中・達成済みは BoardQuest の guid で見分ける(QuestType は週ごとの依頼で使い回す)
        quest->SetBoardQuestGuid(boardQuest->GetGuid().Value());

        if (!owner->PlayerStatus().Quest().Subscribe(quest))
            return;
        owner->SaveStatus();
        questModel_->MarkSelectedTaking();

        PlaySe(acceptSound_);
        Refresh();
    }

    void EventBoardPresenter::RestoreFacility()
    {
        if (!CanRestoreSelected())
            return;

        const auto owner = suspendedAvatar_.lock();
        if (!owner || !restorationModel_->RestoreSelected())
        {
            PlaySe(refuseSound_);
            return;
        }
        owner->SaveStatus();

        PlaySe(restoreSound_);
        Refresh();
    }

    void EventBoardPresenter::PlaySe(const FIELD(Asset::SoundFile)& sound) const
    {
        Sound::UiSoundBank::Play(sound.get());
    }

    void EventBoardPresenter::Refresh()
    {
        switch (currentTab_)
        {
        case EventBoardTabType::Quest:
            questModel_->MarkMainStoryRead(questReadLog_);
            if (const auto page = view_->QuestPage())
                page->Bind(*questModel_);
            break;
        case EventBoardTabType::Event:
            if (const auto page = view_->EventPage())
                page->Bind(*eventModel_);
            break;
        case EventBoardTabType::Notice:
            noticeModel_->MarkSelectedRead();
            if (const auto page = view_->NoticePage())
                page->Bind(*noticeModel_);
            break;
        case EventBoardTabType::Restoration:
            if (const auto page = view_->RestorationPage())
                page->Bind(*restorationModel_);
            break;
        }

        if (const auto tab = view_->Tab(EventBoardTabType::Notice))
            tab->SetBadgeCount(noticeModel_->UnreadCount());
        view_->ShowConfirmHint(ConfirmHint());
        UpdatePreview();
    }

    void EventBoardPresenter::UpdatePreview()
    {
        std::optional<GameCore::Story::Facility> wanted;
        if (!isClosed_ && currentTab_ == EventBoardTabType::Restoration)
        {
            const auto entry = restorationModel_->Selected();
            if (entry && entry->state != RestorationBoardState::Locked)
                wanted = entry->facility->Facility();
        }
        if (wanted == previewFacility_)
            return;

        EndPreview();
        previewFacility_ = wanted;
        if (!wanted)
            return;
        if (const auto gate = Prop::RestorationGate::Find(*wanted))
            gate->BeginPreview();
    }

    void EventBoardPresenter::EndPreview()
    {
        if (!previewFacility_)
            return;

        if (const auto gate = Prop::RestorationGate::Find(*previewFacility_))
            gate->EndPreview();
        previewFacility_.reset();
    }

    void EventBoardPresenter::Close()
    {
        if (isClosed_)
            return;
        isClosed_ = true;
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);
        EndPreview();

        screen_->Close();
        Entity().lock()->OnDestroy();
    }

    void EventBoardPresenter::OnDestroy()
    {
        EndPreview();
    }

    void EventBoardPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("board_", board_);
        ImGuiHelper::OnDrawInputField("acceptSound_", acceptSound_);
        ImGuiHelper::OnDrawInputField("restoreSound_", restoreSound_);
        ImGuiHelper::OnDrawInputField("refuseSound_", refuseSound_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardPresenter);
#pragma endregion
