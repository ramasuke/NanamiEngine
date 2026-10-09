#include "Ui_NpcChatting.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../Core/Game/Settings/GameSettings.h"
#include "../../Sound/UiSoundBank.h"
#include "../../../Core/Input/InputAliases.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        bool IsAdvanceInputDown()
        {
            if (Keyboard::IsDown(Key::E))
                return true;

            const GamepadState pad = Gamepad::Get();
            return pad.connected && pad.IsDown(GamepadButton::Y);
        }

        // NOTE: 話しかけた時の押しっぱなしで送らないよう、押し始めだけを拾う
        class AdvanceInput final
        {
        public:
            bool IsPressed()
            {
                const bool isDown = IsAdvanceInputDown();
                const bool isPressed = isDown && !wasDown_;
                wasDown_ = isDown;
                return isPressed;
            }

        private:
            bool wasDown_ = true;
        };

        // NOTE: 先頭から count 文字分のバイト数 (UTF-8 の文字の途中で切らない)
        size_t Utf8PrefixBytes(const std::string& text, const size_t count)
        {
            size_t chars = 0;
            for (size_t i = 0; i < text.size(); ++i)
            {
                if ((static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
                    continue;
                if (chars == count)
                    return i;
                ++chars;
            }
            return text.size();
        }
    }

    Coroutine::Task<void> NpcChatting::OnDisplayChatAsync(
        const std::string& npcName,
        const Asset::NpcChat& npcChat,
        const bool followsAdvanceSetting) const
    {
        if (!npcNameTextBox_)
            co_return;
        
        isDisplaying_ = true;
        Entity().lock()->SetEnable(true);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::ChatOpen);

        npcNameTextBox_->SetText(npcName);
        
        const float chatCharInterval_secs         = GameCore::GameSettings::GetInstance().GetChatTextCharInterval_secs();
        const float chatTextSentenceInterval_secs = GameCore::GameSettings::GetInstance().GetChatTextSentenceInterval_secs();
        const bool  isManualAdvance = followsAdvanceSetting
            && GameCore::GameSettings::GetInstance().GetChatAdvanceMode() == GameCore::ChatAdvanceMode::Manual;

        AdvanceInput advance;

        const auto& chats = npcChat.Get();
        for (size_t page = 0; page < chats.size(); ++page)
        {
            const auto& chat = chats[page];
            if (!textRenderer_)
                break;

            ShowPage(page, chats.size());

            textRenderer_->SetFont(chat.Font());
            textRenderer_->SetTextColor(chat.TextColor());

            const std::string& fullText = chat.Text();
            float elapsed_secs = 0.0f;
            size_t shownBytes = 0;
            while (textRenderer_)
            {
                // NOTE: 押したら残りを一気に出す
                const size_t count = advance.IsPressed() || chatCharInterval_secs <= 0.0f
                    ? fullText.size()
                    : static_cast<size_t>(elapsed_secs / chatCharInterval_secs) + 1;
                const size_t bytes = Utf8PrefixBytes(fullText, count);
                if (bytes != shownBytes)
                {
                    shownBytes = bytes;
                    textRenderer_->SetText(fullText.substr(0, bytes));
                }
                if (shownBytes >= fullText.size())
                    break;

                co_await Coroutine::WaitYield();
                elapsed_secs += Time::DeltaTime();
            }

            if (!textRenderer_)
                break;

            // NOTE: 手動送りでは、最後のページも押すまで閉じない
            elapsed_secs = 0.0f;
            while (isManualAdvance || elapsed_secs < chatTextSentenceInterval_secs)
            {
                co_await Coroutine::WaitYield();
                if (advance.IsPressed())
                    break;
                elapsed_secs += Time::DeltaTime();
            }
        }

        Entity().lock()->SetEnable(false);

        // NOTE: 送りキーを押したまま表示中を解くと、すぐ話しかけ直してしまう
        while (IsAdvanceInputDown())
            co_await Coroutine::WaitYield();

        isDisplaying_ = false;
    }

    void NpcChatting::ShowPage(const size_t index, const size_t count) const
    {
        if (!pageText_)
            return;

        std::string marks;
        if (count > 1)
        {
            for (size_t i = 0; i < count; ++i)
                marks += i <= index ? "●" : "○";
        }
        pageText_->SetText(marks);
    }

    void NpcChatting::OnDrawGui()
    {
        if (ImGui::Button("Enable"))
        {
            Entity().lock()->SetEnable(!IsEnable());
        }
        
        ImGuiHelper::OnDrawInputField("textRenderer_", textRenderer_);
        ImGuiHelper::OnDrawInputField("npcNameTextBox_", npcNameTextBox_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGuiHelper::OnDrawInputField("pageText_", pageText_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::NpcChatting);
#pragma endregion
