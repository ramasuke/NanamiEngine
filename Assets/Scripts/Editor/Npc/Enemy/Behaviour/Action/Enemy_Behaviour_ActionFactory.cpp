#include "Enemy_Behaviour_ActionFactory.h"

#include "../../../../../Core/Game/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionBase.h"

namespace Editor::Npc::Enemy::Behaviour
{
    void ActionFactory::Register(const std::string& typeName, CreateFunc func)
    {
        creatableActionFactories_[typeName] = std::move(func);
    }

    std::unique_ptr<GameCore::Npc::Enemy::Behaviour::ActionBase> ActionFactory::Create(const std::string& typeName) const
    {
        const auto it = creatableActionFactories_.find(typeName);
        if (it == creatableActionFactories_.end())
        {
            return nullptr;
        }

        return it->second();
    }
}
