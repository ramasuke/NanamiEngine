#include "PlayerAvatar_SwordMan_StatusPresenter.h"

#include "../SwordManAvatarStatus.h"
#include "../../../../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
#include "../../../../../../GamePlay/Ui/PlayerStatus/Ui_LowHealthScreenEffect.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    void StatusPresenter::Initialize(
        const Ui::PlayerStatus& playerStatusView,
        const GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus& playerStatusModel)
    {
        StatusPresenterBase::Initialize(
            playerStatusView,
            playerStatusModel);

        if (lowHealthScreenEffectName_.empty())
            return;
        if (const auto lowHealthScreenEffect = GameObject::CatchChild<Ui::LowHealthScreenEffect>(Entity(), lowHealthScreenEffectName_))
            lowHealthScreenEffect->Initialize(playerStatusModel);
    }

    void StatusPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("lowHealthScreenEffectName_", lowHealthScreenEffectName_);
    }
}
