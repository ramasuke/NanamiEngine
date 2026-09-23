#include "Ui_RoomCodeHud.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../../Network/Game_CustomNetworkRunner.h"

namespace GamePlay::Ui
{
    namespace
    {
        /** @brief 通信していないシーンでも置けるよう、ランナーが居ないときは空 */
        std::string CurrentRoomCode()
        {
            const auto* runner = dynamic_cast<Network::CustomNetworkRunner*>(
                Module::Network::NetworkRunnerBase::TryGetInstance());
            return runner ? runner->RelayRoomCode() : std::string();
        }
    }

    void RoomCodeHud::OnStart()
    {
        shownCode_.clear();
        Show(CurrentRoomCode());
    }

    void RoomCodeHud::OnUpdate()
    {
        // 部屋に入るのはシーンに入ったあとなので、番号は少し遅れて決まる
        if (const std::string code = CurrentRoomCode(); code != shownCode_)
            Show(code);
    }

    void RoomCodeHud::Show(const std::string& code)
    {
        shownCode_ = code;

        if (const auto root = visualRoot_.get())
            root->SetEnable(!code.empty());
        if (const auto text = codeText_.get())
            text->SetText(FormatCode(code));
    }

    std::string RoomCodeHud::FormatCode(const std::string& code) const
    {
        if (codeGroupSize_ <= 0)
            return code;

        std::string formatted;
        for (size_t i = 0; i < code.size(); ++i)
        {
            if (i > 0 && i % static_cast<size_t>(codeGroupSize_) == 0)
                formatted += codeGroupSeparator_;
            formatted += code[i];
        }
        return formatted;
    }

    void RoomCodeHud::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("visualRoot_", visualRoot_);
        ImGuiHelper::OnDrawInputField("codeText_", codeText_);
        ImGuiHelper::OnDrawInputField("codeGroupSize_", codeGroupSize_);
        ImGuiHelper::OnDrawInputField("codeGroupSeparator_", codeGroupSeparator_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::RoomCodeHud);
#pragma endregion
