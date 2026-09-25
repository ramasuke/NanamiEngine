#include "Coroutine_LoadSceneAsync.h"

#include "../WaitUntil/Coroutine_WaitUntil.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"

namespace Coroutine
{
    Task<SceneLoadResult> LoadSceneAsync(const std::string filePath, const NanamiEngine::R4::CancellationToken token)
    {
        const auto gameWindow = NanamiEngine::Core::Application::ApplicationBase::GameWindow();
        if (!gameWindow->BeginLoadSceneAsync(filePath))
            co_return SceneLoadResult{};

        co_await WaitUntil([gameWindow, token] { return !gameWindow->IsSceneLoading() || token.IsCancellationRequested(); });
        if (token.IsCancellationRequested())
            co_return SceneLoadResult{ SceneLoadStatus::Cancelled, nullptr };

        const auto scene = gameWindow->LastAsyncLoadedScene().lock();
        if (gameWindow->HasSceneLoadFailed() || !scene)
            co_return SceneLoadResult{};

        co_return SceneLoadResult{ SceneLoadStatus::Succeeded, scene };
    }
}
