#include "Engine_Physics_RigidBody.h"

#include "../../../../Core/Application/ApplicationBase.h"
#include "../../../../Core/Physics/Physics.h"
#include "../../BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "../../JoltUtility/Engine_Physics_JoltUtility.h"
#include "../../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Component
{
    namespace
    {
        Physics::BodyAssembler& RigidBodyBodies()
        {
            return Core::Application::ApplicationBase::Physics().Bodies();
        }
    }

    void RigidBody::SetMotionType(const Physics::MotionType motionType)
    {
        if (motionType_ == motionType)
            return;

        motionType_ = motionType;
        RigidBodyBodies().MarkDirty(*this);
    }

    void RigidBody::SetGravity(const bool isGravity)
    {
        if (isGravity_ == isGravity)
            return;

        isGravity_ = isGravity;
        RigidBodyBodies().MarkDirty(*this);
    }

    void RigidBody::SetFreezePhysics(const Physics::Constraints constraints)
    {
        if (constraints_ == constraints)
            return;

        constraints_ = constraints;
        RigidBodyBodies().MarkDirty(*this);
    }

    void RigidBody::SetPartOfParent(const bool isPartOfParent)
    {
        if (isPartOfParent_ == isPartOfParent)
            return;

        isPartOfParent_ = isPartOfParent;
        RigidBodyBodies().MarkDirty(*this);
    }

    glm::vec3 RigidBody::LinearVelocity() const
    {
        const auto bodyId = RigidBodyBodies().BodyOf(*this);
        return bodyId ? Physics::GetLinearVelocity(*bodyId) : glm::vec3(0.0f);
    }

    void RigidBody::SetLinearVelocity(const glm::vec3& velocity) const
    {
        if (const auto bodyId = RigidBodyBodies().BodyOf(*this))
            Physics::SetLinearVelocity(*bodyId, velocity);
    }

    void RigidBody::AddLinearVelocity(const glm::vec3& velocity) const
    {
        if (const auto bodyId = RigidBodyBodies().BodyOf(*this))
            Physics::AddLinearVelocity(*bodyId, velocity);
    }

    glm::vec3 RigidBody::AngularVelocity() const
    {
        const auto bodyId = RigidBodyBodies().BodyOf(*this);
        return bodyId ? Physics::GetAngularVelocity(*bodyId) : glm::vec3(0.0f);
    }

    void RigidBody::SetAngularVelocity(const glm::vec3& angularVelocity) const
    {
        if (const auto bodyId = RigidBodyBodies().BodyOf(*this))
            Physics::SetAngularVelocity(*bodyId, angularVelocity);
    }

    void RigidBody::AddTorque(const glm::vec3& torque) const
    {
        if (const auto bodyId = RigidBodyBodies().BodyOf(*this))
            Physics::AddTorque(*bodyId, torque);
    }

    std::optional<glm::vec3> RigidBody::CenterOfMassPosition() const
    {
        const auto bodyId = RigidBodyBodies().BodyOf(*this);
        if (!bodyId)
            return std::nullopt;

        return Physics::GetCenterOfMassPosition(*bodyId);
    }

    std::optional<std::pair<glm::vec3, glm::vec3>> RigidBody::WorldBounds() const
    {
        const auto bodyId = RigidBodyBodies().BodyOf(*this);
        if (!bodyId)
            return std::nullopt;

        return Physics::GetWorldSpaceBounds(*bodyId);
    }

    void RigidBody::OnAwake()
    {
        RigidBodyBodies().Register(*this);
    }

    void RigidBody::OnBeginPhysics()
    {
        RigidBodyBodies().PushTransform(*this);
    }

    void RigidBody::OnUpdatedPhysics()
    {
        RigidBodyBodies().PullTransform(*this);
    }

    void RigidBody::OnDestroy()
    {
        RigidBodyBodies().Unregister(*this);
    }

    void RigidBody::OnDrawGui()
    {
        if (Physics::MotionType motionType = motionType_; Physics::DrawChoiceMotionTypeGui(("Motion Type##" + GetGuid().Value()).c_str(), motionType))
            SetMotionType(motionType);

        float mass = mass_;
        if (ImGui::DragFloat(("Mass##" + GetGuid().Value()).c_str(), &mass, 0.01f, 0.0f))
        {
            mass_ = mass;
            RigidBodyBodies().MarkDirty(*this);
        }

        if (bool isGravity = isGravity_; ImGui::Checkbox(("Gravity##" + GetGuid().Value()).c_str(), &isGravity))
            SetGravity(isGravity);

        if (bool isPartOfParent = isPartOfParent_; ImGui::Checkbox(("Part Of Parent##" + GetGuid().Value()).c_str(), &isPartOfParent))
            SetPartOfParent(isPartOfParent);

        ImGui::Separator();
        ImGui::Text("Constraints");
        Physics::Constraints constraints = constraints_;
        Physics::DrawConstraintCheckBoxsGui(constraints);
        SetFreezePhysics(constraints);

        ImGui::Separator();
        ImGui::Text("Colliders: %zu", RigidBodyBodies().AttachedColliderCount(*this));
        if (ImGui::Button(("Set Zero LinearVelocity##" + GetGuid().Value()).c_str()))
            SetLinearVelocity(glm::vec3(0.0f));
        if (ImGui::Button(("Set Zero AngularVelocity##" + GetGuid().Value()).c_str()))
            SetAngularVelocity(glm::vec3(0.0f));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::RigidBody);
#pragma endregion
