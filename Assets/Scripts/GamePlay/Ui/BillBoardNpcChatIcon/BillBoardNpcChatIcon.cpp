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

        // 表示された瞬間のポップ（拡大して少し行き過ぎて戻る + フェードイン）
        constexpr float POP_DURATION_SECS = 0.25f;
        constexpr float MIN_SCALE_RATE    = 0.001f;

        constexpr float SURPRISE_FLOAT_AMPLITUDE     = 0.2f;
        constexpr float SURPRISE_FLOAT_SPEED         = 2.0f;
        // 周期の先頭で枠を光が走り、周期の最後にコトッと傾く（傾いた直後に次の光が走る）
        constexpr float SURPRISE_CYCLE_SECS          = 3.0f;
        constexpr float SURPRISE_SWEEP_DURATION_SECS = 0.6f;
        constexpr float SURPRISE_TILT_DURATION_SECS  = 0.5f;
        constexpr float SURPRISE_TILT_ANGLE          = 0.2f;

        constexpr float CHATTABLE_BOUNCE_AMPLITUDE = 0.12f;
        constexpr float CHATTABLE_BOUNCE_SPEED     = 4.0f;

        constexpr float CHATTING_BREATH_SCALE       = 0.05f;
        constexpr float CHATTING_BREATH_PERIOD_SECS = 1.6f;

        void PlayPop(LibCore::Tween::TweenPlayer<float>& popScale, LibCore::Tween::TweenPlayer<float>& popAlpha)
        {
            popScale.Play(tweeny::from(0.0f).to(1.0f)
                .during(LibCore::Tween::Ms(POP_DURATION_SECS))
                .via(LibCore::Tween::Ease(LibCore::EaseType::OutBack)));
            popAlpha.Play(tweeny::from(0.0f).to(1.0f)
                .during(LibCore::Tween::Ms(POP_DURATION_SECS))
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
        const IconMotion motion)
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
            state.shownTime_secs = POP_DURATION_SECS;
            state.isCaptured     = true;
            PlayPop(state.popScale, state.popAlpha);
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
            PlayPop(state.popScale, state.popAlpha);
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
            offset.y = std::sin(time * SURPRISE_FLOAT_SPEED) * SURPRISE_FLOAT_AMPLITUDE;

            const float cycleElapsed = std::fmod(time, SURPRISE_CYCLE_SECS);
            if (cycleElapsed < SURPRISE_SWEEP_DURATION_SECS)
                sweepT = cycleElapsed / SURPRISE_SWEEP_DURATION_SECS;

            // 周期の最後の SURPRISE_TILT_DURATION_SECS 秒だけ、減衰しながら左右に傾く
            const float tiltElapsed = cycleElapsed - (SURPRISE_CYCLE_SECS - SURPRISE_TILT_DURATION_SECS);
            if (tiltElapsed > 0.0f)
            {
                const float tiltT = tiltElapsed / SURPRISE_TILT_DURATION_SECS;
                angle += std::sin(tiltT * 3.0f * PI) * (1.0f - tiltT) * SURPRISE_TILT_ANGLE;
            }
            break;
        }
        case IconMotion::Chattable:
            offset.y = -std::abs(std::sin(time * CHATTABLE_BOUNCE_SPEED)) * CHATTABLE_BOUNCE_AMPLITUDE;
            break;
        case IconMotion::Chatting:
        {
            const float breath = 0.5f - 0.5f * std::cos(time * 2.0f * PI / CHATTING_BREATH_PERIOD_SECS);
            scaleRate *= 1.0f + breath * CHATTING_BREATH_SCALE;
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
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BillBoardNpcChatIcon);
#pragma endregion
