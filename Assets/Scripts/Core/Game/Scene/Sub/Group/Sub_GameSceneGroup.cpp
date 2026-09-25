#include "Sub_GameSceneGroup.h"

#include <ranges>
#include <utility>

#include "Engine/Module/Exception/Engine_Module_Exception.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
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

    Coroutine::Task<bool> GameSceneGroup::PushAsync(const SceneType type)
    {
        if (scenes_.contains(type))
            co_return true;

        // NOTE: 今は同期で読み込む。非同期にしても呼び出し側は変わらない
        try
        {
            const auto scene = factory_->Create(type);
            scene->Init();
            scenes_[type] = scene;
        }
        catch (const NanamiEngine::Module::Exception::NanamiException& exception)
        {
            // Scene ファイルの破損などで Push に失敗した。登録しないので Pop 側は何もしない
            NanamiEngine::Module::LogError("SubGameSceneGroup: シーンの Push に失敗しました: " + std::string(exception.what()));
            co_return false;
        }
        co_return true;
    }

    void GameSceneGroup::Pop(const SceneType& type)
    {
        const auto it = scenes_.find(type);
        if (it == scenes_.end())
            return;

        // Dispose 中に Push / Pop されても壊れないよう、先に外してから片付ける
        const auto scene = it->second;
        scenes_.erase(it);
        scene->Dispose();
    }

    void GameSceneGroup::Clear()
    {
        for (const auto& scene : std::exchange(scenes_, {}) | std::views::values)
        {
            scene->Dispose();
        }
    }

    void GameSceneGroup::OnDrawGui() const
    {
        for (const auto& scene : scenes_ | std::views::values)
        {
            scene->OnDrawGui();
        }
    }
}