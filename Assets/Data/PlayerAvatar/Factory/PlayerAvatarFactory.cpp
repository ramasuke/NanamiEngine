#include "PlayerAvatarFactory.h"

#include "../../../Scripts/Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Scripts/Core/Game/Game.h"
#include "../../../Scripts/GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../../Scripts/GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/Status/PlayerAvatarStatus.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/Status/Presenter/PlayerAvatar_OtherPlayer_StatusPresenter.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/SwordMan/Status/Presenter/PlayerAvatar_SwordMan_StatusPresenter.h"
#include "../../../Scripts/Core/Game/Scene/Sub/Content/OtherPlayerStatusUI/OtherPlayerStatusUIScene.h"
#include "../../../Scripts/Core/Game/Scene/Sub/Group/Sub_GameSceneGroup.h"
#include "../../../Scripts/GamePlay/Ui/OtherPlayerStatusUIGroup/OtherPlayerStatusUiGroup.h"
#include "../../../Scripts/GamePlay/Ui/PlayerStatus/Ui_PlayerStatus.h"

namespace NanamiEngine::Module::Asset
{
    PlayerAvatarFactory::PlayerAvatarFactory(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
        
    }

    std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> PlayerAvatarFactory::SummonSwordManAvatar(
        const glm::vec3& summonPosition,
        const std::shared_ptr<GameObject::IGameObject>& parent)
    {
        const auto playerAvatarObject = Scene::GameObject::Instantiate(swordManPrefab_.get(), summonPosition).lock();
        auto playerAvatar= playerAvatarObject->Components().Catch<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>().lock();
        
        playerAvatarObject->Transform().SetParent(parent);
        return playerAvatar;
    }

    std::shared_ptr<GameCore::IPlayerAvatar> PlayerAvatarFactory::LoadInitedPlayerAvatar(
        const GameCore::PlayerAvatar::PlayerAvatarType& type,
        const glm::vec3& summonPosition,
        const std::shared_ptr<GameObject::IGameObject>& parent,
        const bool enableInputAction,
        const std::shared_ptr<GameCore::PlayerAvatar::IPlayerAvatarStatus>& presetStatus)
    {
        std::shared_ptr<GameCore::IPlayerAvatar> playerAvatar;

        switch (type)
        {
        case GameCore::PlayerAvatar::PlayerAvatarType::SwordMan:
            {
                std::weak_ptr<GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup> swordmanCameraGroup;
                if (enableInputAction)
                {
                    swordmanCameraGroup = Scene::GameObject::Instantiate(swordManCameraGroupPrefab_.get(), summonPosition)
                        .lock()
                        ->Components()
                        .Catch<GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup>();
                }

                auto presetSwordManStatus = std::dynamic_pointer_cast<GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus>(presetStatus);
                auto status = presetSwordManStatus
                    ? presetSwordManStatus
                    : GameCore::PlayerAvatar::LoadStatus<GameCore::PlayerAvatar::SwordMan::SwordManAvatarTraits>();
                playerAvatar = LoadInitedPlayerAvatarImpl<
                    GamePlay::PlayerAvatar::SwordMan::SwordManAvatar,
                    GameCore::PlayerAvatar::SwordMan::SwordManAvatarTraits>(
                    swordManPrefab_.get(),
                    summonPosition,
                    parent,
                    status,
                    swordmanCameraGroup.lock(),
                    enableInputAction);

                if (enableInputAction)
                {
                    auto swordManStatusUiPrefab = Scene::GameObject::Instantiate(*swordManStatusUiPrefab_.get());
                    auto swordManStatusUi = swordManStatusUiPrefab.lock()->Components().Catch<GamePlay::Ui::PlayerStatus>();
                    auto swordManPresenterObj = Scene::GameObject::Instantiate(*swordManStatusPresenterPrefab_.get());
                    /** StatusPresenter */
                    auto swordmanStatusPresenter = swordManPresenterObj.lock()->Components().Catch<GamePlay::PlayerAvatar::SwordMan::StatusPresenter>();
                    swordmanStatusPresenter.lock()->Initialize(*swordManStatusUi.lock(), *status);
                }
                break;
            }

        case GameCore::PlayerAvatar::PlayerAvatarType::Gunner:
            {
                break;
            }

        default:
            {
                break;
            }
        }

        if (!enableInputAction)
        {
            auto statusUiPrefab = Scene::GameObject::Instantiate(*otherPlayerAvatarStatusUiPrefab_.get());
            auto statusUi = statusUiPrefab.lock()->Components().Catch<GamePlay::Ui::PlayerStatus>();
            auto presenterObj = Scene::GameObject::Instantiate(*otherPlayerAvatarStatusPresenterPrefab_.get());
            /** StatusPresenter */
            auto statusPresenter = presenterObj.lock()->Components().Catch<GamePlay::PlayerAvatar::OtherPlayer::StatusPresenter>();
            statusPresenter.lock()->Initialize(*statusUi.lock(), playerAvatar->PlayerStatus());
            GameCore::Game::Instance()
                .SubScenes()
                .Catch<GameCore::Scene::Sub::OtherPlayerStatusUiScene>(
                    GameCore::Scene::Sub::SceneType::OtherPlayerStatus)
                ->Context()
                .Ui()
                .AddPlayerStatus(statusUi);
        }
        return playerAvatar;
    }

    void PlayerAvatarFactory::OnDrawGui()
    {
        ScriptableObject::OnDrawGui();
        ImGuiHelper::OnDrawInputField("swordManPrefab_", swordManPrefab_);
        ImGuiHelper::OnDrawInputField("swordManCameraGroupPrefab_", swordManCameraGroupPrefab_);
        ImGuiHelper::OnDrawInputField("swordManStatusUiPrefab_", swordManStatusUiPrefab_);
        ImGuiHelper::OnDrawInputField("swordManStatusPresenterPrefab_", swordManStatusPresenterPrefab_);
        ImGuiHelper::OnDrawInputField("otherPlayerAvatarStatusUiPrefab_", otherPlayerAvatarStatusUiPrefab_);
        ImGuiHelper::OnDrawInputField("otherPlayerAvatarStatusPresenterPrefab_", otherPlayerAvatarStatusPresenterPrefab_);
    }
}
