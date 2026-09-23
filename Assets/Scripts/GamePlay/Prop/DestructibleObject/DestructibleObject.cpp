#include "DestructibleObject.h"

#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../Pickup/GamePlay_LootDrop.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    const GameCore::StatusParameter::Health DestructibleObject::MIN_HEALTH = GameCore::StatusParameter::Health(0);

    void DestructibleObject::OnTakeDamage(
        const std::unique_ptr<GameCore::IDamage> context)
    {
        if (isBroken_)
            return;

        currentHealth_ = GameCore::StatusParameter::Health(currentHealth_.Value() - context->DamageValue());
        if (onDamageParticle_)
        {
            Scene::GameObject::Instantiate(onDamageParticle_.get(), Transform().GetWorldPos());
        }

        if (currentHealth_ <= MIN_HEALTH)
        {
            isBroken_ = true;
            if (destroyParticle_)
            {
                Scene::GameObject::Instantiate(destroyParticle_.get(), BreakPosition());
            }
            if (dropTable_)
            {
                Pickup::DropLoot(*dropTable_.get(), BreakPosition());
            }

            Entity().lock()->OnDestroy();
        }
    }

    glm::vec3 DestructibleObject::BreakPosition()
    {
        return particlePos_ ? particlePos_->Transform().GetWorldPos() : Transform().GetWorldPos();
    }

    void DestructibleObject::OnDestroy()
    {
        std::cout << "DestructibleObject::OnDestroy" << '\n';
    }

    void DestructibleObject::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("currentHealth_", currentHealth_);
        ImGuiHelper::OnDrawInputField("onDamageParticle_", onDamageParticle_);
        ImGuiHelper::OnDrawInputField("destroyParticle_", destroyParticle_);
        ImGuiHelper::OnDrawInputField("particlePos_", particlePos_);
        ImGuiHelper::OnDrawInputField("dropTable_", dropTable_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::DestructibleObject);
#pragma endregion
