#pragma once
#include "Condition_ICondition.h"
#include "cereal/types/polymorphic.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "../../../../Data/Decoration/Data_DecorationData.h"

namespace GameCore::Condition
{
    /** @brief 島の飾り decoration_ を持っていれば満たす */
    class DecorationOwnedCondition final : public ICondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const ConditionContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::DecorationData) decoration_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ICondition>(this));
            archive(CEREAL_NVP(decoration_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ICondition>(this));
            if (version >= 0) archive(CEREAL_NVP(decoration_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Condition::DecorationOwnedCondition, 0);
#pragma endregion
