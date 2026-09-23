#include "PlayerHitShakeReceiver.h"

#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/ModelRenderer/ModelRenderer.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    constexpr float SHAKE_ANGULAR_FREQUENCY = 6.2831853f * 18.0f;
}

namespace GamePlay::PlayerAvatar
{
    void PlayerHitShakeReceiver::Play(const glm::vec3& direction, const float amplitude, const float duration_secs)
    {
        // EnemyBaseのOnAwake中にRequireComponentで動的追加されるため、Awakeに頼らずここで取得する
        if (modelRenderer_.expired())
            modelRenderer_ = Components().Catch<Component::ModelRenderer>();

        direction_     = direction;
        duration_secs_ = duration_secs;
        if (duration_secs <= 0.0f)
        {
            envelope_.Stop();
            return;
        }

        // NOTE: a + (b - a) * OutQuad(t) で b = 0 なので amplitude * (1 - t)^2
        envelope_.Play(tweeny::from(amplitude).to(0.0f)
            .during(LibCore::Tween::Ms(duration_secs))
            .via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
    }

    void PlayerHitShakeReceiver::OnUpdate()
    {
        if (!envelope_.IsPlaying())
            return;

        const auto modelRenderer = modelRenderer_.lock();
        if (!modelRenderer)
        {
            envelope_.Stop();
            return;
        }

        if (envelope_.Tick(Time::DeltaTime()))
        {
            modelRenderer->SetRenderOffset(glm::vec3(0.0f));
            return;
        }

        // 当たった瞬間に押し込まれ、減衰しながら振動して戻る
        const float elapsed_secs = envelope_.Progress() * duration_secs_;
        modelRenderer->SetRenderOffset(direction_ * (envelope_.Value() * std::cos(elapsed_secs * SHAKE_ANGULAR_FREQUENCY)));
    }

    void PlayerHitShakeReceiver::OnDrawGui()
    {
        ImGui::Text("isPlaying: %s", envelope_.IsPlaying() ? "true" : "false");
        ImGui::Text("elapsed / duration: %.3f / %.3f", envelope_.Progress() * duration_secs_, duration_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::PlayerHitShakeReceiver);
#pragma endregion
