#pragma once
#include "Field.h"

namespace NanamiEngine
{
    template<typename T>
    Core::Object::Field<T> CreateField(const std::weak_ptr<T> context)
    {
        return Core::Object::Field<T>(context);
    }
}
