#include "Npc_Behaviour_NodeFactory.h"

namespace Editor::Npc::Behaviour
{
    void NodeFactory::ChangeBehaviourTreeType(const BehaviourTreeType type)
    {
        currentBehaviourTreeType_ = type;
    }
    
    void NodeFactory::Register(const std::string& typeName, CreateFunc createFunction)
    {
        creatableNodeFactories_.emplace(typeName, std::move(createFunction));
    }

    void NodeFactory::RegisterAction(const BehaviourTreeType& type, CreateFunc createFunction)
    {
        creatableActionNodeFactories_.emplace(type, std::move(createFunction));
    }

    std::shared_ptr<NodeBase> NodeFactory::Create(const std::string& typeName) const
    {
        // ActionNode は current の Action を生成
        if (typeName == ACTION_NODE_NAME)
        {
            if (const auto it = creatableActionNodeFactories_.find(currentBehaviourTreeType_); it != creatableActionNodeFactories_.end())
            {
                return it->second();
            }
            return nullptr;
        }

        // 通常ノード
        if (const auto it = creatableNodeFactories_.find(typeName); it != creatableNodeFactories_.end())
        {
            return it->second();
        }

        return nullptr;
    }

    std::unordered_map<std::string, NodeFactory::CreateFunc> NodeFactory::CreatableNodes() const
    {
        auto result = creatableNodeFactories_;
        if (const auto it = creatableActionNodeFactories_.find(currentBehaviourTreeType_); it != creatableActionNodeFactories_.end())
        {
            result.emplace(ACTION_NODE_NAME, it->second);
        }

        return result;
    }
}
