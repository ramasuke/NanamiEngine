#include "Engine_Scene_AsyncSceneLoader.h"

#include <algorithm>
#include <exception>
#include <utility>

#include "../../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Scene
{
    AsyncSceneLoader::~AsyncSceneLoader()
    {
        JoinWorker();
    }

    bool AsyncSceneLoader::Begin(const std::string& filePath)
    {
        if (phase_.load(std::memory_order_acquire) != Phase::Idle)
            return false;

        JoinWorker();
        filePath_ = filePath;
        errorMessage_.clear();
        content_ = Scene::DeserializedContent();
        progress_.total.store(0, std::memory_order_release);
        progress_.done .store(0, std::memory_order_release);
        hasFailedSinceLastBegin_.store(false, std::memory_order_release);

        phase_.store(Phase::Deserializing, std::memory_order_release);
        worker_ = std::thread([this]
        {
            try
            {
                Scene::Deserialize(filePath_, content_, &progress_);
            }
            catch (const std::exception& exception)
            {
                errorMessage_ = exception.what();
            }
            catch (...)
            {
                errorMessage_ = "unknown exception";
            }
            // 途中まで積まれた GameObject も、破棄はメインスレッドに任せる
            phase_.store(errorMessage_.empty() ? Phase::Ready : Phase::Failed, std::memory_order_release);
        });
        return true;
    }

    void AsyncSceneLoader::Step()
    {
        if (phase_.load(std::memory_order_acquire) != Phase::Failed)
            return;

        JoinWorker();
        Module::LogError("AsyncSceneLoader: シーンの読み込みに失敗しました (" + filePath_ + "): " + errorMessage_);
        // DiscardContent が Idle へ戻すので、失敗したことは別に覚えておかないと外から観測できない
        hasFailedSinceLastBegin_.store(true, std::memory_order_release);
        DiscardContent();
    }

    float AsyncSceneLoader::DeserializeProgress01() const
    {
        const int total = progress_.total.load(std::memory_order_acquire);
        if (total <= 0)
            return 0.0f;

        const int done = progress_.done.load(std::memory_order_acquire);
        return std::clamp(static_cast<float>(done) / static_cast<float>(total), 0.0f, 1.0f);
    }

    bool AsyncSceneLoader::HasFailedSinceLastBegin() const
    {
        return hasFailedSinceLastBegin_.load(std::memory_order_acquire);
    }

    void AsyncSceneLoader::Cancel()
    {
        if (phase_.load(std::memory_order_acquire) == Phase::Idle)
            return;

        JoinWorker();
        DiscardContent();
    }

    bool AsyncSceneLoader::IsBusy() const
    {
        const Phase phase = phase_.load(std::memory_order_acquire);
        return phase == Phase::Deserializing || phase == Phase::Ready;
    }

    std::shared_ptr<Scene> AsyncSceneLoader::TryTakeLoadedScene()
    {
        if (phase_.load(std::memory_order_acquire) != Phase::Ready)
            return nullptr;

        JoinWorker();
        auto scene = std::make_shared<Scene>(filePath_, std::move(content_));
        content_ = Scene::DeserializedContent();
        errorMessage_.clear();
        phase_.store(Phase::Idle, std::memory_order_release);
        return scene;
    }

    void AsyncSceneLoader::JoinWorker()
    {
        if (worker_.joinable())
            worker_.join();
    }

    void AsyncSceneLoader::DiscardContent()
    {
        content_ = Scene::DeserializedContent();
        errorMessage_.clear();
        filePath_.clear();
        phase_.store(Phase::Idle, std::memory_order_release);
    }
}
