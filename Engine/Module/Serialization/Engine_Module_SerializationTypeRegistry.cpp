#include "Engine_Module_SerializationTypeRegistry.h"

#include <Windows.h>

namespace NanamiEngine::Module::Serialization
{
    SerializationTypeRegistry& SerializationTypeRegistry::Instance()
    {
        // 登録は各 .cpp の static 初期化から呼ばれるので、初回呼び出しで作る
        static SerializationTypeRegistry instance;
        return instance;
    }

    void SerializationTypeRegistry::Register(const std::type_info& type, const std::type_info& base, const char* name, const void* moduleAnchor)
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCWSTR>(moduleAnchor), &module);

        records_.push_back({
            .type   = std::type_index(type),
            .base   = std::type_index(base),
            .name   = name != nullptr ? name : "",
            .module = module,
        });
    }
}
