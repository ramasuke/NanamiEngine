#include "Ui_LockOnReticle.h"

#include <algorithm>
#include <cmath>

#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Core/Game/PlayerAvatar/CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "../../../Core/Game/PlayerAvatar/LockOnTarget/ILockOnTarget.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float PI = 3.14159265f;

        constexpr float CANDIDATE_FADE_SECS          = 0.15f;
        constexpr float CANDIDATE_PULSE_PERIOD_SECS  = 1.4f;
        constexpr float LOCKED_BREATH_PERIOD_SECS    = 1.6f;
        // 確定演出でブラケットが回りながらスナップしてくる角度
        constexpr float ENGAGE_BRACKET_ANGLE         = PI * 0.25f;
        constexpr float MIN_SCALE_RATE               = 0.001f;

        float EaseOutBack(const float t)
        {
            constexpr float overshoot = 1.70158f;
            const float x = t - 1.0f;
            return x * x * ((overshoot + 1.0f) * x + overshoot) + 1.0f;
        }

        float EaseOutQuad(const float t)
        {
            return 1.0f - (1.0f - t) * (1.0f - t);
        }
    }

    void LockOnReticle::InitRenderer()
    {
    }

    std::shared_ptr<GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase> LockOnReticle::CatchCameraGroup()
    {
        if (const auto cameraGroup = cameraGroup_.lock())
            return cameraGroup;

        const auto parent = Transform().GetParent();
        if (!parent)
            return nullptr;

        cameraGroup_ = parent->Components().Catch<GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase>();
        return cameraGroup_.lock();
    }

    void LockOnReticle::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();
        elapsed_secs_ += deltaTime;
        ringAngle_    += ringRotateSpeed_ * deltaTime;

        const auto cameraGroup = CatchCameraGroup();
        const bool isLockedOn  = cameraGroup && cameraGroup->IsLockedOn();

        // 対象が死亡して weak_ptr が切れた場合も、ロック解除と同じく解除演出にする。部位や別の敵へ切り替えたら確定演出をやり直す
        const auto target     = isLockedOn ? cameraGroup->LockOnAim() : nullptr;
        const bool isEngaged  = target != nullptr;
        if (isEngaged && (!wasEngaged_ || target != lockedTarget_.lock()))
        {
            phase_          = Phase::Engaging;
            phaseTime_secs_ = 0.0f;
            lockedTarget_   = target;
        }
        else if (!isEngaged && wasEngaged_)
        {
            // lockedTarget_ は残し、生きていれば解除演出中も対象に追従させる
            phase_          = Phase::Releasing;
            phaseTime_secs_ = 0.0f;
        }
        wasEngaged_ = isEngaged;

        phaseTime_secs_ += deltaTime;
        if (phase_ == Phase::Engaging && phaseTime_secs_ >= engageDuration_secs_)
            phase_ = Phase::Locked;
        if (phase_ == Phase::Releasing && phaseTime_secs_ >= releaseDuration_secs_)
        {
            phase_ = Phase::Hidden;
            lockedTarget_.reset();
        }

        const auto candidate = cameraGroup && !isLockedOn ? cameraGroup->LockOnCandidate().lock() : nullptr;
        if (candidate)
            candidateTarget_ = candidate;

        // 候補が消えた後も、最後の位置でフェードアウトさせる
        const float fadeStep = CANDIDATE_FADE_SECS > 0.0f ? deltaTime / CANDIDATE_FADE_SECS : 1.0f;
        candidateFade_ = candidate
            ? std::min(1.0f, candidateFade_ + fadeStep)
            : std::max(0.0f, candidateFade_ - fadeStep);
    }

    void LockOnReticle::OnUserInterfaceRender()
    {
        if (!IsEnable())
            return;

        // 対象の位置は全ての Update が終わった描画時点で取る（Update 順による1フレーム遅れを避ける）
        if (candidateFade_ > 0.0f)
        {
            if (const auto candidate = candidateTarget_.lock())
                candidatePointWorld_ = GameCore::PlayerAvatar::LockOnPositionOf(*candidate);

            const float pulse = 0.75f + 0.25f * std::sin(elapsed_secs_ * 2.0f * PI / CANDIDATE_PULSE_PERIOD_SECS);
            DrawSprite(
                candidateSprite_.get(),
                candidatePointWorld_,
                candidateScale_,
                0.0f,
                EaseOutQuad(candidateFade_) * candidateAlpha_ * pulse);
        }

        if (phase_ == Phase::Hidden)
            return;

        if (const auto target = lockedTarget_.lock())
            lockOnPointWorld_ = GameCore::PlayerAvatar::LockOnPositionOf(*target);

        float scaleRate    = 1.0f;
        float alpha        = 1.0f;
        float bracketAngle = 0.0f;
        float breathRate   = 1.0f;
        switch (phase_)
        {
        case Phase::Engaging:
        {
            const float t = engageDuration_secs_ > 0.0f ? std::clamp(phaseTime_secs_ / engageDuration_secs_, 0.0f, 1.0f) : 1.0f;
            scaleRate    = engageStartScaleRate_ + (1.0f - engageStartScaleRate_) * EaseOutBack(t);
            alpha        = EaseOutQuad(t);
            bracketAngle = ENGAGE_BRACKET_ANGLE * (1.0f - EaseOutBack(t));
            break;
        }
        case Phase::Locked:
            breathRate = 1.0f + lockedBreathScale_ * (0.5f - 0.5f * std::cos(elapsed_secs_ * 2.0f * PI / LOCKED_BREATH_PERIOD_SECS));
            break;
        case Phase::Releasing:
        {
            const float t = releaseDuration_secs_ > 0.0f ? std::clamp(phaseTime_secs_ / releaseDuration_secs_, 0.0f, 1.0f) : 1.0f;
            scaleRate = 1.0f + (releaseEndScaleRate_ - 1.0f) * EaseOutQuad(t);
            alpha     = 1.0f - EaseOutQuad(t);
            break;
        }
        case Phase::Hidden:
            return;
        }

        const float scale = reticleScale_ * std::max(scaleRate, MIN_SCALE_RATE);
        DrawSprite(ringSprite_   .get(), lockOnPointWorld_, scale,              ringAngle_,   alpha);
        DrawSprite(bracketSprite_.get(), lockOnPointWorld_, scale * breathRate, bracketAngle, alpha);
    }

    void LockOnReticle::DrawSprite(
        const std::shared_ptr<Asset::SpriteFile>& sprite,
        const glm::vec3& worldPos,
        const float scale,
        const float angle,
        const float alpha)
    {
        if (!sprite || alpha <= 0.0f || scale <= 0.0f)
            return;

        const VECTOR screenPos = ConvWorldPosToScreenPos(VGet(worldPos.x, worldPos.y, worldPos.z));
        // z が 0..1 の外ならカメラの視界外（背後など）
        if (screenPos.z < 0.0f || screenPos.z > 1.0f)
            return;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f));
        DrawRotaGraphF(screenPos.x, screenPos.y, scale, angle, sprite->GetDxLibHandle(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    }

    void LockOnReticle::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("renderOrder_",          renderOrder_);
        ImGuiHelper::OnDrawInputField("ringSprite_",           ringSprite_);
        ImGuiHelper::OnDrawInputField("bracketSprite_",        bracketSprite_);
        ImGuiHelper::OnDrawInputField("candidateSprite_",      candidateSprite_);
        ImGuiHelper::OnDrawInputField("reticleScale_",         reticleScale_);
        ImGuiHelper::OnDrawInputField("candidateScale_",       candidateScale_);
        ImGuiHelper::OnDrawInputField("candidateAlpha_",       candidateAlpha_);
        ImGuiHelper::OnDrawInputField("ringRotateSpeed_",      ringRotateSpeed_);
        ImGuiHelper::OnDrawInputField("lockedBreathScale_",    lockedBreathScale_);
        ImGuiHelper::OnDrawInputField("engageDuration_secs_",  engageDuration_secs_);
        ImGuiHelper::OnDrawInputField("engageStartScaleRate_", engageStartScaleRate_);
        ImGuiHelper::OnDrawInputField("releaseDuration_secs_", releaseDuration_secs_);
        ImGuiHelper::OnDrawInputField("releaseEndScaleRate_",  releaseEndScaleRate_);    }
}
