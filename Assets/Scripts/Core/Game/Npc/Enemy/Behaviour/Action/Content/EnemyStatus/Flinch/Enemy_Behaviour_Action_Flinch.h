#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "../../../../../../../Damage/Flinch/Game_Damage_FlinchResistance.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * @brief 怯み耐性を超える怯み値の攻撃を受けたら flinch_secs_ の間 Running を返し続ける。
     *        Selector で攻撃・移動の枝より前に置くと、その間は下位の枝(実行中の攻撃も)が止まる
     */
    class Flinch final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoReset() override;
        void       DoDrawGui() override;

        [[serialize(0)]] Damage::FlinchResistance flinchResistance_;
        [[serialize(0)]] int animatorSetParam_ = 0;
        [[serialize(0)]] float flinch_secs_ = 0.6f;
        // OnDamage のノックバックを使う敵は false にする(同じ Tick で速度を上書きしてしまう)
        [[serialize(0)]] bool isStopHorizontalMove_ = true;

        bool  isFlinching_ = false;
        float during_secs_ = 0.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(flinchResistance_));
            archive(CEREAL_NVP(animatorSetParam_));
            archive(CEREAL_NVP(flinch_secs_));
            archive(CEREAL_NVP(isStopHorizontalMove_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(flinchResistance_));
            if (version >= 0) archive(CEREAL_NVP(animatorSetParam_));
            if (version >= 0) archive(CEREAL_NVP(flinch_secs_));
            if (version >= 0) archive(CEREAL_NVP(isStopHorizontalMove_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(Flinch, "EnemyStatus::Flinch")
}
