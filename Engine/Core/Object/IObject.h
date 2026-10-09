#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../Module/Guid/Guid.h"
#include "../cereal/include/cereal/types/polymorphic.hpp"

namespace NanamiEngine::Module::Object
{
    class NANAMI_API IObject
    {
    public:
        virtual ~IObject() = default;
        [[nodiscard]] virtual const Guid& GetGuid() const = 0;
        // NOTE: Inspector に表示する GUI
        virtual void OnDrawGui() = 0;
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            
        }
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            
        }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Object::IObject, 0);