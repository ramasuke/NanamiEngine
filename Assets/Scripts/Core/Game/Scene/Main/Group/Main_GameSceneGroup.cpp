#include "Main_GameSceneGroup.h"

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Exception/Engine_Module_Exception.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include <cassert>
#include <exception>
#include <stdexcept>
#include <utility>
#include "../Content/FirstTouchDownMainIsLand/FirstTouchDownMainIsLandScene.h"
#include "../Content/GrassLand/GrassLandScene.h"
#include "../Content/MainIslandScene/MainIsLandScene.h"
#include "../Content/Title/TitleScene.h"

namespace GameCore::Scene::Main
{
    GameSceneGroup::GameSceneGroup(
        std::vector<std::weak_ptr<SceneContextBase>> sceneContexts,
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack,
        const std::shared_ptr<GamePlay::Ui::LoadingScreenUi>& loadingScreen)
        : sceneContexts_(std::move(sceneContexts))
        , loadingScreen_(loadingScreen)
    {
        // シーンは Game と同じだけ生きるので、this を捕まえてよい
        const auto baseContext = GameSceneBaseContext(subSceneStack, loadingScreen,
            [this](const SceneType type, const SceneTransitionOptions options) { RequestChangeScene(type, options); });

        AddScene(SceneType::Title, std::make_shared<TitleScene>(
            CatchContext<TitleSceneContext>(), baseContext));

        AddScene(SceneType::FirstTouchDownMainIsLand, std::make_shared<FirstTouchDownMainIsLandScene>(
            CatchContext<FirstTouchDownMainIsLandSceneContext>(), baseContext));

        AddScene(SceneType::MainIsland, std::make_shared<MainIslandScene>(
            CatchContext<MainIslandSceneContext>(), baseContext));

        AddScene(SceneType::GrassLand, std::make_shared<GrassLandScene>(
            CatchContext<GrassLandSceneContext>(), baseContext));
    }

    void GameSceneGroup::Update()
    {
        // 覆っている途中・入場待ちの途中に来た要求は、行き先を差し替えて覆い直す
        if (pendingRequest_)
            BeginCovering();

        if (phase_ == Phase::Covering)
        {
            if (loadingScreen_ && !loadingScreen_->IsCoverOpaque())
                return;

            const auto request = *std::exchange(coveringRequest_, std::nullopt);
            SwitchScene(request);
            return;
        }

        if (phase_ == Phase::Entering)
        {
            const auto current = currentScene_.lock();
            if (current && !current->IsEntered())
                return;

            if (loadingScreen_)
                loadingScreen_->BeginHide();
            phase_ = Phase::Idle;
        }
    }

    void GameSceneGroup::OnDrawGui()
    {
        for (const auto& scene : scenes_ | std::views::values)
        {
            scene->OnDrawGui();
        }
    }

    void GameSceneGroup::RequestChangeScene(const SceneType type, const SceneTransitionOptions options)
    {
        assert(scenes_.contains(type) && "Scene not registered");

        // 同じフレームに複数来たら最後の 1 件だけを通す。途中の行き先を読み込んでも捨てるだけになる
        pendingRequest_ = ChangeRequest{ type, options };
    }

    void GameSceneGroup::BeginCovering()
    {
        coveringRequest_ = std::exchange(pendingRequest_, std::nullopt);
        if (loadingScreen_)
            loadingScreen_->Show(currentSceneType_, coveringRequest_->type, coveringRequest_->options);

        phase_ = Phase::Covering;
    }

    void GameSceneGroup::SwitchScene(const ChangeRequest& request)
    {
        Time::SkipNextFrame();
        Time::SkipNextFrame();

        // 前の入場がまだ読み込み中なら捨てる。残すと、読み終えたときにメインシーンへ差し込まれてしまう
        const auto gameWindow = Core::Application::ApplicationBase::GameWindow();
        if (gameWindow->IsSceneLoading())
            gameWindow->CancelSceneLoad();

        if (const auto current = currentScene_.lock())
        {
            current->Dispose();
        }

        const auto& next = scenes_.at(request.type);
        currentScene_     = next;
        currentSceneType_ = request.type;
        phase_            = Phase::Entering;
        try
        {
            next->Init();
            next->Enter();
        }
        catch (const std::exception& exception)
        {
            NanamiEngine::Module::LogError("GameSceneGroup: シーン遷移に失敗しました: " + std::string(exception.what()));
            // 入場が終わる見込みが無いので、ロード画面を明けられるように手放す
            currentScene_.reset();
        }
    }

    void GameSceneGroup::AddScene(const SceneType type, std::shared_ptr<IGameScene> scene)
    {
        scenes_[type] = std::move(scene);
    }
}
