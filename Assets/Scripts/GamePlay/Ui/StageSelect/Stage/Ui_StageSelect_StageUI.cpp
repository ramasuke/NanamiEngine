#include "Ui_StageSelect_StageUI.h"

#include "../../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace GamePlay::Ui
{
    void StageSelectStageUi::OnAwake()
    {
        selectButton_ = GameObject::CatchChild<NanamiUi::Button>(Entity(), selectButtonName_);
        
        // selectButton_->OnClick().subscribe([]
        // {
        //     GameCore::Game::Instance().Scenes().RequestChangeScene<>()
        // });
    }
}
