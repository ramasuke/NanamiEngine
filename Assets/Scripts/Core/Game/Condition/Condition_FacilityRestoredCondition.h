#pragma once
#include "Condition_ICondition.h"
#include "cereal/types/polymorphic.hpp"
#include "../Story/Story_Facility.h"

namespace GameCore::Condition
{
    /** @brief facility_ が直っていれば解放 */
    class FacilityRestoredCondition final : public ICondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const ConditionContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] Story::Facility facility_ = Story::Facility::Dock;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ICondition>(this));
            archive(CEREAL_NVP(facility_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ICondition>(this));
            if (version >= 0) archive(CEREAL_NVP(facility_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Condition::FacilityRestoredCondition, 0);
#pragma endregion
