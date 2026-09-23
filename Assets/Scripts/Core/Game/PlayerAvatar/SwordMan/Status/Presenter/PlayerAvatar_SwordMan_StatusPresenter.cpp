#include "PlayerAvatar_SwordMan_StatusPresenter.h"

#include "../SwordManAvatarStatus.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../GamePlay/Ui/ControlGuide/Ui_SwordManControlGuide.h"
#include "../../../../../../GamePlay/Ui/ItemBar/Ui_ItemBar.h"
#include "../../../../../../GamePlay/Ui/ItemBar/Ui_ItemBarSource.h"
#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../GamePlay/Ui/PauseMenu/Presenter/PauseMenuPresenter.h"
#include "../../../../../../GamePlay/Ui/PlayerStatus/Ui_LowHealthScreenEffect.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    void StatusPresenter::Initialize(
        const Ui::PlayerStatus& playerStatusView,
        const GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus& playerStatusModel,
        const std::weak_ptr<SwordManAvatar>& swordManAvatar)
    {
        StatusPresenterBase::Initialize(
            playerStatusView,
            playerStatusModel);

        if (lowHealthScreenEffect_)
            lowHealthScreenEffect_->Initialize(playerStatusModel);

        if (itemBarPrefab_)
        {
            if (const auto itemBarObject = Scene::GameObject::Instantiate(*itemBarPrefab_.get(), Entity().lock()).lock())
            {
                if (const auto itemBar = itemBarObject->Components().Catch<Ui::ItemBar>().lock())
                    itemBar->Initialize(std::make_shared<Ui::AvatarItemBarSource<SwordManAvatar, GameCore::PlayerAvatar::SwordMan::ISwordManAvatarTransitionVisitor>>(swordManAvatar));
            }
        }

        if (pauseMenuPrefab_)
        {
            if (const auto pauseMenuObject = Scene::GameObject::Instantiate(*pauseMenuPrefab_.get(), Entity().lock()).lock())
            {
                if (const auto pauseMenu = pauseMenuObject->Components().Catch<Ui::PauseMenuPresenter>().lock())
                    pauseMenu->Initialize(swordManAvatar);
            }
        }

        if (!controlGuidePrefab_)
            return;
        if (const auto controlGuideObject = Scene::GameObject::Instantiate(*controlGuidePrefab_.get(), Entity().lock()).lock())
        {
            if (const auto controlGuide = controlGuideObject->Components().Catch<Ui::SwordManControlGuide>().lock())
                controlGuide->Initialize(swordManAvatar);
        }
    }

    void StatusPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("lowHealthScreenEffect_", lowHealthScreenEffect_);
        ImGuiHelper::OnDrawInputField("controlGuidePrefab_", controlGuidePrefab_);
        ImGuiHelper::OnDrawInputField("itemBarPrefab_", itemBarPrefab_);
        ImGuiHelper::OnDrawInputField("pauseMenuPrefab_", pauseMenuPrefab_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::SwordMan::StatusPresenter);
#pragma endregion
