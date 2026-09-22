#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "PlayerAvatar_ITakeableQuest.h"
#include "PlayerAvatar_QuestContext.h"
#include "cereal/cereal.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar::Quest
{
    /**
     * @brief 受注中の職業を問わないクエスト(メインストーリー・依頼)の入れ物。剣士と魔術師の QuestGroup が同じものを持つ
     */
    class QuestList final
    {
    public:
        void StartAll(const QuestContext& context) const;
        void Add(const std::shared_ptr<ITakeableQuest>& quest, const QuestContext& context);
        void Remove(const QuestType& type);

        [[nodiscard]] bool Contains(const QuestType& type) const;
        /** @return 受注していなければ nullptr */
        [[nodiscard]] const ITakeableQuest* Find(const QuestType& type) const;

        void OnDrawGui() const;

    private:
        [[serialize(0)]] std::vector<std::shared_ptr<ITakeableQuest>> quests_;

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
                std::shared_ptr<ITakeableQuest> quest;
                archive(quest);
                quests_.emplace_back(std::move(quest));
            }
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::QuestList, 0)
