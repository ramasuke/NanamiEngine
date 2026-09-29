#pragma once
#include "Reward_IReward.h"
#include "Engine/Core/Object/Field/Field.h"
#include "../../../../Data/Decoration/Data_DecorationData.h"

namespace GameCore::Reward
{
    /** @brief 島の飾り decoration_ を手に入れる。もう持っていれば何も起きない */
    class DecorationReward final : public IReward
    {
    public:
        void Grant(const RewardContext& context) const override;
        [[nodiscard]] std::string DisplayText() const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::DecorationData) decoration_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IReward>(this));
            archive(CEREAL_NVP(decoration_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IReward>(this));
            if (version >= 0) archive(CEREAL_NVP(decoration_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Reward::DecorationReward, 0);
#pragma endregion
