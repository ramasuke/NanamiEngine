#include "EventBoardPresenter.h"

#include <algorithm>
#include <chrono>

#include "DxLib.h"

#include "../../../Sound/SoundPlayer.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_StoryQuestBase.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 左スティックを方向キーとして読むためのしきい値
        constexpr short EVENT_BOARD_STICK_DEADZONE = 12000;
    }

    bool EventBoardPresenter::isOpen_ = false;

    void EventBoardPresenter::OnStart()
    {
        if (isOpen_)
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

        questModel_ = std::make_unique<QuestBoardModel>(
            board ? board->Quests() : std::vector<std::shared_ptr<Asset::BoardQuest>>{},
            now,
            owner ? &owner->PlayerStatus().Quest() : nullptr,
            owner ? &owner->PlayerStatus().CompletedQuest() : nullptr,
            questPage ? questPage->MaxVisibleRows() : 0);
        eventModel_ = std::make_unique<EventBoardModel>(
            board ? board->Notices() : std::vector<std::shared_ptr<Asset::EventNotice>>{},
            now,
            eventPage ? eventPage->MaxVisibleRows() : 0);
        noticeModel_ = std::make_unique<NoticeBoardModel>(
            board ? board->Announcements() : std::vector<std::shared_ptr<Asset::Announcement>>{},
            now,
            noticePage ? noticePage->MaxVisibleRows() : 0);

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

        questModel_ ->Cursor().OnSelectionChanged().subscribe([this](size_t) { Refresh(); });
        eventModel_ ->Cursor().OnSelectionChanged().subscribe([this](size_t) { Refresh(); });
        noticeModel_->Cursor().OnSelectionChanged().subscribe([this](size_t) { Refresh(); });

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
            Accept();

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

    void EventBoardPresenter::Accept()
    {
        if (!CanAcceptSelected())
            return;

        const auto owner = suspendedAvatar_.lock();
        const auto quest = GameCore::PlayerAvatar::StoryQuestBase::Clone(questModel_->Selected()->quest->Quest());
        if (!owner || !quest)
            return;

        owner->PlayerStatus().Quest().Subscribe(quest);
        owner->SaveStatus();
        questModel_->MarkSelectedTaking();

        if (const auto sound = acceptSound_.get())
            Sound::SoundPlayer::PlaySe(*sound, Sound::SoundPlayer::Position());
        Refresh();
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
        }

        if (const auto tab = view_->Tab(EventBoardTabType::Notice))
            tab->SetBadgeCount(noticeModel_->UnreadCount());
        view_->ShowAcceptHint(CanAcceptSelected());
    }

    void EventBoardPresenter::Close()
    {
        if (isClosed_)
            return;
        isClosed_ = true;

        if (const auto owner = suspendedAvatar_.lock())
            owner->EnableStateMachiine();

        Entity().lock()->OnDestroy();
    }

    void EventBoardPresenter::OnDestroy()
    {
        isOpen_ = false;
    }

    void EventBoardPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("board_", board_);
        ImGuiHelper::OnDrawInputField("acceptSound_", acceptSound_);
    }
}
