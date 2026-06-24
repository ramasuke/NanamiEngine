#pragma once
#include "../../../../../../../GamePlay/Ui/OtherPlayerStatusUIGroup/OtherPlayerStatusUiGroup.h"
#include "../../../Context/Sub_SceneContextBase.h"

namespace GameCore::Scene::Sub
{
    class OtherPlayerStatusUiSceneContext final : public SceneContextBase
    {
    public:
        [[nodiscard]] GamePlay::Ui::OtherPlayerStatusUiGroup& Ui() const { return *ui_.get(); }
        
    private:
        void DoInitialize() override;
        
        [[serialize(0)]] FIELD(GamePlay::Ui::OtherPlayerStatusUiGroup) ui_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<SceneContextBase>(this));
            archive(CEREAL_NVP(ui_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 1) archive(cereal::base_class<SceneContextBase>(this));
            archive(CEREAL_NVP(ui_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext, 1);
CEREAL_REGISTER_TYPE(GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::Sub::SceneContextBase, GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext);
#pragma endregion
