#pragma once
#include <string>
#include <vector>

#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto FRIENDLY_NPC_WALKING_ROUTE_EXTENSION_LABEL = ".frinedlyNpcWalkingRoute";
    
    class NpcWalkingRoute final : public ScriptableObject
    {
    public:
        explicit NpcWalkingRoute(const std::string& contentPath = "");
        [[nodiscard]] const std::vector<glm::vec3>& Get() const { return walkingRoute_; }
        
    private:
        void OnDrawRouteGizmo();
        
        std::vector<glm::vec3> walkingRoute_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));

            const int count = static_cast<int>(walkingRoute_.size());
            archive(cereal::make_nvp("count", count));

            for (int i = 0; i < count; ++i)
            {
                archive(cereal::make_nvp(("item_" + std::to_string(i)).c_str(), walkingRoute_[i]));
            }
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));

            int count = 0;
            archive(cereal::make_nvp("count", count));
            walkingRoute_.resize(count);
            for (int i = 0; i < count; ++i)
            {
                archive(cereal::make_nvp(("item_" + std::to_string(i)).c_str(), walkingRoute_[i]));
            }
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(NpcWalkingRoute, FRIENDLY_NPC_WALKING_ROUTE_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::NpcWalkingRoute, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::NpcWalkingRoute);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::NpcWalkingRoute);
#pragma endregion
