#include "PlayerHitShakeReceiver.h"

#include <cmath>

#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/Component/ModelRenderer/ModelRenderer.h"

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
        amplitude_     = amplitude;
        duration_secs_ = duration_secs;
        elapsed_secs_  = 0.0f;
        isPlaying_     = duration_secs > 0.0f;
    }

    void PlayerHitShakeReceiver::OnUpdate()
    {
        if (!isPlaying_)
            return;

        const auto modelRenderer = modelRenderer_.lock();
        if (!modelRenderer)
        {
            isPlaying_ = false;
            return;
        }

        elapsed_secs_ += Time::DeltaTime();
        if (elapsed_secs_ >= duration_secs_)
        {
            isPlaying_ = false;
            modelRenderer->SetRenderOffset(glm::vec3(0.0f));
            return;
        }

        // 当たった瞬間に押し込まれ、減衰しながら振動して戻る
        const float remain = 1.0f - elapsed_secs_ / duration_secs_;
        modelRenderer->SetRenderOffset(direction_ * (amplitude_ * remain * remain * std::cos(elapsed_secs_ * SHAKE_ANGULAR_FREQUENCY)));
    }

    void PlayerHitShakeReceiver::OnDrawGui()
    {
        ImGui::Text("isPlaying: %s", isPlaying_ ? "true" : "false");
        ImGui::Text("elapsed / duration: %.3f / %.3f", elapsed_secs_, duration_secs_);
    }
}
