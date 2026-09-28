#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::Condition
{
    class ICondition;

    /// 解放条件を名前から作る。インスペクタの「条件を足す」が列挙する
    class ConditionFactory final : public SingletonBase<ConditionFactory>
    {
    public:
        using Creator = std::function<std::shared_ptr<ICondition>()>;

        template <typename T>
        void Register(const std::string& name)
        {
            static_assert(std::is_base_of_v<ICondition, T>, "T must inherit from ICondition");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

            factories_[name] = [] { return std::make_shared<T>(); };
        }

        [[nodiscard]] const std::map<std::string, Creator>& CreatableConditions() const { return factories_; }

    private:
        std::map<std::string, Creator> factories_;
    };
}

#define REGISTER_CONDITION(TYPE)                                                        \
namespace {                                                                                          \
struct TYPE##ConditionAutoRegister {                                                      \
TYPE##ConditionAutoRegister() {                                                           \
GameCore::Condition::ConditionFactory::Instance().Register<TYPE>(#TYPE); \
}                                                                                                    \
};                                                                                                   \
static TYPE##ConditionAutoRegister global_##TYPE##ConditionAutoRegister;       \
}
