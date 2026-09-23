#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    class IQuestUnlockCondition;

    /// 解放条件を名前から作る。インスペクタの「条件を足す」が列挙する
    class QuestUnlockConditionFactory final : public SingletonBase<QuestUnlockConditionFactory>
    {
    public:
        using Creator = std::function<std::shared_ptr<IQuestUnlockCondition>()>;

        template <typename T>
        void Register(const std::string& name)
        {
            static_assert(std::is_base_of_v<IQuestUnlockCondition, T>, "T must inherit from IQuestUnlockCondition");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

            factories_[name] = [] { return std::make_shared<T>(); };
        }

        [[nodiscard]] const std::map<std::string, Creator>& CreatableConditions() const { return factories_; }

    private:
        std::map<std::string, Creator> factories_;
    };
}

#define REGISTER_QUEST_UNLOCK_CONDITION(TYPE)                                                        \
namespace {                                                                                          \
struct TYPE##QuestUnlockConditionAutoRegister {                                                      \
TYPE##QuestUnlockConditionAutoRegister() {                                                           \
GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockConditionFactory::Instance().Register<TYPE>(#TYPE); \
}                                                                                                    \
};                                                                                                   \
static TYPE##QuestUnlockConditionAutoRegister global_##TYPE##QuestUnlockConditionAutoRegister;       \
}
