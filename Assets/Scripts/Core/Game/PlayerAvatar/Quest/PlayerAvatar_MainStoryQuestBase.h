#pragma once
#include "PlayerAvatar_ITakeableQuest.h"
#include "PlayerAvatar_QuestContext.h"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar
{
    // NOTE: メインストーリーのクエストの土台。全職業が受けられ、報酬は初回達成時のみ
    class MainStoryQuestBase : public Quest::ITakeableQuest
    {
    public:
        explicit MainStoryQuestBase();
        virtual ~MainStoryQuestBase() override;
        void StartQuest(const Quest::QuestContext& context) override;
        void OnDrawGui() override;

    protected:
        // NOTE: Template Method パターン
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
