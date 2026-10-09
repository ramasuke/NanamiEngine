#pragma once
#include "Reward_IReward.h"
#include "../StatusParameter/Money/Money.h"

namespace GameCore::Reward
{
    // NOTE: amount_ を所持金に足す
    class MoneyReward final : public IReward
    {
    public:
        MoneyReward() = default;
        explicit MoneyReward(StatusParameter::Money amount);

        // NOTE: 報酬がお金だけだったころのデータ(rewardMoney_)を読み替える。0 以下なら空
        [[nodiscard]] static Rewards FromLegacy(StatusParameter::Money amount);

        void Grant(const RewardContext& context) const override;
        [[nodiscard]] std::string DisplayText() const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

        [[nodiscard]] StatusParameter::Money Amount() const { return amount_; }

    private:
        [[serialize(0)]] StatusParameter::Money amount_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IReward>(this));
            archive(CEREAL_NVP(amount_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IReward>(this));
            if (version >= 0) archive(CEREAL_NVP(amount_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Reward::MoneyReward, 0);
#pragma endregion
