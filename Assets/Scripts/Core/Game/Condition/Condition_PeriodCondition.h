#pragma once
#include "Condition_ICondition.h"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/string.hpp"

namespace GameCore::Condition
{
    /**
     * @brief startAt_ <= 今 < endAt_ なら満たす。"YYYY-MM-DD HH:MM"(日本時間)
     * @note 空欄の側は制限なし。書式が崩れていれば満たさない
     */
    class PeriodCondition final : public ICondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const ConditionContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] std::string startAt_;
        [[serialize(0)]] std::string endAt_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ICondition>(this));
            archive(CEREAL_NVP(startAt_));
            archive(CEREAL_NVP(endAt_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ICondition>(this));
            if (version >= 0) archive(CEREAL_NVP(startAt_));
            if (version >= 0) archive(CEREAL_NVP(endAt_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Condition::PeriodCondition, 0);
#pragma endregion
