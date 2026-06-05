#include "Engine_Physics_ColliderBase.h"

#include "fwd.hpp"
#include "../../../../Core/Application/Time/Time.h"
#include "../../../../Core/Physics/Physics.h"
#include "../../../GameObject/Transform/Transform.h"
#include "../../Engine_Physics_Physics.h"
#include "Jolt/Physics/Body/AllowedDOFs.h"
#include "detail/type_quat.hpp"
#include "ext/quaternion_geometric.hpp"

namespace NanamiEngine::Module::Component
{
    namespace
    {
        JPH::EAllowedDOFs ToAllowedDOFs(const Physics::Constraints constraints)
        {
            uint8_t dofs = static_cast<uint8_t>(JPH::EAllowedDOFs::All);

            if (HasConstraint(constraints, Physics::Constraints::FreezePosX))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationX);
            if (HasConstraint(constraints, Physics::Constraints::FreezePosY))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationY);
            if (HasConstraint(constraints, Physics::Constraints::FreezePosZ))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationZ);
            if (HasConstraint(constraints, Physics::Constraints::FreezeRotX))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationX);
            if (HasConstraint(constraints, Physics::Constraints::FreezeRotY))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationY);
            if (HasConstraint(constraints, Physics::Constraints::FreezeRotZ))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationZ);

            return static_cast<JPH::EAllowedDOFs>(dofs);
        }

        void ApplyFreezeToVelocities(
            const Physics::Constraints constraints,
            JPH::Vec3& linearVelocity,
            JPH::Vec3& angularVelocity)
        {
            if (HasConstraint(constraints, Physics::Constraints::FreezePosX)) linearVelocity.SetX(0.0f);
            if (HasConstraint(constraints, Physics::Constraints::FreezePosY)) linearVelocity.SetY(0.0f);
            if (HasConstraint(constraints, Physics::Constraints::FreezePosZ)) linearVelocity.SetZ(0.0f);
            if (HasConstraint(constraints, Physics::Constraints::FreezeRotX)) angularVelocity.SetX(0.0f);
            if (HasConstraint(constraints, Physics::Constraints::FreezeRotY)) angularVelocity.SetY(0.0f);
            if (HasConstraint(constraints, Physics::Constraints::FreezeRotZ)) angularVelocity.SetZ(0.0f);
        }
    }

    std::pair<JPH::Vec3, JPH::Quat> ColliderBase::CalcWorldTransformInternal() const
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
        switch (bodyInterface.GetMotionType(bodyId_))
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
        RecreateBody(JPH::Vec3::sZero(), JPH::Vec3::sZero());
    }

    void ColliderBase::RecreateBody(const JPH::Vec3& linearVelocity, const JPH::Vec3& angularVelocity)
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
            ToAllowedDOFs(constraints_),
            &userData_
        );

        if (bodyInterface.GetMotionType(bodyId_) == JPH::EMotionType::Dynamic)
        {
            bodyInterface.SetLinearVelocity(bodyId_, linearVelocity);
            bodyInterface.SetAngularVelocity(bodyId_, angularVelocity);
        }
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
            ToAllowedDOFs(constraints_),
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
    
        if (emotionType_ == JPH::EMotionType::Dynamic)
        {
            const JPH::RVec3 position = bodyTransform.GetTranslation();
            const JPH::Quat  rotation = bodyTransform.GetQuaternion();
            glm::vec3 newPos(position.GetX(), position.GetY(), position.GetZ());
            glm::quat newRot(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

            Transform().SetWorldPos(newPos - newRot * offset_ * Transform().GetWorldScale());
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
        ApplyFreezeToVelocities(constraints_, linearVelocity, angularVelocity);

        RecreateBody(linearVelocity, angularVelocity);
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
