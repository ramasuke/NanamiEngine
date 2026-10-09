#include "UiFlow_UiScreen.h"

#include "UiFlow_ScreenStack.h"
#include "../../ControlLock/ControlLock.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::UiFlow
{
    UiScreen::UiScreen()
    {
        input_.SetGate(this);
    }

    UiScreen::~UiScreen()
    {
        Detach();
    }

    bool UiScreen::Open()
    {
        if (IsOpen())
            return false;

        auto& stack = ScreenStack::Instance();
        if (!screenId_.empty() && stack.IsOpen(screenId_))
            return false;

        if (auto* const top = stack.Top())
            top->Cover();

        stack.Push(*this);
        state_            = ScreenState::Opened;
        isDestroyPending_ = false;
        if (locksPlayerControl_)
        {
            controlLock_.Set(ControlLock::Service::Instance().AcquireLabeled(
                screenId_, ControlLock::Channel::PlayerControl));
        }

        // NOTE: 開くのに使った入力を押したままでも、開いた直後の入力として拾わない
        input_.SetRepeat(repeatDelay_secs_, repeatInterval_secs_);
        input_.WaitForRelease();
        onOpened_.OnNext(R4::Unit{});
        return true;
    }

    void UiScreen::Close()
    {
        if (!IsOpen())
            return;

        const bool wasFocused = IsFocused();
        Detach();
        onClosed_.OnNext(R4::Unit{});

        if (wasFocused)
        {
            if (auto* const top = ScreenStack::Instance().Top())
                top->Reveal();
        }
        isDestroyPending_ = destroysOnClose_;
    }

    void UiScreen::OnUpdate()
    {
        if (!isDestroyPending_)
            return;

        isDestroyPending_ = false;
        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    void UiScreen::OnDestroy()
    {
        if (!IsOpen())
            return;

        const bool wasFocused = IsFocused();
        Detach();
        if (!wasFocused)
            return;

        if (auto* const top = ScreenStack::Instance().Top())
            top->Reveal();
    }

    void UiScreen::Detach()
    {
        if (!IsOpen())
            return;

        ScreenStack::Instance().Remove(*this);
        state_ = ScreenState::Closed;
        controlLock_.Dispose();
    }

    void UiScreen::Cover()
    {
        state_ = ScreenState::Covered;
        onCovered_.OnNext(R4::Unit{});
    }

    void UiScreen::Reveal()
    {
        // NOTE: 上の画面を閉じるのに使った入力で、こちらまで動かない
        state_ = ScreenState::Opened;
        input_.WaitForRelease();
        onRevealed_.OnNext(R4::Unit{});
    }

    void UiScreen::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("screenId_", screenId_);
        ImGuiHelper::OnDrawInputField("locksPlayerControl_", locksPlayerControl_);
        ImGuiHelper::OnDrawInputField("destroysOnClose_", destroysOnClose_);
        ImGuiHelper::OnDrawInputField("repeatDelay_secs_", repeatDelay_secs_);
        ImGuiHelper::OnDrawInputField("repeatInterval_secs_", repeatInterval_secs_);
        ImGui::Text("state: %d", static_cast<int>(state_));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::UiFlow::UiScreen);
#pragma endregion
