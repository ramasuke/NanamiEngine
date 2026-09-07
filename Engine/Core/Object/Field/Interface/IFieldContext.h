#pragma once
#include "../../../../Module/Guid/Guid.h"

namespace NanamiEngine::Core::Object
{
    class IFieldContext
    {
    public:
        ~IFieldContext() = default;
        virtual void Init() = 0;
        [[nodiscard]] virtual const Guid& GetGuid() const = 0;
    };
}
