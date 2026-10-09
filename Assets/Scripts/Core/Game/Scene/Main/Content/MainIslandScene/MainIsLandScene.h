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

        // NOTE: 操作中のアバターを、同じ場所に別キャラで作り直す
        // NOTE: ローカルの再生成だけ (ネットワーク中の切り替えは未対応)
        void SwitchPlayerAvatar(PlayerAvatar::PlayerAvatarType type);

        // NOTE: 島が巣へ引かれていく演出を流して DragonNest へ移る。演出中は何もしない
        void BeginNestDeparture();
        
    private:
        [[nodiscard]] std::vector<Sub::SceneType> SubScenes() const override;
        Coroutine::Task<EnterResult> OnEnterAsync(NanamiEngine::R4::CancellationToken token) override;
        void OnEntered() override {}
        void Enter    () override;
        void DoExit  () override;
        void OnDrawGui() override;
        // NOTE: 拠点が読めなければタイトルへ戻す
        [[nodiscard]] std::optional<SceneType> FallbackSceneOnFailure() const override { return SceneType::Title; }
        // NOTE: 狩り場のご褒美を出す。初めて戻ったときは演出から
        void ApplyStageRewards();
        
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        Asset::PlayerAvatarAttachments attachments_;
        bool isDeparting_ = false;
    };
}
