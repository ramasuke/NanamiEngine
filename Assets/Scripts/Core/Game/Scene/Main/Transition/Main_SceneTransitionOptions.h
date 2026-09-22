#pragma once

namespace GameCore::Scene::Main
{
    /** @brief シーン遷移の付帯情報。ロード画面の見せ方に使う */
    struct SceneTransitionOptions
    {
        /** ステージを踏破して戻るときに、地図のステージへ「達成」の印を押す */
        bool isStageCleared = false;
    };
}
