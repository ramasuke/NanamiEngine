#pragma once
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "vec2.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../../Data/LoadingRoute/Data_LoadingRouteData.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace GamePlay::Ui
{
    /**
     * @brief ロード画面の紙の航路図。飛行船が航路を進んだ距離で読み込みの進み具合を見せる。
     *
     * 描画は汎用のレンダラ(ImageRenderer / BlendImageRenderer / TextRenderer)に任せ、ここでは
     * Transform・スプライト・文字を書き換えるだけにする。部品はすべて prefab に常駐させる
     * (ロード中はメインシーンが居ないので Instantiate できない)。
     * 時間は呼び出し側が渡す壁時計を使う。ロード中は DeltaTime が 0 になるため
     *
     * camera_ の子に地図一式(机・地図・島・航路・飛行船)を置き、camera_ を拡大して飛行船を追わせる。
     * 手前の雲(frontClouds_)は camera_ の外に置き、地図より速く流して奥行きを出す
     */
    class LoadingRouteMap final : public Component::ComponentBase
    {
    public:
        /** @brief 航路を切り替える。ロード画面を出すたびに呼ぶ */
        void Begin(const Asset::LoadingRouteData& route, bool isStageCleared);
        /**
         * @param progress01 表示上の進み具合(後戻りしない値を渡す)
         * @param clockSecs  動きに使う壁時計の秒。途切れずに増え続ける値を渡す
         */
        void Tick(float progress01, float clockSecs);
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
        void CaptureCloudBases();
        void LayoutRouteDashes();
        void UpdateRouteDashes(float progress01);
        void UpdateShip(const RoutePoint& point, float clockSecs);
        void UpdateTrail(float progress01, float clockSecs);
        void UpdateCamera(const glm::vec2& focus, float clockSecs) const;
        void UpdateClouds(float clockSecs) const;
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
        [[serialize(0)]] FIELD(Component::ImageRenderer) destCircle_;
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

        std::vector<glm::vec2> routeSamples_;
        std::vector<float> routeLengths_;
        std::vector<glm::vec2> cloudShadowBases_;
        std::vector<glm::vec2> frontCloudBases_;
        std::vector<char> dashPassed_;
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
        bool isCloudBaseCaptured_ = false;

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
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LoadingRouteMap, 0)
