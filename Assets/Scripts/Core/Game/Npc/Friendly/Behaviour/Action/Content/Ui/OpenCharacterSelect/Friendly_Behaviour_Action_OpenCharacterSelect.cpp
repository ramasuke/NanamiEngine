#include "Friendly_Behaviour_Action_OpenCharacterSelect.h"

#include "../../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../GamePlay/Ui/CharacterSelect/Presenter/CharacterSelectPresenter.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::OpenCharacterSelect::DoTick(const TickContext& context)
    {
        const auto prefab = prefab_.get();
        if (!prefab)
            return TickStatus::Failure;

        // UIは world 座標がそのままスクリーン座標
        const auto ui = Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f)).lock();
        if (!ui)
            return TickStatus::Failure;

        if (const auto presenter = ui->Components().Catch<GamePlay::Ui::CharacterSelectPresenter>().lock())
            presenter->Bind(podium_.get());
        return TickStatus::Success;
    }

    void Action::OpenCharacterSelect::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("prefab_", prefab_);
        ImGuiHelper::OnDrawInputField("podium_", podium_);
    }
}
