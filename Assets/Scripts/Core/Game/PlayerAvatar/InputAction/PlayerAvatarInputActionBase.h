#pragma once
#include <memory>
#include "Engine/Core/Platform/Input/Input.h"
#include "../Input/PlayerAvatarInput.h"
#include "PlayerAvatarInputDevice.h"

namespace GameCore::PlayerAvatar
{
    class PlayerAvatarInputActionBase
    {
    public:
        virtual ~PlayerAvatarInputActionBase() = default;
        void OnUpdate();
        /** @brief 最後に触られた入力機器。操作ガイドのグリフ切り替えに使う */
        [[nodiscard]] PlayerAvatarInputDevice CurrentDevice() const { return currentDevice_; }
        void Enable();
        void Disable();
        virtual void OnDrawGui() = 0;

    protected:
        ///以下サンドボックスパターン
        template <typename ReadValueT>
        using Input = std::shared_ptr<PlayerAvatarInput<ReadValueT>>;
        template <typename ReadValueT>
        using InputRef = PlayerAvatarInput<ReadValueT>&;

        template <typename InputT>
        std::shared_ptr<InputT> MakeInputAction()
        {
            auto input = std::make_shared<InputT>();
            inputs_.push_back(input);
            return input;
        }
        Input<void> MakeInputAction(const std::function<bool()>& checkInput);
        using Key         = NanamiEngine::Platform::Input::Key;
        using PadButton   = NanamiEngine::Platform::Input::GamepadButton;
        using MouseButton = NanamiEngine::Platform::Input::MouseButton;
        [[nodiscard]] const NanamiEngine::Platform::Input::GamepadState& Gamepad() const { return gamepad_; }
        [[nodiscard]] static bool IsKeyDown  (const Key key)            { return NanamiEngine::Platform::Input::Keyboard::IsDown(key); }
        [[nodiscard]] static bool IsMouseDown(const MouseButton button) { return NanamiEngine::Platform::Input::Mouse::IsDown(button); }
        [[nodiscard]] bool        IsPadDown  (const PadButton button) const { return gamepad_.IsDown(button); }
        /** @brief このフレームのホイール回転量。奥へ回すと正 */
        [[nodiscard]] int MouseWheelDelta() const { return mouseWheelDelta_; }

    private:
        void UpdateMouseWheel();
        void UpdateCurrentDevice();

        std::vector<std::shared_ptr<IPlayerAvatarInput>> inputs_;
        NanamiEngine::Platform::Input::GamepadState gamepad_;
        PlayerAvatarInputDevice currentDevice_ = PlayerAvatarInputDevice::KeyboardMouse;
        int previousMouseX_ = 0;
        int previousMouseY_ = 0;
        // 累積値をリセットせずに読み、前回との差を取る。リセットすると複数のアバターで1回分を取り合う
        int previousMouseWheel_ = NanamiEngine::Platform::Input::Mouse::WheelRotation(false);
        int mouseWheelDelta_ = 0;
    };
}
