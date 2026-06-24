#pragma once
#include <cstddef>
#include <string>

#include "cereal/cereal.hpp"
#include "../../Libs/rxcpp/operators/rx-all.hpp"
#include "../Rx/SerializableSubject/unit/unit.h"

namespace NanamiEngine::Core::Network { class ByteBuffer; }

namespace NanamiEngine::Module::AnimationTree
{
    class IAnimationParameter
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

        [[nodiscard]] virtual rxcpp::observable<LibCore::Rx::unit> OnChanged() const = 0;
        virtual void WriteValueTo(NanamiEngine::Core::Network::ByteBuffer& buffer) const = 0;
        virtual void ReadValueFrom(const NanamiEngine::Core::Network::ByteBuffer& buffer, size_t& offset) = 0;
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::IAnimationParameter, 0);