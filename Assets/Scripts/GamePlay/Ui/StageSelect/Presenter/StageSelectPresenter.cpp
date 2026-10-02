#include "StageSelectPresenter.h"

#include <algorithm>

#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../Core/Game/Condition/Condition_Clock.h"
#include "../UI_StageSelect.h"
#include "../Room/Ui_StageSelect_RoomUi.h"
#include "../../../../Core/Game/Game.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/Decoration/Decoration_DecorationCollection.h"
#include "../../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void StageSelectPresenter::OnStart()
    {
        screen_ = RequireComponent<UiFlow::UiScreen>();
        if (!screen_->Open())
        {
            Entity().lock()->OnDestroy();
            return;
        }

        view_  = RequireComponent<StageSelectUi>();
        model_ = std::make_unique<StageSelectModel>(view_->Stages());

        const auto owner = GameCore::PlayerAvatar::Owner();
        const GameCore::Condition::ConditionContext unlockContext{
            GameCore::Story::StoryProgress::Instance(),
            owner ? &owner->PlayerStatus().CompletedQuest() : nullptr,
            GameCore::Condition::Clock::Now(),
            GameCore::Decoration::DecorationCollection::Instance() };

        const auto& stages = model_->Stages();
        isHidden_.assign(stages.size(), false);
        for (size_t i = 0; i < stages.size(); ++i)
        {
            if (const auto stage = stages[i].lock())
            {
                const bool isLocked = !stage->Data()->IsUnlocked(unlockContext);
                if (isLocked && stage->Data()->HidesWhenLocked())
                {
                    isHidden_[i] = true;
                    stage->Entity().lock()->SetEnable(false);
                    continue;
                }
                stage->SetLocked(isLocked);
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

        screen_->Input().Map()
            .SetStickThreshold(static_cast<std::int16_t>(stickThreshold_))
            .AddKey(UiFlow::UiAction::Submit, Platform::Input::Key::Space);
        ApplyInputMap();
    }

    void StageSelectPresenter::OnUpdate()
    {
        if (!model_)
            return;

        using UiFlow::UiAction;
        auto& input = screen_->Input();
        if (input.IsPressed(UiAction::Submit))
            TryEnterWorld();

        if (input.IsPressed(UiAction::Cancel))
        {
            Close();
            return;
        }

        UpdateRoomInput();
    }

    void StageSelectPresenter::ApplyInputMap() const
    {
        using UiFlow::UiAction;
        using UiFlow::StickDirection;
        using Platform::Input::GamepadButton;
        using Platform::Input::Key;

        const bool isDigitInput = roomMode_ == Network::RelayRoom::Mode::Join;
        auto& map = screen_->Input().Map();
        map.Set(UiAction::TabPrev,   { { Key::Left  }, { GamepadButton::LeftShoulder  } })
            .Set(UiAction::TabNext,   { { Key::Right }, { GamepadButton::RightShoulder } })
            .Set(UiAction::Left,      { {}, { GamepadButton::DPadLeft  }, { StickDirection::Left  } })
            .Set(UiAction::Right,     { {}, { GamepadButton::DPadRight }, { StickDirection::Right } })
            .Set(UiAction::ValueUp,   { {}, { GamepadButton::DPadUp    }, { StickDirection::Up    } })
            .Set(UiAction::ValueDown, { {}, { GamepadButton::DPadDown  }, { StickDirection::Down  } })
            .Set(UiAction::Up,        { { Key::Up   }, {}, { StickDirection::RightStickUp   } })
            .Set(UiAction::Down,      { { Key::Down }, {}, { StickDirection::RightStickDown } });

        if (isDigitInput)
            return;

        map.AddButton(UiAction::Up,   GamepadButton::DPadUp)  .AddStick(UiAction::Up,   StickDirection::Up)
           .AddButton(UiAction::Down, GamepadButton::DPadDown).AddStick(UiAction::Down, StickDirection::Down);
    }

    void StageSelectPresenter::UpdateRoomInput()
    {
        using UiFlow::UiAction;
        auto& input = screen_->Input();

        if (input.IsPressed(UiAction::TabPrev))
            CycleMode(-1);
        if (input.IsPressed(UiAction::TabNext))
            CycleMode(1);
        if (input.IsPressed(UiAction::Up))
            MoveStage(-1);
        if (input.IsPressed(UiAction::Down))
            MoveStage(1);

        if (roomMode_ != Network::RelayRoom::Mode::Join)
            return;

        const int previousCursor = cursor_;
        if (input.IsPressed(UiAction::Left))
            MoveCursor(-1);
        if (input.IsPressed(UiAction::Right))
            MoveCursor(1);
        if (cursor_ != previousCursor)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        if (input.IsPressed(UiAction::ValueUp))
            SetDigit(cursor_ < static_cast<int>(roomCode_.size()) ? (roomCode_[cursor_] - '0' + 1) % 10 : 0);
        if (input.IsPressed(UiAction::ValueDown))
            SetDigit(cursor_ < static_cast<int>(roomCode_.size()) ? (roomCode_[cursor_] - '0' + 9) % 10 : 9);
        if (input.IsPressed(UiAction::Erase))
            Erase();
        if (const int digit = input.PressedDigit(); digit >= 0)
        {
            SetDigit(digit);
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
        ApplyInputMap();
    }

    void StageSelectPresenter::MoveStage(const int delta)
    {
        const int count = static_cast<int>(model_->Stages().size());
        if (count == 0)
            return;

        // NOTE: 未選択なら先頭から。隠した行は飛ばす
        const int step = model_->HasSelection() ? delta : 1;
        int next = model_->HasSelection() ? static_cast<int>(model_->SelectedIndex()) : -1;
        for (int tried = 0; tried < count; ++tried)
        {
            next = (next + step + count) % count;
            if (!isHidden_[static_cast<size_t>(next)])
                break;
        }
        if (isHidden_[static_cast<size_t>(next)])
            return;
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        model_->SelectStage(static_cast<size_t>(next));
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

    void StageSelectPresenter::Close()
    {
        if (!view_ || view_->IsEnteringWorld())
            return;

        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);
        screen_->Close();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageSelectPresenter);
#pragma endregion
