#include "Data_NpcChat.h"

Asset::NpcChat::NpcChat(const std::string& contentPath)
    : ScriptableObject(contentPath)
{
}

void Asset::NpcChat::OnDrawGui()
{
    ImGui::SeparatorText("NpcChat");
    int count = static_cast<int>(npcChatTexts_.size());
    if (ImGui::InputInt("Count", &count))
    {
        count = (std::max)(count, 0);
        npcChatTexts_.resize(count);
    }

    ImGui::Spacing();

    for (size_t i = 0; i < npcChatTexts_.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        ImGui::Separator();

        ImGui::Text("NpcChatText[%zu]", i);

        npcChatTexts_[i].OnDrawGui();

        if (ImGui::Button("Remove"))
        {
            npcChatTexts_.erase(npcChatTexts_.begin() + i);
            ImGui::PopID();
            break;
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Add Text"))
        npcChatTexts_.emplace_back();
}
