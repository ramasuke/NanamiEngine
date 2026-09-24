#pragma once
#include "../PlayerAvatar_MainStoryQuestBase.h"
#include "../PlayerAvatar_QuestType.h"
#include "../../../Npc/Enemy/Type/EnemyKind.h"
#include "../../../Story/Story_StoryFlag.h"
#include "Packages/R4/R4.h"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar::Quest::MainStory
{
    /**
     * @brief メインストーリーの討伐。受注してから enemyKind_ を1体倒したら達成。
     * 受注の前に clearedFlag_ が立っていれば(もう倒していれば)受注した時点で達成
     */
    class DefeatMainStoryQuest final : public MainStoryQuestBase
    {
    public:
        DefeatMainStoryQuest();
        ~DefeatMainStoryQuest() override;

        [[nodiscard]] const PlayerAvatar::QuestType& QuestType() const override { return questType_; }

    private:
        void DoStartQuest(const QuestContext& context) override;
        void DoDrawGui() override;
        void Complete(ICompleteQuestGroup& completedQuests);

        [[serialize(0)]] PlayerAvatar::QuestType questType_   = PlayerAvatar::QuestType::GrassLandTyrant;
        [[serialize(0)]] Npc::Enemy::EnemyKind   enemyKind_   = Npc::Enemy::EnemyKind::Tyrannosaurus;
        [[serialize(0)]] Story::StoryFlag        clearedFlag_ = Story::StoryFlag::GrassLandCleared;
        NanamiEngine::R4::Disposable             subscription_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<MainStoryQuestBase>(this));
            archive(CEREAL_NVP(questType_));
            archive(CEREAL_NVP(enemyKind_));
            archive(CEREAL_NVP(clearedFlag_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<MainStoryQuestBase>(this));
            if (version >= 0) archive(CEREAL_NVP(questType_));
            if (version >= 0) archive(CEREAL_NVP(enemyKind_));
            if (version >= 0) archive(CEREAL_NVP(clearedFlag_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::MainStory::DefeatMainStoryQuest, 0);
#pragma endregion
