#pragma once

namespace NanamiEngine::Core::Object
{
    class IFieldContext
    {
    public:
        ~IFieldContext() = default;
        virtual void Init() = 0;
    };
}
