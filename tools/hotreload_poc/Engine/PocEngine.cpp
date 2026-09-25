#include "PocEngine.h"

#include <sstream>
#include <typeinfo>

#include <../cereal/include/cereal/types/memory.hpp>
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Engine/Module/Serialization/Engine_Module_SharedStaticObject.h"

namespace Poc
{
    std::shared_ptr<Component> Engine::CreateEngineComponent()
    {
        auto component = std::make_shared<EngineComponent>();
        component->id = 7;
        component->speed = 2.5f;
        return component;
    }

    std::string Engine::SaveJson(const std::shared_ptr<Component>& component)
    {
        std::stringstream stream;
        {
            cereal::JSONOutputArchive archive(stream);
            archive(cereal::make_nvp("component", component));
        }
        return stream.str();
    }

    std::shared_ptr<Component> Engine::LoadJson(const std::string& json)
    {
        std::stringstream stream(json);
        cereal::JSONInputArchive archive(stream);
        std::shared_ptr<Component> component;
        archive(cereal::make_nvp("component", component));
        return component;
    }

    std::string Engine::SaveBinary(const std::shared_ptr<Component>& component)
    {
        std::stringstream stream;
        {
            cereal::PortableBinaryOutputArchive archive(stream);
            archive(component);
        }
        return stream.str();
    }

    std::shared_ptr<Component> Engine::LoadBinary(const std::string& bytes)
    {
        std::stringstream stream(bytes);
        cereal::PortableBinaryInputArchive archive(stream);
        std::shared_ptr<Component> component;
        archive(component);
        return component;
    }

    std::string Engine::TypeName(const Component& component)
    {
        return typeid(component).name();
    }

    Stats Engine::GetStats()
    {
        using namespace cereal::detail;
        Stats stats;
        stats.inputJson    = StaticObject<InputBindingMap <cereal::JSONInputArchive>>           ::getInstance().map.size();
        stats.inputBinary  = StaticObject<InputBindingMap <cereal::PortableBinaryInputArchive>> ::getInstance().map.size();
        stats.outputJson   = StaticObject<OutputBindingMap<cereal::JSONOutputArchive>>          ::getInstance().map.size();
        stats.outputBinary = StaticObject<OutputBindingMap<cereal::PortableBinaryOutputArchive>>::getInstance().map.size();
        const auto& casters = StaticObject<PolymorphicCasters>::getInstance();
        stats.casterBases = casters.map.size();
        for (const auto& [base, derivedMap] : casters.map)
            stats.casterEntries += derivedMap.size();
        stats.reverseEntries = casters.reverseMap.size();
        stats.versions      = StaticObject<Versions>::getInstance().mapping.size();
        stats.sharedStatics = NanamiEngine::Module::Serialization::SharedStaticObjects::Count();
        stats.records       = NanamiEngine::Module::Serialization::SerializationTypeRegistry::Instance().Records().size();
        return stats;
    }
}

NANAMI_REGISTER_TYPE(Poc::EngineComponent, Poc::Component);
