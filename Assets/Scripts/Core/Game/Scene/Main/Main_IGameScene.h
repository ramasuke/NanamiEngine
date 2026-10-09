#pragma once
#include <memory>

namespace GameCore::Scene
{
    class SceneContextBase;
}

namespace GameCore::Scene::Main
{
    // NOTE: 読み込みを速くするため、Scene インスタンスは使い回す
    class IGameScene
    {
    public:
        virtual ~IGameScene() = default;

        // NOTE: Sceneが変更される直前の事前処理
        virtual void Init()      = 0;
        
        // NOTE: このシーンが現在のシーンになった時の処理
        virtual void Enter()    = 0;
        
        // NOTE: 別のシーンへ変わった後の後処理。インスタンスの破棄時ではない
        virtual void Exit()      = 0;

        // NOTE: ゲーム終了時に、セーブせずに読み込んだシーンを外す
        // NOTE: Exit と違い DoExit を呼ばない
        virtual void Dispose()   = 0;

        // NOTE: Init で始めた読み込みと入場の準備が済んだか
        [[nodiscard]] virtual bool IsEntered() const = 0;

        // NOTE: このシーンのコンテキスト(GameManage.scene に常駐)
        [[nodiscard]] virtual std::shared_ptr<SceneContextBase> BaseContext() const = 0;
        
        virtual void OnDrawGui() = 0;
    };
}