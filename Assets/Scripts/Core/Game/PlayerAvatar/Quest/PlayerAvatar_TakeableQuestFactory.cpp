#include "PlayerAvatar_TakeableQuestFactory.h"
#include <cassert>

#include "PlayerAvatar_ITakeableQuest.h"

namespace GameCore::PlayerAvatar
{
    std::shared_ptr<Quest::ITakeableQuest>
    TakeableQuestFactory::Create(const std::string& name)
    {
        const auto it = factories_.find(name);
        assert(it != factories_.end() && "Quest not registered");

        return it->second();
    }

    bool TakeableQuestFactory::IsRegistered(const std::string& name) const
    {
        return factories_.contains(name);
    }

    const std::unordered_map<std::string, std::function<std::shared_ptr<Quest::ITakeableQuest>()>>&
    TakeableQuestFactory::CreatableQuests() const
    {
        return factories_;
    }
}
