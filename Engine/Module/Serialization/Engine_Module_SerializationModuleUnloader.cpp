#include "Engine_Module_SerializationModuleUnloader.h"

#include <algorithm>
#include <iterator>
#include <typeindex>
#include <vector>

#include <../cereal/include/cereal/archives/json.hpp>
#include <../cereal/include/cereal/archives/portable_binary.hpp>
#include <../cereal/include/cereal/types/polymorphic.hpp>

#include "Engine_Module_SerializationTypeRegistry.h"
#include "Engine_Module_SharedStaticObject.h"

namespace
{
    using namespace NanamiEngine::Module::Serialization;

    template <class Archive>
    std::size_t EraseInputBinding(const std::string& name)
    {
        auto& map = cereal::detail::StaticObject<cereal::detail::InputBindingMap<Archive>>::getInstance().map;
        return map.erase(name);
    }

    template <class Archive>
    std::size_t EraseOutputBinding(const std::type_index type)
    {
        auto& map = cereal::detail::StaticObject<cereal::detail::OutputBindingMap<Archive>>::getInstance().map;
        return map.erase(type);
    }

    /** caster の vtable が module にあるか (実体はヒープにあるので、アドレスではなく vtable で見る) */
    bool IsCasterOf(const cereal::detail::PolymorphicCaster* caster, const void* module)
    {
        const void* const vtable = *reinterpret_cast<const void* const*>(caster);
        return SerializationTypeRegistry::ModuleOf(vtable) == module;
    }

    void EraseReverse(cereal::detail::PolymorphicCasters& casters, const std::type_index derived, const std::type_index base)
    {
        auto range = casters.reverseMap.equal_range(derived);
        for (auto it = range.first; it != range.second;)
        {
            it = (it->second == base) ? casters.reverseMap.erase(it) : std::next(it);
        }
    }
}

namespace NanamiEngine::Module::Serialization
{
    ModuleUnloadReport SerializationModuleUnloader::Unregister(const void* module)
    {
        ModuleUnloadReport report;
        auto& casters = cereal::detail::StaticObject<cereal::detail::PolymorphicCasters>::getInstance();
        const auto records = SerializationTypeRegistry::Instance().RecordsOfModule(module);

        // 1. 記録から: 保存・復元の関数 (name / type で引く)
        for (const auto& record : records)
        {
            if (record.name.empty())
                continue;
            report.inputBindings  += EraseInputBinding <cereal::JSONInputArchive>           (record.name);
            report.inputBindings  += EraseInputBinding <cereal::PortableBinaryInputArchive> (record.name);
            report.outputBindings += EraseOutputBinding<cereal::JSONOutputArchive>          (record.type);
            report.outputBindings += EraseOutputBinding<cereal::PortableBinaryOutputArchive>(record.type);
        }

        // 2. 記録から: 型の関係。派生として登録された型は、直接の基底だけでなく cereal が推移的に足した祖先にも
        //    入っている (map[祖先][派生]、reverseMap[派生] = 祖先) ので、派生をキーに全部消す。
        //    その型を基底とする項目 (map[型]) も、派生はすべて同じ DLL の型なので丸ごと消してよい
        for (const auto& record : records)
        {
            for (auto& [base, derivedMap] : casters.map)
            {
                report.casters += derivedMap.erase(record.type);
            }
            casters.map.erase(record.type);
            casters.reverseMap.erase(record.type);
        }

        // 3. 保険: 記録に無くても、vtable がその DLL にある caster を含む項目は消す (経路の途中に DLL の型がある推移的な項目など)
        for (auto baseIt = casters.map.begin(); baseIt != casters.map.end();)
        {
            auto& derivedMap = baseIt->second;
            for (auto derivedIt = derivedMap.begin(); derivedIt != derivedMap.end();)
            {
                const bool owned = std::any_of(derivedIt->second.begin(), derivedIt->second.end(),
                    [module](const cereal::detail::PolymorphicCaster* caster) { return IsCasterOf(caster, module); });
                if (owned)
                {
                    EraseReverse(casters, derivedIt->first, baseIt->first);
                    derivedIt = derivedMap.erase(derivedIt);
                    ++report.sweptCasters;
                }
                else
                {
                    ++derivedIt;
                }
            }
            baseIt = derivedMap.empty() ? casters.map.erase(baseIt) : std::next(baseIt);
        }

        // 4. 記録そのものと、その DLL が作った StaticObject の実体 (bind_to_archives、caster、InputBindingCreator など)
        report.records = records.size();
        SerializationTypeRegistry::Instance().RemoveModule(module);
        report.sharedStatics = SharedStaticObjects::ReleaseOwnedBy(module);
        return report;
    }

    void SerializationModuleUnloader::ClearClassVersions()
    {
        cereal::detail::StaticObject<cereal::detail::Versions>::getInstance().mapping.clear();
    }

    std::size_t SerializationModuleUnloader::CountLeftoverCasters(const void* module)
    {
        const auto& casters = cereal::detail::StaticObject<cereal::detail::PolymorphicCasters>::getInstance();
        std::size_t count = 0;
        for (const auto& [base, derivedMap] : casters.map)
        {
            for (const auto& [derived, chain] : derivedMap)
            {
                for (const auto* caster : chain)
                {
                    if (IsCasterOf(caster, module))
                        ++count;
                }
            }
        }
        return count;
    }
}
