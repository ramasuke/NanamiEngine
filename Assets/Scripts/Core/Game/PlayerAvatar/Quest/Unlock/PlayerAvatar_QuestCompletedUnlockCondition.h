#pragma once
#include "PlayerAvatar_IQuestUnlockCondition.h"
#include "cereal/types/polymorphic.hpp"
#include "../PlayerAvatar_QuestType.h"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    /** @brief questType_ を達成済みなら解放 */
    class QuestCompletedUnlockCondition final : public IQuestUnlockCondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const QuestUnlockContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] PlayerAvatar::QuestType questType_ = PlayerAvatar::QuestType::GrasslandHyenaCull;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            archive(CEREAL_NVP(questType_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            if (version >= 0) archive(CEREAL_NVP(questType_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Unlock::QuestCompletedUnlockCondition, 0);
#pragma endregion
