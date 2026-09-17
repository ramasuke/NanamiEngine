#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "../Scene.h"

namespace NanamiEngine::Scene
{
    /**
     * @brief .scene のデシリアライズをワーカースレッドで行う。
     *        GameObject の生成まではワーカーで済ませ、InitGameObject 以降は
     *        メインスレッドが受け取ってから行う
     */
    class AsyncSceneLoader final
    {
    public:
        AsyncSceneLoader() = default;
        ~AsyncSceneLoader();
        AsyncSceneLoader(const AsyncSceneLoader&)            = delete;
        AsyncSceneLoader& operator=(const AsyncSceneLoader&) = delete;

        /** @brief 読み込みを開始する。既に読み込み中なら何もせず false を返す */
        bool Begin(const std::string& filePath);
        /** @brief メインスレッドから毎フレーム呼ぶ。失敗をログに出して Idle へ戻す */
        void Step();
        /** @brief 実行中の読み込みを捨てる。ワーカーの完了は待つ */
        void Cancel();
        [[nodiscard]] bool IsBusy() const;
        /** @brief デシリアライズ済みのルート GameObject の割合。まだ総数が読めていなければ 0 */
        [[nodiscard]] float DeserializeProgress01() const;
        /** @brief 直近の Begin 以降に読み込みが失敗したか。Step が Failed を捌いた後も残る */
        [[nodiscard]] bool HasFailedSinceLastBegin() const;
        /** @brief 完了していれば Scene を組み立てて返す。まだなら nullptr */
        [[nodiscard]] std::shared_ptr<Scene> TryTakeLoadedScene();

    private:
        enum class Phase
        {
            Idle,
            Deserializing,
            Ready,
            Failed,
        };

        void JoinWorker();
        /** @brief ワーカーが作った GameObject をメインスレッドで破棄する */
        void DiscardContent();

        std::thread                 worker_;
        std::atomic<Phase>          phase_ = Phase::Idle;
        std::string                 filePath_;
        Scene::DeserializedContent  content_;
        Scene::DeserializeProgress  progress_;
        std::atomic<bool>           hasFailedSinceLastBegin_ = false;
        std::string                 errorMessage_;
    };
}
