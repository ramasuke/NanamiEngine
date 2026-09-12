#pragma once
#include "vec2.hpp"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/Scene/Main/Type/MainSceneType.h"
#include "../../../Libs/LibCore/cereal/glm/GlmHelper.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto STAGE_DATA_EXTENSION_LABEL = ".stageData";

    class StageData final : public ScriptableObject
    {
    public:
        explicit StageData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&              DisplayName      () const { return displayName_;       }
        [[nodiscard]] GameCore::Scene::Main::SceneType SceneType        () const { return sceneType_;         }
        [[nodiscard]] const glm::vec2&                 MapMarkerPosition() const { return mapMarkerPosition_; }
        [[nodiscard]] bool                             IsCleared        () const { return isCleared_;        }

    private:
        [[serialize(0)]] std::string                              displayName_;
        [[serialize(0)]] GameCore::Scene::Main::SceneType          sceneType_ = GameCore::Scene::Main::SceneType::GrassLand;
        [[serialize(1)]] glm::vec2                                mapMarkerPosition_ = glm::vec2(960.0f, 540.0f);
        [[serialize(1)]] bool                                     isCleared_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(displayName_));
            archive(CEREAL_NVP(sceneType_));
            archive(CEREAL_NVP(mapMarkerPosition_));
            archive(CEREAL_NVP(isCleared_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(displayName_));
            if (version >= 0) archive(CEREAL_NVP(sceneType_));
            if (version >= 1) archive(CEREAL_NVP(mapMarkerPosition_));
            if (version >= 1) archive(CEREAL_NVP(isCleared_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(StageData, STAGE_DATA_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::StageData, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::StageData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::StageData);
#pragma endregion
