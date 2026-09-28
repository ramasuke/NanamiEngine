#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../Sound/PlaySE/Enemy_Behaviour_Action_PlaySE.h"
#include "../../Wait/Seconds/Enemy_Behaviour_Action_WaitSeconds.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class PlayAnimation final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoReset() override;
        void DoDrawGui() override;
        
        [[serialize(0)]] int animatorSetParamNumber_ = 0;
        [[serialize(2)]] WaitSeconds waitAnimationSound_secs_;
        [[serialize(2)]] PlaySE animationSound_;
        /** @brief 0 なら即 Success。>0 ならこの秒数 Running を返す（後ろに WaitSeconds を置く代わり） */
        [[serialize(3)]] float holdSeconds_ = 0.0f;
        bool isSoundPending_ = true;
        float hold_secs_ = 0.0f;
        std::uint64_t lastTickIndex_ = 0;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(animatorSetParamNumber_));
            archive(CEREAL_NVP(waitAnimationSound_secs_));
            archive(CEREAL_NVP(animationSound_));
            archive(CEREAL_NVP(holdSeconds_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(animatorSetParamNumber_));
            if (version >= 2) archive(CEREAL_NVP(waitAnimationSound_secs_));
            if (version >= 2) archive(CEREAL_NVP(animationSound_));
            if (version >= 3) archive(CEREAL_NVP(holdSeconds_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(PlayAnimation, "EnemyStatus::PlayAnimation")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::PlayAnimation, 3)
