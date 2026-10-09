#include "PlayerAvatar_QuestList.h"

#include <algorithm>
#include <utility>

#include "PlayerAvatar_QuestType.h"

namespace GameCore::PlayerAvatar::Quest
{
    void QuestList::StartAll(const QuestContext& context) const
    {
        // NOTE: 開始したそばから達成して Remove されうるので、写しを回す
        const auto quests = quests_;
        for (const auto& quest : quests)
        {
            quest->StartQuest(context);
        }
    }

    bool QuestList::Add(const std::shared_ptr<ITakeableQuest>& quest, const QuestContext& context)
    {
        if (!quest || ContainsSame(*quest))
            return false;

        quests_.push_back(quest);
        quest->StartQuest(context);
        return true;
    }

    void QuestList::Remove(const QuestType& type)
    {
        std::erase_if(quests_, [&type](const auto& quest)
        {
            return quest->QuestType() == type;
        });
    }

    void QuestList::Remove(const ITakeableQuest* quest)
    {
        std::erase_if(quests_, [quest](const auto& taking)
        {
            return taking.get() == quest;
        });
    }

    void QuestList::Merge(const std::vector<std::shared_ptr<ITakeableQuest>>& quests)
    {
        for (const auto& quest : quests)
        {
            if (quest && !ContainsSame(*quest))
                quests_.push_back(quest);
        }
    }

    std::vector<std::shared_ptr<ITakeableQuest>> QuestList::Release()
    {
        return std::exchange(quests_, {});
    }

    bool QuestList::Contains(const QuestType& type) const
    {
        return Find(type) != nullptr;
    }

    bool QuestList::ContainsSame(const ITakeableQuest& quest) const
    {
        return std::ranges::any_of(quests_, [&quest](const auto& taking)
        {
            return taking->IsSameQuest(quest);
        });
    }

    bool QuestList::ContainsBoardQuest(const std::string& boardQuestGuid, const QuestType& type) const
    {
        return std::ranges::any_of(quests_, [&boardQuestGuid, &type](const auto& taking)
        {
            const auto& guid = taking->BoardQuestGuid();
            return guid.empty() ? taking->QuestType() == type : guid == boardQuestGuid;
        });
    }

    const ITakeableQuest* QuestList::Find(const QuestType& type) const
    {
        const auto found = std::ranges::find_if(quests_, [&type](const auto& quest)
        {
            return quest->QuestType() == type;
        });
        return found != quests_.end() ? found->get() : nullptr;
    }

    void QuestList::OnDrawGui() const
    {
        for (const auto& quest : quests_)
        {
            quest->OnDrawGui();
        }
    }
}
