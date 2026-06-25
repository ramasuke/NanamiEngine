#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** プレイヤーとの距離を計測し、distance_ 以内なら Success、超えたら Failure を返すアクション。 */
    class ToPlayerDistance final : public ActionBase
    {
    public:
        ToPlayerDistance()          = default;
        ~ToPlayerDistance() override = default;

    private:
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] float distance_ = 5.0f;
        [[serialize(1)]] bool isInnerDistance_ = true;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(distance_));
            archive(CEREAL_NVP(isInnerDistance_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(distance_));
            if (version >= 1) archive(CEREAL_NVP(isInnerDistance_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ToPlayerDistance, "Basic::ToPlayerDistance")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ToPlayerDistance, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ToPlayerDistance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::ToPlayerDistance
)
