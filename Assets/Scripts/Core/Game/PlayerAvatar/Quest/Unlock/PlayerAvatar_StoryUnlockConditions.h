#pragma once
#include "PlayerAvatar_IQuestUnlockCondition.h"
#include "cereal/types/polymorphic.hpp"
#include "../../../Story/Story_Facility.h"
#include "../../../Story/Story_StoryFlag.h"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    /** @brief storyFlag_ が立っていれば解放 */
    class StoryFlagUnlockCondition final : public IQuestUnlockCondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const QuestUnlockContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] Story::StoryFlag storyFlag_ = Story::StoryFlag::PrologueCleared;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            archive(CEREAL_NVP(storyFlag_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            if (version >= 0) archive(CEREAL_NVP(storyFlag_));
        }
#pragma endregion
    };

    /** @brief facility_ が直っていれば解放 */
    class FacilityRestoredUnlockCondition final : public IQuestUnlockCondition
    {
    public:
        [[nodiscard]] bool IsSatisfied(const QuestUnlockContext& context) const override;
        [[nodiscard]] std::string Describe() const override;
        void OnDrawGui() override;

    private:
        [[serialize(0)]] Story::Facility facility_ = Story::Facility::Dock;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            archive(CEREAL_NVP(facility_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IQuestUnlockCondition>(this));
            if (version >= 0) archive(CEREAL_NVP(facility_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Unlock::StoryFlagUnlockCondition, 0);
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Unlock::FacilityRestoredUnlockCondition, 0);
#pragma endregion
