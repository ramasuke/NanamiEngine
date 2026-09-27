#pragma once
#include "../../../Context/Sub_SceneContextBase.h"

namespace GameCore::Scene::Sub
{
    class StageReturnUiSceneContext final : public SceneContextBase
    {
    private:
        void DoInitialize() override;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<SceneContextBase>(this));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 1) archive(cereal::base_class<SceneContextBase>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::Sub::StageReturnUiSceneContext, 1);
#pragma endregion
