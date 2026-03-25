#include "PlayerAvatar_QuestFactory.h"
#include <cassert>

namespace GameCore::PlayerAvatar
{
    std::shared_ptr<QuestBase>
    QuestFactory::Create(const std::string& name)
    {
        const auto it = factories_.find(name);
        assert(it != factories_.end() && "Quest not registered");

        return it->second();
    }

    bool QuestFactory::IsRegistered(const std::string& name) const
    {
        return factories_.contains(name);
    }

    const std::unordered_map<std::string, std::function<std::shared_ptr<QuestBase>()>>&
    QuestFactory::CreatableQuests() const
    {
        return factories_;
    }
}
