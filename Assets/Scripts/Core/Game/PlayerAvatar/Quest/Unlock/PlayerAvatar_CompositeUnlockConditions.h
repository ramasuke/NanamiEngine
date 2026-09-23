#pragma once
#include "PlayerAvatar_IQuestUnlockCondition.h"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    /** @brief conditions_ のどれか1つを満たせば解放。空なら解放しない */
    class AnyOfUnlockCondition final : public IQuestUnlockCondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const QuestUnlockContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] QuestUnlockConditions conditions_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            archive(CEREAL_NVP(conditions_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            if (version >= 0) archive(CEREAL_NVP(conditions_));
        }
#pragma endregion
    };

    /** @brief condition_ を満たしていなければ解放。空なら解放 */
    class NotUnlockCondition final : public IQuestUnlockCondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const QuestUnlockContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] std::shared_ptr<IQuestUnlockCondition> condition_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            archive(CEREAL_NVP(condition_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            if (version >= 0) archive(CEREAL_NVP(condition_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Unlock::AnyOfUnlockCondition, 0);
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Unlock::NotUnlockCondition, 0);
#pragma endregion
