#pragma once
#include <cstdint>
#include <memory>

#include "cereal/cereal.hpp"
#include "../../Reward/Reward_IReward.h"
#include "../../Reward/Reward_MoneyReward.h"
#include "../../StatusParameter/Money/Money.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    struct QuestContext;
    
    class ITakeableQuest
    {
    public:
        virtual ~ITakeableQuest() = default;
        virtual void StartQuest(const QuestContext& context) = 0;
        virtual void OnDrawGui() = 0;
        [[nodiscard]] virtual const PlayerAvatar::QuestType& QuestType() const = 0;
        /** @brief true なら達成のたびに報酬を出し、達成済みとして残さない(何度でも受けられる) */
        [[nodiscard]] virtual bool IsRepeatable() const { return false; }

        [[nodiscard]] std::shared_ptr<ITakeableQuest> Clone() const;

        /** @brief 達成時にプレイヤーへ渡すもの */
        [[nodiscard]] const Reward::Rewards& Rewards() const { return rewards_; }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(rewards_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            // NOTE: version 0 は報酬がお金だけだった。受注中のセーブと .boardQuest.meta を読めるよう MoneyReward に読み替える
            if (version < 1)
            {
                StatusParameter::Money rewardMoney;
                archive(cereal::make_nvp("rewardMoney_", rewardMoney));
                rewards_ = Reward::MoneyReward::FromLegacy(rewardMoney);
                return;
            }
            archive(CEREAL_NVP(rewards_));
        }

    protected:
        void DrawRewardGui() { Reward::RewardList::DrawListGui("rewards_", rewards_); }

    private:
        [[serialize(1)]] Reward::Rewards rewards_;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::ITakeableQuest, 1)
