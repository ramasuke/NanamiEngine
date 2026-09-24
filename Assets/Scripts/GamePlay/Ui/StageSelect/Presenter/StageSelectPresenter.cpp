#include "StageSelectPresenter.h"

#include <algorithm>

#include "DxLib.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "../UI_StageSelect.h"
#include "../Room/Ui_StageSelect_RoomUi.h"
#include "../../../../Core/Game/Game.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        bool IsPadButton(const XINPUT_STATE& pad, const int button)
        {
            return pad.Buttons[button] != 0;
        }
    }

    void StageSelectPresenter::OnStart()
    {
        view_  = RequireComponent<StageSelectUi>();
        model_ = std::make_unique<StageSelectModel>(view_->Stages());

        const auto owner = GameCore::PlayerAvatar::Owner();
        const GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockContext unlockContext{
            &GameCore::Story::StoryProgress::Instance(),
            owner ? &owner->PlayerStatus().CompletedQuest() : nullptr };

        const auto& stages = model_->Stages();
        for (size_t i = 0; i < stages.size(); ++i)
        {
            if (const auto stage = stages[i].lock())
            {
                stage->SetLocked(!stage->Data()->IsUnlocked(unlockContext));
                stage->SubscribeOnClickSelectButton([this, i]
                {
                    model_->SelectStage(i);
                });
            }
        }

        model_->OnSelectionChanged().Subscribe([this](const size_t index)
        {
            view_->HighlightSelectedStage(index);

            const auto stage = model_->Stages()[index].lock();
            if (!stage)
                return;

            view_->SetWorldEnterButtonEnabled(!stage->IsLocked());
            if (stage->IsLocked())
            {
                view_->HideMapMarker();
                view_->ShowLockedStageDetail(*stage->Data());
                return;
            }

            view_->ShowMapMarker(stage->MapMarkerPosition(), stage->IsCleared());
            view_->ShowStageDetail(*stage->Data());
        }).AddTo(this);

        view_->SetWorldEnterButtonEnabled(false);
        view_->ShowNoSelectionDetail();

        view_->OnWorldEnterButtonClicked().Subscribe([this](NanamiUi::MouseState)
        {
            TryEnterWorld();
        }).AddTo(this);

        if (const auto room = view_->Room())
        {
            room->OnLeftArrowClicked().Subscribe([this](NanamiUi::MouseState) { CycleMode(-1); }).AddTo(this);
            room->OnRightArrowClicked().Subscribe([this](NanamiUi::MouseState) { CycleMode(1); }).AddTo(this);
        }
        ApplyRoomToView();

        // 開いた直後に押しっぱなしを拾わない
        previousInput_ = ReadRoomInput();
    }

    void StageSelectPresenter::OnUpdate()
    {
        XINPUT_STATE xInput{};
        GetJoypadXInputState(DX_INPUT_PAD1, &xInput);
        const bool isConfirmPressed = CheckHitKey(KEY_INPUT_RETURN) || CheckHitKey(KEY_INPUT_SPACE) || IsPadButton(xInput, XINPUT_BUTTON_A);

        if (isConfirmPressed && !wasConfirmPressed_)
            TryEnterWorld();

        wasConfirmPressed_ = isConfirmPressed;

        const RoomInput input = ReadRoomInput();
        UpdateRoomInput(input);
        previousInput_ = input;
    }

    StageSelectPresenter::RoomInput StageSelectPresenter::ReadRoomInput() const
    {
        XINPUT_STATE pad{};
        GetJoypadXInputState(DX_INPUT_PAD1, &pad);

        RoomInput input;
        input.previousMode = CheckHitKey(KEY_INPUT_LEFT) != 0 || IsPadButton(pad, XINPUT_BUTTON_LEFT_SHOULDER);
        input.nextMode     = CheckHitKey(KEY_INPUT_RIGHT) != 0 || IsPadButton(pad, XINPUT_BUTTON_RIGHT_SHOULDER);
        input.cursorLeft   = IsPadButton(pad, XINPUT_BUTTON_DPAD_LEFT) || pad.ThumbLX < -stickThreshold_;
        input.cursorRight  = IsPadButton(pad, XINPUT_BUTTON_DPAD_RIGHT) || pad.ThumbLX > stickThreshold_;
        input.digitUp      = IsPadButton(pad, XINPUT_BUTTON_DPAD_UP) || pad.ThumbLY > stickThreshold_;
        input.digitDown    = IsPadButton(pad, XINPUT_BUTTON_DPAD_DOWN) || pad.ThumbLY < -stickThreshold_;
        input.erase        = CheckHitKey(KEY_INPUT_BACK) != 0 || IsPadButton(pad, XINPUT_BUTTON_X);

        for (int digit = 0; digit <= 9; ++digit)
        {
            if (CheckHitKey(KEY_INPUT_0 + digit) || CheckHitKey(KEY_INPUT_NUMPAD0 + digit))
            {
                input.typedDigit = digit;
                break;
            }
        }
        return input;
    }

    void StageSelectPresenter::UpdateRoomInput(const RoomInput& input)
    {
        if (input.previousMode && !previousInput_.previousMode)
            CycleMode(-1);
        if (input.nextMode && !previousInput_.nextMode)
            CycleMode(1);

        if (roomMode_ != Network::RelayRoom::Mode::Join)
            return;

        const int previousCursor = cursor_;
        if (input.cursorLeft && !previousInput_.cursorLeft)
            MoveCursor(-1);
        if (input.cursorRight && !previousInput_.cursorRight)
            MoveCursor(1);
        if (cursor_ != previousCursor)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        if (input.digitUp && !previousInput_.digitUp)
            SetDigit(cursor_ < static_cast<int>(roomCode_.size()) ? (roomCode_[cursor_] - '0' + 1) % 10 : 0);
        if (input.digitDown && !previousInput_.digitDown)
            SetDigit(cursor_ < static_cast<int>(roomCode_.size()) ? (roomCode_[cursor_] - '0' + 9) % 10 : 9);
        if (input.erase && !previousInput_.erase)
            Erase();
        if (input.typedDigit >= 0 && previousInput_.typedDigit != input.typedDigit)
        {
            SetDigit(input.typedDigit);
            MoveCursor(1);
        }
    }

    void StageSelectPresenter::CycleMode(const int delta)
    {
        constexpr int MODE_COUNT = Network::RelayRoom::MODE_COUNT;
        const int next = (static_cast<int>(roomMode_) + delta + MODE_COUNT) % MODE_COUNT;
        roomMode_ = static_cast<Network::RelayRoom::Mode>(next);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Tab);
        roomCode_.clear();
        cursor_ = 0;
        ApplyRoomToView();
    }

    void StageSelectPresenter::SetDigit(const int digit)
    {
        const char c = static_cast<char>('0' + digit);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Digit);
        if (cursor_ < static_cast<int>(roomCode_.size()))
            roomCode_[cursor_] = c;
        else if (static_cast<int>(roomCode_.size()) < CodeLength())
            roomCode_.push_back(c);
        ApplyRoomToView();
    }

    void StageSelectPresenter::MoveCursor(const int delta)
    {
        // 入れていない桁より先へは行かない
        const int last = std::min(static_cast<int>(roomCode_.size()), CodeLength() - 1);
        cursor_ = std::clamp(cursor_ + delta, 0, last);
        ApplyRoomToView();
    }

    int StageSelectPresenter::CodeLength() const
    {
        const auto room = view_ ? view_->Room() : nullptr;
        return room ? room->CodeLength() : 0;
    }

    void StageSelectPresenter::Erase()
    {
        if (roomCode_.empty())
            return;

        roomCode_.pop_back();
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Digit);
        cursor_ = static_cast<int>(roomCode_.size());
        ApplyRoomToView();
    }

    void StageSelectPresenter::ApplyRoomToView() const
    {
        if (const auto room = view_ ? view_->Room() : nullptr)
            room->ShowRoom(roomMode_, roomCode_, cursor_, IsRoomReady());
    }

    bool StageSelectPresenter::IsRoomReady() const
    {
        return roomMode_ != Network::RelayRoom::Mode::Join
            || static_cast<int>(roomCode_.size()) == CodeLength();
    }

    bool StageSelectPresenter::IsSelectedStageLocked() const
    {
        const auto stage = model_->Stages()[model_->SelectedIndex()].lock();
        return !stage || stage->IsLocked();
    }

    void StageSelectPresenter::TryEnterWorld()
    {
        if (!model_ || !model_->HasSelection() || IsSelectedStageLocked())
        {
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Refuse);
            return;
        }

        // 番号が揃うまでは出発させない
        if (!IsRoomReady())
        {
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Refuse);
            ApplyRoomToView();
            return;
        }

        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Stamp);
        GameCore::Game::Instance().Matchmaker().SetNextRoom({ roomMode_, roomCode_ });
        view_->EnterWorld(model_->SelectedSceneType());
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageSelectPresenter);
#pragma endregion
