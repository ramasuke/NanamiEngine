#include "Ui_LoadingRouteMap.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

#include "Engine/Module/GameObject/Transform/Transform.h"

namespace
{
    /** ロード画面の配置の基準にしている画面の大きさ */
    constexpr glm::vec2 LOADING_ROUTE_MAP_SCREEN_CENTER = glm::vec2(960.0f, 540.0f);
    constexpr int LOADING_ROUTE_MAP_SAMPLE_COUNT = 128;
    /** 飛行船の後ろに並べる煙の、進み具合でのずらし幅 */
    constexpr float LOADING_ROUTE_MAP_TRAIL_STEP = 0.035f;

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
        CaptureCloudBases();

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
        if (const auto ship = ship_.get())
            ship->SetSprite(shipRightSprite_.get());

        ApplyElementVisibility();
        Tick(0.0f, 0.0f);
    }

    void LoadingRouteMap::SetShown(const bool isShown)
    {
        isShown_ = isShown;
        ApplyElementVisibility();
        UpdateTrail(lastProgress01_, lastClockSecs_);
    }

    void LoadingRouteMap::Tick(const float progress01, const float clockSecs)
    {
        const float progress = std::clamp(progress01, 0.0f, 1.0f);
        lastProgress01_ = progress;
        lastClockSecs_  = clockSecs;
        const RoutePoint point = isHover_ ? HoverAt(clockSecs) : RouteAt(progress);

        if (!isHover_)
            UpdateRouteDashes(progress);

        UpdateShip(point, clockSecs);
        UpdateTrail(progress, clockSecs);

        // 飛行船そのものより少し後ろを追うと、画面の中で飛行船が前へ出ていくように見える
        const glm::vec2 focus = isHover_ ? hoverCenter_ : RouteAt(std::max(0.0f, progress - cameraLag_)).position;
        UpdateCamera(focus, clockSecs);
        UpdateClouds(clockSecs);
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

    void LoadingRouteMap::CaptureCloudBases()
    {
        // prefab に置いた位置を流れの基準にする。動かし始める前に一度だけ覚える
        if (isCloudBaseCaptured_)
            return;

        isCloudBaseCaptured_ = true;
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

    void LoadingRouteMap::LayoutRouteDashes()
    {
        const std::size_t count = routeDashes_.size();
        dashPassed_.assign(count, static_cast<char>(-1));

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto dash = routeDashes_[i].get();
            if (!dash)
                continue;

            if (isHover_)
                continue;

            // 線の両端は島の絵に掛かるので、端を少し残して等間隔に並べる
            const float at = (static_cast<float>(i) + 0.5f) / static_cast<float>(count);
            const RoutePoint point = RouteAt(at);
            LoadingRouteMapSetLocalPos(dash->Transform(), point.position);
            dash->Transform().SetLocalRot(LoadingRouteMapRotationZ(std::atan2(point.tangent.y, point.tangent.x)));
        }
    }

    void LoadingRouteMap::UpdateRouteDashes(const float progress01)
    {
        const std::size_t count = routeDashes_.size();
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto dash = routeDashes_[i].get();
            if (!dash)
                continue;

            const float at = (static_cast<float>(i) + 0.5f) / static_cast<float>(count);
            const char isPassed = at <= progress01 ? 1 : 0;
            if (dashPassed_[i] == isPassed)
                continue;

            // 通り過ぎた区間を赤インクでなぞる。差し替えは変わったときだけ
            dashPassed_[i] = isPassed;
            dash->SetSprite(isPassed ? dashPassedSprite_.get() : dashSprite_.get());
        }
    }

    void LoadingRouteMap::UpdateShip(const RoutePoint& point, const float clockSecs)
    {
        const auto ship = ship_.get();
        if (!ship)
            return;

        // 左へ進むときは左向きの絵に替える。回転で裏返すと上下が逆さになる
        if (std::abs(point.tangent.x) > 0.001f)
        {
            const bool isFacingLeft = point.tangent.x < 0.0f;
            if (isFacingLeft != isShipFacingLeft_)
            {
                isShipFacingLeft_ = isFacingLeft;
                ship->SetSprite(isFacingLeft ? shipLeftSprite_.get() : shipRightSprite_.get());
            }
        }

        const glm::vec2 heading = isShipFacingLeft_ ? -point.tangent : point.tangent;
        const float tiltLimit = glm::radians(shipTiltLimitDeg_);
        const float tilt = std::clamp(std::atan2(heading.y, std::max(heading.x, 0.001f)), -tiltLimit, tiltLimit);
        const float bob = std::sin(clockSecs * 2.4f) * shipBobPx_;

        LoadingRouteMapSetLocalPos(ship->Transform(), point.position + glm::vec2(0.0f, -shipLiftPx_ + bob));
        ship->Transform().SetLocalRot(LoadingRouteMapRotationZ(tilt));

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

    void LoadingRouteMap::UpdateCamera(const glm::vec2& focus, const float clockSecs) const
    {
        const auto camera = camera_.get();
        if (!camera)
            return;

        // 画面中心を基準に拡大し、追う点が中心へ寄るようにずらす。机の端が見えないよう、ずらす量には上限を掛ける
        const float zoom = cameraZoom_ + std::sin(clockSecs * 0.9f) * cameraZoomWobble_;
        glm::vec2 pan = -zoom * cameraFollow_ * (focus - LOADING_ROUTE_MAP_SCREEN_CENTER);
        pan = glm::clamp(pan, -cameraMaxPan_, cameraMaxPan_);
        pan += glm::vec2(std::sin(clockSecs * 0.7f) * 4.0f, std::sin(clockSecs * 1.1f) * 3.0f);

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

    void LoadingRouteMap::ApplyElementVisibility() const
    {
        if (const auto caption = fromCaptionText_.get())
            caption->SetEnable(isShown_ && hasFromCaption_);
        if (const auto caption = toCaptionText_.get())
            caption->SetEnable(isShown_ && hasToCaption_);
        if (const auto destCircle = destCircle_.get())
            destCircle->SetEnable(isShown_ && hasDestCircle_);
        if (const auto stamp = clearedStamp_.get())
            stamp->SetEnable(isShown_ && isStageCleared_);

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
    }
}
