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

        /**
         * @brief 島が古竜の巣へ引かれていく演出を流して、巣 (SceneType::DragonNest) へ移る。
         *        教官の BT (Story::DepartForNest) から呼ばれる。流れている間は何もしない
         */
        void BeginNestDeparture();
        
    private:
        [[nodiscard]] std::vector<Sub::SceneType> SubScenes() const override;
        Coroutine::Task<EnterResult> OnEnterAsync(NanamiEngine::R4::CancellationToken token) override;
        void OnEntered() override {}
        void Enter    () override;
        void DoDispose  () override;
        void OnDrawGui() override;
        /** @brief 拠点が読めなければタイトルへ戻す */
        [[nodiscard]] std::optional<SceneType> FallbackSceneOnFailure() const override { return SceneType::Title; }
        /**
         * @brief 狩り場のご褒美を出す。緑の浮遊石は島の底に、噴水の島と階段は拠点の島の横に、光の浮遊石も島の底に。
         *        初めて戻ったときは、石が飛んできてはまる → 島がせり上がって階段が架かる、の演出から
         */
        void ApplyStageRewards();
        
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        Asset::PlayerAvatarAttachments attachments_;
        bool isDeparting_ = false;
    };
}
