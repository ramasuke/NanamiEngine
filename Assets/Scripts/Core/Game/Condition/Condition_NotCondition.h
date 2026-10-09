#pragma once
#include "Condition_ICondition.h"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Condition
{
    // NOTE: condition_ を満たしていなければ解放。空なら解放
    class NotCondition final : public ICondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const ConditionContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] std::shared_ptr<ICondition> condition_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ICondition>(this));
            archive(CEREAL_NVP(condition_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ICondition>(this));
            if (version >= 0) archive(CEREAL_NVP(condition_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Condition::NotCondition, 0);
#pragma endregion
