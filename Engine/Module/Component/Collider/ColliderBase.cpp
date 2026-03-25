#include "ColliderBase.h"

#include "fwd.hpp"
#include "../../../Core/Application/Time/Time.h"
#include "../../../Core/Physics/Physics.h"
#include "../../GameObject/Transform/Transform.h"
#include "detail/type_quat.hpp"
#include "ext/quaternion_geometric.hpp"

Component::ColliderBase::~ColliderBase()
{
    auto& physics = Core::Application::ApplicationBase::Physics();
    JPH::BodyInterface& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();

    if (!bodyId_.IsInvalid())
    {
        bodyInterface.RemoveBody (bodyId_);
        bodyInterface.DestroyBody(bodyId_);
        bodyId_ = JPH::BodyID();
    }
}

void Component::ColliderBase::ChangeEmotionType(const JPH::EMotionType& type)
{
    if (emotionType_ == type)
        return;

    emotionType_ = type;
    
    if (BodyId().IsInvalid())
        return;

    auto& physics = Core::Application::ApplicationBase::Physics();
    JPH::BodyInterface& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();

    bodyInterface.RemoveBody (BodyId());
    bodyInterface.DestroyBody(BodyId());
    bodyId_ = JPH::BodyID();

    const glm::vec3 worldPos = Transform().GetWorldPos() + Transform().GetWorldRot() * offset_ * Transform().GetWorldScale();
    const JPH::Vec3 jphPos(worldPos.x, worldPos.y, worldPos.z);

    const glm::quat rot = glm::normalize(Transform().GetWorldRot());
    const JPH::Quat jphRot(rot.x, rot.y, rot.z, rot.w);

    bodyId_ = physics.CreateCollider(
        CreateColliderShape(),
        jphPos,
        jphRot,
        emotionType_,
        isSensor_,
        IsGravity(),
        &Components()
    );
}

void Component::ColliderBase::ChangeIsGravity(const bool isGravity)
{
    if (isGravity_ == isGravity)
        return;

    isGravity_ = isGravity;

    if (BodyId().IsInvalid())
        return;

    auto& physics = Core::Application::ApplicationBase::Physics();
    JPH::BodyInterface& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();

    // 一旦削除
    bodyInterface.RemoveBody (BodyId());
    bodyInterface.DestroyBody(BodyId());
    bodyId_ = JPH::BodyID();

    // 再生成
    const glm::vec3 worldPos =
        Transform().GetWorldPos() +
        Transform().GetWorldRot() * offset_ * Transform().GetWorldScale();

    const JPH::Vec3 jphPos(worldPos.x, worldPos.y, worldPos.z);

    const glm::quat rot = glm::normalize(Transform().GetWorldRot());
    const JPH::Quat jphRot(rot.x, rot.y, rot.z, rot.w);

    bodyId_ = physics.CreateCollider(
        CreateColliderShape(),
        jphPos,
        jphRot,
        emotionType_,
        isSensor_,
        isGravity_,
        &Components()
    );
}

void Component::ColliderBase::OnAwake()
{
    const auto transformScale = Transform().GetWorldScale();
    const auto boxColliderPosition = offset_ * transformScale *  + Transform().GetWorldPos();
    
    bodyId_ = Core::Application::ApplicationBase::Physics().CreateCollider(
        CreateColliderShape(),
        JPH::Vec3(boxColliderPosition.x, boxColliderPosition.y, boxColliderPosition.z),
        JPH::Quat::sIdentity(),
        emotionType_,
        isSensor_,
        IsGravity(),
        &Components()
    );
}

void Component::ColliderBase::OnBeginPhysics()
{
    auto& physics = Core::Application::ApplicationBase::Physics();
    JPH::BodyInterface& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();

    const glm::vec3 worldPos = Transform().GetWorldPos() + Transform().GetWorldRot() * offset_ * Transform().GetWorldScale();
    const JPH::Vec3 jphPos(worldPos.x, worldPos.y, worldPos.z);
    const glm::quat rawRot  = Transform().GetWorldRot();
    const glm::quat normRot = glm::normalize(rawRot);
    const JPH::Quat jphRot(normRot.x, normRot.y, normRot.z, normRot.w);
    const JPH::Quat finalRot = jphRot;

    //現在の状態を物理エンジン内のオブジェクトに設定を同期
    switch (emotionType_)
    {
    case JPH::EMotionType::Dynamic:
        bodyInterface.SetPositionAndRotation(bodyId_, jphPos, finalRot, JPH::EActivation::Activate);
        break;
    case JPH::EMotionType::Kinematic:
        bodyInterface.MoveKinematic(
            bodyId_,
            jphPos,
            finalRot,
            Time::DeltaTime());
        break;
    case JPH::EMotionType::Static:
        bodyInterface.SetPositionAndRotation(bodyId_, jphPos, finalRot, JPH::EActivation::DontActivate);
    }
}

void Component::ColliderBase::OnUpdatedPhysics()
{
    auto& physics = Core::Application::ApplicationBase::Physics();
    const JPH::BodyInterface& bodyInterface = physics.GetPhysicsSystem().GetBodyInterface();
    const JPH::RMat44 bodyTransform = bodyInterface.GetCenterOfMassTransform(bodyId_);

    const JPH::RVec3 pos      = bodyTransform.GetTranslation();
    const JPH::Quat  rotation = bodyTransform.GetQuaternion();

    if (emotionType_ == JPH::EMotionType::Dynamic)
    {
        auto newPos = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
        auto newRot = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

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
        Transform().SetWorldPos(newPos - newRot * offset_ * Transform().GetWorldScale());
        Transform().SetWorldRot(newRot);
    }
}

void Component::ColliderBase::BasedOnDrawgui()
{
    ImGuiHelper::OnDrawInputField("isGravity_", isGravity_);
}

void Component::ColliderBase::OnDestroy()
{
    Core::Application::ApplicationBase::Physics().UnSubscribeEngineCollider(bodyId_);
}
