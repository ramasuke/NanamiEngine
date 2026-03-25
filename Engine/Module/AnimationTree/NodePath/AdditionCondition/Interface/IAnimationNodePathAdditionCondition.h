#pragma once
#include "../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "cereal/cereal.hpp"

namespace NanamiEngine::Module::AnimationTree
{
    class IAnimationNodePathAdditionCondition 
    {
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
        }
        
        virtual ~IAnimationNodePathAdditionCondition() = default;
        [[nodiscard]] virtual bool Check(BlackBoard::ParameterGroup& additionParameterGroup) = 0;
        virtual void OnDrawGui() = 0;
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition, 0);