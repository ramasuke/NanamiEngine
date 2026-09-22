#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../../Data/Enemy/Factory/EnemyFactory.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class CallAllies final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[nodiscard]] bool RollHowl() const;
        void BeginHowl(const TickContext& context) const;
        void AlertNearbyAllies(const TickContext& context) const;

        [[serialize(0)]] int         successRate_            = 50;
        [[serialize(0)]] float       callRadius_             = 30.0f;
        [[serialize(0)]] int         animatorSetParamNumber_ = 15;
        [[serialize(0)]] float       howlSeconds_            = 1.6f;
        [[serialize(0)]] std::string alertKeyName_           = "Alert";
        [[serialize(0)]] int         alertValue_             = 1;
        [[serialize(0)]] float       reDecideAfterSeconds_   = 3.0f;
        [[serialize(0)]] FIELD(Asset::EnemyFactory) enemyFactory_;
        [[serialize(0)]] FIELD(Asset::SoundFile)    howlSound_;

        //NOTE: Tickが途切れた = 一度見失ったとみなして抽選し直すための、前回Tickの時刻
        float lastTickedTime_secs_ = -1.0f;
        bool  isHowling_           = false;
        float howlElapsed_secs_    = 0.0f;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(successRate_));
            archive(CEREAL_NVP(callRadius_));
            archive(CEREAL_NVP(animatorSetParamNumber_));
            archive(CEREAL_NVP(howlSeconds_));
            archive(CEREAL_NVP(alertKeyName_));
            archive(CEREAL_NVP(alertValue_));
            archive(CEREAL_NVP(reDecideAfterSeconds_));
            archive(CEREAL_NVP(enemyFactory_));
            archive(CEREAL_NVP(howlSound_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(successRate_));
            if (version >= 0) archive(CEREAL_NVP(callRadius_));
            if (version >= 0) archive(CEREAL_NVP(animatorSetParamNumber_));
            if (version >= 0) archive(CEREAL_NVP(howlSeconds_));
            if (version >= 0) archive(CEREAL_NVP(alertKeyName_));
            if (version >= 0) archive(CEREAL_NVP(alertValue_));
            if (version >= 0) archive(CEREAL_NVP(reDecideAfterSeconds_));
            if (version >= 0) archive(CEREAL_NVP(enemyFactory_));
            if (version >= 0) archive(CEREAL_NVP(howlSound_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(CallAllies, "Hyena::CallAllies")
}
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::CallAllies)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::CallAllies)
