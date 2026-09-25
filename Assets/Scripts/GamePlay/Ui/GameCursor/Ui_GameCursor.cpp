#include "Ui_GameCursor.h"

#include "Engine/Core/Platform/Input/Input.h"
#include "Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void GameCursor::OnStart()
    {
        isVisible_ = true;
        SetVisible(false);
        SetPressed(false);
        pressRemaining_secs_ = 0.0f;
        isHolding_ = false;
        animScale_ = 1.0f;
        wasMouseDown_ = Platform::Input::Mouse::IsDown(Platform::Input::MouseButton::Left);
        UpdateScale();
    }

    void GameCursor::OnUpdate()
    {
        const glm::ivec2 mouse = Platform::Input::Mouse::Position();
        const int mouseX = mouse.x;
        const int mouseY = mouse.y;
        Transform().SetWorldPos(glm::vec3(static_cast<float>(mouseX), static_cast<float>(mouseY), 0.0f));

        SetVisible(ShouldShow(mouseX, mouseY));

        const bool isMouseDown = Platform::Input::Mouse::IsDown(Platform::Input::MouseButton::Left);
        UpdatePress(isMouseDown);
        wasMouseDown_ = isMouseDown;

        UpdateScale();
    }

    void GameCursor::UpdatePress(const bool isMouseDown)
    {
        if (isVisible_ && isMouseDown && !wasMouseDown_)
        {
            isHolding_ = true;
            pressRemaining_secs_ = pressDuration_secs_;
            SetPressed(true);
        }
        else if (isHolding_ && !isMouseDown)
        {
            isHolding_ = false;
            animScale_ = releaseScale_;
        }

        if (pressRemaining_secs_ > 0.0f)
            pressRemaining_secs_ -= Time::DeltaTime();

        // NOTE: 長押し中は押した絵のまま、離した後も最短 pressDuration_secs_ は残す
        if (!isHolding_ && pressRemaining_secs_ <= 0.0f)
            SetPressed(false);
    }

    void GameCursor::UpdateScale()
    {
        const float target = isHolding_ ? holdScale_ : 1.0f;
        const float t = 1.0f - std::exp(-scaleSpeed_ * Time::DeltaTime());
        animScale_ += (target - animScale_) * t;

        if (const auto root = visualRoot_.get())
            root->Transform().SetLocalScale(glm::vec3(baseScale_ * animScale_));
    }

    bool GameCursor::ShouldShow(const int mouseX, const int mouseY) const
    {
        using NanamiEngine::Core::Application::Configuration::APPLICATION_MODE;
        using NanamiEngine::Core::Application::Configuration::ApplicationMode;
        using NanamiEngine::Core::Application::Configuration::AppConfiguration;

        // NOTE: エディタは ImGui を触るため OS カーソルのままにする
        if constexpr (APPLICATION_MODE != ApplicationMode::Game)
            return false;

        if (!NanamiUi::Button::IsAnyActive())
            return false;

        if (NanamiEngine::CineMachine::Behaviour::ThirdPersonCameraBehaviour::IsMousePinned())
            return false;
        if (!Platform::Input::IsWindowActive())
            return false;

        return mouseX >= 0 && mouseY >= 0
            && mouseX < AppConfiguration::GetWindowWidth()
            && mouseY < AppConfiguration::GetWindowHeight();
    }

    void GameCursor::SetVisible(const bool visible)
    {
        if (visible == isVisible_)
            return;

        isVisible_ = visible;
        if (const auto root = visualRoot_.get())
            root->SetEnable(visible);

        if (!visible)
        {
            isHolding_ = false;
            pressRemaining_secs_ = 0.0f;
            animScale_ = 1.0f;
            SetPressed(false);
        }
    }

    void GameCursor::SetPressed(const bool pressed)
    {
        if (const auto idle = idle_.get())
            idle->SetEnable(!pressed);
        if (const auto press = press_.get())
            press->SetEnable(pressed);
    }

    void GameCursor::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("visualRoot_", visualRoot_);
        ImGuiHelper::OnDrawInputField("idle_", idle_);
        ImGuiHelper::OnDrawInputField("press_", press_);
        ImGuiHelper::OnDrawInputField("pressDuration_secs_", pressDuration_secs_);
        ImGuiHelper::OnDrawInputField("baseScale_", baseScale_);
        ImGuiHelper::OnDrawInputField("holdScale_", holdScale_);
        ImGuiHelper::OnDrawInputField("releaseScale_", releaseScale_);
        ImGuiHelper::OnDrawInputField("scaleSpeed_", scaleSpeed_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameCursor);
#pragma endregion
