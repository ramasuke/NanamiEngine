#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <type_traits>

#include "../../../../../../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class ITakeableSwordManQuest;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class QuestFactory final : public SingletonBase<QuestFactory>
    {
    public:
        template<typename T>
        void Register(const std::string& questId);

        [[nodiscard]] const std::unordered_map<std::string, std::function<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>()>>& CreatableQuests() const;

    private:
        std::unordered_map<std::string, std::function<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>()>> factories_;
    };

    template<typename T>
    void QuestFactory::Register(const std::string& questId)
    {
        static_assert(std::is_base_of_v<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest, T>, "T must inherit from ITakeableSwordManQuest");
        static_assert(std::is_default_constructible_v<T>, "Quest must be default constructible");

        factories_[questId] = []
        {
            return std::make_shared<T>();
        };
    }
}

#define REGISTER_SWORDMAN_QUEST(TYPE)                             \
namespace {                                                       \
struct TYPE##QuestAutoRegister {                                  \
TYPE##QuestAutoRegister() {                                       \
GameCore::PlayerAvatar::SwordMan::Quest::QuestFactory::Instance() \
.Register<TYPE>(#TYPE);                                           \
}                                                                 \
};                                                                \
static TYPE##QuestAutoRegister global_##TYPE##QuestAutoRegister;  \
}