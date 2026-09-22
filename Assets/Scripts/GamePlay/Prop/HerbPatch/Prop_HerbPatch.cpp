#include "Prop_HerbPatch.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../../Pickup/GamePlay_LootDrop.h"
#include "../../Sound/SoundPlayer.h"

namespace GamePlay::Prop
{
    void HerbPatch::OnStart()
    {
        if (const auto icon = chatIcon_.get())
            icon->Show(true, false, false);
    }

    void HerbPatch::OnInteractable()
    {
        if (const auto icon = chatIcon_.get())
            icon->OnChattable();
    }

    void HerbPatch::OnExitInteractable()
    {
        if (const auto icon = chatIcon_.get())
            icon->OnExitChattable();
    }

    void HerbPatch::OnInteract()
    {
        if (isHarvested_)
            return;
        isHarvested_ = true;

        const glm::vec3 position = DropPosition();
        Pickup::DropItem(item_.get(), count_, position);
        if (harvestParticle_)
            Scene::GameObject::Instantiate(harvestParticle_.get(), position);
        if (harvestSound_)
            Sound::SoundPlayer::PlaySe(*harvestSound_.get(), position);

        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    const GameObject::Transform& HerbPatch::InteractableTransform() const
    {
        return Transform();
    }

    glm::vec3 HerbPatch::DropPosition() const
    {
        return dropPoint_ ? dropPoint_->Transform().GetWorldPos() : Transform().GetWorldPos();
    }

    void HerbPatch::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("item_", item_);
        ImGuiHelper::OnDrawInputField("count_", count_);
        ImGuiHelper::OnDrawInputField("dropPoint_", dropPoint_);
        ImGuiHelper::OnDrawInputField("harvestParticle_", harvestParticle_);
        ImGuiHelper::OnDrawInputField("harvestSound_", harvestSound_);
        ImGuiHelper::OnDrawInputField("chatIcon_", chatIcon_);
    }
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::HerbPatch, 0)
