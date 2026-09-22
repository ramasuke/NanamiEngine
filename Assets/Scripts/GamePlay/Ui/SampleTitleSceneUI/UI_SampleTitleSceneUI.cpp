#include "UI_SampleTitleSceneUI.h"

#include <stdexcept>

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../AssetUpdate/Presenter/AssetUpdatePresenter.h"

namespace GamePlay::Ui
{
    void SampleTitleScene::OnStart()
    {
        gameStartButton_->OnClick().Subscribe([this](NanamiUi::MouseState)
        {
            OnGameStart();
        }).AddTo(this);
        gameExitButton_ ->OnClick().Subscribe([this](NanamiUi::MouseState)
        {
        }).AddTo(this);

        const auto prefab = assetUpdatePrefab_.get();
        if (!prefab)
        {
            Module::LogWarning("[AssetUpdater] assetUpdatePrefab_ が未設定なので、配信の更新は確認しません");
            return;
        }

        // UIは world 座標がそのままスクリーン座標
        if (const auto ui = Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f)).lock())
            assetUpdate_ = ui->Components().Catch<AssetUpdatePresenter>();
    }

    void SampleTitleScene::OnGameStart()
    {
        // 更新が済んでいなければ荷札を出し直す。確認中・受け取り中の押下は受け流す
        if (const auto assetUpdate = assetUpdate_.lock(); assetUpdate && !assetUpdate->TryStartGame())
            return;

        switch (GameCore::LoadGameProgression())
        {
        case GameCore::GameProgresion::FirstTouchDownMainIsLand:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::FirstTouchDownMainIsLand);
            break;
        case GameCore::GameProgresion::MainIsland:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::MainIsland);
            break;
        case GameCore::GameProgresion::GrassLandStage:
            GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::GrassLand);
            break;
        default:
            throw std::runtime_error("Gameの進行状況に応じたScene遷移が定義されていません。");
        }
    }

    void SampleTitleScene::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("gameStartButton_", gameStartButton_);
        ImGuiHelper::OnDrawInputField("gameExitButton_" , gameExitButton_);
        ImGuiHelper::OnDrawInputField("assetUpdatePrefab_", assetUpdatePrefab_);
    }
}
