#include "Prop_TreasureChest.h"

#include <numbers>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "../../Pickup/GamePlay_LootDrop.h"
#include "../../Sound/SoundPlayer.h"

namespace GamePlay::Prop
{
    namespace
    {
        /** 揺れで跳ねる高さ (ローカル) */
        constexpr float SHAKE_HOP = 0.12f;
    }

    void TreasureChest::OnStart()
    {
        if (const auto icon = chatIcon_.get())
            icon->Show(true, false, false);
        if (body_)
            bodyClosedPose_ = ClosedPose{ body_->Transform().GetLocalPos(), body_->Transform().GetLocalRot() };
        if (lid_)
            lidClosedPose_ = ClosedPose{ lid_->Transform().GetLocalPos(), lid_->Transform().GetLocalRot() };
    }

    void TreasureChest::OnUpdate()
    {
        if (!isOpened_ || isSpilled_)
            return;

        if (!isLidOpening_)
        {
            shakeElapsed_secs_ += Time::DeltaTime();
            if (shakeElapsed_secs_ < shakeDuration_secs_)
            {
                ApplyShake(shakeElapsed_secs_);
                return;
            }
            ApplyShake(shakeDuration_secs_);
            StartOpenLid();
        }

        const bool finished = lidTween_.Tick(Time::DeltaTime());
        SetLidAngle(lidTween_.Value());

        if (finished)
            SpillLoot();
    }

    void TreasureChest::ApplyShake(const float elapsed_secs)
    {
        // NOTE: だんだん強く揺れ、最後の 15% で収まる。elapsed_secs == shakeDuration_secs_ で閉じた姿勢に戻る
        const float progress = shakeDuration_secs_ > 0.0f ? glm::clamp(elapsed_secs / shakeDuration_secs_, 0.0f, 1.0f) : 1.0f;
        const float envelope = glm::smoothstep(0.0f, 0.7f, progress) * (1.0f - glm::smoothstep(0.85f, 1.0f, progress));
        const float wave     = std::sin(2.0f * std::numbers::pi_v<float> * shakeFrequency_hz_ * elapsed_secs);

        // NOTE: 箱の底 (ルート原点) を中心に左右へ傾けるので、本体とフタを同じ回転で動かす
        const glm::quat tilt = glm::angleAxis(glm::radians(shakeAngle_deg_ * envelope * wave), glm::vec3(0.0f, 0.0f, 1.0f));
        const glm::vec3 hop  = glm::vec3(0.0f, SHAKE_HOP * envelope * std::abs(wave), 0.0f);

        const auto apply = [&](const std::shared_ptr<GameObject::IGameObject>& target, const std::optional<ClosedPose>& closed)
        {
            if (!target || !closed)
                return;
            target->Transform().SetLocalPos(tilt * closed->pos + hop);
            target->Transform().SetLocalRot(tilt * closed->rot);
        };
        apply(body_ ? body_.get() : nullptr, bodyClosedPose_);
        apply(lid_ ? lid_.get() : nullptr, lidClosedPose_);
    }

    void TreasureChest::StartOpenLid()
    {
        isLidOpening_ = true;
        // NOTE: ease-out back なので少し開きすぎてから戻る
        lidTween_.Play(tweeny::from(0.0f).to(openAngle_deg_)
            .during(LibCore::Tween::Ms(openDuration_secs_))
            .via(LibCore::Tween::Ease(LibCore::EaseType::OutBack, 1.7f)));
        if (openSound_)
            Sound::SoundPlayer::PlaySe(*openSound_.get(), Transform().GetWorldPos());
    }

    void TreasureChest::SetLidAngle(const float angle_deg)
    {
        if (!lid_ || !lidClosedPose_)
            return;

        lid_->Transform().SetLocalRot(lidClosedPose_->rot * glm::angleAxis(glm::radians(angle_deg), glm::vec3(1.0f, 0.0f, 0.0f)));
    }

    void TreasureChest::SpillLoot()
    {
        isSpilled_ = true;

        const glm::vec3 position = DropPosition();
        if (openParticle_)
            Scene::GameObject::Instantiate(openParticle_.get(), position);
        if (dropTable_)
            Pickup::DropLoot(*dropTable_.get(), position);
    }

    void TreasureChest::OnInteractable()
    {
        if (isOpened_)
            return;
        if (const auto icon = chatIcon_.get())
            icon->OnChattable();
    }

    void TreasureChest::OnExitInteractable()
    {
        if (isOpened_)
            return;
        if (const auto icon = chatIcon_.get())
            icon->OnExitChattable();
    }

    void TreasureChest::OnInteract()
    {
        if (isOpened_)
            return;
        isOpened_ = true;
        shakeElapsed_secs_ = 0.0f;

        if (const auto icon = chatIcon_.get())
            icon->Hide();
        if (const auto particle = idleParticle_.get())
        {
            // NOTE: Stop だけだと Loop が再生時間経過で再開するので無効化もする
            particle->Stop();
            particle->SetEnable(false);
        }
    }

    const GameObject::Transform& TreasureChest::InteractableTransform() const
    {
        return Transform();
    }

    glm::vec3 TreasureChest::DropPosition() const
    {
        return dropPoint_ ? dropPoint_->Transform().GetWorldPos() : Transform().GetWorldPos();
    }

    void TreasureChest::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("dropTable_", dropTable_);
        ImGuiHelper::OnDrawInputField("lid_", lid_);
        ImGuiHelper::OnDrawInputField("dropPoint_", dropPoint_);
        ImGuiHelper::OnDrawInputField("openParticle_", openParticle_);
        ImGuiHelper::OnDrawInputField("openSound_", openSound_);
        ImGuiHelper::OnDrawInputField("chatIcon_", chatIcon_);
        ImGuiHelper::OnDrawInputField("idleParticle_", idleParticle_);
        ImGuiHelper::OnDrawInputField("openAngle_deg_", openAngle_deg_);
        ImGuiHelper::OnDrawInputField("openDuration_secs_", openDuration_secs_);
        ImGuiHelper::OnDrawInputField("body_", body_);
        ImGuiHelper::OnDrawInputField("shakeDuration_secs_", shakeDuration_secs_);
        ImGuiHelper::OnDrawInputField("shakeAngle_deg_", shakeAngle_deg_);
        ImGuiHelper::OnDrawInputField("shakeFrequency_hz_", shakeFrequency_hz_);
        ImGui::Text("opened: %s  spilled: %s", isOpened_ ? "true" : "false", isSpilled_ ? "true" : "false");
    }
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::TreasureChest)
