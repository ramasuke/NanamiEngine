#include "Engine_Physics_JoltUtility.h"

#include "../../../Core/Application/ApplicationBase.h"
#include "../../../Core/Physics/Physics.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/TransformedShape.h"
#include "trigonometric.hpp"

glm::vec3 NanamiEngine::Module::Physics::GetCenterOfMassPosition(const JPH::BodyID& bodyId)
{
    const auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    return ToVec3(bodyInterface.GetCenterOfMassPosition(bodyId));
}

std::pair<glm::vec3, glm::vec3> NanamiEngine::Module::Physics::GetWorldSpaceBounds(const JPH::BodyID& bodyId)
{
    const auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    const JPH::AABox bounds = bodyInterface.GetTransformedShape(bodyId).GetWorldSpaceBounds();
    return { ToVec3(bounds.mMin), ToVec3(bounds.mMax) };
}

glm::vec3 NanamiEngine::Module::Physics::GetLinearVelocity(const JPH::BodyID& bodyId)
{
    const auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    return ToVec3(bodyInterface.GetLinearVelocity(bodyId));
}

void NanamiEngine::Module::Physics::SetLinearVelocity(const JPH::BodyID& bodyId, const glm::vec3& velocity)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    bodyInterface.SetLinearVelocity(bodyId, ToJPHVec3(velocity));
}

void NanamiEngine::Module::Physics::AddLinearVelocity(const JPH::BodyID& bodyId, const glm::vec3& velocity)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();
    const JPH::Vec3 current = bodyInterface.GetLinearVelocity(bodyId);

    bodyInterface.SetLinearVelocity(
        bodyId,
        current + ToJPHVec3(velocity)
    );
}

glm::vec3 NanamiEngine::Module::Physics::GetAngularVelocity(const JPH::BodyID& bodyId)
{
    const auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    // Jolt → rad/s
    const JPH::Vec3 angVelRad = bodyInterface.GetAngularVelocity(bodyId);

    // rad → deg
    return glm::degrees(ToVec3(angVelRad));
}

void NanamiEngine::Module::Physics::SetAngularVelocity(
    const JPH::BodyID& bodyId,
    const glm::vec3& angularVelocity)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    // deg → rad
    const glm::vec3 rad = glm::radians(angularVelocity);

    bodyInterface.SetAngularVelocity(bodyId, ToJPHVec3(rad));
}

void NanamiEngine::Module::Physics::AddTorque(const JPH::BodyID& bodyId, const glm::vec3& torque)
{
    auto& bodyInterface =
        Core::Application::ApplicationBase::Physics()
        .GetPhysicsSystem()
        .GetBodyInterface();

    bodyInterface.AddTorque(bodyId, ToJPHVec3(torque));
}

JPH::RefConst<JPH::Shape> NanamiEngine::Module::Physics::CreateBoxShape(const JPH::Vec3& halfSize)
{
    return new JPH::BoxShape(halfSize);
}

JPH::RefConst<JPH::Shape> NanamiEngine::Module::Physics::CreateCapsuleShape(float halfHeight, float radius)
{
    return new JPH::CapsuleShape(halfHeight, radius);
}

JPH::RefConst<JPH::Shape> NanamiEngine::Module::Physics::CreateSphereShape(const float radius)
{
    return new JPH::SphereShape(radius);
}

JPH::RefConst<JPH::Shape> NanamiEngine::Module::Physics::CreateCylinderShape(float halfHeight, float radius)
{
    return new JPH::CylinderShape(halfHeight, radius);
}
