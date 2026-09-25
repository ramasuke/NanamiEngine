#include "Engine_Module_SerializationTypeRegistry.h"

#include <algorithm>
#include <Windows.h>

namespace NanamiEngine::Module::Serialization
{
    SerializationTypeRegistry& SerializationTypeRegistry::Instance()
    {
        static SerializationTypeRegistry instance;
        return instance;
    }

    void SerializationTypeRegistry::Record(
        const std::type_index type, const std::type_index base, std::string name, const void* addressInModule)
    {
        void* const module = ModuleOf(addressInModule);
        std::lock_guard lock(mutex_);
        records_.push_back({ type, base, std::move(name), module });
    }

    std::vector<SerializationTypeRecord> SerializationTypeRegistry::Records() const
    {
        std::lock_guard lock(mutex_);
        return records_;
    }

    std::vector<SerializationTypeRecord> SerializationTypeRegistry::RecordsOfModule(const void* module) const
    {
        std::lock_guard lock(mutex_);
        std::vector<SerializationTypeRecord> result;
        for (const auto& record : records_)
        {
            if (record.module == module)
                result.push_back(record);
        }
        return result;
    }

    void SerializationTypeRegistry::RemoveModule(const void* module)
    {
        std::lock_guard lock(mutex_);
        std::erase_if(records_, [module](const SerializationTypeRecord& record) { return record.module == module; });
    }

    void* SerializationTypeRegistry::ModuleOf(const void* address)
    {
        HMODULE module = nullptr;
        // NOTE: UNCHANGED_REFCOUNT: 参照カウントを増やさない (増やすと FreeLibrary で外れなくなる)
        if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                static_cast<LPCWSTR>(address),
                &module))
        {
            return nullptr;
        }
        return module;
    }
}
