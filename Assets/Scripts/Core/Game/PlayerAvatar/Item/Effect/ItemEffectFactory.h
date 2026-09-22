#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "IItemEffect.h"
#include "Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::PlayerAvatar::Item
{
    class ItemEffectFactory final : public SingletonBase<ItemEffectFactory>
    {
    public:
        template<typename T>
        void Register(const std::string& effectName);

        [[nodiscard]] const std::map<std::string, std::function<std::shared_ptr<IItemEffect>()>>& CreatableEffects() const { return factories_; }

    private:
        std::map<std::string, std::function<std::shared_ptr<IItemEffect>()>> factories_;
    };

    template<typename T>
    void ItemEffectFactory::Register(const std::string& effectName)
    {
        static_assert(std::is_base_of_v<IItemEffect, T>, "T must inherit from IItemEffect");
        static_assert(std::is_default_constructible_v<T>, "ItemEffect must be default constructible");

        factories_[effectName] = []
        {
            return std::make_shared<T>();
        };
    }
}

#define REGISTER_ITEM_EFFECT(TYPE)                                        \
namespace {                                                               \
struct TYPE##ItemEffectAutoRegister {                                     \
TYPE##ItemEffectAutoRegister() {                                          \
GameCore::PlayerAvatar::Item::ItemEffectFactory::Instance()               \
.Register<TYPE>(#TYPE);                                                   \
}                                                                         \
};                                                                        \
static TYPE##ItemEffectAutoRegister global_##TYPE##ItemEffectAutoRegister; \
}
