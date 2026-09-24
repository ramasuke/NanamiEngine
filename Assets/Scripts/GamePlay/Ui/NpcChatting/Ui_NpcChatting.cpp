#include "Ui_NpcChatting.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../Core/Game/Settings/GameSettings.h"
#include "../../Sound/UiSoundBank.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    Coroutine::Task<void> NpcChatting::OnDisplayChatAsync(
        const std::string& npcName,
        const Asset::NpcChat& npcChat) const
    {
        if (!npcNameTextBox_)
            co_return;
        
        isDisplaying_ = true;
        Entity().lock()->SetEnable(true);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::ChatOpen);

        npcNameTextBox_->SetText(npcName);
        
        const float chatCharInterval_secs         = GameCore::GameSettings::GetInstance().GetChatTextCharInterval_secs();
        const float chatTextSentenceInterval_secs = GameCore::GameSettings::GetInstance().GetChatTextSentenceInterval_secs();

        for (const auto& chat : npcChat.Get())
        {
            if (!textRenderer_)
                break;
            
            textRenderer_->SetFont(chat.Font());
            textRenderer_->SetTextColor(chat.TextColor());

            const std::string& fullText = chat.Text();
            std::string currentText;
            currentText.reserve(fullText.size());

            for (const char charCharText : fullText)
            {
                currentText.push_back(charCharText);
                if (!textRenderer_)
                    break;
                
                textRenderer_->SetText(currentText);
                co_await Coroutine::WaitForSeconds(chatCharInterval_secs);
            }

            if (!textRenderer_)
                break;
            
            co_await Coroutine::WaitForSeconds(chatTextSentenceInterval_secs);
        }

        isDisplaying_ = false;
        Entity().lock()->SetEnable(false);
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
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::NpcChatting);
#pragma endregion
