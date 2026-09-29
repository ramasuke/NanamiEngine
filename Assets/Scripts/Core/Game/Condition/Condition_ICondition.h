#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cereal/cereal.hpp"
#include "Condition_ConditionContext.h"

namespace GameCore::Condition
{
    class ICondition
    {
    public:
        virtual ~ICondition() = default;
        [[nodiscard]] virtual bool IsSatisfied(const ConditionContext& context) const = 0;
        [[nodiscard]] virtual std::string Describe() const = 0;
        virtual void OnDrawGui() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const {}
        template<class Archive> void load(Archive& archive, const std::uint32_t version) {}
    };

    using Conditions = std::vector<std::shared_ptr<ICondition>>;
}

CEREAL_CLASS_VERSION(GameCore::Condition::ICondition, 0)
