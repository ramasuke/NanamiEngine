#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <type_traits>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::PlayerAvatar
{
    class StoryQuestBase;
}

namespace GameCore::PlayerAvatar
{
    class StoryQuestFactory final : public SingletonBase<StoryQuestFactory>
    {
    public:
        template <typename T>
        void Register(const std::string& name)
        {
            static_assert(std::is_base_of_v<StoryQuestBase, T>, "T must inherit from PlayerAvatar::StoryQuestBase");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

            factories_[name] = [] { return std::make_shared<T>(); };
        }

        std::shared_ptr<StoryQuestBase> Create(const std::string& name);
        [[nodiscard]] bool IsRegistered(const std::string& name) const;
        [[nodiscard]] const std::unordered_map<std::string, std::function<std::shared_ptr<StoryQuestBase>()>>& CreatableQuests() const;

    private:
        std::unordered_map<std::string, std::function<std::shared_ptr<StoryQuestBase>()>> factories_;
    };
}

#define REGISTER_STORY_QUEST(TYPE)                                                   \
namespace {                                                                          \
struct TYPE##StoryQuestAutoRegister {                                                \
TYPE##StoryQuestAutoRegister() {                                                     \
GameCore::PlayerAvatar::StoryQuestFactory::Instance().Register<TYPE>(#TYPE);         \
}                                                                                    \
};                                                                                   \
static TYPE##StoryQuestAutoRegister global_##TYPE##StoryQuestAutoRegister;           \
}

