#include "Ui_CannonCooldownGauge.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "DxLib.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/DxLib/ShiftJis.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float TAU = std::numbers::pi_v<float> * 2.0f;
        constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;

        constexpr LibCore::Tween::EaseFunctor EASE_OUT_CUBIC{ LibCore::EaseType::OutCubic };
        constexpr LibCore::Tween::EaseFunctor SPARK_POP_EASE{ LibCore::EaseType::OutBack, 1.9f };

        class ScopedDrawState final
        {
        public:
            ScopedDrawState()
                : drawMode_(GetDrawMode())
            {
                GetDrawBlendMode(&blendMode_, &blendParam_);
                GetDrawBright(&brightR_, &brightG_, &brightB_);
            }
            ~ScopedDrawState()
            {
                SetDrawMode(drawMode_);
                SetDrawBlendMode(blendMode_, blendParam_);
                SetDrawBright(brightR_, brightG_, brightB_);
            }
            ScopedDrawState(const ScopedDrawState&) = delete;
            ScopedDrawState& operator=(const ScopedDrawState&) = delete;

        private:
            int drawMode_   = DX_DRAWMODE_NEAREST;
            int blendMode_  = DX_BLENDMODE_NOBLEND;
            int blendParam_ = 0;
            int brightR_    = 255;
            int brightG_    = 255;
            int brightB_    = 255;
        };

        float Rate(const float elapsed, const float duration)
        {
            return duration > 0.0f ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
        }

        void SetAlphaBlend(const int blendMode, const float alpha)
        {
            SetDrawBlendMode(blendMode, static_cast<int>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f));
        }

        glm::vec2 Rotate(const glm::vec2& v, const float angle)
        {
            const float c = std::cos(angle);
            const float s = std::sin(angle);
            return glm::vec2(v.x * c - v.y * s, v.x * s + v.y * c);
        }
    }

    void CannonCooldownGauge::Show()
    {
        isReady_            = false;
        hasCountedDown_     = false;
        readyElapsed_secs_  = 0.0f;
        shootElapsed_secs_  = 1000.0f;
        Entity().lock()->SetEnable(true);
    }

    void CannonCooldownGauge::Hide()
    {
        Entity().lock()->SetEnable(false);
    }

    void CannonCooldownGauge::SetCooldown(const float remain_secs, const float total_secs)
    {
        remain_secs_ = std::max(remain_secs, 0.0f);
        total_secs_  = total_secs;

        const bool isReady = remain_secs_ <= 0.0f;
        if (isReady && !isReady_)
        {
            readyElapsed_secs_ = 0.0f;
            if (hasCountedDown_)
                Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::HudReady);
        }
        isReady_ = isReady;
        if (!isReady)
        {
            hasCountedDown_ = true;
            lastCount_ = static_cast<int>(std::ceil(remain_secs_));
        }
    }

    void CannonCooldownGauge::PlayShoot()
    {
        shootElapsed_secs_ = 0.0f;
    }

    void CannonCooldownGauge::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();
        time_secs_         += deltaTime;
        readyElapsed_secs_  = std::min(readyElapsed_secs_ + deltaTime, 1000.0f);
        shootElapsed_secs_  = std::min(shootElapsed_secs_ + deltaTime, 1000.0f);
    }

    CannonCooldownGauge::Pose CannonCooldownGauge::EvaluatePose() const
    {
        Pose pose;

        if (isReady_)
        {
            const float u      = readyElapsed_secs_;
            const float flash  = (1.0f - Rate(u, readyFlashDuration_secs_)) * (1.0f - Rate(u, readyFlashDuration_secs_));
            const float settle = Rate(u, wobbleSettle_secs_);
            const float flicker = 1.0f + 0.14f * std::sin(time_secs_ * 41.0f) + 0.08f * std::sin(time_secs_ * 23.0f);

            pose.gaugePercent  = 100.0f;
            pose.isGold        = true;
            pose.flash         = flash;
            pose.scale         = 1.0f + readyPunchAmplitude_ * std::sin(u * readyPunchFrequency_) * std::exp(-u * readyPunchDamping_);
            pose.halo          = (0.5f + 0.22f * std::sin(TAU * haloPulseFrequency_hz_ * u)) * settle + 0.5f * flash;
            pose.shockwaveRate = u < shockwaveDuration_secs_ ? u / shockwaveDuration_secs_ : -1.0f;
            pose.bombDim       = bombDimRate_ * (1.0f - Rate(u, bombBrighten_secs_));
            pose.bombAngle     = wobbleAngle_deg_ * DEG_TO_RAD * std::sin(TAU * wobbleFrequency_hz_ * u) * settle;
            pose.spark         = SPARK_POP_EASE.Ease(Rate(u, sparkPopDuration_secs_)) * flicker;
            pose.isPromptLit   = true;
            pose.promptFlash   = flash;

            const float countFade = Rate(u, countFadeOut_secs_);
            if (countFade < 1.0f)
            {
                pose.count      = lastCount_;
                pose.countScale = 1.0f + 0.5f * countFade;
                pose.countAlpha = 1.0f - countFade;
            }
            return pose;
        }

        const float coolingRate = total_secs_ > 0.0f ? std::clamp(1.0f - remain_secs_ / total_secs_, 0.0f, 1.0f) : 1.0f;
        const float u = shootElapsed_secs_;

        pose.gaugePercent = coolingRate * 100.0f;
        pose.isTipVisible = true;
        pose.scale        = 1.0f - recoilAmplitude_ * std::sin(u * recoilFrequency_) * std::exp(-u * recoilDamping_);
        pose.bombDim      = bombDimRate_;
        pose.count        = lastCount_;
        pose.countAlpha   = 1.0f;

        if (u < drainDuration_secs_)
        {
            const float drain = Rate(u, drainDuration_secs_);
            pose.gaugePercent = (1.0f - drain) * 100.0f;
            pose.isGold       = true;
            pose.isTipVisible = false;
            pose.halo         = 0.5f * (1.0f - drain);
            pose.isPromptLit  = true;
            pose.count        = 0;
        }
        else
        {
            const float pop = Rate(u - drainDuration_secs_, countPopIn_secs_);
            pose.countScale = 1.5f - 0.5f * EASE_OUT_CUBIC.Ease(pop);
            pose.countAlpha = pop;
        }

        if (u < launchDuration_secs_)
        {
            const float launch = Rate(u, launchDuration_secs_);
            pose.bombDim    = 0.0f;
            pose.bombScale  = 1.0f - launchShrinkRate_ * launch;
            pose.bombOffset = launchOffset_ * EASE_OUT_CUBIC.Ease(launch);
            pose.bombAlpha  = 1.0f - launch;
        }
        else
        {
            pose.bombAlpha = Rate(u - launchDuration_secs_, bombFadeIn_secs_);
        }
        return pose;
    }

    void CannonCooldownGauge::DrawCenteredText(const std::string& utf8Text, const glm::vec2& centre, const float scale, const Color32& color, const float alpha) const
    {
        const int fontHandle = font_->DxLibHandle();
        const std::string sjis = LibCore::Dxlib::Utf8ToShiftJis(utf8Text);
        const float width  = static_cast<float>(GetDrawExtendStringWidthToHandle(scale, sjis.c_str(), static_cast<int>(sjis.size()), fontHandle));
        const float height = static_cast<float>(GetFontSizeToHandle(fontHandle)) * scale;

        SetAlphaBlend(DX_BLENDMODE_ALPHA, alpha);
        DrawExtendStringFToHandle(
            centre.x - width * 0.5f,
            centre.y - height * 0.5f,
            scale,
            scale,
            sjis.c_str(),
            color.ToDxColor(),
            fontHandle,
            font_->EdgeColor().ToDxColor());
    }

    void CannonCooldownGauge::OnUserInterfaceRender()
    {
        if (!IsEnable() || !frameSprite_ || !fillTealSprite_ || !fillGoldSprite_ || !bombSprite_ || !promptPillSprite_ || !promptMouseSprite_ || !font_)
            return;

        const ScopedDrawState drawState;
        SetDrawMode(DX_DRAWMODE_BILINEAR);

        const Pose pose = EvaluatePose();
        const auto worldPos = Transform().GetWorldPos();
        const glm::vec2 root = glm::vec2(worldPos.x, worldPos.y);
        const double scale = pose.scale;

        SetAlphaBlend(DX_BLENDMODE_ALPHA, 1.0f);
        DrawRotaGraphF(root.x + pillOffset_.x, root.y + pillOffset_.y, 1.0, 0.0, promptPillSprite_->GetDxLibHandle(), TRUE);
        if (promptPillGlowSprite_ && pose.promptFlash > 0.0f)
        {
            SetAlphaBlend(DX_BLENDMODE_ALPHA, pose.promptFlash);
            DrawRotaGraphF(root.x + pillOffset_.x, root.y + pillOffset_.y, 1.0, 0.0, promptPillGlowSprite_->GetDxLibHandle(), TRUE);
        }
        const auto& mouseSprite = pose.isPromptLit && promptMouseLitSprite_ ? promptMouseLitSprite_ : promptMouseSprite_;
        SetAlphaBlend(DX_BLENDMODE_ALPHA, 1.0f);
        DrawRotaGraphF(root.x + mouseOffset_.x, root.y + mouseOffset_.y, 1.0, 0.0, mouseSprite->GetDxLibHandle(), TRUE);

        if (haloSprite_ && pose.halo > 0.0f)
        {
            SetAlphaBlend(DX_BLENDMODE_ALPHA, pose.halo);
            DrawRotaGraphF(root.x, root.y, scale, 0.0, haloSprite_->GetDxLibHandle(), TRUE);
        }
        if (shockwaveSprite_ && pose.shockwaveRate >= 0.0f && shockwaveSpriteRadius_ > 0.0f)
        {
            const float radius = shockwaveStartRadius_ + (shockwaveEndRadius_ - shockwaveStartRadius_) * EASE_OUT_CUBIC.Ease(pose.shockwaveRate);
            SetAlphaBlend(DX_BLENDMODE_ADD, std::pow(1.0f - pose.shockwaveRate, 1.5f));
            DrawRotaGraphF(root.x, root.y, radius / shockwaveSpriteRadius_, 0.0, shockwaveSprite_->GetDxLibHandle(), TRUE);
        }

        SetAlphaBlend(DX_BLENDMODE_ALPHA, 1.0f);
        DrawRotaGraphF(root.x, root.y, scale, 0.0, frameSprite_->GetDxLibHandle(), TRUE);

        const auto& fillSprite = pose.isGold ? fillGoldSprite_ : fillTealSprite_;
        DrawCircleGaugeF(root.x, root.y, pose.gaugePercent, fillSprite->GetDxLibHandle(), 0.0, scale, FALSE, FALSE);
        if (fillFlashSprite_ && pose.flash > 0.0f)
        {
            SetAlphaBlend(DX_BLENDMODE_ALPHA, pose.flash);
            DrawCircleGaugeF(root.x, root.y, pose.gaugePercent, fillFlashSprite_->GetDxLibHandle(), 0.0, scale, FALSE, FALSE);
        }
        if (tipSprite_ && pose.isTipVisible && pose.gaugePercent > 1.0f)
        {
            const float angle = pose.gaugePercent / 100.0f * TAU;
            const float radius = gaugeRadius_ * pose.scale;
            SetAlphaBlend(DX_BLENDMODE_ALPHA, 1.0f);
            DrawRotaGraphF(root.x + std::sin(angle) * radius, root.y - std::cos(angle) * radius, 1.0, 0.0, tipSprite_->GetDxLibHandle(), TRUE);
        }

        const glm::vec2 bombPivot = root + (bombPivotOffset_ + pose.bombOffset) * pose.scale;
        const float bombScale = pose.scale * pose.bombScale;
        if (pose.bombAlpha > 0.0f)
        {
            const int bright = static_cast<int>((1.0f - std::clamp(pose.bombDim, 0.0f, 1.0f)) * 255.0f);
            SetDrawBright(bright, bright, bright);
            SetAlphaBlend(DX_BLENDMODE_ALPHA, pose.bombAlpha);
            DrawRotaGraph2F(bombPivot.x, bombPivot.y, bombPivotInSprite_.x, bombPivotInSprite_.y, bombScale, pose.bombAngle, bombSprite_->GetDxLibHandle(), TRUE);
            SetDrawBright(255, 255, 255);
        }

        if (sparkSprite_ && pose.spark > 0.0f)
        {
            const glm::vec2 sparkPos = bombPivot + Rotate(sparkOffsetFromBombPivot_, pose.bombAngle) * bombScale;
            SetAlphaBlend(DX_BLENDMODE_ALPHA, 1.0f);
            DrawRotaGraphF(sparkPos.x, sparkPos.y, pose.spark * bombScale, time_secs_ * 3.0f, sparkSprite_->GetDxLibHandle(), TRUE);

            if (emberSprite_ && pose.shockwaveRate >= 0.0f && emberSpriteRadius_ > 0.0f)
            {
                const float distance = emberDistance_ * EASE_OUT_CUBIC.Ease(pose.shockwaveRate);
                const float fade = 1.0f - pose.shockwaveRate;
                const float emberScale = (3.2f * fade + 0.4f) / emberSpriteRadius_;
                SetAlphaBlend(DX_BLENDMODE_ALPHA, fade);
                for (int i = 0; i < emberCount_; ++i)
                {
                    const float angle = emberStartAngle_deg_ * DEG_TO_RAD + TAU * static_cast<float>(i) / static_cast<float>(std::max(emberCount_, 1));
                    DrawRotaGraphF(sparkPos.x + std::cos(angle) * distance, sparkPos.y + std::sin(angle) * distance, emberScale, 0.0, emberSprite_->GetDxLibHandle(), TRUE);
                }
            }
        }

        DrawCenteredText(
            pose.isPromptLit ? readyPromptText_ : coolingPromptText_,
            root + promptTextOffset_,
            promptTextScale_,
            pose.isPromptLit ? promptReadyTextColor_ : promptTextColor_,
            1.0f);

        if (pose.count > 0 && pose.countAlpha > 0.0f)
            DrawCenteredText(std::to_string(pose.count), root + countTextOffset_ * pose.scale, countTextScale_ * pose.countScale * pose.scale, countTextColor_, pose.countAlpha);
    }

    void CannonCooldownGauge::OnDrawGui()
    {
        float previewRemain = remain_secs_;
        if (ImGui::SliderFloat("remain_secs (preview)", &previewRemain, 0.0f, 10.0f))
            SetCooldown(previewRemain, 10.0f);
        if (ImGui::Button("Play Ready"))
        {
            isReady_ = false;
            SetCooldown(0.0f, total_secs_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Shoot"))
        {
            PlayShoot();
            SetCooldown(total_secs_, total_secs_);
        }
        ImGui::Text("isReady_: %d  readyElapsed_secs_: %.2f  shootElapsed_secs_: %.2f", isReady_, readyElapsed_secs_, shootElapsed_secs_);

        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("frameSprite_", frameSprite_);
        ImGuiHelper::OnDrawInputField("fillTealSprite_", fillTealSprite_);
        ImGuiHelper::OnDrawInputField("fillGoldSprite_", fillGoldSprite_);
        ImGuiHelper::OnDrawInputField("fillFlashSprite_", fillFlashSprite_);
        ImGuiHelper::OnDrawInputField("tipSprite_", tipSprite_);
        ImGuiHelper::OnDrawInputField("haloSprite_", haloSprite_);
        ImGuiHelper::OnDrawInputField("shockwaveSprite_", shockwaveSprite_);
        ImGuiHelper::OnDrawInputField("bombSprite_", bombSprite_);
        ImGuiHelper::OnDrawInputField("sparkSprite_", sparkSprite_);
        ImGuiHelper::OnDrawInputField("emberSprite_", emberSprite_);
        ImGuiHelper::OnDrawInputField("promptPillSprite_", promptPillSprite_);
        ImGuiHelper::OnDrawInputField("promptPillGlowSprite_", promptPillGlowSprite_);
        ImGuiHelper::OnDrawInputField("promptMouseSprite_", promptMouseSprite_);
        ImGuiHelper::OnDrawInputField("promptMouseLitSprite_", promptMouseLitSprite_);
        ImGuiHelper::OnDrawInputField("font_", font_);
        ImGuiHelper::OnDrawInputField("gaugeRadius_", gaugeRadius_);
        ImGuiHelper::OnDrawInputField("shockwaveSpriteRadius_", shockwaveSpriteRadius_);
        ImGuiHelper::OnDrawInputField("emberSpriteRadius_", emberSpriteRadius_);
        ImGui::InputFloat2("bombPivotOffset_", &bombPivotOffset_.x);
        ImGui::InputFloat2("bombPivotInSprite_", &bombPivotInSprite_.x);
        ImGui::InputFloat2("sparkOffsetFromBombPivot_", &sparkOffsetFromBombPivot_.x);
        ImGui::InputFloat2("pillOffset_", &pillOffset_.x);
        ImGui::InputFloat2("mouseOffset_", &mouseOffset_.x);
        ImGui::InputFloat2("promptTextOffset_", &promptTextOffset_.x);
        ImGui::InputFloat2("countTextOffset_", &countTextOffset_.x);
        ImGuiHelper::OnDrawInputField("coolingPromptText_", coolingPromptText_);
        ImGuiHelper::OnDrawInputField("readyPromptText_", readyPromptText_);
        ImGuiHelper::OnDrawInputField("promptTextScale_", promptTextScale_);
        ImGuiHelper::OnDrawInputField("countTextScale_", countTextScale_);
        ImGuiHelper::OnDrawInputField("promptTextColor_", promptTextColor_);
        ImGuiHelper::OnDrawInputField("promptReadyTextColor_", promptReadyTextColor_);
        ImGuiHelper::OnDrawInputField("countTextColor_", countTextColor_);
        ImGuiHelper::OnDrawInputField("bombDimRate_", bombDimRate_);
        ImGuiHelper::OnDrawInputField("readyFlashDuration_secs_", readyFlashDuration_secs_);
        ImGuiHelper::OnDrawInputField("readyPunchAmplitude_", readyPunchAmplitude_);
        ImGuiHelper::OnDrawInputField("readyPunchFrequency_", readyPunchFrequency_);
        ImGuiHelper::OnDrawInputField("readyPunchDamping_", readyPunchDamping_);
        ImGuiHelper::OnDrawInputField("shockwaveDuration_secs_", shockwaveDuration_secs_);
        ImGuiHelper::OnDrawInputField("shockwaveStartRadius_", shockwaveStartRadius_);
        ImGuiHelper::OnDrawInputField("shockwaveEndRadius_", shockwaveEndRadius_);
        ImGuiHelper::OnDrawInputField("emberCount_", emberCount_);
        ImGuiHelper::OnDrawInputField("emberDistance_", emberDistance_);
        ImGuiHelper::OnDrawInputField("sparkPopDuration_secs_", sparkPopDuration_secs_);
        ImGuiHelper::OnDrawInputField("wobbleAngle_deg_", wobbleAngle_deg_);
        ImGuiHelper::OnDrawInputField("wobbleFrequency_hz_", wobbleFrequency_hz_);
        ImGuiHelper::OnDrawInputField("haloPulseFrequency_hz_", haloPulseFrequency_hz_);
        ImGuiHelper::OnDrawInputField("drainDuration_secs_", drainDuration_secs_);
        ImGuiHelper::OnDrawInputField("launchDuration_secs_", launchDuration_secs_);
        ImGui::InputFloat2("launchOffset_", &launchOffset_.x);
        ImGuiHelper::OnDrawInputField("recoilAmplitude_", recoilAmplitude_);
        emberCount_ = std::max(emberCount_, 0);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGuiHelper::OnDrawInputField("bombBrighten_secs_", bombBrighten_secs_);
        ImGuiHelper::OnDrawInputField("wobbleSettle_secs_", wobbleSettle_secs_);
        ImGuiHelper::OnDrawInputField("countFadeOut_secs_", countFadeOut_secs_);
        ImGuiHelper::OnDrawInputField("countPopIn_secs_", countPopIn_secs_);
        ImGuiHelper::OnDrawInputField("bombFadeIn_secs_", bombFadeIn_secs_);
        ImGuiHelper::OnDrawInputField("launchShrinkRate_", launchShrinkRate_);
        ImGuiHelper::OnDrawInputField("recoilFrequency_", recoilFrequency_);
        ImGuiHelper::OnDrawInputField("recoilDamping_", recoilDamping_);
        ImGuiHelper::OnDrawInputField("emberStartAngle_deg_", emberStartAngle_deg_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::CannonCooldownGauge);
#pragma endregion
