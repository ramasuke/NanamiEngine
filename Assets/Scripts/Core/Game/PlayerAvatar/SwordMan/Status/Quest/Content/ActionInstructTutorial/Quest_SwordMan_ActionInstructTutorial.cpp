#include "Quest_SwordMan_ActionInstructTutorial.h"

#include "Quest_SwordMan_ActionInstructTutorialModel.h"
#include "Quest_SwordMan_ActionInstructTutorialPresenter.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../GamePlay/Ui/ActionInstructTutorial/SwordMan/Ui_SwordMan_ActionInstructTutorial.h"
#include "../../../../../../Npc/Friendly/Behaviour/Action/TickContext/Friendly_Behaviour_TickContext.h"
#include "../../../../../Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    ActionInstructTutorial:: ActionInstructTutorial()
    {
        
    }
    ActionInstructTutorial::~ActionInstructTutorial() = default;
    
    void ActionInstructTutorial::StartQuest(
        const IObservableStatusEvent& event,
        PlayerAvatar::Quest::CompletedQuestGroup& completedQuestGroup)
    {
        const auto questUi = Scene::GameObject::Instantiate(questUiPrefab_.get(), glm::vec3{0.0f, 0.0f, 0.0f});
        const auto actionInstructTutorialUi = questUi.lock()->Components().Catch<GamePlay::Ui::SwordManActionInstructTutorial>();
        
        presenter_ = std::make_unique<ActionInstructTutorialPresenter>(std::make_unique<ActionInstructTutorialModel>(event), actionInstructTutorialUi);
        
        Coroutine::StartCoroutine(StartQuestAsync(completedQuestGroup));
    }

    Coroutine::Task<void> ActionInstructTutorial::StartQuestAsync(PlayerAvatar::Quest::CompletedQuestGroup& completedQuestGroup)
    {
        co_await presenter_->SubscribeModelEventToViewAsync();

        completedQuestGroup.Subscribe(QuestType());
    }

    void ActionInstructTutorial::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("questUiPrefab_", questUiPrefab_);
    }
}
