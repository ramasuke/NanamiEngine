#include "NanamiModule.h"

#include <Windows.h>

namespace NanamiEngine::Core
{
    void* ModuleOf(const void* address)
    {
        if (address == nullptr)
            return nullptr;
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(address), &module))
            return nullptr;
        return module;
    }

    void* ModuleOfVTable(const void* object)
    {
        if (object == nullptr)
            return nullptr;
        return ModuleOf(*static_cast<void* const*>(object));
    }
}
