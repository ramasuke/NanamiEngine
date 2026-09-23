#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../../Data/Enemy/Factory/EnemyFactory.h"
#include "../../../../../../Enemy/Type/EnemyKind.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class SampleFirstEventDragonChat final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void AppearFirstEventDragon(const TickContext& context);
        
        FIELD(Asset::EnemyFactory)         enemyFactory_;
        Enemy::EnemyKind                   enemyKind_ = Enemy::EnemyKind::NormalBoss;
        glm::vec3                          appearFirstEventDragonPosition_;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(appearFirstEventDragonPosition_));
            archive(CEREAL_NVP(enemyFactory_));
            archive(CEREAL_NVP(enemyKind_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            // v0 はドラゴンのプレハブを直接持っていた。今は EnemyFactory 側にあるので読み捨てる
            [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) firstEventDragonPrefab_;
            if (version == 0) archive(CEREAL_NVP(firstEventDragonPrefab_));
            if (version >= 0) archive(CEREAL_NVP(appearFirstEventDragonPosition_));
            if (version >= 1) archive(CEREAL_NVP(enemyFactory_));
            if (version >= 1) archive(CEREAL_NVP(enemyKind_));
        }
#pragma endregion
    };
    REGISTER_FRIENDLY_ACTION_WITH_NAME(SampleFirstEventDragonChat, "Other::SummonFirstEventDragon")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::SampleFirstEventDragonChat, 1)
