#include "PlayerAvatarStateAction.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Engine/Module/Component/Collider/ICollider.h"
#include "../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../Engine/Module/Physics/Physics_.h"
#include "../../CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "ext/quaternion_geometric.hpp"

namespace GameCore::PlayerAvatar::State
{
    PlayerAvatarStateAction::PlayerAvatarStateAction(
        const std::shared_ptr<IPlayerAvatarStateContext>& stateContext)
            : stateContext_(stateContext)
    {
        
    }
    
    void PlayerAvatarStateAction::ForwardMove(const glm::vec3& inputVelocity, const float rotateSpeed) const
    {
        const glm::vec3 cameraForward = glm::normalize(glm::vec3(stateContext_->CameraGroup().CurrentCamera().Transform().GetWorldRot() * glm::vec3(0,0,-1)));
        const glm::vec3 cameraRight   = glm::normalize(glm::vec3(stateContext_->CameraGroup().CurrentCamera().Transform().GetWorldRot() * glm::vec3(1,0, 0)));
        const glm::vec3 xzVelocity = cameraForward * inputVelocity.z + cameraRight * inputVelocity.x;
        glm::vec3 currentVelocity = Physics::GetLinearVelocity(stateContext_->PlayerAvatarCollider().BodyId());
        currentVelocity.x = xzVelocity.x;
        currentVelocity.z = xzVelocity.z;

        Physics::SetLinearVelocity(stateContext_->PlayerAvatarCollider().BodyId(), currentVelocity);
        RotateTowards(glm::vec3(xzVelocity.x, 0, xzVelocity.z), rotateSpeed);
    }
    
    void PlayerAvatarStateAction::RotateTowards(
        const glm::vec3& direction, const float rotateSpeed) const
    {
        if (glm::length2(direction) < 0.0001f)
            return;
    
        auto& playerTransform = stateContext_->PlayerAvatarTransform();
        const auto targetForward = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
    
        glm::vec3 currentForward = glm::normalize(glm::vec3(playerTransform.GetWorldRot() * glm::vec3(0, 0, -1)));
        currentForward.y = 0.0f;
        currentForward = glm::normalize(currentForward);
    
        const glm::quat currentRot = playerTransform.GetWorldRot();
        const glm::quat deltaRot   = glm::rotation(currentForward, targetForward);
        const glm::quat targetRot  = deltaRot * currentRot;
    
        // 実際に回す割合
        const float angleDiff = glm::angle(deltaRot);
        float t = angleDiff < 0.0001f ? 1.0f : glm::min(1.0f, rotateSpeed * Time::DeltaTime() / angleDiff);
        
        // 徐々に回転
        playerTransform.SetWorldRot(glm::slerp(currentRot, targetRot, t));
    }

    void PlayerAvatarStateAction::Jump(const glm::vec3& direction) const
    {
        Physics::AddForce(stateContext_->PlayerAvatarCollider().BodyId(), direction);
    }
}

