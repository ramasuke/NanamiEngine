#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Coroutine/Task/Task.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../../../Data/LoadingRoute/Data_LoadingRouteData.h"
#include "../../../Core/Game/Scene/Main/Loading/Main_SceneLoadStep.h"
#include "../../../Core/Game/Scene/Main/Transition/Main_SceneTransitionOptions.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Hint/Ui_LoadingHintCard.h"
#include "Map/Ui_LoadingRouteMap.h"

namespace GamePlay::Ui
{
    /**
     * @brief シーン遷移中に出す全画面ロード画面(紙の航路図)。
     *
     * StageLoadingScene に常駐させる。ロード中は新規オブジェクトの暖機が止まりうるため、
     * ロード開始より前から登録済みでないと動かない。
     * 出し入れは黒を挟む: 黒い幕(cover_)で画面を覆い切ってから地図(visualRoot_)を出し、幕を明けて地図を見せる。
     * 片付けるときも幕で地図を覆ってから地図を消し、幕を明けてゲーム画面へ戻す。
     * アニメーションは Time::DeltaTime() を使わない。ChangeMainScene が SkipNextFrame を
     * 60 回積むので、その間 DeltaTime() は 0 を返し続ける
     */
    class LoadingScreenUi final : public Component::ComponentBase,
                                  public LifeCycleCallback::IStartable,
                                  public LifeCycleCallback::IUpdatable
    {
    public:
        /**
         * @brief 表示を始める。既に出ているときは幕を下ろさずに航路だけ差し替える
         * @param from まだどのシーンにも居ない(起動直後)なら空
         */
        void Show(
            std::optional<GameCore::Scene::Main::SceneType> from,
            GameCore::Scene::Main::SceneType to,
            const GameCore::Scene::Main::SceneTransitionOptions& options);
        void SetStep(GameCore::Scene::Main::SceneLoadStep step);
        /** @brief 読み込みに失敗したことを表示する */
        void Fail(const std::string& message);
        /** @brief 最低表示時間と進捗の詰めが終わり次第、消えていく */
        void BeginHide();
        [[nodiscard]] bool IsShown() const { return phase_ != Phase::Hidden; }
        /** @brief ゲーム画面が完全に隠れているか。この間ならシーンを入れ替えても見えない */
        [[nodiscard]] bool IsCoverOpaque() const;
        /** @brief ゲーム画面が隠れ切るまで待つ */
        [[nodiscard]] Coroutine::Task<void> WaitCoverOpaqueAsync() const;

    private:
        enum class Phase : std::uint8_t
        {
            Hidden,
            /** 幕でゲーム画面を覆っていく */
            CoveringGame,
            /** 地図を出し、幕を明けていく */
            RevealingMap,
            Visible,
            /** 幕で地図を覆っていく */
            CoveringMap,
            /** 地図を消し、幕を明けてゲーム画面へ戻す */
            RevealingGame,
        };

        void OnStart() override;
        void OnUpdate() override;
        /** @brief timeScale にも SkipNextFrame にも影響されない壁時計の差分を返す */
        [[nodiscard]] float TickWallClockSeconds();
        [[nodiscard]] std::shared_ptr<Asset::LoadingRouteData> FindRoute(
            std::optional<GameCore::Scene::Main::SceneType> from,
            GameCore::Scene::Main::SceneType to) const;
        void UpdateCoverFade(float deltaSecs);
        /** @brief 幕を from から to へ動かす。途中から折り返しても、幕全体を fullFadeSecs で動かす速さのまま */
        void PlayCover(float from, float to, float fullFadeSecs);
        void UpdateProgress(float deltaSecs);
        void UpdateStatusText();
        void SetVisualEnabled(bool isEnabled);
        void ApplyCoverBlendRate() const;
        [[nodiscard]] float CalcRawProgress() const;
        [[nodiscard]] bool CanHide() const;

        [[serialize(1)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) cover_;
        [[serialize(1)]] FIELD(LoadingRouteMap) routeMap_;
        /** 遷移ごとの航路。出発地まで一致するものを、どこからでも使えるものより優先する */
        [[serialize(1)]] std::vector<FIELD(Asset::LoadingRouteData)> routes_;
        [[serialize(1)]] FIELD(NanamiUi::TextRenderer) statusText_;
        [[serialize(1)]] FIELD(NanamiUi::TextRenderer) percentText_;
        [[serialize(1)]] FIELD(LoadingHintCard) hintCard_;
        [[serialize(1)]] float fadeInSecs_ = 0.25f;
        [[serialize(1)]] float fadeOutSecs_ = 0.30f;
        [[serialize(1)]] float minShowSecs_ = 1.60f;
        [[serialize(1)]] float progressFollowRate_ = 6.0f;
        [[serialize(1)]] float finishSecs_ = 0.35f;

        Phase phase_ = Phase::Hidden;
        GameCore::Scene::Main::SceneLoadStep step_ = GameCore::Scene::Main::SceneLoadStep::Idle;
        std::string routeStatusText_;
        std::string statusMessage_;
        bool hasNetworkStep_ = false;
        float stepElapsedSecs_ = 0.0f;
        float shownElapsedSecs_ = 0.0f;
        /** 地図の動きに使う時計。表示し直しても巻き戻さない */
        float animationSecs_ = 0.0f;
        LibCore::Tween::TweenPlayer<float> coverTween_;
        float displayedProgress_ = 0.0f;
        int lastShownPercent_ = -1;
        int lastTickMs_ = 0;
        bool isVisualEnabled_ = false;
        bool isHideRequested_ = false;
        bool isStatusDirty_ = true;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(cover_));
            archive(CEREAL_NVP(routeMap_));
            archive(CEREAL_NVP(routes_));
            archive(CEREAL_NVP(statusText_));
            archive(CEREAL_NVP(percentText_));
            archive(CEREAL_NVP(hintCard_));
            archive(CEREAL_NVP(fadeInSecs_));
            archive(CEREAL_NVP(fadeOutSecs_));
            archive(CEREAL_NVP(minShowSecs_));
            archive(CEREAL_NVP(progressFollowRate_));
            archive(CEREAL_NVP(finishSecs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            // version 0 はステージカードの画面。部品の組み方がまるごと違うので読まない
            if (version < 1)
                return;

            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(cover_));
            archive(CEREAL_NVP(routeMap_));
            archive(CEREAL_NVP(routes_));
            archive(CEREAL_NVP(statusText_));
            archive(CEREAL_NVP(percentText_));
            archive(CEREAL_NVP(hintCard_));
            archive(CEREAL_NVP(fadeInSecs_));
            archive(CEREAL_NVP(fadeOutSecs_));
            archive(CEREAL_NVP(minShowSecs_));
            archive(CEREAL_NVP(progressFollowRate_));
            archive(CEREAL_NVP(finishSecs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::LoadingScreenUi, 1);
