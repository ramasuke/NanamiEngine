#include "Data_Chat.h"

void GamePlay::Data::Chat::OnDrawGui()
{
    ImGui::SeparatorText("NpcChatText");
    font_.OnDrawGui();

    ImGui::Text("Text");
    static constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    strncpy_s(buffer, BUFFER_SIZE, text_.c_str(), _TRUNCATE);
    buffer[BUFFER_SIZE - 1] = '\0';

    if (ImGui::InputTextMultiline(
            "##text_multiline",
            buffer,
            BUFFER_SIZE,
            ImVec2(-1.0f, 150.0f)))
    {
        text_ = buffer;
    }

    ImGuiHelper::OnDrawInputField("color_", textColor_);
}
