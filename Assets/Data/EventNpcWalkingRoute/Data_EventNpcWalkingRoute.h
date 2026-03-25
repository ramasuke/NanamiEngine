#pragma once
#include <string>
#include <vector>

#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "RoutePoint/Data_Event_Npc_WalkingRoute_RoutePoint.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto EVENT_NPC_WALKING_ROUTE_EXTENSION_LABEL = ".eventNpcWalkingRoute";

    class EventNpcWalkingRoute final : public ScriptableObject
    {
    public:
        explicit EventNpcWalkingRoute(const std::string& contentPath = "");
        [[nodiscard]] const std::vector<Data::EventNpcWalkingRoute::RoutePoint>& Get() const { return route_; }

    private:
        void OnDrawRouteGizmo() const;

        std::vector<Data::EventNpcWalkingRoute::RoutePoint> route_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));

            const std::uint32_t count = static_cast<std::uint32_t>(route_.size());
            archive(cereal::make_nvp("count", count));

            for (std::uint32_t i = 0; i < count; ++i)
            {
                archive(cereal::make_nvp(("route_" + std::to_string(i)).c_str(), route_[i]));
            }
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));

            std::uint32_t count = 0;
            archive(cereal::make_nvp("count", count));

            route_.clear();
            route_.reserve(count);

            for (std::uint32_t i = 0; i < count; ++i)
            {
                Data::EventNpcWalkingRoute::RoutePoint point;
                archive(cereal::make_nvp(("route_" + std::to_string(i)).c_str(), point));
                route_.push_back(std::move(point));
            }
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(EventNpcWalkingRoute, EVENT_NPC_WALKING_ROUTE_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::EventNpcWalkingRoute, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::EventNpcWalkingRoute);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::EventNpcWalkingRoute);
#pragma endregion