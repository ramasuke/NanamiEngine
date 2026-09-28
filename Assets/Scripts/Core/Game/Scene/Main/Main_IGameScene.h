#pragma once

namespace GameCore::Scene::Main
{
    /**
     * @note Sceneインスタンスを使い回す設計なのは、Sceneの読み込み処理を出来るだけ高速にさせるため。
     */
    class IGameScene
    {
    public:
        virtual ~IGameScene() = default;

        /**
         * @brief Sceneが変更される直前の事前処理
         */
        virtual void Init()      = 0;
        
        /** @brief currentSceneがthisに変更された時の処理  */
        virtual void Enter()    = 0;
        
        /**
         * @brief Sceneが変更されたされた時の後処理
         * @warning Sceneインスタンスが破棄されたときの関数ではなく、Sceneが変更された後に呼ばれるだけの処理。
         */
        virtual void Exit()      = 0;

        /**
         * @brief Game の破棄に合わせて、セーブせずに読み込んだシーンを外す
         * @note Exit と違いはDoExitを呼ばない
         */
        virtual void Dispose()   = 0;

        /**
         * @brief Init で始めた読み込みと入場の準備が済んだか。
         */
        [[nodiscard]] virtual bool IsEntered() const = 0;
        
        virtual void OnDrawGui() = 0;
    };
}