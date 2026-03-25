#include "SwordMan_QuestFactory.h"

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    const std::unordered_map<std::string, std::function<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>()>>&
        QuestFactory::CreatableQuests() const
    {
        return factories_;
    }
}
