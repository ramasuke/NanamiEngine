#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <type_traits>
#include "../../../../../../../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class ISwordManEndQuestEvent;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class IEndQuestEventFactory final : public SingletonBase<IEndQuestEventFactory>
    {
    public:

        template<typename T>
        void Register(const std::string& id);

        [[nodiscard]]
        const std::unordered_map<
            std::string,
            std::function<std::shared_ptr<Npc::Friendly::Behaviour::Action::ISwordManEndQuestEvent>()>
        >& CreatableEvents() const;

    private:

        std::unordered_map<
            std::string,
            std::function<std::shared_ptr<Npc::Friendly::Behaviour::Action::ISwordManEndQuestEvent>()>
        > factories_;
    };



    template<typename T>
    void IEndQuestEventFactory::Register(const std::string& id)
    {
        static_assert(std::is_base_of_v<Npc::Friendly::Behaviour::Action::ISwordManEndQuestEvent, T>,
            "T must inherit from ISwordManEndQuestEvent");

        static_assert(std::is_default_constructible_v<T>,
            "Event must be default constructible");

        factories_[id] = []
        {
            return std::make_shared<T>();
        };
    }
}

#define REGISTER_SWORDMAN_END_QUEST_EVENT(TYPE)                                     \
namespace {                                                                         \
struct TYPE##AutoRegister {                                                         \
TYPE##AutoRegister() {                                                              \
GameCore::PlayerAvatar::SwordMan::Quest::SwordMan_IEndQuestEventFactory::Instance() \
.Register<TYPE>(#TYPE);                                                             \
}                                                                                   \
};                                                                                  \
static TYPE##AutoRegister global_##TYPE##AutoRegister;                              \
}