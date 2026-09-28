#pragma once
#include <string>
#include <vector>

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * @brief 一番近いプレイヤーへの水平角度で、最初に当てはまった範囲の値をブラックボードに書く
     *
     * Selector[Seq[ToPlayerAngle, WriteBlackBoard]..., WriteBlackBoard] の代わり。角度の向きは ToPlayerAngle と同じ。
     * どの範囲にも入らなければ、useFallback_ なら fallbackValue_ を書いて Success、そうでなければ Failure。
     */
    class PlayerAngleDispatch final : public ActionBase
    {
    public:
        struct Range
        {
            float minDegree_ = -35.0f;
            float maxDegree_ = 35.0f;
            bool useAbsolute_ = false;
            int value_ = 0;

            void OnDrawGui();

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(CEREAL_NVP(minDegree_));
                archive(CEREAL_NVP(maxDegree_));
                archive(CEREAL_NVP(useAbsolute_));
                archive(CEREAL_NVP(value_));
            }
        };

    private:
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] std::string keyName_;
        [[serialize(0)]] std::vector<Range> ranges_;
        [[serialize(0)]] bool useFallback_ = true;
        [[serialize(0)]] int fallbackValue_ = 0;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(keyName_));
            archive(CEREAL_NVP(ranges_));
            archive(CEREAL_NVP(useFallback_));
            archive(CEREAL_NVP(fallbackValue_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(keyName_));
            if (version >= 0) archive(CEREAL_NVP(ranges_));
            if (version >= 0) archive(CEREAL_NVP(useFallback_));
            if (version >= 0) archive(CEREAL_NVP(fallbackValue_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(PlayerAngleDispatch, "Basic::PlayerAngleDispatch")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::PlayerAngleDispatch, 0)
