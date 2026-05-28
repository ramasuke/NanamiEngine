#include "Engine_Physics_ColliderBase.h"

#include "fwd.hpp"
#include "../../../../Core/Application/Time/Time.h"
#include "../../../../Core/Physics/Physics.h"
#include "../../../GameObject/Transform/Transform.h"
#include "../../Engine_Physics_Physics.h"
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
        default:
            break;
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

        userData_ = Physics::UserData(Entity());
        bodyId_ = physics.CreateCollider(
            CreateColliderShape(),
            pos,
            rot,
            emotionType_,
            mass_,
            isSensor_,
            isGravity_,
            layer_,
            &userData_
        );
    }
    
    ColliderBase::~ColliderBase()
    {
    }
    
    void ColliderBase::OnAwake()
    {
        auto& physics = Core::Application::ApplicationBase::Physics();
        auto [position, rotation] = CalcWorldTransformInternal();

        userData_ = Physics::UserData(Entity());
        bodyId_ = physics.CreateCollider(
            CreateColliderShape(),
            position,
            rotation,
            emotionType_,
            mass_,
            isSensor_,
            isGravity_,
            layer_,
            &userData_
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
    
        const JPH::RVec3 position = bodyTransform.GetTranslation();
        const JPH::Quat  rotation = bodyTransform.GetQuaternion();
    
        if (emotionType_ == JPH::EMotionType::Dynamic)
        {
            glm::vec3 newPos(position.GetX(), position.GetY(), position.GetZ());
            glm::quat newRot(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
    
            const glm::vec3 currentPos = Transform().GetWorldPos();
    
            if (HasConstraint(constraints_, Physics::Constraints::FreezePosX)) newPos.x = currentPos.x;
            if (HasConstraint(constraints_, Physics::Constraints::FreezePosY)) newPos.y = currentPos.y;
            if (HasConstraint(constraints_, Physics::Constraints::FreezePosZ)) newPos.z = currentPos.z;
    
            // Freeze Rotation
            const glm::quat curRot = Transform().GetWorldRot();
    
            glm::vec3 newEuler = glm::eulerAngles(newRot);
            const glm::vec3 curEuler = glm::eulerAngles(curRot);
    
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
        auto& physics = Core::Application::ApplicationBase::Physics();
        auto& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();

        if (!bodyId_.IsInvalid())
        {
            physics.UnSubscribeEngineCollider(bodyId_);

            bodyInterface.RemoveBody(bodyId_);
            bodyInterface.DestroyBody(bodyId_);
            bodyId_ = JPH::BodyID();
        }
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

    void ColliderBase::SetFreezePhysics(const Physics::Constraints& freeze)
    {
        if (constraints_ == freeze)
            return;

        constraints_ = freeze;

        if (BodyId().IsInvalid())
            return;

        auto& physics = Core::Application::ApplicationBase::Physics();
        auto& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();

        // 拘束違反を防ぐために、速度リセット
        JPH::Vec3 linearVelocity = bodyInterface.GetLinearVelocity(bodyId_);
        JPH::Vec3 angularVelocity = bodyInterface.GetAngularVelocity(bodyId_);

        // Freeze Position
        if (HasConstraint(constraints_, Physics::Constraints::FreezePosX)) linearVelocity.SetX(0.0f);
        if (HasConstraint(constraints_, Physics::Constraints::FreezePosY)) linearVelocity.SetY(0.0f);
        if (HasConstraint(constraints_, Physics::Constraints::FreezePosZ)) linearVelocity.SetZ(0.0f);

        // Freeze Rotation
        if (HasConstraint(constraints_, Physics::Constraints::FreezeRotX)) angularVelocity.SetX(0.0f);
        if (HasConstraint(constraints_, Physics::Constraints::FreezeRotY)) angularVelocity.SetY(0.0f);
        if (HasConstraint(constraints_, Physics::Constraints::FreezeRotZ)) angularVelocity.SetZ(0.0f);

        bodyInterface.SetLinearVelocity (bodyId_, linearVelocity);
        bodyInterface.SetAngularVelocity(bodyId_, angularVelocity);
    }

    void ColliderBase::BasedOnDrawgui()
    {
        DrawChoiceLayerGui("layer_", layer_);
        ImGuiHelper::OnDrawInputField("isGravity_", isGravity_);
        ImGuiHelper::OnDrawInputField("mass_", mass_);
        if (ImGui::TreeNode("option"))
        {
            if (ImGui::Button("Set ZeloLinearVelocity"))
            {
                Physics::SetLinearVelocity(bodyId_, glm::vec3{0.0f, 0.0f, 0.0f});
            }
            if (ImGui::Button("Set ZeloAngularVelocity"))
            {
                Physics::SetAngularVelocity(bodyId_, glm::vec3{0.0f, 0.0f, 0.0f});
            }
                
            ImGui::TreePop();
            ImGui::Spacing();
        }
    }
}