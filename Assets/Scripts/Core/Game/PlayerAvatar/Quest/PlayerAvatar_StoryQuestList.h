#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "PlayerAvatar_StoryQuestBase.h"
#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar::Quest
{
    /**
     * @brief 受注中の StoryQuest の入れ物。剣士と魔術師の QuestGroup が同じものを持つ
     */
    class StoryQuestList final
    {
    public:
        void StartAll(const QuestContext& context) const;
        void Add(const std::shared_ptr<StoryQuestBase>& quest, const QuestContext& context);
        void Remove(const QuestType& type);

        [[nodiscard]] bool Contains(const QuestType& type) const;
        /** @return 受注していなければ nullopt */
        [[nodiscard]] std::optional<StatusParameter::Money> RewardOf(const QuestType& type) const;

        void OnDrawGui() const;

    private:
        [[serialize(0)]] std::vector<std::shared_ptr<StoryQuestBase>> quests_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            std::size_t count = quests_.size();
            archive(CEREAL_NVP(count));

            for (const auto& quest : quests_)
            {
                archive(quest);
            }
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            std::size_t count = 0;
            archive(CEREAL_NVP(count));

            quests_.clear();
            quests_.reserve(count);

            for (std::size_t i = 0; i < count; ++i)
            {
                std::shared_ptr<StoryQuestBase> quest;
                archive(quest);
                quests_.emplace_back(std::move(quest));
            }
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::StoryQuestList, 0)
