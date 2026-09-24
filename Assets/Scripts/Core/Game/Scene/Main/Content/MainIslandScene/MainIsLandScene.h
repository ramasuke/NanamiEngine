#pragma once
#include "../../Base/Main_GameSceneBase.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "Context/MainIsLandSceneContext.h"

namespace GameCore::Scene::Main
{
    class MainIslandScene final : public GameMainSceneBase<MainIslandSceneContext>
    {
    public:
        explicit MainIslandScene(const std::weak_ptr<MainIslandSceneContext>& context, GameSceneBaseContext baseContext);
        ~MainIslandScene() override;

        /**
         * @brief 操作中のアバターを、同じ場所に別キャラで作り直す。
         * ネットワーク中の切り替えは想定していないので、ローカルの再生成だけで済ませている。
         */
        void SwitchPlayerAvatar(PlayerAvatar::PlayerAvatarType type);
        
    private:
        void Init     () override;
        Coroutine::Task<void> OnEnterAsync(int generation);
        void Enter    () override;
        void DoDispose  () override;
        void OnDrawGui() override;
        /** @brief 拠点が読めなければタイトルへ戻す */
        [[nodiscard]] std::optional<SceneType> FallbackSceneOnFailure() const override { return SceneType::Title; }
        /**
         * @brief 草原のご褒美を出す。緑の浮遊石は島の底に、噴水の島と階段は拠点の島の横に。
         *        初めて戻ったときは、石が飛んできてはまる → 島がせり上がって階段が架かる、の演出から
         */
        void ApplyGrassLandReward();
        
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        Asset::PlayerAvatarAttachments attachments_;
    };
}
