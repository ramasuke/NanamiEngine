#include "Main_GameSceneGroup.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Engine/Module/Exception/Engine_Module_Exception.h"
#include "../../../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
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
        const std::shared_ptr<Sub::IGameSceneStack>& subSceneStack)
        : sceneContexts_(std::move(sceneContexts))
    {
        AddScene(SceneType::Title, std::make_shared<TitleScene>(
            CatchContext<TitleSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(SceneType::FirstTouchDownMainIsLand, std::make_shared<FirstTouchDownMainIsLandScene>(
            CatchContext<FirstTouchDownMainIsLandSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(SceneType::MainIsland, std::make_shared<MainIslandScene>(
            CatchContext<MainIslandSceneContext>(),
            GameSceneBaseContext(subSceneStack)));

        AddScene(SceneType::GrassLand, std::make_shared<GrassLandScene>(
            CatchContext<GrassLandSceneContext>(),
            GameSceneBaseContext(subSceneStack)));
    }

    void GameSceneGroup::Update()
    {
        ProcessRequests();
    }

    void GameSceneGroup::OnDrawGui()
    {
        for (const auto& scene : scenes_ | std::views::values)
        {
            scene->OnDrawGui();
        }
    }

    void GameSceneGroup::RequestChangeScene(const SceneType type)
    {
        assert(scenes_.contains(type) && "Scene not registered");

        changeRequests_.push_back(type);
    }

    void GameSceneGroup::ProcessRequests()
    {
        // 例外で途中終了しても同じリクエストが次フレームに再実行されないよう、先にキューを空にしてから処理する
        const auto changeRequests = std::exchange(changeRequests_, {});
        for (const auto& changeRequest : changeRequests)
        {
            Time::SkipNextFrame();
            Time::SkipNextFrame();

            if (const auto current = currentScene_.lock())
            {
                current->Dispose();
            }

            const auto& next = scenes_.at(changeRequest);
            try
            {
                next->Init();
                currentScene_ = next;
                currentScene_.lock()->Enter();
            }
            catch (const NanamiEngine::Module::Exception::NanamiException& exception)
            {
                // Scene ファイルの破損などで遷移に失敗した。前の Scene は既に Dispose 済みなので currentScene_ は更新しない
                // （GameWindow が次フレームで初期 Scene に戻す）
                NanamiEngine::Module::LogError("GameSceneGroup: シーン遷移に失敗しました: " + std::string(exception.what()));
            }
        }
    }

    void GameSceneGroup::AddScene(const SceneType type, std::shared_ptr<IGameScene> scene)
    {
        scenes_[type] = std::move(scene);
    }
}
