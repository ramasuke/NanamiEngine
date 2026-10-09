#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

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
        // NOTE: true なら達成のたびに報酬を出し、達成済みとして残さない(何度でも受けられる)
        [[nodiscard]] virtual bool IsRepeatable() const { return false; }
        // NOTE: 達成済みを QuestType で残すか。false なら掲示板の BoardQuest の guid で残す(QuestType を週ごとに使い回す依頼)
        [[nodiscard]] virtual bool RecordsCompletionByType() const { return true; }

        [[nodiscard]] std::shared_ptr<ITakeableQuest> Clone() const;

        // NOTE: 掲示板から受けたときの BoardQuest の guid。NPC から受けた依頼と古いセーブは空
        [[nodiscard]] const std::string& BoardQuestGuid() const { return boardQuestGuid_; }
        void SetBoardQuestGuid(std::string guid) { boardQuestGuid_ = std::move(guid); }
        // NOTE: 両方に guid があれば guid で、どちらかが空なら QuestType で同じ依頼かを見る
        [[nodiscard]] bool IsSameQuest(const ITakeableQuest& other) const;

        // NOTE: 達成時にプレイヤーへ渡すもの
        [[nodiscard]] const Reward::Rewards& Rewards() const { return rewards_; }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(rewards_));
            archive(CEREAL_NVP(boardQuestGuid_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            // NOTE: 旧版はお金だけの報酬。MoneyReward に読み替える
            if (version < 1)
            {
                StatusParameter::Money rewardMoney;
                archive(cereal::make_nvp("rewardMoney_", rewardMoney));
                rewards_ = Reward::MoneyReward::FromLegacy(rewardMoney);
                return;
            }
            archive(CEREAL_NVP(rewards_));
            if (version >= 2) archive(CEREAL_NVP(boardQuestGuid_));
        }

    protected:
        void DrawRewardGui() { Reward::RewardList::DrawListGui("rewards_", rewards_); }

    private:
        [[serialize(1)]] Reward::Rewards rewards_;
        [[serialize(2)]] std::string     boardQuestGuid_;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::ITakeableQuest, 2)
