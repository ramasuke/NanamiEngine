#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <type_traits>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::PlayerAvatar
{
    class QuestBase;
}

namespace GameCore::PlayerAvatar
{
    class QuestFactory final : public SingletonBase<QuestFactory>
    {
    public:
        template <typename T>
        void Register(const std::string& name)
        {
            static_assert(std::is_base_of_v<QuestBase   , T>, "T must inherit from PlayerAvatar::QuestBase");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

            factories_[name] = [] { return std::make_shared<T>(); };
        }

        std::shared_ptr<QuestBase> Create(const std::string& name);
        [[nodiscard]] bool IsRegistered(const std::string& name) const;
        [[nodiscard]] const std::unordered_map<std::string, std::function<std::shared_ptr<QuestBase>()>>& CreatableQuests() const;

    private:
        std::unordered_map<std::string, std::function<std::shared_ptr<QuestBase>()>> factories_;
    };
}

#define REGISTER_PLAYER_AVATAR_QUEST(TYPE)                       \
namespace {                                                      \
struct TYPE##QuestAutoRegister {                                 \
TYPE##QuestAutoRegister() {                                      \
GameCore::PlayerAvatar::QuestFactory::Instance().Register<TYPE>(#TYPE);        \
}                                                                \
};                                                               \
static TYPE##QuestAutoRegister global_##TYPE##QuestAutoRegister; \
}

