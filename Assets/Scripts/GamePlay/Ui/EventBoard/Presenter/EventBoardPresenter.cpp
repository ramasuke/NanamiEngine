#include "EventBoardPresenter.h"

#include <algorithm>
#include <chrono>

#include "DxLib.h"

#include "../../../Sound/SoundPlayer.h"
#include "../../../Prop/RestorationGate/Prop_RestorationGate.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_ITakeableQuest.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "../../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 左スティックを方向キーとして読むためのしきい値
        constexpr short EVENT_BOARD_STICK_DEADZONE = 12000;
    }

    bool EventBoardPresenter::IsAnotherOpen() const
    {
        bool found = false;
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [this, &found](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                if (found)
                    return;

                const auto presenter = gameObject->Components().Catch<EventBoardPresenter>().lock();
                found = presenter && presenter.get() != this && presenter->isOpen_;
            });
        return found;
    }

    void EventBoardPresenter::OnStart()
    {
        if (IsAnotherOpen())
        {
            isClosed_ = true;
            Entity().lock()->OnDestroy();
            return;
        }
        isOpen_ = true;

        view_ = RequireComponent<EventBoardUi>();
        view_->Build();

        const auto owner = GameCore::PlayerAvatar::Owner();
        suspendedAvatar_ = owner;
        if (owner)
            owner->DisableStateMachine();

        const auto board = board_.get();
        const auto now   = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        const auto questPage  = view_->QuestPage();
        const auto eventPage  = view_->EventPage();
        const auto noticePage = view_->NoticePage();
        const auto restorationPage = view_->RestorationPage();

        questModel_ = std::make_unique<QuestBoardModel>(
            board ? board->Quests() : std::vector<std::shared_ptr<Asset::BoardQuest>>{},
            now,
            owner ? &owner->PlayerStatus().Quest() : nullptr,
            owner ? &owner->PlayerStatus().CompletedQuest() : nullptr,
            &GameCore::Story::StoryProgress::Instance(),
            questPage ? questPage->MaxVisibleRows() : 0);
        eventModel_ = std::make_unique<EventBoardModel>(
            board ? board->Notices() : std::vector<std::shared_ptr<Asset::EventNotice>>{},
            now,
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

        questModel_ ->Cursor().OnSelectionChanged().Subscribe([this](size_t) { Refresh(); }).AddTo(this);
        eventModel_ ->Cursor().OnSelectionChanged().Subscribe([this](size_t) { Refresh(); }).AddTo(this);
        noticeModel_->Cursor().OnSelectionChanged().Subscribe([this](size_t) { Refresh(); }).AddTo(this);
        restorationModel_->Cursor().OnSelectionChanged().Subscribe([this](size_t) { Refresh(); }).AddTo(this);

        // 調べたときの押しっぱなしを、開いた直後の入力として拾わない
        previousKeys_ = ReadKeys();

        // Select は同じ index だと通知を出さないので、初期表示はここで一度だけ作る
        view_->ShowTab(currentTab_);
        Refresh();
    }

    void EventBoardPresenter::OnUpdate()
    {
        if (isClosed_ || !view_)
            return;

        const Keys keys = ReadKeys();

        if (keys.prev && !previousKeys_.prev)
            CurrentCursor().Move(-1);
        if (keys.next && !previousKeys_.next)
            CurrentCursor().Move(1);
        if (keys.tabPrev && !previousKeys_.tabPrev)
            SwitchTab(-1);
        if (keys.tabNext && !previousKeys_.tabNext)
            SwitchTab(1);
        if (keys.confirm && !previousKeys_.confirm)
            Confirm();

        const bool isCancelPressed = keys.cancel && !previousKeys_.cancel;
        previousKeys_ = keys;
        if (isCancelPressed)
            Close();
    }

    EventBoardPresenter::Keys EventBoardPresenter::ReadKeys()
    {
        XINPUT_STATE xInput{};
        GetJoypadXInputState(DX_INPUT_PAD1, &xInput);

        return Keys{
            .prev    = CheckHitKey(KEY_INPUT_UP) || CheckHitKey(KEY_INPUT_W)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_UP] || xInput.ThumbLY > EVENT_BOARD_STICK_DEADZONE,
            .next    = CheckHitKey(KEY_INPUT_DOWN) || CheckHitKey(KEY_INPUT_S)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_DOWN] || xInput.ThumbLY < -EVENT_BOARD_STICK_DEADZONE,
            .tabPrev = CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_A)
                       || xInput.Buttons[XINPUT_BUTTON_LEFT_SHOULDER] || xInput.Buttons[XINPUT_BUTTON_DPAD_LEFT],
            .tabNext = CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_D)
                       || xInput.Buttons[XINPUT_BUTTON_RIGHT_SHOULDER] || xInput.Buttons[XINPUT_BUTTON_DPAD_RIGHT],
            .confirm = CheckHitKey(KEY_INPUT_RETURN) || xInput.Buttons[XINPUT_BUTTON_A],
            .cancel  = CheckHitKey(KEY_INPUT_ESCAPE) || xInput.Buttons[XINPUT_BUTTON_B],
        };
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
            return;

        const auto owner = suspendedAvatar_.lock();
        const auto quest = GameCore::PlayerAvatar::Quest::CloneQuest(questModel_->Selected()->quest->Quest());
        if (!owner || !quest)
            return;

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
        if (const auto file = sound.get())
            Sound::SoundPlayer::PlaySe(*file, Sound::SoundPlayer::Position());
    }

    void EventBoardPresenter::Refresh()
    {
        switch (currentTab_)
        {
        case EventBoardTabType::Quest:
            if (const auto page = view_->QuestPage())
                page->Bind(*questModel_);
            break;
        case EventBoardTabType::Event:
            if (const auto page = view_->EventPage())
                page->Bind(*eventModel_);
            break;
        case EventBoardTabType::Notice:
            // 右に本文が出た時点で読んだとみなす
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
            if (const auto entry = restorationModel_->Selected())
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
        EndPreview();

        if (const auto owner = suspendedAvatar_.lock())
            owner->EnableStateMachiine();

        Entity().lock()->OnDestroy();
    }

    void EventBoardPresenter::OnDestroy()
    {
        EndPreview();
        isOpen_ = false;
    }

    void EventBoardPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("board_", board_);
        ImGuiHelper::OnDrawInputField("acceptSound_", acceptSound_);
        ImGuiHelper::OnDrawInputField("restoreSound_", restoreSound_);
        ImGuiHelper::OnDrawInputField("refuseSound_", refuseSound_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardPresenter);
#pragma endregion
