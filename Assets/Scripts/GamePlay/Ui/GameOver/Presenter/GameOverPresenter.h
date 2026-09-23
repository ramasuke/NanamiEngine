#pragma once
#include <cstdint>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../Core/Game/Scene/Main/Type/MainSceneType.h"

namespace GamePlay::Ui
{
    class GameOverScreenUi;
}

namespace GamePlay::Ui
{
    /**
     * @brief プレイヤーが全員倒れたらゲームオーバーを出し、「もう一度挑む」「タイトルへ戻る」を捌く。
     *
     * 倒れたかどうかは同期されている体力(IsDeath)で見るので、キャラの種類にも
     * ローカル/リモートにも依らない。1人でも立っていれば救助の余地があるので出さない。
     * どちらを選んでも遷移は GameSceneGroup に頼むだけで、ロード画面が覆い切ったところで石版を消す
     */
    class GameOverPresenter final : public Component::ComponentBase,
                                    public LifeCycleCallback::IStartable,
                                    public LifeCycleCallback::IUpdatable
    {
    private:
        enum class Phase : std::uint8_t
        {
            Watching,
            Presenting,
            LeavingByLoading,
            WaitingSceneChange,
        };

        void OnStart() override;
        void OnUpdate() override;
        /** @brief timeScale にも SkipNextFrame にも影響されない壁時計の差分を返す */
        [[nodiscard]] float TickWallClockSeconds();

        void UpdateWatching(float deltaSecs);
        void UpdateInput();
        void BeginGameOver();
        void StartDeathCamera() const;
        void Select(int index);
        void Decide(int index);
        void Retry();
        void ReturnToTitle();
        void RequestSceneChange(GameCore::Scene::Main::SceneType sceneType);
        void Abort();

        [[nodiscard]] static bool AreAllPlayersFallen();
        [[nodiscard]] static bool HasAnyPlayer();

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) deathCameraPrefab_;
        [[serialize(0)]] float fallenConfirmSecs_ = 0.6f;
        [[serialize(0)]] float curtainHoldSecs_ = 0.35f;

        std::shared_ptr<GameOverScreenUi> view_;
        Phase phase_ = Phase::Watching;
        float fallenSecs_ = 0.0f;
        int selection_ = 0;
        int lastTickMs_ = 0;

        bool wasPrevPressed_ = false;
        bool wasNextPressed_ = false;
        bool wasConfirmPressed_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(deathCameraPrefab_));
            archive(CEREAL_NVP(fallenConfirmSecs_));
            archive(CEREAL_NVP(curtainHoldSecs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(deathCameraPrefab_));
            if (version >= 0) archive(CEREAL_NVP(fallenConfirmSecs_));
            if (version >= 0) archive(CEREAL_NVP(curtainHoldSecs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::GameOverPresenter, 0);
