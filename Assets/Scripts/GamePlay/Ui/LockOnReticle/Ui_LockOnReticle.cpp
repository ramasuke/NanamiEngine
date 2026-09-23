#include "Ui_LockOnReticle.h"

#include <algorithm>
#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../Core/Game/PlayerAvatar/CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "../../../Core/Game/PlayerAvatar/LockOnTarget/ILockOnTarget.h"
#include "../../Sound/UiSoundBank.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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
    }

    LockOnReticle::LockOnReticle()
    {
        // 候補マーカーは一定速度で 0<->1 を往復する
        candidateFade_.Set(tweeny::from(0.0f).to(1.0f)
            .during(LibCore::Tween::Ms(CANDIDATE_FADE_SECS))
            .via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
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

        // 対象が死亡して weak_ptr が切れた場合も、ロック解除と同じく解除演出にする。別の敵へ切り替えたら確定演出をやり直す
        const auto target     = isLockedOn ? cameraGroup->LockOnTarget().lock() : nullptr;
        const bool isEngaged  = target != nullptr;
        if (isEngaged && (!wasEngaged_ || target != lockedTarget_.lock()))
        {
            phase_        = Phase::Engaging;
            lockedTarget_ = target;
            PlayEngage();
            Sound::UiSoundBank::Play(Sound::UiSe::HudLockOn);
        }
        else if (!isEngaged && wasEngaged_)
        {
            // lockedTarget_ は残し、生きていれば解除演出中も対象に追従させる
            phase_ = Phase::Releasing;
            PlayRelease();
            Sound::UiSoundBank::Play(Sound::UiSe::HudLockOff);
        }
        wasEngaged_ = isEngaged;

        const bool isPhaseFinished = scaleRateTween_.Tick(deltaTime);
        alphaTween_       .Tick(deltaTime);
        bracketAngleTween_.Tick(deltaTime);
        if (isPhaseFinished && phase_ == Phase::Engaging)
            phase_ = Phase::Locked;
        if (isPhaseFinished && phase_ == Phase::Releasing)
        {
            phase_ = Phase::Hidden;
            lockedTarget_.reset();
        }

        const auto candidate = cameraGroup && !isLockedOn ? cameraGroup->LockOnCandidate().lock() : nullptr;
        if (candidate)
            candidateTarget_ = candidate;

        // 候補が消えた後も、最後の位置でフェードアウトさせる
        if (candidate)
            candidateFade_.PlayForward();
        else
            candidateFade_.PlayBackward();
        candidateFade_.Tick(deltaTime);
    }

    void LockOnReticle::PlayEngage()
    {
        const uint16_t duration = LibCore::Tween::Ms(engageDuration_secs_);
        scaleRateTween_.Play(tweeny::from(engageStartScaleRate_).to(1.0f)
            .during(duration).via(LibCore::Tween::Ease(LibCore::EaseType::OutBack)));
        alphaTween_.Play(tweeny::from(0.0f).to(1.0f)
            .during(duration).via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
        bracketAngleTween_.Play(tweeny::from(ENGAGE_BRACKET_ANGLE).to(0.0f)
            .during(duration).via(LibCore::Tween::Ease(LibCore::EaseType::OutBack)));
    }

    void LockOnReticle::PlayRelease()
    {
        const uint16_t duration = LibCore::Tween::Ms(releaseDuration_secs_);
        scaleRateTween_.Play(tweeny::from(1.0f).to(releaseEndScaleRate_)
            .during(duration).via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
        alphaTween_.Play(tweeny::from(1.0f).to(0.0f)
            .during(duration).via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
    }

    void LockOnReticle::OnUserInterfaceRender()
    {
        if (!IsEnable())
            return;

        // 対象の位置は全ての Update が終わった描画時点で取る（Update 順による1フレーム遅れを避ける）
        if (candidateFade_.Value() > 0.0f)
        {
            if (const auto candidate = candidateTarget_.lock())
                candidatePointWorld_ = GameCore::PlayerAvatar::LockOnPositionOf(*candidate);

            const float pulse = 0.75f + 0.25f * std::sin(elapsed_secs_ * 2.0f * PI / CANDIDATE_PULSE_PERIOD_SECS);
            DrawSprite(
                candidateSprite_.get(),
                candidatePointWorld_,
                candidateScale_,
                0.0f,
                candidateFade_.Value() * candidateAlpha_ * pulse);
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
            scaleRate    = scaleRateTween_.Value();
            alpha        = alphaTween_.Value();
            bracketAngle = bracketAngleTween_.Value();
            break;
        case Phase::Locked:
            breathRate = 1.0f + lockedBreathScale_ * (0.5f - 0.5f * std::cos(elapsed_secs_ * 2.0f * PI / LOCKED_BREATH_PERIOD_SECS));
            break;
        case Phase::Releasing:
            scaleRate = scaleRateTween_.Value();
            alpha     = alphaTween_.Value();
            break;
        case Phase::Hidden:
            return;
        }

        const float scale = reticleScale_ * std::max(scaleRate, MIN_SCALE_RATE);
        DrawSprite(ringSprite_   .get(), lockOnPointWorld_, scale,              ringAngle_,   alpha);
        DrawSprite(bracketSprite_.get(), lockOnPointWorld_, scale * breathRate, bracketAngle, alpha);
    }

    float LockOnReticle::DistanceScaleRate(const glm::vec3& worldPos) const
    {
        // NOTE: GetCameraPosition との距離ではなく、画面位置と同じ ConvWorldPosToScreenPos で
        //       「1ユニットが何ピクセルに映るか」を測り、referenceDistance_ 先でのそれとの比にする
        const float maxRate = std::max(minDistanceScale_, maxDistanceScale_);
        const float tanHalfFov = std::tan(GetCameraFov() * 0.5f);
        int screenWidth = 0, screenHeight = 0;
        GetDrawScreenSize(&screenWidth, &screenHeight);
        if (referenceDistance_ <= 0.0f || tanHalfFov <= 0.0f || screenHeight <= 0)
            return maxRate;

        const VECTOR pos     = VGet(worldPos.x, worldPos.y, worldPos.z);
        const VECTOR a       = ConvWorldPosToScreenPos(pos);
        const VECTOR b       = ConvWorldPosToScreenPos(VAdd(pos, GetCameraUpVector()));
        const float  current = std::hypot(b.x - a.x, b.y - a.y);
        const float  reference = static_cast<float>(screenHeight) * 0.5f / (tanHalfFov * referenceDistance_);

        return std::clamp(current / reference, minDistanceScale_, maxRate);
    }

    void LockOnReticle::DrawSprite(
        const std::shared_ptr<Asset::SpriteFile>& sprite,
        const glm::vec3& worldPos,
        const float scale,
        const float angle,
        const float alpha) const
    {
        if (!sprite || alpha <= 0.0f || scale <= 0.0f)
            return;

        const VECTOR screenPos = ConvWorldPosToScreenPos(VGet(worldPos.x, worldPos.y, worldPos.z));
        // z が 0..1 の外ならカメラの視界外（背後など）
        if (screenPos.z < 0.0f || screenPos.z > 1.0f)
            return;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f));
        DrawRotaGraphF(screenPos.x, screenPos.y, scale * DistanceScaleRate(worldPos), angle, sprite->GetDxLibHandle(), TRUE);
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
        ImGuiHelper::OnDrawInputField("releaseEndScaleRate_",  releaseEndScaleRate_);
        ImGuiHelper::OnDrawInputField("referenceDistance_",    referenceDistance_);
        ImGuiHelper::OnDrawInputField("minDistanceScale_",     minDistanceScale_);
        ImGuiHelper::OnDrawInputField("maxDistanceScale_",     maxDistanceScale_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LockOnReticle);
#pragma endregion
