#pragma once
#include <memory>
#include <vector>

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * @brief 開始からの経過時間でアクションを順に動かす
     *
     * Seq[PlayAnimation, WaitSeconds, ShakeCamera, WaitSeconds, PlaySE, ...] のような演出を 1 ノードにまとめる。
     */
    class ActionTimeline final : public ActionBase
    {
    public:
        struct Cue
        {
            /** @brief 開始する時刻（タイムライン開始からの秒） */
            float at_secs_ = 0.0f;
            /** @brief true なら、このアクションが Running の間はタイムラインを終えない */
            bool waitDone_ = false;
            /**
             * @brief 終わった後もタイムラインが終わるまで毎フレーム Tick する
             * NOTE: Sequence が前の子を毎フレーム Tick し直すのと同じ動き。PlayAnimation の遅延 SE などはこれが前提
             */
            bool keepTicking_ = false;
            std::unique_ptr<ActionBase> action_;

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(CEREAL_NVP(at_secs_));
                archive(CEREAL_NVP(waitDone_));
                archive(CEREAL_NVP(keepTicking_));
                archive(CEREAL_NVP(action_));
            }
        };

        ActionTimeline() = default;
        ActionTimeline(const ActionTimeline&) = delete;
        ActionTimeline& operator=(const ActionTimeline&) = delete;

    private:
        enum class CueState
        {
            Pending,
            Running,
            Done
        };

        TickStatus DoTick(const TickContext& context) override;
        void DoReset() override;
        void DoDrawGui() override;
        void ResetCues();

        [[serialize(0)]] std::vector<Cue> cues_;
        /** @brief この秒数が経つまでは Running（Cue が先に全部終わっても待つ） */
        [[serialize(0)]] float duration_secs_ = 0.0f;
        /** @brief true なら、Cue が Failure を返した時点でタイムラインも Failure */
        [[serialize(0)]] bool failOnChildFailure_ = false;
        /** @brief OnceExecute と同じく、1 回終わった後は Success を返し続ける（RandomSelector が枝を選び直すと戻る） */
        [[serialize(0)]] bool once_ = false;

        std::vector<CueState> cueStates_;
        float elapsed_secs_ = 0.0f;
        bool completed_ = false;
        std::uint64_t lastTickIndex_ = 0;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(cues_));
            archive(CEREAL_NVP(duration_secs_));
            archive(CEREAL_NVP(failOnChildFailure_));
            archive(CEREAL_NVP(once_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(cues_));
            if (version >= 0) archive(CEREAL_NVP(duration_secs_));
            if (version >= 0) archive(CEREAL_NVP(failOnChildFailure_));
            if (version >= 0) archive(CEREAL_NVP(once_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ActionTimeline, "Timeline::ActionTimeline")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ActionTimeline, 0)
