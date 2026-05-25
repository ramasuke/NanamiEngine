#pragma once
#include "../SwordMan/Status/Quest/SwordMan_ITakeableSwordManQuest.h"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

namespace GameCore::PlayerAvatar
{
    class IStatusEvent;
}

namespace GameCore::PlayerAvatar
{
    class QuestBase : public Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest
    {
    public:
        explicit QuestBase();
        virtual ~QuestBase() override;
        void StartQuest(
            const SwordMan::IObservableStatusEvent& event,
            Quest::ICompleteQuestGroup& completedQuestGroup) override;

    protected:
        //templateMethodパターン
        virtual void DoStartQuest(const SwordMan::IObservableStatusEvent& event) = 0;
        virtual void DoDrawGui() = 0;

        //サンドボックスパターン
        
        
    private:
        void OnDrawGui() override;
        
#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<QuestBase>(this));
        }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(cereal::base_class<QuestBase>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::QuestBase, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::QuestBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest, GameCore::PlayerAvatar::QuestBase);
#pragma endregion