#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class WaitSeconds final : public ActionBase
    {
    public:
        TickStatus DoTick(const TickContext& context) override;
        void DoReset() override;

    private:
        [[serialize(0)]] float waitSeconds_ = 0.0f;
        float during_secs_ = 0.0f;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(waitSeconds_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(waitSeconds_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(WaitSeconds, "Wait::Seconds::WaitSeconds")
}
