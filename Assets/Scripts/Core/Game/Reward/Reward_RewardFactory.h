#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::Reward
{
    class IReward;

    /// 報酬を名前から作る。インスペクタの「報酬を足す」が列挙する
    class RewardFactory final : public SingletonBase<RewardFactory>
    {
    public:
        using Creator = std::function<std::shared_ptr<IReward>()>;

        template <typename T>
        void Register(const std::string& name)
        {
            static_assert(std::is_base_of_v<IReward, T>, "T must inherit from IReward");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

            factories_[name] = [] { return std::make_shared<T>(); };
        }

        [[nodiscard]] const std::map<std::string, Creator>& CreatableRewards() const { return factories_; }

    private:
        std::map<std::string, Creator> factories_;
    };
}

#define REGISTER_REWARD(TYPE)                                                       \
namespace {                                                                         \
struct TYPE##RewardAutoRegister {                                                   \
TYPE##RewardAutoRegister() {                                                        \
GameCore::Reward::RewardFactory::Instance().Register<TYPE>(#TYPE);                  \
}                                                                                   \
};                                                                                  \
static TYPE##RewardAutoRegister global_##TYPE##RewardAutoRegister;                  \
}
