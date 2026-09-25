#include "Engine_Module_SerializationTypeRegistry.h"

#include <algorithm>

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
        const Core::ModuleHandle module = Core::ModuleOf(addressInModule);
        std::lock_guard lock(mutex_);
        records_.push_back({ type, base, std::move(name), module });
    }

    std::vector<SerializationTypeRecord> SerializationTypeRegistry::Records() const
    {
        std::lock_guard lock(mutex_);
        return records_;
    }

    std::vector<SerializationTypeRecord> SerializationTypeRegistry::RecordsOfModule(const Core::ModuleHandle module) const
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

    void SerializationTypeRegistry::RemoveModule(const Core::ModuleHandle module)
    {
        std::lock_guard lock(mutex_);
        std::erase_if(records_, [module](const SerializationTypeRecord& record) { return record.module == module; });
    }
}
