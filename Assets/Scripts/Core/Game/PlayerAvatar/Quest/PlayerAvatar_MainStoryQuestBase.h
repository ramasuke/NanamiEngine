#pragma once
#include "PlayerAvatar_ITakeableQuest.h"
#include "PlayerAvatar_QuestContext.h"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar
{
    /**
     * @brief メインストーリーのクエストの土台。職業を問わず、すべての PlayerAvatarType が受けられる。
     * 報酬が出るのは初回の達成だけで、達成済みとして残る
     */
    class MainStoryQuestBase : public Quest::ITakeableQuest
    {
    public:
        explicit MainStoryQuestBase();
        virtual ~MainStoryQuestBase() override;
        void StartQuest(const Quest::QuestContext& context) override;
        void OnDrawGui() override;

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
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::MainStoryQuestBase, 0);
#pragma endregion
