#include "Prop_TreasureChest.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../../Pickup/GamePlay_LootDrop.h"
#include "../../Sound/SoundPlayer.h"

namespace GamePlay::Prop
{
    void TreasureChest::OnStart()
    {
        if (const auto icon = chatIcon_.get())
            icon->Show(true, false, false);
        if (lid_)
            lidClosedRot_ = lid_->Transform().GetLocalRot();
    }

    void TreasureChest::OnUpdate()
    {
        if (!isOpened_ || isSpilled_)
            return;

        openElapsed_secs_ += Time::DeltaTime();
        const float t = openDuration_secs_ > 0.0f ? std::clamp(openElapsed_secs_ / openDuration_secs_, 0.0f, 1.0f) : 1.0f;
        // NOTE: ease-out back なので少し開きすぎてから戻る
        constexpr float OVERSHOOT = 1.7f;
        const float s = t - 1.0f;
        const float eased = 1.0f + (OVERSHOOT + 1.0f) * s * s * s + OVERSHOOT * s * s;
        SetLidAngle(openAngle_deg_ * eased);

        if (t >= 1.0f)
            SpillLoot();
    }

    void TreasureChest::SetLidAngle(const float angle_deg)
    {
        if (!lid_ || !lidClosedRot_)
            return;

        lid_->Transform().SetLocalRot(*lidClosedRot_ * glm::angleAxis(glm::radians(angle_deg), glm::vec3(1.0f, 0.0f, 0.0f)));
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

        if (const auto icon = chatIcon_.get())
            icon->Hide();
        if (const auto particle = idleParticle_.get())
        {
            // NOTE: Stop だけだと Loop が再生時間経過で再開するので無効化もする
            particle->Stop();
            particle->SetEnable(false);
        }
        if (openSound_)
            Sound::SoundPlayer::PlaySe(*openSound_.get(), Transform().GetWorldPos());
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
        ImGui::Text("opened: %s  spilled: %s", isOpened_ ? "true" : "false", isSpilled_ ? "true" : "false");
    }
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::TreasureChest, 1)
