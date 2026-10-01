#include "TitleCameraDirector.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Title
{
    void TitleCameraDirector::OnStart()
    {
        shots_.clear();
        const auto root = shotsRoot_.get();
        if (!root)
            return;

        const auto children = root->Transform().GetChildren();
        for (std::size_t i = 0; i < children.size(); ++i)
        {
            const auto& child = children[i];
            const auto camera = child->Components().Catch<CineMachine::CineMachineVirtualCamera>().lock();
            if (!camera)
                continue;

            const auto ends = child->Transform().GetChildren();
            const auto endCamera = ends.empty() ? nullptr : ends.front()->Components().Catch<CineMachine::CineMachineVirtualCamera>().lock();

            Shot shot;
            shot.camera    = camera;
            shot.endCamera = endCamera ? endCamera : camera;
            shot.duration_secs = i < shotDurations_secs_.size() ? shotDurations_secs_[i] : defaultShotDuration_secs_;
            shot.duration_secs = std::max(shot.duration_secs, 0.5f);
            for (const auto& shotCamera : { camera, shot.endCamera.lock() })
            {
                shotCamera->SetImmediateApply(true);
                shotCamera->OnDisable();
            }
            shots_.push_back(shot);
        }

        ApplyDip(0.0f);
    }

    void TitleCameraDirector::OnUpdate()
    {
        if (shots_.empty())
            return;

        // NOTE: シーン切り替え直後は DeltaTime が 0 で凍っているので、動き出してから始める
        const float deltaSecs = Time::DeltaTime();
        if (!isStarted_)
        {
            BeginShot(0);
            isStarted_ = true;
        }

        elapsed_secs_ += deltaSecs;
        const Shot& shot = shots_[currentIndex_];
        if (elapsed_secs_ >= shot.duration_secs)
        {
            BeginShot((currentIndex_ + 1) % shots_.size());
            return;
        }

        // 終わり際に暗くなり、次のショットの出だしで明ける
        const float halfDip = dipDuration_secs_ * 0.5f;
        if (halfDip <= 0.0f)
            return;
        const float remain_secs = shot.duration_secs - elapsed_secs_;
        const float fadeOut = std::clamp(1.0f - remain_secs / halfDip, 0.0f, 1.0f);
        const float fadeIn  = std::clamp(1.0f - elapsed_secs_ / halfDip, 0.0f, 1.0f);
        ApplyDip(std::max(fadeOut, fadeIn));
    }

    void TitleCameraDirector::BeginShot(const std::size_t index)
    {
        if (const auto previous = shots_[currentIndex_].endCamera.lock())
            previous->OnDisable();

        currentIndex_ = index;
        elapsed_secs_ = 0.0f;
        const Shot& shot = shots_[currentIndex_];
        const auto camera    = shot.camera.lock();
        const auto endCamera = shot.endCamera.lock();
        if (!camera || !endCamera)
            return;

        // 始めの姿勢へは切り、終わりのカメラへはショットの長さをかけて補間する。動き出しと止まり際を緩めてクレーンのように見せる
        if (auto* brain = CineMachine::CinemachineCameraBrain::Instance())
            brain->SnapToVirtualCamera(*camera);
        endCamera->SetBlendIn(shot.duration_secs, LibCore::EaseType::InOutSine);
        endCamera->SetPriority(shotPriority_);
        ApplyDip(dipDuration_secs_ > 0.0f ? 1.0f : 0.0f);
    }

    void TitleCameraDirector::ApplyDip(const float rate) const
    {
        if (const auto mask = dipMask_.get())
            mask->SetBlendRate(std::clamp(static_cast<int>(255.0f * rate), 0, 255));
    }

    void TitleCameraDirector::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("shotsRoot_", shotsRoot_);
        ImGuiHelper::OnDrawInputField("dipMask_", dipMask_);
        ImGuiHelper::OnDrawInputField("shotDurations_secs_", shotDurations_secs_, [this]
        {
            if (ImGui::Button("Add"))
                shotDurations_secs_.push_back(defaultShotDuration_secs_);
        });
        ImGuiHelper::OnDrawInputField("defaultShotDuration_secs_", defaultShotDuration_secs_);
        ImGuiHelper::OnDrawInputField("dipDuration_secs_", dipDuration_secs_);
        ImGuiHelper::OnDrawInputField("shotPriority_", shotPriority_);
        ImGui::Text("shot: %d / %d  elapsed: %.2f", static_cast<int>(currentIndex_), static_cast<int>(shots_.size()), elapsed_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Title::TitleCameraDirector);
#pragma endregion
