#include "Prop_AirShip.h"

#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../Npc/Friendly/FriendlyNpc.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void AirShip::OnShootDown()
    {
        // NOTE: 乗っていた NPC は落ちる前に逃げたことにする
        const auto hidePassengers = [](const auto& self, GameObject::IGameObject& gameObject) -> void
        {
            for (const auto& child : gameObject.Transform().GetChildren())
            {
                if (!child->Components().Catch<Npc::Friendly::FriendlyNpc>().expired())
                {
                    child->SetEnable(false);
                    continue;
                }
                self(self, *child);
            }
        };
        if (const auto ship = Entity().lock())
            hidePassengers(hidePassengers, *ship);

        if (!evacuatePoint_)
            return;

        const glm::vec3 shipPos = Transform().GetWorldPos();
        for (const auto& weakAvatar : GameCore::IPlayerAvatar::PlayerAvatars())
        {
            const auto avatar = weakAvatar.lock();
            if (!avatar || !avatar->IsOwner())
                continue;

            const glm::vec3 offset = glm::abs(avatar->PlayerTransform().GetWorldPos() - shipPos);
            if (glm::any(glm::greaterThan(offset, deckHalfExtents_)))
                continue;

            avatar->PlayerTransform().SetWorldPos(evacuatePoint_->Transform().GetWorldPos());
            avatar->RigidBody().SetLinearVelocity(glm::vec3(0.0f));
        }
    }

    void AirShip::OnAwake()
    {
        originPos_ = Transform().GetWorldPos();
    }

    void AirShip::OnUpdate()
    {

    }

    void AirShip::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("originPos_", originPos_);
        ImGuiHelper::OnDrawInputField("shootDownParticle_", shootDownParticle_);
        ImGuiHelper::OnDrawInputField("evacuatePoint_", evacuatePoint_);
        ImGuiHelper::OnDrawInputField("deckHalfExtents_", deckHalfExtents_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::AirShip);
#pragma endregion
