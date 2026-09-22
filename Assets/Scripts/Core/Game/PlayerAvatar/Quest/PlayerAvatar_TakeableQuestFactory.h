#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <type_traits>

#include "Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::PlayerAvatar::Quest
{
    class ITakeableQuest;
}

namespace GameCore::PlayerAvatar
{
    /// 職業を問わないクエスト(メインストーリー・依頼)を名前から作る。掲示板と NPC のエディタが列挙する
    class TakeableQuestFactory final : public SingletonBase<TakeableQuestFactory>
    {
    public:
        template <typename T>
        void Register(const std::string& name)
        {
            static_assert(std::is_base_of_v<Quest::ITakeableQuest, T>, "T must inherit from PlayerAvatar::Quest::ITakeableQuest");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

            factories_[name] = [] { return std::make_shared<T>(); };
        }

        std::shared_ptr<Quest::ITakeableQuest> Create(const std::string& name);
        [[nodiscard]] bool IsRegistered(const std::string& name) const;
        [[nodiscard]] const std::unordered_map<std::string, std::function<std::shared_ptr<Quest::ITakeableQuest>()>>& CreatableQuests() const;

    private:
        std::unordered_map<std::string, std::function<std::shared_ptr<Quest::ITakeableQuest>()>> factories_;
    };
}

#define REGISTER_TAKEABLE_QUEST(TYPE)                                                \
namespace {                                                                          \
struct TYPE##TakeableQuestAutoRegister {                                             \
TYPE##TakeableQuestAutoRegister() {                                                  \
GameCore::PlayerAvatar::TakeableQuestFactory::Instance().Register<TYPE>(#TYPE);      \
}                                                                                    \
};                                                                                   \
static TYPE##TakeableQuestAutoRegister global_##TYPE##TakeableQuestAutoRegister;     \
}
