#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ChangeToMainIslandScene final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        
        [[serialize(0)]] FIELD(Asset::SceneFile) sceneFile_;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(sceneFile_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(sceneFile_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION_WITH_NAME(ChangeToMainIslandScene, "Scene::ChangeToMainIslandScene")
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeToMainIslandScene)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ChangeToMainIslandScene)
