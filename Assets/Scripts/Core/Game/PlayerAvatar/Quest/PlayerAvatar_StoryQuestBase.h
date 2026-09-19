#pragma once
#include <memory>

#include "PlayerAvatar_ITakeableQuest.h"
#include "PlayerAvatar_QuestContext.h"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar
{
    /**
     * @brief 職業を問わない基本クエストの土台。剣士も魔術師も受けられる
     */
    class StoryQuestBase : public Quest::ITakeableQuest
    {
    public:
        explicit StoryQuestBase();
        virtual ~StoryQuestBase() override;
        void StartQuest(const Quest::QuestContext& context) override;
        void OnDrawGui() override;

        /** @brief 受注のたびに別の実体を渡すための複製。中身の型ごと写す */
        [[nodiscard]] static std::shared_ptr<StoryQuestBase> Clone(const std::shared_ptr<StoryQuestBase>& source);

    protected:
        //templateMethodパターン
        virtual void DoStartQuest(const Quest::QuestContext& context) = 0;
        virtual void DoDrawGui() = 0;

#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Quest::ITakeableQuest>(this));
        }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(cereal::base_class<Quest::ITakeableQuest>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::StoryQuestBase, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::StoryQuestBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Quest::ITakeableQuest, GameCore::PlayerAvatar::StoryQuestBase);
#pragma endregion
