#include "Quest_SwordMan_ActionInstructTutorial.h"

#include "Quest_SwordMan_ActionInstructTutorialModel.h"
#include "Quest_SwordMan_ActionInstructTutorialPresenter.h"
#include "../../SwordMan_QuestContext.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../GamePlay/Ui/ActionInstructTutorial/SwordMan/Ui_SwordMan_ActionInstructTutorial.h"
#include "../../../../../../Npc/Friendly/Behaviour/Action/TickContext/Friendly_Behaviour_TickContext.h"
#include "../../../../../Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "../../../../../Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    ActionInstructTutorial:: ActionInstructTutorial()
    {
        
    }
    ActionInstructTutorial::~ActionInstructTutorial() = default;
    
    void ActionInstructTutorial::StartQuest(const Npc::Friendly::Behaviour::Action::SwordManQuestContext& context)
    {
        const auto questUi = Scene::GameObject::Instantiate(questUiPrefab_.get(), glm::vec3{0.0f, 0.0f, 0.0f});
        const auto actionInstructTutorialUi = questUi.lock()->Components().Catch<GamePlay::Ui::SwordManActionInstructTutorial>();
        if (const auto view = actionInstructTutorialUi.lock())
            view->Initialize(context.guideFocus);

        auto actionInstructTutorialModel = std::make_unique<ActionInstructTutorialModel>(context.statusEvent);
        presenter_ = std::make_unique<ActionInstructTutorialPresenter>(
            std::move(actionInstructTutorialModel), context.guideFocus, actionInstructTutorialUi);
        
        Coroutine::StartCoroutine(StartQuestAsync(context.completedQuests));
    }

    Coroutine::Task<void> ActionInstructTutorial::StartQuestAsync(PlayerAvatar::Quest::ICompleteQuestGroup& completedQuestGroup)
    {
        co_await presenter_->SubscribeModelEventToViewAsync();

        completedQuestGroup.CompleteQuest(QuestType());
    }

    void ActionInstructTutorial::OnDrawGui()
    {
        DrawRewardGui();
        ImGuiHelper::OnDrawInputField("questUiPrefab_", questUiPrefab_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::Quest::ActionInstructTutorial, GameCore::Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest);
#pragma endregion
