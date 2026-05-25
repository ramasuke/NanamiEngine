#pragma once
#include "../../../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "../../../Context/Sub_SceneContextBase.h"

namespace GameCore::Scene::Sub
{
    class ChattingUISceneContext final : public SceneContextBase
    {
    public:
        [[nodiscard]] const GamePlay::Ui::NpcChatting& Npc() const { return *npcChatting_.get(); }
        
    private:
        void DoInitialize() override;
        
        [[serialize(1)]] FIELD(GamePlay::Ui::NpcChatting) npcChatting_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<SceneContextBase>(this));
            archive(CEREAL_NVP(npcChatting_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 1) archive(cereal::base_class<SceneContextBase>(this));
            archive(CEREAL_NVP(npcChatting_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::Sub::ChattingUISceneContext, 1);
CEREAL_REGISTER_TYPE(GameCore::Scene::Sub::ChattingUISceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::Sub::SceneContextBase, GameCore::Scene::Sub::ChattingUISceneContext);
#pragma endregion
