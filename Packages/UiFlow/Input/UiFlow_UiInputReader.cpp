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
            auto& state = states_[i];
            const bool isDown = isActive && map_.IsDown(static_cast<UiAction>(i), pad);

            if (state.isIgnored)
            {
                state = ActionState{};
                state.isIgnored = isDown;
                continue;
            }

            state.isPressed  = isDown && !state.isDown;
            state.isDown     = isDown;
            state.isRepeated = state.isPressed;
            if (!isDown || state.isPressed)
            {
                state.held_secs   = 0.0f;
                state.repeat_secs = 0.0f;
                continue;
            }

            state.held_secs += deltaTime;
            if (state.held_secs < repeatDelay_secs_)
                continue;

            state.repeat_secs += deltaTime;
            if (state.repeat_secs < repeatInterval_secs_)
                continue;

            state.repeat_secs = 0.0f;
            state.isRepeated  = true;
        }
    }
}
