#include "DestructibleObject.h"

#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"

namespace GamePlay::Prop
{
    const GameCore::StatusParameter::Health DestructibleObject::MIN_HEALTH = GameCore::StatusParameter::Health(0);
    
    void DestructibleObject::OnTakeDamage(
        const std::unique_ptr<GameCore::IDamage> context)
    {
        currentHealth_ = GameCore::StatusParameter::Health(currentHealth_.Value() - context->DamageValue());
        if (onDamageParticle_)
        {
            Scene::GameObject::Instantiate(onDamageParticle_.get(), Transform().GetWorldPos());
        }
        
        if (currentHealth_ <= MIN_HEALTH)
        {
            if (destroyParticle_)
            {
                Scene::GameObject::Instantiate(destroyParticle_.get(), particlePos_->Transform().GetWorldPos());
            }
            if (const auto owner = GameCore::PlayerAvatar::Owner())
                owner->PlayerStatus().Wallet().Earn(dropMoney_);

            Entity().lock()->OnDestroy();
        }
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
        ImGuiHelper::OnDrawInputField("dropMoney_", dropMoney_);
    }
}
