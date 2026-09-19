#include "PlayerAvatar_MagicCaster_StatusPresenter.h"

#include "../MagicCasterAvatarStatus.h"
#include "../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../GamePlay/Ui/ControlGuide/Ui_MagicCasterControlGuide.h"
#include "../../../../../../GamePlay/Ui/SpellPalette/Ui_SpellPalette.h"

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
    }
}
