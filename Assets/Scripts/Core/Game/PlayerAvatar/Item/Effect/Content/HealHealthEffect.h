#pragma once
#include "../IItemEffect.h"
#include "../ItemEffectFactory.h"
#include "cereal/archives/json.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar::Item
{
    class HealHealthEffect final : public IItemEffect
    {
    public:
        void Apply(IItemEffectTarget& target) const override;

    private:
        [[serialize(0)]] int amount_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IItemEffect>(this));
            archive(CEREAL_NVP(amount_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IItemEffect>(this));
            if (version >= 0) archive(CEREAL_NVP(amount_));
        }
#pragma endregion
    };

    REGISTER_ITEM_EFFECT(HealHealthEffect)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Item::HealHealthEffect, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::Item::HealHealthEffect);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Item::IItemEffect, GameCore::PlayerAvatar::Item::HealHealthEffect);
#pragma endregion
