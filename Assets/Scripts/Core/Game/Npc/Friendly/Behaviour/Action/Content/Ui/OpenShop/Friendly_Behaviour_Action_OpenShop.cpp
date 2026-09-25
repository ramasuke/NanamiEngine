#include "Friendly_Behaviour_Action_OpenShop.h"

#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../GamePlay/Ui/Shop/Presenter/ShopPresenter.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::OpenShop::DoTick(const TickContext& context)
    {
        const auto prefab = prefab_.get();
        if (!prefab)
            return TickStatus::Failure;

        // UIは world 座標がそのままスクリーン座標
        const auto ui = Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f)).lock();
        if (!ui)
            return TickStatus::Failure;

        if (const auto presenter = ui->Components().Catch<GamePlay::Ui::ShopPresenter>().lock())
            presenter->Bind(stall_.get());
        return TickStatus::Success;
    }

    void Action::OpenShop::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("prefab_", prefab_);
        ImGuiHelper::OnDrawInputField("stall_", stall_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::OpenShop, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
