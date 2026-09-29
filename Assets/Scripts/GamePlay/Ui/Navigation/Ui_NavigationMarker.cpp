#include "Ui_NavigationMarker.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <numbers>

#include "glm.hpp"
#include "Ui_NavigationMemory.h"
#include "Ui_NavigationPresenter.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Platform/Draw2D/Draw2D.h"
#include "Engine/Core/Platform/Render/Camera.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        using LibCore::Dxlib::BlendMode;
        namespace Camera = Platform::Render::Camera;

        constexpr float TAU = std::numbers::pi_v<float> * 2.0f;

        /** @return 目的地の方向を画面上の向き(右が +x、下が +y)にしたもの。カメラの背後でも向きは保つ */
        glm::vec2 ScreenDirection(const glm::vec3& worldPos, const glm::vec2& screenCentre)
        {
            // NOTE: 背後の点は WorldToScreen の x,y が当てにならないので、画面中央の右・下へ向かうワールドの向きに射影する
            const glm::vec3 origin = Camera::ScreenToWorld(glm::vec3(screenCentre, 0.5f));
            const glm::vec3 right  = Camera::ScreenToWorld(glm::vec3(screenCentre + glm::vec2(100.0f, 0.0f), 0.5f)) - origin;
            const glm::vec3 down   = Camera::ScreenToWorld(glm::vec3(screenCentre + glm::vec2(0.0f, 100.0f), 0.5f)) - origin;
            const glm::vec3 toward = worldPos - Camera::Position();

            glm::vec2 direction(0.0f, 1.0f);
            if (glm::length(right) > 0.0f && glm::length(down) > 0.0f)
                direction = glm::vec2(glm::dot(toward, glm::normalize(right)), glm::dot(toward, glm::normalize(down)));
            if (glm::length(direction) < 1e-4f)
                return glm::vec2(0.0f, 1.0f);
            return glm::normalize(direction);
        }
    }

    void NavigationMarker::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();
        time_secs_ += deltaTime;

        const auto& current = NavigationMemory::Instance().Current();
        const auto targetPosition = current ? current->TargetPosition() : std::nullopt;
        const auto player = GameCore::PlayerAvatar::Owner();

        bool isShown = targetPosition && player && !NavigationPresenter::IsQuiet();
        if (isShown)
        {
            distance_ = glm::length(*targetPosition - player->PlayerTransform().GetWorldPos());
            isShown = distance_ > hideDistance_;
        }

        const float step = fade_secs_ > 0.0f ? deltaTime / fade_secs_ : 1.0f;
        visibility_ = std::clamp(visibility_ + (isShown ? step : -step), 0.0f, 1.0f);
    }

    std::string NavigationMarker::DistanceText(const float distance) const
    {
        return std::format("{}m", static_cast<int>(std::round(distance * metersPerUnit_)));
    }

    void NavigationMarker::DrawOrb(const glm::vec2& position, const float scale, const float alpha) const
    {
        if (!orbSprite_)
            return;

        Platform::Draw2D::SetBlendModeAlpha(BlendMode::Add, alpha);
        Platform::Draw2D::DrawRotaGraph(position, scale, 0.0, orbSprite_->GetDxLibHandle());
    }

    void NavigationMarker::DrawEdge(const glm::vec2& position, const glm::vec2& direction, const float alpha) const
    {
        if (!fireflySprite_)
            return;

        // 先頭(画面の縁)ほど明るく、光が内から外へ流れて見えるように位相をずらす
        const float phase = edgeFlowPeriod_secs_ > 0.0f ? time_secs_ / edgeFlowPeriod_secs_ : 0.0f;
        const int count = std::max(edgeFireflyCount_, 1);
        for (int i = 0; i < count; ++i)
        {
            const float rank    = static_cast<float>(i) / static_cast<float>(count);
            const float flicker = 0.7f + 0.3f * std::sin(TAU * (phase + rank));
            const glm::vec2 at  = position - direction * (edgeFireflySpacing_px_ * static_cast<float>(i));

            Platform::Draw2D::SetBlendModeAlpha(BlendMode::Add, alpha * (1.0f - rank * 0.6f) * flicker);
            Platform::Draw2D::DrawRotaGraph(at, edgeFireflyScale_ * (1.0f - rank * 0.3f), 0.0, fireflySprite_->GetDxLibHandle());
        }
    }

    void NavigationMarker::DrawLabel(const glm::vec2& position, const std::string& name, const std::string& distance, const bool alignRight, const float alpha) const
    {
        const int fontHandle = font_->DxLibHandle();
        const auto drawLine = [&](const std::string& text, const glm::vec2& at, const float scale, const Color32& color)
        {
            if (text.empty())
                return;
            const float width = static_cast<float>(Platform::Draw2D::StringWidth(scale, text, fontHandle));
            const glm::vec2 origin(alignRight ? at.x - width : at.x, at.y);
            Platform::Draw2D::SetBlendModeAlpha(BlendMode::Alpha, alpha);
            Platform::Draw2D::DrawString(origin, glm::vec2(scale, scale), text, color, fontHandle, font_->EdgeColor());
        };

        drawLine(name, position, nameScale_, nameColor_);
        drawLine(distance, position + glm::vec2(0.0f, lineGap_px_), distanceScale_, distanceColor_);
    }

    void NavigationMarker::OnUserInterfaceRender()
    {
        if (!IsEnable() || visibility_ <= 0.0f || !font_)
            return;

        const auto& current = NavigationMemory::Instance().Current();
        const auto markerPosition = current ? current->MarkerPosition() : std::nullopt;
        if (!markerPosition)
            return;

        const float nearRate = fadeDistance_ > 0.0f ? std::clamp((distance_ - hideDistance_) / fadeDistance_, 0.0f, 1.0f) : 1.0f;
        const float alpha    = visibility_ * nearRate;
        if (alpha <= 0.0f)
            return;

        const Platform::Draw2D::ScopedDrawState drawState;
        Platform::Draw2D::SetFilterMode(Platform::Draw2D::FilterMode::Bilinear);

        const glm::vec2 screen = glm::vec2(Platform::Draw2D::ScreenSize());
        const glm::vec2 centre = screen * 0.5f;
        const glm::vec3 projected = Camera::WorldToScreen(*markerPosition);
        const bool isOnScreen = projected.z > 0.0f && projected.z < 1.0f
            && projected.x >= edgeMargin_px_ && projected.x <= screen.x - edgeMargin_px_
            && projected.y >= edgeMargin_px_ && projected.y <= screen.y - edgeMargin_px_;
        const std::string distanceText = DistanceText(distance_);

        if (isOnScreen)
        {
            const float cameraDistance = std::max(glm::length(*markerPosition - Camera::Position()), 1.0f);
            const float pulse = orbPulsePeriod_secs_ > 0.0f ? std::sin(TAU * time_secs_ / orbPulsePeriod_secs_) : 0.0f;
            const float scale = std::clamp(orbReferenceDistance_ / cameraDistance, orbMinScale_, orbMaxScale_) * (1.0f + orbPulseRate_ * pulse);
            const glm::vec2 at(projected.x, projected.y);

            DrawOrb(at, scale, alpha);
            DrawLabel(at + labelOffset_px_, current->label, distanceText, false, alpha);
            return;
        }

        // 画面外: 目的地の向きへ伸ばした線が、縁から edgeMargin_px_ 内側の枠に当たる所
        const glm::vec2 direction = ScreenDirection(*markerPosition, centre);
        const glm::vec2 half = centre - glm::vec2(edgeMargin_px_);
        const float reachX = std::abs(direction.x) > 1e-4f ? half.x / std::abs(direction.x) : std::numeric_limits<float>::max();
        const float reachY = std::abs(direction.y) > 1e-4f ? half.y / std::abs(direction.y) : std::numeric_limits<float>::max();
        const glm::vec2 edge = centre + direction * std::min(reachX, reachY);

        DrawEdge(edge, direction, alpha);

        // 添え書きは蛍の列の内側。右の縁なら右揃え
        const float inset = edgeFireflySpacing_px_ * static_cast<float>(std::max(edgeFireflyCount_, 1)) + 12.0f;
        const glm::vec2 labelAt = edge - direction * inset + glm::vec2(0.0f, -lineGap_px_ * 0.5f);
        DrawLabel(labelAt, current->label, distanceText, direction.x > 0.3f, alpha);
    }

    void NavigationMarker::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("orbSprite_", orbSprite_);
        ImGuiHelper::OnDrawInputField("fireflySprite_", fireflySprite_);
        ImGuiHelper::OnDrawInputField("font_", font_);
        ImGuiHelper::OnDrawInputField("metersPerUnit_", metersPerUnit_);
        ImGuiHelper::OnDrawInputField("hideDistance_", hideDistance_);
        ImGuiHelper::OnDrawInputField("fadeDistance_", fadeDistance_);
        ImGuiHelper::OnDrawInputField("orbReferenceDistance_", orbReferenceDistance_);
        ImGuiHelper::OnDrawInputField("orbMinScale_", orbMinScale_);
        ImGuiHelper::OnDrawInputField("orbMaxScale_", orbMaxScale_);
        ImGuiHelper::OnDrawInputField("orbPulsePeriod_secs_", orbPulsePeriod_secs_);
        ImGuiHelper::OnDrawInputField("orbPulseRate_", orbPulseRate_);
        ImGuiHelper::OnDrawInputField("labelOffset_px_", labelOffset_px_);
        ImGuiHelper::OnDrawInputField("nameScale_", nameScale_);
        ImGuiHelper::OnDrawInputField("distanceScale_", distanceScale_);
        ImGuiHelper::OnDrawInputField("lineGap_px_", lineGap_px_);
        ImGuiHelper::OnDrawInputField("nameColor_", nameColor_);
        ImGuiHelper::OnDrawInputField("distanceColor_", distanceColor_);
        ImGuiHelper::OnDrawInputField("edgeMargin_px_", edgeMargin_px_);
        ImGuiHelper::OnDrawInputField("edgeFireflyCount_", edgeFireflyCount_);
        ImGuiHelper::OnDrawInputField("edgeFireflySpacing_px_", edgeFireflySpacing_px_);
        ImGuiHelper::OnDrawInputField("edgeFireflyScale_", edgeFireflyScale_);
        ImGuiHelper::OnDrawInputField("edgeFlowPeriod_secs_", edgeFlowPeriod_secs_);
        ImGuiHelper::OnDrawInputField("fade_secs_", fade_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::NavigationMarker);
#pragma endregion
