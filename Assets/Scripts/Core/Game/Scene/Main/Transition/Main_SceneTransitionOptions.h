#pragma once

namespace GameCore::Scene::Main
{
    // NOTE: シーン遷移の付帯情報。ロード画面の見せ方と、出入りするシーンの分岐に使う
    struct SceneTransitionOptions
    {
        // NOTE: ステージを踏破して戻るときに、地図のステージへ「達成」の印を押す
        bool isStageCleared = false;
        // NOTE: ゲームオーバーからのやり直し
        bool isRetry = false;
    };
}
