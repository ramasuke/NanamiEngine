#pragma once
#include <vector>
#include <memory>
#include <cstdint>

#include "../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../AdditionCondition/Interface/IAnimationNodePathAdditionCondition.h"
#include "../AdditionCondition/AnimationNodePathAdditionCondition.h"

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationNodePathAdditionConditionGroup final
    {
    public:
        void OnDrawGui();
        [[nodiscard]] bool Check(BlackBoard::ParameterGroup& additionParameterGroup) const;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            std::size_t count = conditions_.size();
            archive(count);
            for (const auto& condition : conditions_)
            {
                archive(condition);
            }
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            std::size_t count = 0;
            archive(count);
            conditions_.clear();
            conditions_.reserve(count);
            for (std::size_t i = 0; i < count; ++i)
            {
                std::shared_ptr<IAnimationNodePathAdditionCondition> condition;
                archive(condition);
                conditions_.push_back(condition);
            }
        }

    private:
        std::vector<std::shared_ptr<IAnimationNodePathAdditionCondition>> conditions_;
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionConditionGroup, 0);