#include "Friendly_Behaviour_ActionFactory.h"

#include "../../../../../Core/Game/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionBase.h"

namespace Editor::Npc::Friendly::Behaviour
{
    void ActionFactory::Register(const std::string& typeName, CreateFunc func)
    {
        creatableActionFactories_[typeName] = std::move(func);
    }

    std::unique_ptr<GameCore::Npc::Friendly::Behaviour::ActionBase> ActionFactory::Create(const std::string& typeName) const
    {
        const auto it = creatableActionFactories_.find(typeName);
        if (it == creatableActionFactories_.end())
        {
            return nullptr;
        }

        return it->second();
    }
}
