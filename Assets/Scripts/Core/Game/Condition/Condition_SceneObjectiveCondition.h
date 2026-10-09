#pragma once
#include <string>

#include "Condition_ICondition.h"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/string.hpp"

namespace GameCore::Condition
{
    // NOTE: 今のメインシーンに一時的な目標 objectiveId_ が立っていれば満たす
    class SceneObjectiveCondition final : public ICondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const ConditionContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] std::string objectiveId_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ICondition>(this));
            archive(CEREAL_NVP(objectiveId_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ICondition>(this));
            if (version >= 0) archive(CEREAL_NVP(objectiveId_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Condition::SceneObjectiveCondition, 0);
#pragma endregion
