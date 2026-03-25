#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../../Type/BehaviourTreeType.h"

namespace Editor::Npc::Behaviour
{
    class NodeBase;
    constexpr auto ACTION_NODE_NAME = "ActionNode";
    

    class NodeFactory final : public SingletonBase<NodeFactory>
    {
    public:
        using CreateFunc = std::function<std::shared_ptr<NodeBase>()>;

        void ChangeBehaviourTreeType(BehaviourTreeType type);
        /// ノード型登録
        void Register(const std::string& typeName, CreateFunc createFunction);
        void RegisterAction(const BehaviourTreeType& type, CreateFunc createFunction);
        /// ノード生成
        [[nodiscard]] std::shared_ptr<NodeBase> Create(const std::string& typeName) const;
        /// 登録済み型一覧（エディタ用）
        [[nodiscard]] std::unordered_map<std::string, CreateFunc> CreatableNodes() const;

    private:
        std::unordered_map<std::string, CreateFunc> creatableNodeFactories_;
        std::unordered_map<BehaviourTreeType, CreateFunc> creatableActionNodeFactories_;
        
        BehaviourTreeType currentBehaviourTreeType_ = BehaviourTreeType::FriendlyNpc;
    };
}

#define REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(TYPE) \
namespace {                                             \
struct TYPE##AutoRegister                               \
{                                                       \
TYPE##AutoRegister()                                    \
{                                                       \
auto& factory =                                         \
Editor::Npc::Behaviour::NodeFactory::Instance();        \
factory.Register(                                       \
#TYPE,                                                  \
[]() -> std::shared_ptr<                                \
Editor::Npc::Behaviour::NodeBase>                       \
{                                                       \
return std::make_shared<TYPE>();                        \
});                                                     \
}                                                       \
};                                                      \
static TYPE##AutoRegister global_##TYPE##AutoRegister;  \
}

#define REGISTER_CREATABLE_ACTION_NODE_FACTORY(BEHAVIOUR_TREE_TYPE, TYPE) \
namespace {                                                               \
struct TYPE##AutoRegister                                                 \
{                                                                         \
TYPE##AutoRegister()                                                      \
{                                                                         \
auto& factory = Editor::Npc::Behaviour::NodeFactory::Instance();          \
factory.RegisterAction(                                                   \
BEHAVIOUR_TREE_TYPE,                                                      \
[]() -> std::shared_ptr<                                                  \
Editor::Npc::Behaviour::NodeBase>                                         \
{                                                                         \
return std::make_shared<TYPE>();                                          \
});                                                                       \
}                                                                         \
};                                                                        \
static TYPE##AutoRegister global_##TYPE##AutoRegister;                    \
}