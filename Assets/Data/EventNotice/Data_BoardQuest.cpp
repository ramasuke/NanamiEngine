#include "Data_BoardQuest.h"

#include <algorithm>

#include "../../Scripts/Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestType.h"
#include "../../Scripts/Core/Game/PlayerAvatar/Quest/PlayerAvatar_StoryQuestFactory.h"

namespace NanamiEngine::Module::Asset
{
    BoardQuest::BoardQuest(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void BoardQuest::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("title_", title_);
        LibCore::ImGuiHelper::OnDrawInputField("clientName_", clientName_);
        LibCore::ImGuiHelper::OnDrawInputField("rank_", rank_);
        rank_ = std::clamp(rank_, 1, BOARD_QUEST_MAX_RANK);
        LibCore::ImGuiHelper::OnDrawInputField("goalText_", goalText_);
        LibCore::ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                descriptionLines_.emplace_back();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("stage_", stage_);
        LibCore::ImGuiHelper::OnDrawInputField("event_", event_);

        if (!ImGui::CollapsingHeader("quest_", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const auto& quests = GameCore::PlayerAvatar::StoryQuestFactory::Instance().CreatableQuests();
        if (quests.empty())
            ImGui::TextDisabled("StoryQuest がまだ登録されていません");

        for (const auto& [questName, createQuest] : quests)
        {
            if (ImGui::Selectable(questName.c_str()))
            {
                quest_ = createQuest();
            }
        }

        if (quest_)
        {
            ImGui::Separator();
            ImGui::Text("Current: %s", GameCore::PlayerAvatar::ToString(quest_->QuestType()).data());
            quest_->OnDrawGui();
        }
    }
}
