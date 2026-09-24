#pragma once
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "vec2.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/CircleGaugeRenderer/CircleGaugeRenderer.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../../Data/LoadingRoute/Data_LoadingRouteData.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    /**
     * @brief ロード画面の紙の航路図。飛行船が航路を進んだ距離で読み込みの進み具合を見せる。
     */
    class LoadingRouteMap final : public Component::ComponentBase
    {
    public:
        /** @brief 航路を切り替える。ロード画面を出すたびに呼ぶ */
        void Begin(const Asset::LoadingRouteData& route, bool isStageCleared);
        /**
         * @param progress01 表示上の進み具合(後戻りしない値を渡す)
         * @param clockSecs  動きに使う壁時計の秒。途切れずに増え続ける値を渡す
         * @param deltaSecs  前回からの壁時計の秒。tween を進める
         */
        void Tick(float progress01, float clockSecs, float deltaSecs);
        /**
         * @brief 地図一式の表示を切り替える。親の SetEnable は子の有効フラグをまとめて書き換えるだけなので、
         *        航路ごとに出し分けている部品はここで付け直す
         */
        void SetShown(bool isShown);

    private:
        struct RoutePoint
        {
            glm::vec2 position;
            glm::vec2 tangent;
        };

        /** @brief 航路を弧長で等間隔に引けるよう、ベジェを細かく刻んで累積長を持つ */
        void BuildRouteSamples(const Asset::LoadingRouteData& route);
        [[nodiscard]] RoutePoint RouteAt(float progress01) const;
        [[nodiscard]] RoutePoint HoverAt(float clockSecs) const;
        /** @brief prefab に置いた位置や大きさを、動かし始める前に一度だけ覚える */
        void CapturePrefabBases();
        /** @brief 揺れの往復を回し始める。既に回っていれば触らない */
        void StartSways();
        void LayoutRouteDashes();
        void UpdateRouteDashes(float progress01, float deltaSecs);
        void UpdateShip(const RoutePoint& point, float deltaSecs);
        void UpdateTrail(float progress01, float clockSecs);
        void UpdateCamera(const glm::vec2& focus, float deltaSecs);
        void UpdateClouds(float clockSecs) const;
        /** @brief 行き先の〇とクリア済みの判を最初から描き直す */
        void RestartDrawIns();
        void UpdateDestCircle(float deltaSecs);
        void UpdateStamp(float deltaSecs);
        void ApplyElementVisibility() const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) camera_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) ship_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) shipRightSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) shipLeftSprite_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) shipShadow_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::BlendImageRenderer)> trail_;
        [[serialize(0)]] std::vector<FIELD(Component::ImageRenderer)> routeDashes_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) dashSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) dashPassedSprite_;
        [[serialize(0)]] FIELD(NanamiUi::CircleGaugeRenderer) destCircle_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) clearedStamp_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::BlendImageRenderer)> cloudShadows_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::BlendImageRenderer)> frontClouds_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) kickerText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) titleText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) fromCaptionText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) toCaptionText_;
        /** 画面中心を基準にした拡大率。少し寄せておくと、追いかけて動かす余地ができる */
        [[serialize(0)]] float cameraZoom_ = 1.16f;
        [[serialize(0)]] float cameraZoomWobble_ = 0.012f;
        /** 1 で飛行船を画面中央に据える。0 で動かさない */
        [[serialize(0)]] float cameraFollow_ = 0.6f;
        /** カメラが追う点を、飛行船の何割ぶん後ろに取るか */
        [[serialize(0)]] float cameraLag_ = 0.06f;
        [[serialize(0)]] glm::vec2 cameraMaxPan_ = glm::vec2(250.0f, 140.0f);
        /** 飛行船を航路からどれだけ浮かせて描くか。影は航路の上に落とす */
        [[serialize(0)]] float shipLiftPx_ = 33.0f;
        [[serialize(0)]] float shipBobPx_ = 3.0f;
        [[serialize(0)]] float shipTiltLimitDeg_ = 14.0f;
        [[serialize(0)]] glm::vec2 shadowOffset_ = glm::vec2(12.0f, 9.0f);
        [[serialize(0)]] float cloudShadowSpeed_ = 36.0f;
        [[serialize(0)]] float frontCloudSpeed_ = 150.0f;
        /** 雲がこの x の範囲を出たら反対側へ回す */
        [[serialize(0)]] glm::vec2 cloudWrapRangeX_ = glm::vec2(-600.0f, 2520.0f);
        /** 地図が出てから〇を描き始めるまで。黒幕が明けるのを待つ */
        [[serialize(1)]] float destCircleDelay_secs_ = 0.35f;
        /** 〇を一周描くのにかける時間 */
        [[serialize(1)]] float destCircleDraw_secs_ = 0.5f;
        /** 〇を描き終えてから判を押すまで */
        [[serialize(2)]] float stampDelay_secs_ = 0.15f;
        /** 判を押し込むのにかける時間 */
        [[serialize(2)]] float stampPress_secs_ = 0.2f;
        /** 判が現れた瞬間の大きさ(倍率) */
        [[serialize(2)]] float stampStartScale_ = 1.8f;
        /** 点線が赤くなった瞬間の大きさ(倍率) */
        [[serialize(2)]] float dashPopScale_ = 1.5f;
        [[serialize(2)]] float dashPop_secs_ = 0.25f;
        /** 飛行船が潰れて裏返るまでの時間 */
        [[serialize(2)]] float shipFlip_secs_ = 0.3f;
        [[serialize(3)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        /** 飛行船の後ろに並べる煙の、進み具合でのずらし幅 */
        [[serialize(4)]] float trailStep_ = 0.035f;
        /** 揺れの片道の秒。周期が揃わないようにずらしてある */
        [[serialize(4)]] float shipBobHalf_secs_ = 1.31f;
        [[serialize(4)]] float zoomSwayHalf_secs_ = 3.49f;
        [[serialize(4)]] float panSwayXHalf_secs_ = 4.49f;
        [[serialize(4)]] float panSwayYHalf_secs_ = 2.86f;
        [[serialize(4)]] glm::vec2 panSwayPx_ = glm::vec2(4.0f, 3.0f);

        std::vector<glm::vec2> routeSamples_;
        std::vector<float> routeLengths_;
        std::vector<glm::vec2> cloudShadowBases_;
        std::vector<glm::vec2> frontCloudBases_;
        std::vector<char> dashPassed_;
        std::vector<glm::vec3> dashBaseScales_;
        std::vector<LibCore::Tween::TweenPlayer<float>> dashPopTweens_;
        glm::vec3 shipBaseScale_ = glm::vec3(1.0f);
        glm::vec3 stampBaseScale_ = glm::vec3(1.0f);
        bool isShown_ = false;
        bool hasFromCaption_ = false;
        bool hasToCaption_ = false;
        bool hasDestCircle_ = false;
        bool isStageCleared_ = false;
        float lastProgress01_ = 0.0f;
        float lastClockSecs_ = 0.0f;
        bool isHover_ = false;
        glm::vec2 hoverCenter_ = glm::vec2(0.0f);
        glm::vec2 hoverRadius_ = glm::vec2(0.0f);
        float hoverLapSecs_ = 7.0f;
        float cloudDirection_ = -1.0f;
        bool isShipFacingLeft_ = false;
        /** 裏返している途中は、進む向きと絵の向きが食い違う */
        bool isShipSpriteLeft_ = false;
        bool isPrefabBaseCaptured_ = false;
        LibCore::Tween::TweenPlayer<float> destCircleTween_;
        LibCore::Tween::TweenPlayer<float> stampTween_;
        /** 判は押し始めるまで隠しておく */
        bool isStampPressing_ = false;
        LibCore::Tween::TweenPlayer<float> shipFlipTween_;
        /** 揺れは -1..1 を往復させ、振幅を掛けて使う */
        LibCore::Tween::TweenPlayer<float> shipBobTween_;
        LibCore::Tween::TweenPlayer<float> cameraZoomSwayTween_;
        LibCore::Tween::TweenPlayer<float> cameraPanSwayXTween_;
        LibCore::Tween::TweenPlayer<float> cameraPanSwayYTween_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(camera_));
            archive(CEREAL_NVP(ship_));
            archive(CEREAL_NVP(shipRightSprite_));
            archive(CEREAL_NVP(shipLeftSprite_));
            archive(CEREAL_NVP(shipShadow_));
            archive(CEREAL_NVP(trail_));
            archive(CEREAL_NVP(routeDashes_));
            archive(CEREAL_NVP(dashSprite_));
            archive(CEREAL_NVP(dashPassedSprite_));
            archive(CEREAL_NVP(destCircle_));
            archive(CEREAL_NVP(clearedStamp_));
            archive(CEREAL_NVP(cloudShadows_));
            archive(CEREAL_NVP(frontClouds_));
            archive(CEREAL_NVP(kickerText_));
            archive(CEREAL_NVP(titleText_));
            archive(CEREAL_NVP(fromCaptionText_));
            archive(CEREAL_NVP(toCaptionText_));
            archive(CEREAL_NVP(cameraZoom_));
            archive(CEREAL_NVP(cameraZoomWobble_));
            archive(CEREAL_NVP(cameraFollow_));
            archive(CEREAL_NVP(cameraLag_));
            archive(CEREAL_NVP(cameraMaxPan_));
            archive(CEREAL_NVP(shipLiftPx_));
            archive(CEREAL_NVP(shipBobPx_));
            archive(CEREAL_NVP(shipTiltLimitDeg_));
            archive(CEREAL_NVP(shadowOffset_));
            archive(CEREAL_NVP(cloudShadowSpeed_));
            archive(CEREAL_NVP(frontCloudSpeed_));
            archive(CEREAL_NVP(cloudWrapRangeX_));
            archive(CEREAL_NVP(destCircleDelay_secs_));
            archive(CEREAL_NVP(destCircleDraw_secs_));
            archive(CEREAL_NVP(stampDelay_secs_));
            archive(CEREAL_NVP(stampPress_secs_));
            archive(CEREAL_NVP(stampStartScale_));
            archive(CEREAL_NVP(dashPopScale_));
            archive(CEREAL_NVP(dashPop_secs_));
            archive(CEREAL_NVP(shipFlip_secs_));
            archive(CEREAL_NVP(uiSounds_));
            archive(CEREAL_NVP(trailStep_));
            archive(CEREAL_NVP(shipBobHalf_secs_));
            archive(CEREAL_NVP(zoomSwayHalf_secs_));
            archive(CEREAL_NVP(panSwayXHalf_secs_));
            archive(CEREAL_NVP(panSwayYHalf_secs_));
            archive(CEREAL_NVP(panSwayPx_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(camera_));
            if (version >= 0) archive(CEREAL_NVP(ship_));
            if (version >= 0) archive(CEREAL_NVP(shipRightSprite_));
            if (version >= 0) archive(CEREAL_NVP(shipLeftSprite_));
            if (version >= 0) archive(CEREAL_NVP(shipShadow_));
            if (version >= 0) archive(CEREAL_NVP(trail_));
            if (version >= 0) archive(CEREAL_NVP(routeDashes_));
            if (version >= 0) archive(CEREAL_NVP(dashSprite_));
            if (version >= 0) archive(CEREAL_NVP(dashPassedSprite_));
            if (version >= 0) archive(CEREAL_NVP(destCircle_));
            if (version >= 0) archive(CEREAL_NVP(clearedStamp_));
            if (version >= 0) archive(CEREAL_NVP(cloudShadows_));
            if (version >= 0) archive(CEREAL_NVP(frontClouds_));
            if (version >= 0) archive(CEREAL_NVP(kickerText_));
            if (version >= 0) archive(CEREAL_NVP(titleText_));
            if (version >= 0) archive(CEREAL_NVP(fromCaptionText_));
            if (version >= 0) archive(CEREAL_NVP(toCaptionText_));
            if (version >= 0) archive(CEREAL_NVP(cameraZoom_));
            if (version >= 0) archive(CEREAL_NVP(cameraZoomWobble_));
            if (version >= 0) archive(CEREAL_NVP(cameraFollow_));
            if (version >= 0) archive(CEREAL_NVP(cameraLag_));
            if (version >= 0) archive(CEREAL_NVP(cameraMaxPan_));
            if (version >= 0) archive(CEREAL_NVP(shipLiftPx_));
            if (version >= 0) archive(CEREAL_NVP(shipBobPx_));
            if (version >= 0) archive(CEREAL_NVP(shipTiltLimitDeg_));
            if (version >= 0) archive(CEREAL_NVP(shadowOffset_));
            if (version >= 0) archive(CEREAL_NVP(cloudShadowSpeed_));
            if (version >= 0) archive(CEREAL_NVP(frontCloudSpeed_));
            if (version >= 0) archive(CEREAL_NVP(cloudWrapRangeX_));
            if (version >= 1) archive(CEREAL_NVP(destCircleDelay_secs_));
            if (version >= 1) archive(CEREAL_NVP(destCircleDraw_secs_));
            if (version >= 2) archive(CEREAL_NVP(stampDelay_secs_));
            if (version >= 2) archive(CEREAL_NVP(stampPress_secs_));
            if (version >= 2) archive(CEREAL_NVP(stampStartScale_));
            if (version >= 2) archive(CEREAL_NVP(dashPopScale_));
            if (version >= 2) archive(CEREAL_NVP(dashPop_secs_));
            if (version >= 2) archive(CEREAL_NVP(shipFlip_secs_));
            if (version >= 3) archive(CEREAL_NVP(uiSounds_));
            if (version >= 4) archive(CEREAL_NVP(trailStep_));
            if (version >= 4) archive(CEREAL_NVP(shipBobHalf_secs_));
            if (version >= 4) archive(CEREAL_NVP(zoomSwayHalf_secs_));
            if (version >= 4) archive(CEREAL_NVP(panSwayXHalf_secs_));
            if (version >= 4) archive(CEREAL_NVP(panSwayYHalf_secs_));
            if (version >= 4) archive(CEREAL_NVP(panSwayPx_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::LoadingRouteMap, 4);
