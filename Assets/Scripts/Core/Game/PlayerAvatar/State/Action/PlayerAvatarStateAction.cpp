#include "PlayerAvatarStateAction.h"

#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "ext/quaternion_geometric.hpp"

namespace GameCore::PlayerAvatar::State
{
    PlayerAvatarStateAction::PlayerAvatarStateAction(
        const std::shared_ptr<IPlayerAvatarStateContext>& stateContext)
            : stateContext_(stateContext)
    {
        
    }
    
    void PlayerAvatarStateAction::MoveForward(const glm::vec3& inputVelocity, const float rotateSpeed) const
    {
        const glm::quat cameraRot = stateContext_->CameraGroup().CurrentCamera().Transform().GetWorldRot();
        glm::vec3 flatForward = cameraRot * glm::vec3(0, 0, -1);
        flatForward.y = 0.0f;
        if (glm::length2(flatForward) < 0.0001f)
        {
            // 真上・真下を向いているときは画面の上方向を前とみなす
            flatForward = cameraRot * glm::vec3(0, 1, 0);
            flatForward.y = 0.0f;
        }
        const glm::vec3 cameraForward = glm::normalize(flatForward);
        const glm::vec3 cameraRight   = glm::cross(cameraForward, glm::vec3(0, 1, 0));
        const glm::vec3 xzVelocity = cameraForward * inputVelocity.z + cameraRight * inputVelocity.x;
        const glm::vec3 walkableVelocity = LimitToWalkableSlope(glm::vec3(xzVelocity.x, 0.0f, xzVelocity.z));
        glm::vec3 currentVelocity = stateContext_->PlayerAvatarRigidBody().LinearVelocity();
        currentVelocity.x = walkableVelocity.x;
        currentVelocity.z = walkableVelocity.z;

        stateContext_->PlayerAvatarRigidBody().SetLinearVelocity(currentVelocity);
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
        const float t = angleDiff < 0.0001f ? 1.0f : glm::min(1.0f, rotateSpeed * Time::DeltaTime() / angleDiff);
        
        // 徐々に回転
        playerTransform.SetWorldRot(glm::slerp(currentRot, targetRot, t));
    }

    void PlayerAvatarStateAction::Jump(const glm::vec3& direction) const
    {
        glm::vec3 currentVelocity = stateContext_->PlayerAvatarRigidBody().LinearVelocity();
        currentVelocity.y = 0.0f;
        stateContext_->PlayerAvatarRigidBody().SetLinearVelocity(currentVelocity + direction);
    }

    glm::vec3 PlayerAvatarStateAction::LimitToWalkableSlope(const glm::vec3& horizontalVelocity) const
    {
        if (glm::length2(horizontalVelocity) < 0.0001f)
            return horizontalVelocity;

        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const float radius = stateContext_->SlopeCheckRadius();
        const auto hit = Physics::SphereCast(stateContext_->PlayerAvatarFeatStepPos() + glm::vec3(0.0f, stateContext_->SlopeCheckUpOffset() + radius, 0.0f),
                                             radius,
                                             horizontalVelocity, stateContext_->SlopeCheckDistance(),
                                             mask);
        if (!hit.Hit())
            return horizontalVelocity;

        const glm::vec3& normal = hit.Normal();
        if (normal.y >= std::cos(glm::radians(stateContext_->MaxWalkableSlope_deg())))
            return horizontalVelocity;

        // 急な面は壁とみなし、面に沿って横へ滑る成分だけ残す
        const glm::vec3 wallNormal(normal.x, 0.0f, normal.z);
        if (glm::length2(wallNormal) < 0.0001f)
            return horizontalVelocity;

        const glm::vec3 wallDirection = glm::normalize(wallNormal);
        const float intoWall = glm::dot(horizontalVelocity, wallDirection);
        return intoWall < 0.0f ? horizontalVelocity - intoWall * wallDirection : horizontalVelocity;
    }
}

