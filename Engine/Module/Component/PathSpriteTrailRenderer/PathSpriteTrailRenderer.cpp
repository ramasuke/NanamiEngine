#include "PathSpriteTrailRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "glm.hpp"
#include "../../../Core/Application/Time/Time.h"
#include "../../../Core/Platform/Draw2D/Draw2D.h"
#include "../../../Core/Platform/Render/Billboard.h"
#include "../../../Core/Platform/Render/Shader.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Component
{
    namespace
    {
        constexpr float TAU = std::numbers::pi_v<float> * 2.0f;

        float SmoothStep(const float edge0, const float edge1, const float x)
        {
            const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }
    }

    void PathSpriteTrailRenderer::SetPath(const std::span<const glm::vec3> points)
    {
        path_.assign(points.begin(), points.end());
        pathLength_ = 0.0f;
        for (std::size_t i = 1; i < path_.size(); ++i)
            pathLength_ += glm::length(path_[i] - path_[i - 1]);
    }

    void PathSpriteTrailRenderer::ClearPath()
    {
        path_.clear();
        pathLength_ = 0.0f;
    }

    void PathSpriteTrailRenderer::SetVisibility(const float visibility)
    {
        visibility_ = std::clamp(visibility, 0.0f, 1.0f);
    }

    void PathSpriteTrailRenderer::OnUpdate()
    {
        time_secs_ += Time::DeltaTime();
    }

    void PathSpriteTrailRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || visibility_ <= 0.0f || !sprite_ || path_.size() < 2 || spacing_ <= 0.0f || trailLength_ <= 0.0f)
            return;

        const Platform::Draw2D::ScopedDrawState drawState;
        // NOTE: 加算の光が互いや手前の粒を消さないよう、奥行きは見るが書かない
        Platform::Render::RenderState::SetZBufferWrite(false);

        const int   handle  = sprite_->GetDxLibHandle();
        const float travel  = time_secs_ * flowSpeed_;
        const float wrapped = std::fmod(travel, spacing_);
        const int   passed  = static_cast<int>(std::floor(travel / spacing_));
        const int   count   = static_cast<int>(trailLength_ / spacing_);

        // NOTE: 折れ線を前から歩き、distance の点を求める(distance は増える一方なので区間を持ち越す)
        std::size_t segment      = 1;
        float       segmentStart = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            const float distance = startOffset_ + static_cast<float>(i) * spacing_ + wrapped;
            if (distance >= pathLength_)
                break;

            float segmentLength = glm::length(path_[segment] - path_[segment - 1]);
            while (segmentStart + segmentLength < distance && segment + 1 < path_.size())
            {
                segmentStart += segmentLength;
                ++segment;
                segmentLength = glm::length(path_[segment] - path_[segment - 1]);
            }
            const float along = segmentLength > 0.0f ? (distance - segmentStart) / segmentLength : 0.0f;
            glm::vec3 position = glm::mix(path_[segment - 1], path_[segment], std::clamp(along, 0.0f, 1.0f));

            // NOTE: 粒ごとに揺れ方を変える。列が流れても同じ粒は同じ揺れを続けるよう、流れた数で番号を振り直す
            const float id    = static_cast<float>(i - passed);
            const float phase = id * 1.7f + (driftPeriod_secs_ > 0.0f ? TAU * time_secs_ / driftPeriod_secs_ : 0.0f);
            position += glm::vec3(std::sin(phase), 0.6f * std::sin(phase * 1.3f) + 1.0f, std::cos(phase * 0.8f)) * driftAmplitude_;
            position.y += lift_;

            const float rate    = (distance - startOffset_) / trailLength_;
            const float fade    = SmoothStep(0.0f, 0.12f, rate) * (1.0f - SmoothStep(0.55f, 1.0f, rate));
            const float endFade = std::clamp((pathLength_ - distance) / (spacing_ * 2.0f), 0.0f, 1.0f);
            const float twinkle = 0.75f + 0.25f * std::sin(time_secs_ * 5.0f + id * 2.1f);
            const float alpha   = visibility_ * alphaRate_ * fade * endFade * twinkle;
            if (alpha <= 0.0f)
                continue;

            Platform::Draw2D::SetBlendModeAlpha(blendMode_, alpha);
            Platform::Render::Billboard::Draw(position, size_, 0.0f, handle);
        }

        Platform::Render::RenderState::SetZBufferWrite(true);
    }

    void PathSpriteTrailRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("sprite_", sprite_);
        int mode = static_cast<int>(blendMode_);
        if (ImGui::Combo("blendMode_", &mode, LibCore::Dxlib::BlendModeLabelNames, IM_ARRAYSIZE(LibCore::Dxlib::BlendModeLabelNames)))
        {
            blendMode_ = static_cast<LibCore::Dxlib::BlendMode>(mode);
        }
        ImGuiHelper::OnDrawInputField("trailLength_", trailLength_);
        ImGuiHelper::OnDrawInputField("startOffset_", startOffset_);
        ImGuiHelper::OnDrawInputField("spacing_", spacing_);
        ImGuiHelper::OnDrawInputField("lift_", lift_);
        ImGuiHelper::OnDrawInputField("size_", size_);
        ImGuiHelper::OnDrawInputField("flowSpeed_", flowSpeed_);
        ImGuiHelper::OnDrawInputField("driftAmplitude_", driftAmplitude_);
        ImGuiHelper::OnDrawInputField("driftPeriod_secs_", driftPeriod_secs_);
        ImGuiHelper::OnDrawInputField("alphaRate_", alphaRate_);

        ImGui::Text("path points: %zu, length: %.1f, visibility: %.2f", path_.size(), pathLength_, visibility_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::PathSpriteTrailRenderer);
#pragma endregion
