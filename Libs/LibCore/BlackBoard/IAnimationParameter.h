#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "cereal/cereal.hpp"

namespace NanamiEngine::Module::AnimationTree
{
    class NANAMI_API IAnimationParameter
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

        virtual ~IAnimationParameter() = default;
        [[nodiscard]] virtual const std::string& Name() const = 0;
        virtual void OnDrawGui() = 0;
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::IAnimationParameter, 0);
