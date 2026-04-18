#pragma once
#include <type_traits>

#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Core::Application
{
    template<typename T>
    concept LifeCycleCallbackType = std::is_base_of_v<Module::Object::IObject, T>;
}
