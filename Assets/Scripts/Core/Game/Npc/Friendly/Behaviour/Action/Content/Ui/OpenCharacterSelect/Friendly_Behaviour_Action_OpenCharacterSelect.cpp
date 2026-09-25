#include "Friendly_Behaviour_Action_OpenCharacterSelect.h"

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../GamePlay/Ui/CharacterSelect/Presenter/CharacterSelectPresenter.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    /**
     * @brief 今動いているシーンから、名簿の揃った展示台を探す
     * @details podium_ は BehaviourTree(アセット)からシーンを指す参照なので、エディタの Play 中は
     *          コピー元(編集側)の展示台を掴んでしまい、そちらは OnStart を通らず名簿が空のまま
     */
    std::shared_ptr<GamePlay::Prop::CharacterPodium> FindActivePodium(const std::shared_ptr<GamePlay::Prop::CharacterPodium>& assigned)
    {
        if (assigned && !assigned->Characters().empty())
            return assigned;

        std::shared_ptr<GamePlay::Prop::CharacterPodium> found;
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [&found](const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& gameObject)
            {
                if (found)
                    return;

                const auto podium = gameObject->Components().Catch<GamePlay::Prop::CharacterPodium>().lock();
                if (podium && !podium->Characters().empty())
                    found = podium;
            });
        return found;
    }
}

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::OpenCharacterSelect::DoTick(const TickContext& context)
    {
        const auto prefab = prefab_.get();
        if (!prefab)
            return TickStatus::Failure;

        const auto podium = FindActivePodium(podium_.get());
        if (!podium)
        {
            NanamiEngine::Module::LogError("OpenCharacterSelect: シーンに名簿の揃った CharacterPodium が見つかりません");
            return TickStatus::Failure;
        }

        // UIは world 座標がそのままスクリーン座標
        const auto ui = Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f)).lock();
        if (!ui)
            return TickStatus::Failure;

        if (const auto presenter = ui->Components().Catch<GamePlay::Ui::CharacterSelectPresenter>().lock())
            presenter->Bind(podium);
        return TickStatus::Success;
    }

    void Action::OpenCharacterSelect::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("prefab_", prefab_);
        ImGuiHelper::OnDrawInputField("podium_", podium_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::OpenCharacterSelect, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
