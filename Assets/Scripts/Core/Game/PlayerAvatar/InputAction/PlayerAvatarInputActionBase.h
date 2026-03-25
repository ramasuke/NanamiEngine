#pragma once
#include <memory>
#include "DxDataTypeWin.h"
#include "DxLib.h"
#include "../Input/PlayerAvatarInput.h"

namespace GameCore::PlayerAvatar
{
    class PlayerAvatarInputActionBase
    {
    public:
        virtual ~PlayerAvatarInputActionBase() = default;
        void OnUpdate();
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

    private:
        std::vector<std::shared_ptr<IPlayerAvatarInput>> inputs_;
        XINPUT_STATE xInput_ = {};
    };
}
