#pragma once
#include "../../../../Module/Guid/Guid.h"
#include "../GuidRemap/GuidRemap.h"

namespace NanamiEngine::Core::Object
{
    class IFieldContext
    {
    public:
        ~IFieldContext() = default;
        virtual void Init() = 0;
        virtual void RemapGuid(const GuidRemap& guidRemap) = 0;
        [[nodiscard]] virtual const Guid& GetGuid() const = 0;
    };
}
