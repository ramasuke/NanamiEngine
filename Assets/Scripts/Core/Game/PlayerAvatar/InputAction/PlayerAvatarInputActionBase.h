#pragma once
#include <memory>
#include "DxDataTypeWin.h"
#include "DxLib.h"
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
        [[nodiscard]] const XINPUT_STATE& XInput() const { return xInput_; }
        /** @brief このフレームのホイール回転量。奥へ回すと正 */
        [[nodiscard]] int MouseWheelDelta() const { return mouseWheelDelta_; }

    private:
        void UpdateMouseWheel();
        void UpdateCurrentDevice();

        std::vector<std::shared_ptr<IPlayerAvatarInput>> inputs_;
        XINPUT_STATE xInput_ = {};
        PlayerAvatarInputDevice currentDevice_ = PlayerAvatarInputDevice::KeyboardMouse;
        int previousMouseX_ = 0;
        int previousMouseY_ = 0;
        // 累積値をリセットせずに読み、前回との差を取る。リセットすると複数のアバターで1回分を取り合う
        int previousMouseWheel_ = GetMouseWheelRotVol(FALSE);
        int mouseWheelDelta_ = 0;
    };
}
