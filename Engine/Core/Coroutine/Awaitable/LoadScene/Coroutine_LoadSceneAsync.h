#pragma once
#include <memory>
#include <string>

#include "../../Task/Task.h"
#include "../../../../Module/Scene/Scene.h"
#include "../../../../../Packages/R4/R4.h"

namespace Coroutine
{
    enum class SceneLoadStatus
    {
        Succeeded,
        Failed,
        /** 読み込みの途中で token がキャンセルされた */
        Cancelled,
    };

    /** @note 既定値は Failed。コルーチン内の例外で既定値が返っても成功扱いにならない */
    struct SceneLoadResult final
    {
        SceneLoadStatus status = SceneLoadStatus::Failed;
        std::shared_ptr<NanamiEngine::Scene::Scene> scene;

        [[nodiscard]] explicit operator bool() const { return status == SceneLoadStatus::Succeeded && scene; }
        [[nodiscard]] bool IsCancelled() const { return status == SceneLoadStatus::Cancelled; }
    };

    /**
     * @brief シーンをワーカースレッドで読み込み、メインシーンへ差し替わるまで待つ。待っている間もフレームは回る
     * @note キャンセルされても読み込みは止めない。捨てるのは GameWindow::CancelSceneLoad の役目
     */
    Task<SceneLoadResult> LoadSceneAsync(std::string filePath, NanamiEngine::R4::CancellationToken token = {});
}
