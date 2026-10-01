#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * @brief 今の BGM をフェードアウトして止め、bgm_ があればフェードインで流す (空なら無音にするだけ)
     * NOTE: 毎 Tick 呼ぶとやり直すので OnceExecute か ActionTimeline の Cue に置く
     */
    class FadeBGM final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] FIELD(Asset::SoundFile) bgm_;
        [[serialize(0)]] float fadeOut_secs_ = 2.0f;
        [[serialize(0)]] float fadeIn_secs_ = 0.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(bgm_));
            archive(CEREAL_NVP(fadeOut_secs_));
            archive(CEREAL_NVP(fadeIn_secs_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(bgm_));
            if (version >= 0) archive(CEREAL_NVP(fadeOut_secs_));
            if (version >= 0) archive(CEREAL_NVP(fadeIn_secs_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(FadeBGM, "Sound::FadeBGM")
}
