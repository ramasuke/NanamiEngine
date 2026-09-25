#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../Module/Guid/Guid.h"
#include "../GuidRemap/GuidRemap.h"

namespace NanamiEngine::Core::Object
{
    class NANAMI_API IFieldContext
    {
    public:
        ~IFieldContext() = default;
        virtual void Init() = 0;
        virtual void RemapGuid(const GuidRemap& guidRemap) = 0;
        [[nodiscard]] virtual const Guid& GetGuid() const = 0;
    };
}
