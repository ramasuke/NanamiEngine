#include "ColliderBase.h"

#include "fwd.hpp"
#include "../../../Core/Application/Time/Time.h"
#include "../../../Core/Physics/Physics.h"
#include "../../GameObject/Transform/Transform.h"
#include "detail/type_quat.hpp"
#include "ext/quaternion_geometric.hpp"

namespace NanamiEngine::Module::Component
{
    const std::pair<JPH::Vec3, JPH::Quat>& ColliderBase::CalcWorldTransformInternal() const
    {
        const auto& transform = Transform();
    
        const glm::vec3 worldPos =
            transform.GetWorldPos() +
            transform.GetWorldRot() * offset_ * transform.GetWorldScale();
    
        const glm::quat normRot = glm::normalize(transform.GetWorldRot());
    
        return {
            JPH::Vec3(worldPos.x, worldPos.y, worldPos.z),
            JPH::Quat(normRot.x, normRot.y, normRot.z, normRot.w)
        };
    }
    
    void ColliderBase::ApplyTransformToBody(
        JPH::BodyInterface& bodyInterface,
        const JPH::Vec3& pos,
        const JPH::Quat& rot) const
    {
        switch (emotionType_)
        {
        case JPH::EMotionType::Dynamic:
            bodyInterface.SetPositionAndRotationWhenChanged(
                bodyId_, pos, rot, JPH::EActivation::Activate);
            break;
    
        case JPH::EMotionType::Kinematic:
            bodyInterface.MoveKinematic(
                bodyId_, pos, rot, Time::DeltaTime());
            break;
        case JPH::EMotionType::Static:
            break;
        default: ;
        }
    }
    
    void ColliderBase::RecreateBody()
    {
        auto& physics = Core::Application::ApplicationBase::Physics();
        auto& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();
    
        if (!bodyId_.IsInvalid())
        {
            bodyInterface.RemoveBody(bodyId_);
            bodyInterface.DestroyBody(bodyId_);
            bodyId_ = JPH::BodyID();
        }
    
        auto [pos, rot] = CalcWorldTransformInternal();
    
        bodyId_ = physics.CreateCollider(
            CreateColliderShape(),
            pos,
            rot,
            emotionType_,
            mass_,
            isSensor_,
            isGravity_,
            &Components()
        );
    }
    
    ColliderBase::~ColliderBase()
    {
        auto& physics = Core::Application::ApplicationBase::Physics();
        auto& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();
    
        if (!bodyId_.IsInvalid())
        {
            bodyInterface.RemoveBody(bodyId_);
            bodyInterface.DestroyBody(bodyId_);
            bodyId_ = JPH::BodyID();
        }
    }
    
    void ColliderBase::OnAwake()
    {
        auto& physics = Core::Application::ApplicationBase::Physics();
        auto& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();
    
        auto [position, rotation] = CalcWorldTransformInternal();
        
        bodyId_ = physics.CreateCollider(
            CreateColliderShape(),
            position,
            rotation,
            emotionType_,
            mass_,
            isSensor_,
            isGravity_,
            &Components()
        );
    }
    
    void ColliderBase::OnBeginPhysics()
    {
        if (bodyId_.IsInvalid())
            return;
    
        auto& physics = Core::Application::ApplicationBase::Physics();
        auto& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();
    
        auto [pos, rot] = CalcWorldTransformInternal();
    
        ApplyTransformToBody(bodyInterface, pos, rot);
    }
    
    void ColliderBase::OnUpdatedPhysics()
    {
        if (bodyId_.IsInvalid())
            return;
    
        auto& physics = Core::Application::ApplicationBase::Physics();
        const auto& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();
    
        const JPH::RMat44 bodyTransform =
            bodyInterface.GetCenterOfMassTransform(bodyId_);
    
        const JPH::RVec3 pos = bodyTransform.GetTranslation();
        const JPH::Quat  rotation = bodyTransform.GetQuaternion();
    
        if (emotionType_ == JPH::EMotionType::Dynamic)
        {
            glm::vec3 newPos(pos.GetX(), pos.GetY(), pos.GetZ());
            glm::quat newRot(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
    
            // --- Freeze Position ---
            glm::vec3 curPos = Transform().GetWorldPos();
    
            if (HasConstraint(constraints_, Physics::Constraints::FreezePosX)) newPos.x = curPos.x;
            if (HasConstraint(constraints_, Physics::Constraints::FreezePosY)) newPos.y = curPos.y;
            if (HasConstraint(constraints_, Physics::Constraints::FreezePosZ)) newPos.z = curPos.z;
    
            // --- Freeze Rotation ---
            glm::quat curRot = Transform().GetWorldRot();
    
            glm::vec3 newEuler = glm::eulerAngles(newRot);
            glm::vec3 curEuler = glm::eulerAngles(curRot);
    
            if (HasConstraint(constraints_, Physics::Constraints::FreezeRotX)) newEuler.x = curEuler.x;
            if (HasConstraint(constraints_, Physics::Constraints::FreezeRotY)) newEuler.y = curEuler.y;
            if (HasConstraint(constraints_, Physics::Constraints::FreezeRotZ)) newEuler.z = curEuler.z;
    
            newRot = glm::quat(newEuler);
    
            Transform().SetWorldPos(
                newPos - newRot * offset_ * Transform().GetWorldScale());
            Transform().SetWorldRot(newRot);
        }
    }
    
    void ColliderBase::OnDestroy()
    {
        Core::Application::ApplicationBase::Physics()
            .UnSubscribeEngineCollider(bodyId_);
    }
    
    void ColliderBase::SetMotionType(const JPH::EMotionType& type)
    {
        if (emotionType_ == type)
            return;
    
        emotionType_ = type;
    
        if (BodyId().IsInvalid())
            return;
    
        RecreateBody();
    }
    
    void ColliderBase::SetGravity(const bool isGravity)
    {
        if (isGravity_ == isGravity)
            return;
    
        isGravity_ = isGravity;
    
        if (BodyId().IsInvalid())
            return;
    
        RecreateBody();
    }
    
    void ColliderBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("isGravity_", isGravity_);
        ImGuiHelper::OnDrawInputField("mass_", mass_);
    }
}