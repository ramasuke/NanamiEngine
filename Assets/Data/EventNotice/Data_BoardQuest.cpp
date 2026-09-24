#include "Data_BoardQuest.h"

#include <algorithm>

#include "../../Scripts/Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestType.h"
#include "../../Scripts/Core/Game/PlayerAvatar/Quest/PlayerAvatar_TakeableQuestFactory.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    BoardQuest::BoardQuest(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    bool BoardQuest::IsUnlocked(const GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockContext& context) const
    {
        return GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockConditionList::AreAllSatisfied(unlockConditions_, context);
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
        GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockConditionList::DrawListGui("unlockConditions_", unlockConditions_);
        LibCore::ImGuiHelper::OnDrawInputField("lockedText_", lockedText_);

        if (!ImGui::CollapsingHeader("quest_", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const auto& quests = GameCore::PlayerAvatar::TakeableQuestFactory::Instance().CreatableQuests();
        if (quests.empty())
            ImGui::TextDisabled("受注できるクエストがまだ登録されていません");

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

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(BoardQuest, BOARD_QUEST_EXTENSION_LABEL, "EventBoard")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::BoardQuest);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::BoardQuest);
#pragma endregion
