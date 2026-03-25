#pragma once
#include "../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../SwordMan_ITakeableSwordManQuest.h"
#include "../../SwordMan_QuestFactory.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Quest/PlayerAvatar_QuestType.h"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class WriteBlackBoard;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class ActionInstructTutorialPresenter;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class ActionInstructTutorial final : public Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest
    {
    public:
        ActionInstructTutorial();
        ~ActionInstructTutorial() override;

    private:
        void StartQuest(
            const IObservableStatusEvent& event,
            PlayerAvatar::Quest::CompletedQuestGroup& completedQuestGroup) override;
        [[nodiscard]] const PlayerAvatar::QuestType& QuestType() const override { return QuestType::SwordManActionInstructTutorial; }

        Coroutine::Task<void> StartQuestAsync(PlayerAvatar::Quest::CompletedQuestGroup& completedQuestGroup);

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) questUiPrefab_;
        std::unique_ptr<ActionInstructTutorialPresenter> presenter_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ITakeableSwordManQuest>(this));
            archive(CEREAL_NVP(questUiPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ITakeableSwordManQuest>(this));
            archive(CEREAL_NVP(questUiPrefab_));
        }
#pragma endregion
    };

    REGISTER_SWORDMAN_QUEST(ActionInstructTutorial)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::Quest::ActionInstructTutorial, 0);
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::Quest::ActionInstructTutorial);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest, GameCore::PlayerAvatar::SwordMan::Quest::ActionInstructTutorial);
#pragma endregion