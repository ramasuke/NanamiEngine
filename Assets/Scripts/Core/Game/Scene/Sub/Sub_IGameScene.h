#pragma once

namespace GameCore::Scene::Sub
{
    class IGameScene
    {
    public:
        virtual ~IGameScene() = default;

        // NOTE: 積まれる直前の事前処理。インスタンス生成時の初期化ではない
        virtual void Init()      = 0;
        
        // NOTE: 外された時の後処理。インスタンスの破棄時ではない
        virtual void Dispose()   = 0;
        
        // NOTE: シーン状態のデバッグ描画
        virtual void OnDrawGui() = 0;
    };
}
