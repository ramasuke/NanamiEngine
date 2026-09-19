#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** @brief 部位破壊やカウンターで立ったスタンフラグを消化し、ダウンから起き上がるまでを進める */
    class Stun final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] std::string stunStateKeyName_ = "StunState";
        [[serialize(0)]] int downAnimationNumber_ = 0;
        [[serialize(0)]] int getUpAnimationNumber_ = 0;
        [[serialize(0)]] float downDuration_secs_ = 3.0f;
        [[serialize(0)]] float getUpDuration_secs_ = 1.2f;

        // WaitSeconds は一度 Success になると待たなくなるので、PhysicsAttack と同じく自前で持つ
        float during_secs_ = 0.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(stunStateKeyName_));
            archive(CEREAL_NVP(downAnimationNumber_));
            archive(CEREAL_NVP(getUpAnimationNumber_));
            archive(CEREAL_NVP(downDuration_secs_));
            archive(CEREAL_NVP(getUpDuration_secs_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(stunStateKeyName_));
            if (version >= 0) archive(CEREAL_NVP(downAnimationNumber_));
            if (version >= 0) archive(CEREAL_NVP(getUpAnimationNumber_));
            if (version >= 0) archive(CEREAL_NVP(downDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(getUpDuration_secs_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(Stun, "EnemyStatus::Stun")
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::Stun)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::Stun)
