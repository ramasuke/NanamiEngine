#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cereal/cereal.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include "Reward_RewardContext.h"
#include "../Condition/Condition_ICondition.h"

namespace GameCore::PlayerAvatar
{
    class Wallet;
}

namespace GameCore::Reward
{
    // NOTE: 依頼を達成したときに出るもの 1 つ。conditions_ を満たさなければ出さない(空なら出す)
    class IReward
    {
    public:
        virtual ~IReward() = default;
        virtual void Grant(const RewardContext& context) const = 0;
        // NOTE: 画面の報酬欄に出す文字（例: "1,500 G"）
        [[nodiscard]] virtual std::string DisplayText() const = 0;
        // NOTE: インスペクタの見出し
        [[nodiscard]] virtual std::string Describe() const = 0;
        virtual void OnDrawGui() = 0;

        [[nodiscard]] const Condition::Conditions& Conditions() const { return conditions_; }
        [[nodiscard]] bool IsGiven(const Condition::ConditionContext& context) const;

    protected:
        void DrawConditionsGui();

    private:
        [[serialize(0)]] Condition::Conditions conditions_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(conditions_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(conditions_));
        }
#pragma endregion
    };

    // NOTE: 保存データにはそのまま配列で書かれるので、クラスにせず vector のままにする
    using Rewards = std::vector<std::shared_ptr<IReward>>;

    // NOTE: Rewards に対する付与・表示と GUI
    class RewardList final
    {
    public:
        RewardList() = delete;

        // NOTE: conditions_ を満たす報酬だけ渡す
        static void GrantAll(const Rewards& rewards, const RewardContext& context);
        // NOTE: 手元のプレイヤーに渡す。条件は今の物語・達成済み・時刻・飾りで判定する
        static void GrantToLocalPlayer(const Rewards& rewards, PlayerAvatar::Wallet& wallet);

        // NOTE: 出る報酬を "1,500 G ＋ 群狼の旗" のように繋ぐ。1つも無ければ "―"
        [[nodiscard]] static std::string DisplayText(const Rewards& rewards, const Condition::ConditionContext& context);

        // NOTE: 種類を選んで足す・消す・中身を編集する GUI
        static void DrawListGui(const std::string& label, Rewards& rewards);
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Reward::IReward, 0);
#pragma endregion
