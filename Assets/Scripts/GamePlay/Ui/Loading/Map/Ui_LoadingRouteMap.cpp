#include "Ui_LoadingRouteMap.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../Sound/UiSoundBank.h"

namespace
{
    using LibCore::EaseType;
    using LibCore::Tween::Ease;
    using LibCore::Tween::Ms;
    using LibCore::Tween::TweenPlayer;

    /** ロード画面の配置の基準にしている画面の大きさ */
    constexpr glm::vec2 LOADING_ROUTE_MAP_SCREEN_CENTER = glm::vec2(960.0f, 540.0f);
    constexpr int LOADING_ROUTE_MAP_SAMPLE_COUNT = 128;
    /** 飛行船の後ろに並べる煙の、進み具合でのずらし幅 */
    constexpr float LOADING_ROUTE_MAP_TRAIL_STEP = 0.035f;
    /** 揺れの片道の秒。周期が揃わないようにずらしてある */
    constexpr float LOADING_ROUTE_MAP_SHIP_BOB_SECS    = 1.31f;
    constexpr float LOADING_ROUTE_MAP_ZOOM_SWAY_SECS   = 3.49f;
    constexpr float LOADING_ROUTE_MAP_PAN_SWAY_X_SECS  = 4.49f;
    constexpr float LOADING_ROUTE_MAP_PAN_SWAY_Y_SECS  = 2.86f;
    constexpr glm::vec2 LOADING_ROUTE_MAP_PAN_SWAY_PX  = glm::vec2(4.0f, 3.0f);

    /** @brief -1..1 を行って戻ってを繰り返す */
    void LoadingRouteMapStartSway(TweenPlayer<float>& sway, const float halfSecs)
    {
        if (sway.IsPlaying())
            return;

        sway.Set(tweeny::from(-1.0f).to(1.0f).during(Ms(halfSecs)).via(Ease(EaseType::InOutSine)));
        sway.PlayForward();
    }

    float LoadingRouteMapTickSway(TweenPlayer<float>& sway, const float deltaSecs)
    {
        if (sway.Tick(deltaSecs))
        {
            if (sway.IsForward())
                sway.PlayBackward();
            else
                sway.PlayForward();
        }
        return sway.Value();
    }

    /** @brief 再生していないときは等倍 */
    float LoadingRouteMapScaleRate(const TweenPlayer<float>& tween)
    {
        return tween.IsPlaying() ? tween.Value() : 1.0f;
    }

    glm::vec2 LoadingRouteMapBezier(const Asset::LoadingRouteData& route, const float t)
    {
        const float u = 1.0f - t;
        return u * u * u * route.P0()
             + 3.0f * u * u * t * route.P1()
             + 3.0f * u * t * t * route.P2()
             + t * t * t * route.P3();
    }

    glm::quat LoadingRouteMapRotationZ(const float radians)
    {
        return glm::angleAxis(radians, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    /** @brief 奥行き(z)は prefab のまま残して、平面の位置だけ動かす */
    void LoadingRouteMapSetLocalPos(NanamiEngine::Module::GameObject::Transform& transform, const glm::vec2& position)
    {
        transform.SetLocalPos(glm::vec3(position, transform.GetLocalPos().z));
    }
}

namespace GamePlay::Ui
{
    void LoadingRouteMap::Begin(const Asset::LoadingRouteData& route, const bool isStageCleared)
    {
        CapturePrefabBases();
        StartSways();

        hasFromCaption_ = !route.FromCaption().empty();
        hasToCaption_   = !route.ToCaption().empty();
        hasDestCircle_  = route.HasDestCircle();
        isStageCleared_ = isStageCleared;
        isHover_        = route.IsHover();
        hoverCenter_    = route.HoverCenter();
        hoverRadius_    = route.HoverRadius();
        hoverLapSecs_   = std::max(route.HoverLapSecs(), 0.5f);
        cloudDirection_ = route.CloudDirection() < 0.0f ? -1.0f : 1.0f;

        BuildRouteSamples(route);
        LayoutRouteDashes();

        if (const auto kicker = kickerText_.get())
            kicker->SetText(route.KickerText());
        if (const auto title = titleText_.get())
            title->SetText(route.TitleText());

        const auto applyCaption = [](const std::shared_ptr<NanamiUi::TextRenderer>& caption, const std::string& text, const glm::vec2& position)
        {
            if (!caption || text.empty())
                return;

            caption->SetText(text);
            LoadingRouteMapSetLocalPos(caption->Transform(), position);
        };
        applyCaption(fromCaptionText_.get(), route.FromCaption(), route.FromCaptionPosition());
        applyCaption(toCaptionText_.get(), route.ToCaption(), route.ToCaptionPosition());

        if (const auto destCircle = destCircle_.get())
        {
            LoadingRouteMapSetLocalPos(destCircle->Transform(), route.DestCirclePosition());
            destCircle->Transform().SetLocalScale(glm::vec3(route.DestCircleScale(), route.DestCircleScale(), 1.0f));
        }

        if (const auto stamp = clearedStamp_.get())
        {
            LoadingRouteMapSetLocalPos(stamp->Transform(), route.StampPosition());
        }

        // 向きが決まるまでは右向きで出す
        isShipFacingLeft_ = false;
        isShipSpriteLeft_ = false;
        shipFlipTween_.Stop();
        if (const auto ship = ship_.get())
            ship->SetSprite(shipRightSprite_.get());

        RestartDrawIns();
        ApplyElementVisibility();
        Tick(0.0f, 0.0f, 0.0f);
    }

    void LoadingRouteMap::SetShown(const bool isShown)
    {
        isShown_ = isShown;
        if (isShown)
            RestartDrawIns();
        ApplyElementVisibility();
        UpdateTrail(lastProgress01_, lastClockSecs_);
    }

    void LoadingRouteMap::Tick(const float progress01, const float clockSecs, const float deltaSecs)
    {
        const float progress = std::clamp(progress01, 0.0f, 1.0f);
        lastProgress01_ = progress;
        lastClockSecs_  = clockSecs;
        const RoutePoint point = isHover_ ? HoverAt(clockSecs) : RouteAt(progress);

        if (!isHover_)
            UpdateRouteDashes(progress, deltaSecs);

        UpdateShip(point, deltaSecs);
        UpdateTrail(progress, clockSecs);

        // 飛行船そのものより少し後ろを追うと、画面の中で飛行船が前へ出ていくように見える
        const glm::vec2 focus = isHover_ ? hoverCenter_ : RouteAt(std::max(0.0f, progress - cameraLag_)).position;
        UpdateCamera(focus, deltaSecs);
        UpdateClouds(clockSecs);
        UpdateDestCircle(deltaSecs);
        UpdateStamp(deltaSecs);
    }

    void LoadingRouteMap::BuildRouteSamples(const Asset::LoadingRouteData& route)
    {
        routeSamples_.clear();
        routeLengths_.clear();
        routeSamples_.reserve(LOADING_ROUTE_MAP_SAMPLE_COUNT + 1);
        routeLengths_.reserve(LOADING_ROUTE_MAP_SAMPLE_COUNT + 1);

        float length = 0.0f;
        for (int i = 0; i <= LOADING_ROUTE_MAP_SAMPLE_COUNT; ++i)
        {
            const glm::vec2 sample = LoadingRouteMapBezier(route, static_cast<float>(i) / LOADING_ROUTE_MAP_SAMPLE_COUNT);
            if (!routeSamples_.empty())
                length += glm::length(sample - routeSamples_.back());

            routeSamples_.push_back(sample);
            routeLengths_.push_back(length);
        }
    }

    LoadingRouteMap::RoutePoint LoadingRouteMap::RouteAt(const float progress01) const
    {
        if (routeSamples_.size() < 2)
            return { hoverCenter_, glm::vec2(1.0f, 0.0f) };

        const float target = std::clamp(progress01, 0.0f, 1.0f) * routeLengths_.back();
        const auto upper = std::lower_bound(routeLengths_.begin() + 1, routeLengths_.end(), target);
        const std::size_t index = std::min<std::size_t>(std::distance(routeLengths_.begin(), upper), routeSamples_.size() - 1);

        const glm::vec2& from = routeSamples_[index - 1];
        const glm::vec2& to   = routeSamples_[index];
        const float segment = std::max(routeLengths_[index] - routeLengths_[index - 1], 0.0001f);
        const float ratio = std::clamp((target - routeLengths_[index - 1]) / segment, 0.0f, 1.0f);
        return { from + (to - from) * ratio, to - from };
    }

    LoadingRouteMap::RoutePoint LoadingRouteMap::HoverAt(const float clockSecs) const
    {
        // 時計回りに楕円を回る。行き先の島が地図に無い遷移(タイトルへ戻る等)用
        const float angle = clockSecs / hoverLapSecs_ * 2.0f * std::numbers::pi_v<float>;
        const glm::vec2 position = hoverCenter_ + glm::vec2(std::cos(angle) * hoverRadius_.x, std::sin(angle) * hoverRadius_.y);
        const glm::vec2 tangent  = glm::vec2(-std::sin(angle) * hoverRadius_.x, std::cos(angle) * hoverRadius_.y);
        return { position, tangent };
    }

    void LoadingRouteMap::CapturePrefabBases()
    {
        // 雲は置いた位置を流れの基準に、飛行船・判・点線は置いた大きさを tween の等倍にする
        if (isPrefabBaseCaptured_)
            return;

        isPrefabBaseCaptured_ = true;
        if (const auto ship = ship_.get())
            shipBaseScale_ = ship->Transform().GetLocalScale();
        if (const auto stamp = clearedStamp_.get())
            stampBaseScale_ = stamp->Transform().GetLocalScale();

        dashBaseScales_.clear();
        for (const auto& field : routeDashes_)
        {
            const auto dash = field.get();
            dashBaseScales_.push_back(dash ? dash->Transform().GetLocalScale() : glm::vec3(1.0f));
        }

        const auto capture = [](const std::vector<FIELD(NanamiUi::BlendImageRenderer)>& clouds, std::vector<glm::vec2>& bases)
        {
            bases.clear();
            for (const auto& cloud : clouds)
            {
                const auto renderer = cloud.get();
                bases.push_back(renderer ? glm::vec2(renderer->Transform().GetLocalPos()) : glm::vec2(0.0f));
            }
        };
        capture(cloudShadows_, cloudShadowBases_);
        capture(frontClouds_, frontCloudBases_);
    }

    void LoadingRouteMap::StartSways()
    {
        LoadingRouteMapStartSway(shipBobTween_, LOADING_ROUTE_MAP_SHIP_BOB_SECS);
        LoadingRouteMapStartSway(cameraZoomSwayTween_, LOADING_ROUTE_MAP_ZOOM_SWAY_SECS);
        LoadingRouteMapStartSway(cameraPanSwayXTween_, LOADING_ROUTE_MAP_PAN_SWAY_X_SECS);
        LoadingRouteMapStartSway(cameraPanSwayYTween_, LOADING_ROUTE_MAP_PAN_SWAY_Y_SECS);
    }

    void LoadingRouteMap::LayoutRouteDashes()
    {
        const std::size_t count = routeDashes_.size();
        dashPassed_.assign(count, static_cast<char>(-1));
        dashPopTweens_.assign(count, TweenPlayer<float>());
        dashBaseScales_.resize(count, glm::vec3(1.0f));

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto dash = routeDashes_[i].get();
            if (!dash)
                continue;

            dash->Transform().SetLocalScale(dashBaseScales_[i]);
            if (isHover_)
                continue;

            // 線の両端は島の絵に掛かるので、端を少し残して等間隔に並べる
            const float at = (static_cast<float>(i) + 0.5f) / static_cast<float>(count);
            const RoutePoint point = RouteAt(at);
            LoadingRouteMapSetLocalPos(dash->Transform(), point.position);
            dash->Transform().SetLocalRot(LoadingRouteMapRotationZ(std::atan2(point.tangent.y, point.tangent.x)));
        }
    }

    void LoadingRouteMap::UpdateRouteDashes(const float progress01, const float deltaSecs)
    {
        const std::size_t count = std::min(routeDashes_.size(), dashPassed_.size());
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto dash = routeDashes_[i].get();
            if (!dash)
                continue;

            auto& pop = dashPopTweens_[i];
            const float at = (static_cast<float>(i) + 0.5f) / static_cast<float>(count);
            const char isPassed = at <= progress01 ? 1 : 0;
            if (dashPassed_[i] != isPassed)
            {
                // 通り過ぎた区間を赤インクでなぞる。差し替えは変わったときだけ
                // NOTE: 航路を出した直後(-1 から)は弾ませない
                if (isPassed && dashPassed_[i] == 0)
                {
                    pop.Play(tweeny::from(dashPopScale_).to(1.0f)
                        .during(Ms(dashPop_secs_)).via(Ease(EaseType::OutBack)));
                }
                dashPassed_[i] = isPassed;
                dash->SetSprite(isPassed ? dashPassedSprite_.get() : dashSprite_.get());
            }

            if (!pop.IsPlaying())
                continue;

            pop.Tick(deltaSecs);
            dash->Transform().SetLocalScale(dashBaseScales_[i] * LoadingRouteMapScaleRate(pop));
        }
    }

    void LoadingRouteMap::UpdateShip(const RoutePoint& point, const float deltaSecs)
    {
        const auto ship = ship_.get();
        if (!ship)
            return;

        // 左へ進むときは左向きの絵に替える。回転で裏返すと上下が逆さになるので、横に潰して裏返す
        if (std::abs(point.tangent.x) > 0.001f)
        {
            const bool isFacingLeft = point.tangent.x < 0.0f;
            if (isFacingLeft != isShipFacingLeft_)
            {
                isShipFacingLeft_ = isFacingLeft;
                const float halfSecs = shipFlip_secs_ * 0.5f;
                shipFlipTween_.Play(tweeny::from(1.0f)
                    .to(0.0f).during(Ms(halfSecs)).via(Ease(EaseType::InQuad))
                    .to(1.0f).during(Ms(halfSecs)).via(Ease(EaseType::OutQuad)));
            }
        }

        shipFlipTween_.Tick(deltaSecs);
        // 潰れ切るまでは元の向きの絵のまま
        const bool isSpriteLeft = shipFlipTween_.IsPlaying() && shipFlipTween_.Progress() < 0.5f
            ? !isShipFacingLeft_
            : isShipFacingLeft_;
        if (isSpriteLeft != isShipSpriteLeft_)
        {
            isShipSpriteLeft_ = isSpriteLeft;
            ship->SetSprite(isSpriteLeft ? shipLeftSprite_.get() : shipRightSprite_.get());
        }

        const glm::vec2 heading = isShipFacingLeft_ ? -point.tangent : point.tangent;
        const float tiltLimit = glm::radians(shipTiltLimitDeg_);
        const float tilt = std::clamp(std::atan2(heading.y, std::max(heading.x, 0.001f)), -tiltLimit, tiltLimit);
        const float bob = LoadingRouteMapTickSway(shipBobTween_, deltaSecs) * shipBobPx_;

        LoadingRouteMapSetLocalPos(ship->Transform(), point.position + glm::vec2(0.0f, -shipLiftPx_ + bob));
        ship->Transform().SetLocalRot(LoadingRouteMapRotationZ(tilt));
        // NOTE: 幅 0 の行列にしないよう、潰し切らずに少しだけ残す
        const float flipRate = std::max(LoadingRouteMapScaleRate(shipFlipTween_), 0.02f);
        ship->Transform().SetLocalScale(glm::vec3(shipBaseScale_.x * flipRate, shipBaseScale_.y, shipBaseScale_.z));

        if (const auto shadow = shipShadow_.get())
            LoadingRouteMapSetLocalPos(shadow->Transform(), point.position + shadowOffset_);
    }

    void LoadingRouteMap::UpdateTrail(const float progress01, const float clockSecs)
    {
        for (std::size_t i = 0; i < trail_.size(); ++i)
        {
            const auto puff = trail_[i].get();
            if (!puff)
                continue;

            const float offset = LOADING_ROUTE_MAP_TRAIL_STEP * static_cast<float>(i + 1);
            RoutePoint point;
            bool isVisible = true;
            if (isHover_)
            {
                point = HoverAt(clockSecs - offset * hoverLapSecs_);
            }
            else
            {
                isVisible = progress01 - offset > 0.0f;
                point = RouteAt(std::max(0.0f, progress01 - offset));
            }

            puff->SetEnable(isShown_ && isVisible);
            if (!isVisible)
                continue;

            LoadingRouteMapSetLocalPos(puff->Transform(), point.position + glm::vec2(0.0f, -shipLiftPx_ + 12.0f));
            puff->SetBlendRate(std::max(0, 130 - static_cast<int>(i) * 36));
        }
    }

    void LoadingRouteMap::UpdateCamera(const glm::vec2& focus, const float deltaSecs)
    {
        const float zoomSway = LoadingRouteMapTickSway(cameraZoomSwayTween_, deltaSecs);
        const glm::vec2 panSway = glm::vec2(
            LoadingRouteMapTickSway(cameraPanSwayXTween_, deltaSecs),
            LoadingRouteMapTickSway(cameraPanSwayYTween_, deltaSecs));

        const auto camera = camera_.get();
        if (!camera)
            return;

        // 画面中心を基準に拡大し、追う点が中心へ寄るようにずらす。机の端が見えないよう、ずらす量には上限を掛ける
        const float zoom = cameraZoom_ + zoomSway * cameraZoomWobble_;
        glm::vec2 pan = -zoom * cameraFollow_ * (focus - LOADING_ROUTE_MAP_SCREEN_CENTER);
        pan = glm::clamp(pan, -cameraMaxPan_, cameraMaxPan_);
        pan += panSway * LOADING_ROUTE_MAP_PAN_SWAY_PX;

        const glm::vec2 origin = LOADING_ROUTE_MAP_SCREEN_CENTER - zoom * LOADING_ROUTE_MAP_SCREEN_CENTER + pan;
        camera->Transform().SetLocalPos(glm::vec3(origin, 0.0f));
        camera->Transform().SetLocalScale(glm::vec3(zoom, zoom, 1.0f));
    }

    void LoadingRouteMap::UpdateClouds(const float clockSecs) const
    {
        const float wrapWidth = std::max(cloudWrapRangeX_.y - cloudWrapRangeX_.x, 1.0f);
        const auto drift = [this, clockSecs, wrapWidth](
            const std::vector<FIELD(NanamiUi::BlendImageRenderer)>& clouds,
            const std::vector<glm::vec2>& bases,
            const float speed)
        {
            for (std::size_t i = 0; i < clouds.size() && i < bases.size(); ++i)
            {
                const auto cloud = clouds[i].get();
                if (!cloud)
                    continue;

                // 一枚ずつ速さを変えて、同じ間隔のまま流れていかないようにする
                const float cloudSpeed = speed * (1.0f + 0.22f * static_cast<float>(i % 3));
                float x = bases[i].x - cloudWrapRangeX_.x + cloudDirection_ * cloudSpeed * clockSecs;
                x = std::fmod(x, wrapWidth);
                if (x < 0.0f)
                    x += wrapWidth;

                LoadingRouteMapSetLocalPos(cloud->Transform(), glm::vec2(cloudWrapRangeX_.x + x, bases[i].y));
            }
        };
        drift(cloudShadows_, cloudShadowBases_, cloudShadowSpeed_);
        drift(frontClouds_, frontCloudBases_, frontCloudSpeed_);
    }

    void LoadingRouteMap::RestartDrawIns()
    {
        // 〇は書き始めは速く、閉じるところで緩める。黒幕が明けるのを待ってから描く
        destCircleTween_.Play(tweeny::from(0.0f)
            .to(0.0f).during(Ms(destCircleDelay_secs_))
            .to(1.0f).during(Ms(destCircleDraw_secs_)).via(Ease(EaseType::OutCubic)));
        if (const auto destCircle = destCircle_.get())
            destCircle->SetFillRate(0.0f);

        // 判は〇を描き終えてから、大きく現れて押し込む
        const float stampWaitSecs = (hasDestCircle_ ? destCircleDelay_secs_ + destCircleDraw_secs_ : destCircleDelay_secs_)
                                  + stampDelay_secs_;
        stampTween_.Play(tweeny::from(0.0f)
            .to(0.0f).during(Ms(stampWaitSecs))
            .to(stampStartScale_).during(Ms(0.0f))
            .to(1.0f).during(Ms(stampPress_secs_)).via(Ease(EaseType::OutCubic)));
        isStampPressing_ = false;
    }

    void LoadingRouteMap::UpdateDestCircle(const float deltaSecs)
    {
        destCircleTween_.Tick(deltaSecs);

        const auto destCircle = destCircle_.get();
        if (!destCircle || !hasDestCircle_)
            return;

        destCircle->SetFillRate(destCircleTween_.Value());
    }

    void LoadingRouteMap::UpdateStamp(const float deltaSecs)
    {
        stampTween_.Tick(deltaSecs);

        const auto stamp = clearedStamp_.get();
        if (!stamp || !isStageCleared_)
            return;

        const float scaleRate = stampTween_.Value();
        const bool isPressing = scaleRate > 0.0f;
        if (isPressing != isStampPressing_)
        {
            isStampPressing_ = isPressing;
            if (isPressing)
                Sound::UiSoundBank::Play(Sound::UiSe::Stamp);
            ApplyElementVisibility();
        }

        if (isPressing)
            stamp->Transform().SetLocalScale(stampBaseScale_ * scaleRate);
    }

    void LoadingRouteMap::ApplyElementVisibility() const
    {
        if (const auto caption = fromCaptionText_.get())
            caption->SetEnable(isShown_ && hasFromCaption_);
        if (const auto caption = toCaptionText_.get())
            caption->SetEnable(isShown_ && hasToCaption_);
        if (const auto destCircle = destCircle_.get())
            destCircle->SetEnable(isShown_ && hasDestCircle_);
        if (const auto stamp = clearedStamp_.get())
            stamp->SetEnable(isShown_ && isStageCleared_ && isStampPressing_);

        for (const auto& field : routeDashes_)
        {
            if (const auto dash = field.get())
                dash->SetEnable(isShown_ && !isHover_);
        }
    }

    void LoadingRouteMap::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("camera_", camera_);
        ImGuiHelper::OnDrawInputField("ship_", ship_);
        ImGuiHelper::OnDrawInputField("shipRightSprite_", shipRightSprite_);
        ImGuiHelper::OnDrawInputField("shipLeftSprite_", shipLeftSprite_);
        ImGuiHelper::OnDrawInputField("shipShadow_", shipShadow_);
        ImGuiHelper::OnDrawInputField("trail_", trail_, [this]
        {
            if (ImGui::Button("Add##trail_"))
                trail_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("routeDashes_", routeDashes_, [this]
        {
            if (ImGui::Button("Add##routeDashes_"))
                routeDashes_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("dashSprite_", dashSprite_);
        ImGuiHelper::OnDrawInputField("dashPassedSprite_", dashPassedSprite_);
        ImGuiHelper::OnDrawInputField("destCircle_", destCircle_);
        ImGuiHelper::OnDrawInputField("clearedStamp_", clearedStamp_);
        ImGuiHelper::OnDrawInputField("cloudShadows_", cloudShadows_, [this]
        {
            if (ImGui::Button("Add##cloudShadows_"))
                cloudShadows_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("frontClouds_", frontClouds_, [this]
        {
            if (ImGui::Button("Add##frontClouds_"))
                frontClouds_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("kickerText_", kickerText_);
        ImGuiHelper::OnDrawInputField("titleText_", titleText_);
        ImGuiHelper::OnDrawInputField("fromCaptionText_", fromCaptionText_);
        ImGuiHelper::OnDrawInputField("toCaptionText_", toCaptionText_);
        ImGuiHelper::OnDrawInputField("cameraZoom_", cameraZoom_);
        ImGuiHelper::OnDrawInputField("cameraZoomWobble_", cameraZoomWobble_);
        ImGuiHelper::OnDrawInputField("cameraFollow_", cameraFollow_);
        ImGuiHelper::OnDrawInputField("cameraLag_", cameraLag_);
        ImGuiHelper::OnDrawInputField("cameraMaxPan_", cameraMaxPan_);
        ImGuiHelper::OnDrawInputField("shipLiftPx_", shipLiftPx_);
        ImGuiHelper::OnDrawInputField("shipBobPx_", shipBobPx_);
        ImGuiHelper::OnDrawInputField("shipTiltLimitDeg_", shipTiltLimitDeg_);
        ImGuiHelper::OnDrawInputField("shadowOffset_", shadowOffset_);
        ImGuiHelper::OnDrawInputField("cloudShadowSpeed_", cloudShadowSpeed_);
        ImGuiHelper::OnDrawInputField("frontCloudSpeed_", frontCloudSpeed_);
        ImGuiHelper::OnDrawInputField("cloudWrapRangeX_", cloudWrapRangeX_);
        ImGuiHelper::OnDrawInputField("destCircleDelay_secs_", destCircleDelay_secs_);
        ImGuiHelper::OnDrawInputField("destCircleDraw_secs_", destCircleDraw_secs_);
        ImGuiHelper::OnDrawInputField("stampDelay_secs_", stampDelay_secs_);
        ImGuiHelper::OnDrawInputField("stampPress_secs_", stampPress_secs_);
        ImGuiHelper::OnDrawInputField("stampStartScale_", stampStartScale_);
        ImGuiHelper::OnDrawInputField("dashPopScale_", dashPopScale_);
        ImGuiHelper::OnDrawInputField("dashPop_secs_", dashPop_secs_);
        ImGuiHelper::OnDrawInputField("shipFlip_secs_", shipFlip_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LoadingRouteMap);
#pragma endregion
