#include "Sub_GameSceneGroup.h"

#include <ranges>
#include <utility>

#include "../../../../../../../Engine/Module/Exception/Engine_Module_Exception.h"
#include "../../../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../Sub_IGameScene.h"
#include "../Factory/SubSceneFactory.h"

namespace GameCore::Scene::Sub
{
    GameSceneGroup::GameSceneGroup(
        std::vector<std::weak_ptr<SceneContextBase>> contexts)
        : factory_(std::make_unique<SceneFactory>(contexts))
    {
        
    }

    GameSceneGroup::~GameSceneGroup() = default;

    void GameSceneGroup::Push(const SceneType& type)
    {
        changeRequests_.emplace_back(ChangeRequestType::Push, type);
    }

    void GameSceneGroup::Pop(const SceneType& type)
    {
        changeRequests_.emplace_back(ChangeRequestType::Pop, type);
    }

    void GameSceneGroup::Clear()
    {
        for (const auto& type : scenes_ | std::views::keys)
        {
            changeRequests_.emplace_back(ChangeRequestType::Pop, type);
        }
    }

    void GameSceneGroup::Update()
    {
        ProcessRequests();
    }

    void GameSceneGroup::OnDrawGui() const
    {
        for (const auto& scene : scenes_ | std::views::values)
        {
            scene->OnDrawGui();
        }
    }

    void GameSceneGroup::ProcessRequests()
    {
        // 例外で途中終了しても同じリクエストが次フレームに再実行されないよう、先にキューを空にしてから処理する
        const auto changeRequests = std::exchange(changeRequests_, {});
        for (const auto& changeRequest : changeRequests)
        {
            switch (changeRequest.type)
            {
            case ChangeRequestType::Push:
                {
                    try
                    {
                        const auto scene = factory_->Create(changeRequest.sceneType);
                        scene->Init();
                        scenes_[changeRequest.sceneType] = scene;
                    }
                    catch (const NanamiEngine::Module::Exception::NanamiException& exception)
                    {
                        // Scene ファイルの破損などで Push に失敗した。登録しないので Pop 側は何もしない
                        NanamiEngine::Module::LogError("SubGameSceneGroup: シーンの Push に失敗しました: " + std::string(exception.what()));
                    }
                    break;
                }
            case ChangeRequestType::Pop:
                {
                    auto it = scenes_.find(changeRequest.sceneType);
                    if (it == scenes_.end()) break;

                    it->second->Dispose();
                    scenes_.erase(it);
                    break;
                }
            }
        }
    }
}