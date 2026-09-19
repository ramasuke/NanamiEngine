#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "IMagicSpellEffect.h"
#include "../../../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::Magic
{
    class MagicSpellEffectFactory final : public SingletonBase<MagicSpellEffectFactory>
    {
    public:
        template<typename T>
        void Register(const std::string& effectName);

        [[nodiscard]] const std::map<std::string, std::function<std::shared_ptr<IMagicSpellEffect>()>>& CreatableEffects() const { return factories_; }

    private:
        std::map<std::string, std::function<std::shared_ptr<IMagicSpellEffect>()>> factories_;
    };

    template<typename T>
    void MagicSpellEffectFactory::Register(const std::string& effectName)
    {
        static_assert(std::is_base_of_v<IMagicSpellEffect, T>, "T must inherit from IMagicSpellEffect");
        static_assert(std::is_default_constructible_v<T>, "MagicSpellEffect must be default constructible");

        factories_[effectName] = []
        {
            return std::make_shared<T>();
        };
    }
}

#define REGISTER_MAGIC_SPELL_EFFECT(TYPE)                                         \
namespace {                                                                       \
struct TYPE##MagicSpellEffectAutoRegister {                                       \
TYPE##MagicSpellEffectAutoRegister() {                                            \
GameCore::Magic::MagicSpellEffectFactory::Instance()                              \
.Register<TYPE>(#TYPE);                                                           \
}                                                                                 \
};                                                                                \
static TYPE##MagicSpellEffectAutoRegister global_##TYPE##MagicSpellEffectAutoRegister; \
}
