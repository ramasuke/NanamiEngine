#include "PlayerAvatar_StoryQuestList.h"

#include <algorithm>

namespace GameCore::PlayerAvatar::Quest
{
    void StoryQuestList::StartAll(const QuestContext& context) const
    {
        for (const auto& quest : quests_)
        {
            quest->StartQuest(context);
        }
    }

    void StoryQuestList::Add(const std::shared_ptr<StoryQuestBase>& quest, const QuestContext& context)
    {
        if (!quest)
            return;

        quests_.push_back(quest);
        quest->StartQuest(context);
    }

    void StoryQuestList::Remove(const QuestType& type)
    {
        std::erase_if(quests_, [&type](const auto& quest)
        {
            return quest->QuestType() == type;
        });
    }

    bool StoryQuestList::Contains(const QuestType& type) const
    {
        return std::ranges::any_of(quests_, [&type](const auto& quest)
        {
            return quest->QuestType() == type;
        });
    }

    std::optional<StatusParameter::Money> StoryQuestList::RewardOf(const QuestType& type) const
    {
        for (const auto& quest : quests_)
        {
            if (quest->QuestType() == type)
                return quest->RewardMoney();
        }
        return std::nullopt;
    }

    void StoryQuestList::OnDrawGui() const
    {
        for (const auto& quest : quests_)
        {
            quest->OnDrawGui();
        }
    }
}
