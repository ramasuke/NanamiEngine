#include "PlayerAvatar_CompletedQuestGroup.h"

#include <cereal/types/string.hpp>
#include <cereal/types/unordered_set.hpp>
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar::Quest
{
    CompletedQuestGroup::CompletedQuestGroup()
    {
        Reload();
    }

    void CompletedQuestGroup::Subscribe(const QuestType& completeQuest)
    {
        completedQuests_.insert(completeQuest);
    }

    bool CompletedQuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_.contains(quest);
    }

    void CompletedQuestGroup::SubscribeBoardQuest(const std::string& boardQuestGuid)
    {
        completedBoardQuests_.insert(boardQuestGuid);
    }

    bool CompletedQuestGroup::CheckBoardQuestCompleted(const std::string& boardQuestGuid) const
    {
        return !boardQuestGuid.empty() && completedBoardQuests_.contains(boardQuestGuid);
    }

    void CompletedQuestGroup::Reload()
    {
        completedQuests_ = LocalPrefs::LoadOrDefault<std::unordered_set<QuestType>>(
            COMPLETED_QUEST_SAVE_KEY,
            std::unordered_set<QuestType>());
        completedBoardQuests_ = LocalPrefs::LoadOrDefault<std::unordered_set<std::string>>(
            COMPLETED_BOARD_QUEST_SAVE_KEY,
            std::unordered_set<std::string>());
    }

    void CompletedQuestGroup::Save() const
    {
        LocalPrefs::Save(COMPLETED_QUEST_SAVE_KEY, completedQuests_);
        LocalPrefs::Save(COMPLETED_BOARD_QUEST_SAVE_KEY, completedBoardQuests_);
    }
}
