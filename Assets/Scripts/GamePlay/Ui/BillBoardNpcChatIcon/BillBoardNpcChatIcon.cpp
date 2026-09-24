#include "BillBoardNpcChatIcon.h"

#include <algorithm>
#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float PI = 3.14159265f;

        constexpr float MIN_SCALE_RATE = 0.001f;

        void PlayPop(LibCore::Tween::TweenPlayer<float>& popScale, LibCore::Tween::TweenPlayer<float>& popAlpha, const float duration_secs)
        {
            popScale.Play(tweeny::from(0.0f).to(1.0f)
                .during(LibCore::Tween::Ms(duration_secs))
                .via(LibCore::Tween::Ease(LibCore::EaseType::OutBack)));
            popAlpha.Play(tweeny::from(0.0f).to(1.0f)
                .during(LibCore::Tween::Ms(duration_secs))
                .via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
        }
    }

    void BillBoardNpcChatIcon::Show(
        const bool chattableIcon,
        const bool chattingIcon,
        const bool surpriseIcon)
    {
        isShow_ = true;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, chattableIcon);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , chattingIcon);
        SetIconEnable(surpriseIcon_ .get().get(), savedSurprise_ , surpriseIcon);
    }

    void BillBoardNpcChatIcon::Hide()
    {
        isShow_ = false;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, false);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , false);
        SetIconEnable(surpriseIcon_ .get().get(), savedSurprise_ , false);
    }

    void BillBoardNpcChatIcon::OnChattable()
    {
        if (!isShow_)
            return;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, false);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , true);
    }

    void BillBoardNpcChatIcon::OnExitChattable()
    {
        if (!isShow_)
            return;

        SetIconEnable(chattableIcon_.get().get(), savedChattable_, true);
        SetIconEnable(chattingIcon_ .get().get(), savedChatting_ , false);
    }

    void BillBoardNpcChatIcon::BeginReactionSurprise()
    {
        if (isReactionSurprise_)
            return;

        savedChattable_ = chattableIcon_ && chattableIcon_->IsEnable();
        savedChatting_  = chattingIcon_  && chattingIcon_ ->IsEnable();
        savedSurprise_  = surpriseIcon_  && surpriseIcon_ ->IsEnable();
        isReactionSurprise_ = true;

        if (chattableIcon_) chattableIcon_->SetEnable(false);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(false);
        if (surpriseIcon_)  surpriseIcon_ ->SetEnable(true);
    }

    void BillBoardNpcChatIcon::EndReactionSurprise()
    {
        if (!isReactionSurprise_)
            return;

        isReactionSurprise_ = false;
        if (chattableIcon_) chattableIcon_->SetEnable(savedChattable_);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(savedChatting_);
        if (surpriseIcon_)  surpriseIcon_ ->SetEnable(savedSurprise_);
    }

    void BillBoardNpcChatIcon::SetIconEnable(GameObject::IGameObject* icon, bool& reactionSaved, const bool enable) const
    {
        if (!icon)
            return;

        if (isReactionSurprise_)
            reactionSaved = enable;
        else
            icon->SetEnable(enable);
    }

    void BillBoardNpcChatIcon::OnUpdate()
    {
        // ビックリマークは SetEnableShowChatIcon ではなく GameObjectSetEnable で
        // 直接表示されるため、isShow_ で演出を止めてはいけない
        UpdateIcon(surpriseIcon_ .get(), surpriseRimGlow_.get(), surpriseState_ , IconMotion::Surprise);
        UpdateIcon(chattableIcon_.get(), nullptr               , chattableState_, IconMotion::Chattable);
        UpdateIcon(chattingIcon_ .get(), nullptr               , chattingState_ , IconMotion::Chatting);
    }

    void BillBoardNpcChatIcon::UpdateIcon(
        const std::shared_ptr<GameObject::IGameObject>& object,
        const std::shared_ptr<NanamiUi::BillboardAnimation3D>& rimGlow,
        IconState& state,
        const IconMotion motion) const
    {
        if (!object)
            return;

        // 演出を書き込む前でないと、揺れた後の値を基準にしてしまう
        if (!state.isCaptured)
        {
            const auto billboard = object->Components().Catch<NanamiUi::Billboard3D>().lock();
            if (!billboard)
                return;

            state.billboard      = billboard;
            state.basePos        = object->Transform().GetLocalPos();
            state.baseScale      = object->Transform().GetLocalScale();
            state.baseAngle      = billboard->GetAngle();
            state.wasEnabled     = billboard->IsEnable();
            // シーン読み込み時点で表示済みのアイコンはポップさせない
            state.shownTime_secs = popDuration_secs_;
            state.isCaptured     = true;
            PlayPop(state.popScale, state.popAlpha, popDuration_secs_);
            state.popScale.Complete();
            state.popAlpha.Complete();
        }

        const auto billboard = state.billboard.lock();
        if (!billboard)
            return;

        // Show()/Hide() だけでなく GameObjectSetEnable で直接切り替えられることもあるので、
        // 有効/無効は呼び出し元を問わず毎フレームの変化で検知する
        const bool isEnabled = billboard->IsEnable();
        if (isEnabled && !state.wasEnabled)
        {
            state.shownTime_secs = 0.0f;
            PlayPop(state.popScale, state.popAlpha, popDuration_secs_);
        }
        state.wasEnabled = isEnabled;

        if (!isEnabled)
        {
            // このフレームの OnUpdate 後に有効化されても、前回の姿で一瞬描画されないようにしておく
            billboard->SetAlpha(0.0f);
            if (rimGlow)
                rimGlow->SetAlpha(0.0f);
            return;
        }

        const float deltaTime = Time::DeltaTime();
        state.shownTime_secs += deltaTime;
        state.popScale.Tick(deltaTime);
        state.popAlpha.Tick(deltaTime);
        const float time = state.shownTime_secs;

        glm::vec3 offset    = {};
        float     scaleRate = state.popScale.Value();
        float     angle     = state.baseAngle;
        // 枠を走る光の進み具合 (0..1)。負なら光らせない
        float     sweepT    = -1.0f;

        switch (motion)
        {
        case IconMotion::Surprise:
        {
            offset.y = std::sin(time * surpriseFloatSpeed_) * surpriseFloatAmplitude_;

            const float cycleElapsed = std::fmod(time, surpriseCycle_secs_);
            if (cycleElapsed < surpriseSweepDuration_secs_)
                sweepT = cycleElapsed / surpriseSweepDuration_secs_;

            // 周期の最後の surpriseTiltDuration_secs_ 秒だけ、減衰しながら左右に傾く
            const float tiltElapsed = cycleElapsed - (surpriseCycle_secs_ - surpriseTiltDuration_secs_);
            if (tiltElapsed > 0.0f)
            {
                const float tiltT = tiltElapsed / surpriseTiltDuration_secs_;
                angle += std::sin(tiltT * 3.0f * PI) * (1.0f - tiltT) * surpriseTiltAngle_;
            }
            break;
        }
        case IconMotion::Chattable:
            offset.y = -std::abs(std::sin(time * chattableBounceSpeed_)) * chattableBounceAmplitude_;
            break;
        case IconMotion::Chatting:
        {
            const float breath = 0.5f - 0.5f * std::cos(time * 2.0f * PI / chattingBreathPeriod_secs_);
            scaleRate *= 1.0f + breath * chattingBreathScale_;
            break;
        }
        }

        const float alpha = state.popAlpha.Value();

        object->Transform().SetLocalPos  (state.basePos + offset);
        object->Transform().SetLocalScale(state.baseScale * std::max(scaleRate, MIN_SCALE_RATE));
        billboard->SetAngle(angle);
        billboard->SetAlpha(alpha);

        // 光は子オブジェクトなので位置・スケールは親から引き継ぐ。角度と透明度だけ下地に合わせる
        if (rimGlow)
        {
            const int frameCount = rimGlow->GetFrameCount();
            const bool isSweeping = sweepT >= 0.0f && frameCount > 0;
            if (isSweeping)
                rimGlow->SetFrame(std::min(static_cast<int>(sweepT * static_cast<float>(frameCount)), frameCount - 1));

            rimGlow->SetAngle(angle);
            rimGlow->SetAlpha(isSweeping ? alpha : 0.0f);
        }
    }

    void BillBoardNpcChatIcon::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chattableIcon_", chattableIcon_);
        ImGuiHelper::OnDrawInputField("chattingIcon_", chattingIcon_);
        ImGuiHelper::OnDrawInputField("surpriseIcon_", surpriseIcon_);
        ImGuiHelper::OnDrawInputField("surpriseRimGlow_", surpriseRimGlow_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGuiHelper::OnDrawInputField("popDuration_secs_", popDuration_secs_);
        ImGuiHelper::OnDrawInputField("surpriseFloatAmplitude_", surpriseFloatAmplitude_);
        ImGuiHelper::OnDrawInputField("surpriseFloatSpeed_", surpriseFloatSpeed_);
        ImGuiHelper::OnDrawInputField("surpriseCycle_secs_", surpriseCycle_secs_);
        ImGuiHelper::OnDrawInputField("surpriseSweepDuration_secs_", surpriseSweepDuration_secs_);
        ImGuiHelper::OnDrawInputField("surpriseTiltDuration_secs_", surpriseTiltDuration_secs_);
        ImGuiHelper::OnDrawInputField("surpriseTiltAngle_", surpriseTiltAngle_);
        ImGuiHelper::OnDrawInputField("chattableBounceAmplitude_", chattableBounceAmplitude_);
        ImGuiHelper::OnDrawInputField("chattableBounceSpeed_", chattableBounceSpeed_);
        ImGuiHelper::OnDrawInputField("chattingBreathScale_", chattingBreathScale_);
        ImGuiHelper::OnDrawInputField("chattingBreathPeriod_secs_", chattingBreathPeriod_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BillBoardNpcChatIcon);
#pragma endregion
