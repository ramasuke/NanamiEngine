#include "Game.h"

#include "Scene/Main/Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "Scene/Main/Group/Main_GameSceneGroup.h"
#include "Scene/Main/Content/Title/TitleScene.h"
#include "Scene/Sub/Context/Sub_SceneContextBase.h"
#include "Scene/Sub/Group/Sub_GameSceneGroup.h"
#include "Story/Story_StoryProgress.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore
{
    Game* Game::instance_ = nullptr;
    
    Game::Game()
        : mainScenarioProgression_(std::make_shared<GameProgresion>(GameProgresion::FirstTouchDownMainIsLand))
    {
    }
    
    Game::~Game()
    {
        // NOTE: RemoveComponent は OnDestroy を呼ばないので、デストラクタでも解除する
        if (instance_ == this)
            instance_ = nullptr;
    }
    
    Scene::Sub::GameSceneGroup& Game::SubScenes() const
    {
        return *subSceneGroup_;
    }

    void Game::InitMainSceneGroup()
    {
        sceneGroup_ = std::make_unique<Scene::Main::GameSceneGroup>(
            sceneContexts_->Components().Catches<Scene::SceneContextBase>(),
            subSceneGroup_,
            loadingScreen_);
        
        sceneGroup_->RequestChangeScene(Scene::Main::SceneType::Title);
    }

    void Game::InitSubSceneGroup()
    {
        auto sceneContexts = subSceneContexts_->Components().Catches<Scene::Sub::SceneContextBase>();
        subSceneGroup_ = std::make_shared<Scene::Sub::GameSceneGroup>(sceneContexts);
    }

    void Game::InitStageLoadingScene()
    {
        // メインシーンの入れ替えを跨いで出し続けるので、GameManage.scene と同じく
        // 起動時に読んでそのまま contents_ に残す
        const auto scene = stageLoadingSceneFile_->LoadScene();
        Core::Application::ApplicationBase::GameWindow()->AddContent(scene);
        stageLoadingScene_ = scene;

        scene->ForEachGameObject([this](const std::shared_ptr<GameObject::IGameObject>& gameObject)
        {
            if (loadingScreen_)
                return;

            loadingScreen_ = gameObject->Components().Catch<GamePlay::Ui::LoadingScreenUi>().lock();
        });

        if (!loadingScreen_)
        {
            NanamiEngine::Module::LogError(
                "Game: StageLoadingScene に LoadingScreenUi が見つかりませんでした: " + scene->Name());
        }
    }

    void Game::InitGameOverScene()
    {
        if (!gameOverSceneFile_)
        {
            NanamiEngine::Module::LogError("Game: gameOverSceneFile_ が設定されていないため、ゲームオーバー画面を出せません");
            return;
        }

        // ロード画面と同じく、メインシーンの入れ替えを跨いで残す
        Core::Application::ApplicationBase::GameWindow()->AddContent(gameOverSceneFile_->LoadScene());
    }

    void Game::OnAwake()
    {
        InitStageLoadingScene();
        InitGameOverScene();
        InitSubSceneGroup();
        InitMainSceneGroup();
    }
    
    void Game::OnUpdate()
    {
        sceneGroup_   ->Update();
        subSceneGroup_->Update();
    }
    
    void Game::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;
    }
    
    void Game::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sceneContexts_", sceneContexts_);
        ImGuiHelper::OnDrawInputField("sceneGroup_"   , sceneGroup_   );
        
        if (ImGui::Button("ChangeTitleScene"))
        {
            sceneGroup_->RequestChangeScene(Scene::Main::SceneType::Title);
        }
        if (ImGui::Button("ChangeFirstTouchDownMainIsLandScene"))
        {
            sceneGroup_->RequestChangeScene(Scene::Main::SceneType::FirstTouchDownMainIsLand);
        }
        if (ImGui::Button("ChangeMainIsLandScene"))
        {
            sceneGroup_->RequestChangeScene(Scene::Main::SceneType::MainIsland);
        }
        if (ImGui::Button("ChangeGrassLandScene"))
        {
            sceneGroup_->RequestChangeScene(Scene::Main::SceneType::GrassLand);
        }

        ImGuiHelper::OnDrawInputField("subSceneGroup_"      , subSceneGroup_   );
        ImGuiHelper::OnDrawInputField("subSceneContexts_"   , subSceneContexts_);
        ImGuiHelper::OnDrawInputField("stageLoadingSceneFile_", stageLoadingSceneFile_);
        ImGuiHelper::OnDrawInputField("gameOverSceneFile_", gameOverSceneFile_);

        Story::StoryProgress::Instance().OnDrawGui();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::Game);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, GameCore::Game);
#pragma endregion
