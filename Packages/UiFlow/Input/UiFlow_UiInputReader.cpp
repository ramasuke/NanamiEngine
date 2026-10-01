#include "UiFlow_UiInputReader.h"

#include "../Screen/UiFlow_UiScreen.h"
#include "Engine/Core/Application/Time/Time.h"

namespace NanamiEngine::UiFlow
{
    void UiInputReader::SetMap(UiActionMap map)
    {
        map_ = std::move(map);
    }

    void UiInputReader::SetRepeat(const float delay_secs, const float interval_secs)
    {
        repeatDelay_secs_    = delay_secs;
        repeatInterval_secs_ = interval_secs;
    }

    void UiInputReader::WaitForRelease()
    {
        for (auto& state : states_)
        {
            state = ActionState{};
        }
        anyState_ = ActionState{};
        for (auto& state : digitStates_)
        {
            state = ActionState{};
        }
    }

    bool UiInputReader::IsPressed(const UiAction action)
    {
        return State(action).isPressed;
    }

    bool UiInputReader::IsHeld(const UiAction action)
    {
        return State(action).isDown;
    }

    bool UiInputReader::IsRepeated(const UiAction action)
    {
        return State(action).isRepeated;
    }

    bool UiInputReader::IsAnyPressed()
    {
        Poll();
        return anyState_.isPressed;
    }

    int UiInputReader::PressedDigit()
    {
        Poll();
        for (int digit = 0; digit < DIGIT_COUNT; ++digit)
        {
            if (digitStates_[digit].isPressed)
                return digit;
        }
        return -1;
    }

    void UiInputReader::SetGate(const UiScreen* screen)
    {
        gate_ = screen;
    }

    const UiInputReader::ActionState& UiInputReader::State(const UiAction action)
    {
        Poll();
        return states_[static_cast<std::size_t>(action)];
    }

    void UiInputReader::Poll()
    {
        const std::uint64_t frame = Time::FrameCount();
        if (hasPolled_ && frame == lastPolledFrame_)
            return;

        // 読まれていなかった間に押されたものは、押した瞬間として扱わない
        if (!hasPolled_ || frame - lastPolledFrame_ > 1)
            WaitForRelease();
        hasPolled_       = true;
        lastPolledFrame_ = frame;

        if (gate_ && !gate_->IsFocused())
        {
            WaitForRelease();
            return;
        }

        const bool  isActive  = Platform::Input::IsWindowActive();
        const auto  pad       = Platform::Input::Gamepad::Get();
        const float deltaTime = Time::DeltaTime();

        for (std::size_t i = 0; i < states_.size(); ++i)
        {
            Step(states_[i], isActive && map_.IsDown(static_cast<UiAction>(i), pad), deltaTime);
        }
        Step(anyState_, isActive && (Platform::Input::IsAnyDeviceDown() || pad.IsAnyDown()), deltaTime);
        for (int digit = 0; digit < DIGIT_COUNT; ++digit)
        {
            Step(digitStates_[digit], isActive && Platform::Input::Keyboard::IsDigitDown(digit), deltaTime);
        }
    }

    void UiInputReader::Step(ActionState& state, const bool isDown, const float deltaTime) const
    {
        if (state.isIgnored)
        {
            state = ActionState{};
            state.isIgnored = isDown;
            return;
        }

        state.isPressed  = isDown && !state.isDown;
        state.isDown     = isDown;
        state.isRepeated = state.isPressed;
        if (!isDown || state.isPressed)
        {
            state.held_secs   = 0.0f;
            state.repeat_secs = 0.0f;
            return;
        }

        state.held_secs += deltaTime;
        if (state.held_secs < repeatDelay_secs_)
            return;

        state.repeat_secs += deltaTime;
        if (state.repeat_secs < repeatInterval_secs_)
            return;

        state.repeat_secs = 0.0f;
        state.isRepeated  = true;
    }
}
