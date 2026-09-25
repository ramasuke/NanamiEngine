#include "NanamiModule.h"

#include <Windows.h>

namespace NanamiEngine::Core
{
    ModuleHandle ModuleOf(const void* address)
    {
        if (address == nullptr)
            return {};
        HMODULE module = nullptr;
        // NOTE: UNCHANGED_REFCOUNT: 参照カウントを増やさない (増やすと FreeLibrary で外れなくなる)
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(address), &module))
            return {};
        return ModuleHandle(module);
    }

    ModuleHandle ModuleOfVTable(const void* object)
    {
        if (object == nullptr)
            return {};
        return ModuleOf(*static_cast<void* const*>(object));
    }
}
