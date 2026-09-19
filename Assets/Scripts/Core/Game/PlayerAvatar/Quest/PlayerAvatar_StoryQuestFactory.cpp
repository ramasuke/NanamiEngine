#include "PlayerAvatar_StoryQuestFactory.h"
#include <cassert>

namespace GameCore::PlayerAvatar
{
    std::shared_ptr<StoryQuestBase>
    StoryQuestFactory::Create(const std::string& name)
    {
        const auto it = factories_.find(name);
        assert(it != factories_.end() && "Quest not registered");

        return it->second();
    }

    bool StoryQuestFactory::IsRegistered(const std::string& name) const
    {
        return factories_.contains(name);
    }

    const std::unordered_map<std::string, std::function<std::shared_ptr<StoryQuestBase>()>>&
    StoryQuestFactory::CreatableQuests() const
    {
        return factories_;
    }
}
