#include "NpcChatting.h"

#include "../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../Core/Game/Settings/GameSettings.h"
#include "../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"

namespace GamePlay::Ui
{
    Coroutine::Task<void> NpcChatting::OnDisplayChatAsync(
        const std::string& npcName,
        const Asset::NpcChat& npcChat) const
    {
        Entity().lock()->SetEnable(true);
        npcNameTextBox_->SetText(npcName);
        
        const float chatCharInterval_secs         = GameCore::GameSettings::GetInstance().GetChatTextCharInterval_secs();
        const float chatTextSentenceInterval_secs = GameCore::GameSettings::GetInstance().GetChatTextSentenceInterval_secs();

        for (const auto& chat : npcChat.Get())
        {
            textRenderer_->SetFont(chat.Font());
            textRenderer_->SetTextColor(chat.TextColor());

            const std::string& fullText = chat.Text();
            std::string currentText;
            currentText.reserve(fullText.size());

            for (const char charCharText : fullText)
            {
                currentText.push_back(charCharText);
                textRenderer_->SetText(currentText);

                co_await Coroutine::WaitForSeconds(chatCharInterval_secs);
            }
            
            co_await Coroutine::WaitForSeconds(chatTextSentenceInterval_secs);
        }
        
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
    }
}
