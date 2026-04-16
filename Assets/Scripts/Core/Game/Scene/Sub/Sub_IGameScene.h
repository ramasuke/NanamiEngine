#pragma once

namespace GameCore::Scene::Sub
{
    class IGameScene
    {
    public:
        virtual ~IGameScene() = default;

        /**
         * @brief Sceneが変更される直前の事前処理
         * @warning Sceneインスタンスが生成されたときの初期化関数ではなく、Sceneが変更される直前に呼ばれる。
         */
        virtual void Init()      = 0;
        
        /**
         * @brief Sceneが変更されたされた時の後処理
         * @warning Sceneインスタンスが破棄されたときの関数ではなく、Sceneが変更された後に呼ばれるだけの処理。
         */
        virtual void Dispose()   = 0;
        
        /** @brief Scene状態のDebug描画 */
        virtual void OnDrawGui() = 0;
    };
}
