#include "Ui_GameCursor.h"

#include "DxLib.h"
#include "Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "Engine/Core/Application/Time/Time.h"
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
        wasMouseDown_ = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    }

    void GameCursor::OnUpdate()
    {
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);
        Transform().SetWorldPos(glm::vec3(static_cast<float>(mouseX), static_cast<float>(mouseY), 0.0f));

        SetVisible(ShouldShow(mouseX, mouseY));

        const bool isMouseDown = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
        if (isVisible_ && isMouseDown && !wasMouseDown_)
        {
            pressRemaining_secs_ = pressDuration_secs_;
            SetPressed(true);
        }
        wasMouseDown_ = isMouseDown;

        if (pressRemaining_secs_ > 0.0f)
        {
            pressRemaining_secs_ -= Time::DeltaTime();
            if (pressRemaining_secs_ <= 0.0f)
                SetPressed(false);
        }
    }

    bool GameCursor::ShouldShow(const int mouseX, const int mouseY) const
    {
        using NanamiEngine::Core::Application::Configuration::APPLICATION_MODE;
        using NanamiEngine::Core::Application::Configuration::ApplicationMode;
        using NanamiEngine::Core::Application::Configuration::AppConfiguration;

        // NOTE: エディタは ImGui を触るため OS カーソルのままにする
        if constexpr (APPLICATION_MODE != ApplicationMode::Game)
            return false;

        if (NanamiEngine::CineMachine::Behaviour::ThirdPersonCameraBehaviour::IsMousePinned())
            return false;
        if (GetWindowActiveFlag() == FALSE)
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
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameCursor);
#pragma endregion
