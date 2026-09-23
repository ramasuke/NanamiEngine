#include "PlayerAvatar_MagicCaster_StatusPresenter.h"

#include "../MagicCasterAvatarStatus.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../GamePlay/PlayerAvatar/MagicCaster/MagicCasterAvatar.h"
#include "../../../../../../GamePlay/Ui/ControlGuide/Ui_MagicCasterControlGuide.h"
#include "../../../../../../GamePlay/Ui/ItemBar/Ui_ItemBar.h"
#include "../../../../../../GamePlay/Ui/ItemBar/Ui_ItemBarSource.h"
#include "../../../../../../GamePlay/Ui/SpellPalette/Ui_SpellPalette.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    void StatusPresenter::Initialize(
        const Ui::PlayerStatus& playerStatusView,
        const GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus& playerStatusModel,
        const std::weak_ptr<MagicCasterAvatar>& magicCasterAvatar)
    {
        StatusPresenterBase::Initialize(
            playerStatusView,
            playerStatusModel);

        if (spellPalettePrefab_)
        {
            if (const auto spellPaletteObject = Scene::GameObject::Instantiate(*spellPalettePrefab_.get(), Entity().lock()).lock())
            {
                if (const auto spellPalette = spellPaletteObject->Components().Catch<Ui::SpellPalette>().lock())
                    spellPalette->Initialize(magicCasterAvatar);
            }
        }

        if (itemBarPrefab_)
        {
            if (const auto itemBarObject = Scene::GameObject::Instantiate(*itemBarPrefab_.get(), Entity().lock()).lock())
            {
                if (const auto itemBar = itemBarObject->Components().Catch<Ui::ItemBar>().lock())
                    itemBar->Initialize(std::make_shared<Ui::AvatarItemBarSource<MagicCasterAvatar, GameCore::PlayerAvatar::MagicCaster::IMagicCasterAvatarTransitionVisitor>>(magicCasterAvatar));
            }
        }

        if (!controlGuidePrefab_)
            return;
        if (const auto controlGuideObject = Scene::GameObject::Instantiate(*controlGuidePrefab_.get(), Entity().lock()).lock())
        {
            if (const auto controlGuide = controlGuideObject->Components().Catch<Ui::MagicCasterControlGuide>().lock())
                controlGuide->Initialize(magicCasterAvatar);
        }
    }

    void StatusPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("spellPalettePrefab_", spellPalettePrefab_);
        ImGuiHelper::OnDrawInputField("controlGuidePrefab_", controlGuidePrefab_);
        ImGuiHelper::OnDrawInputField("itemBarPrefab_", itemBarPrefab_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::MagicCaster::StatusPresenter);
#pragma endregion
