#include "Engine_Physics_ColliderBase.h"

#include <DxLib.h>
#include "fwd.hpp"
#include "../../../../Core/Application/Configuration/DebugDraw/ApplicationConfiguration_DebugDraw.h"
#include "../../../../Core/Physics/Physics.h"
#include "../../../GameObject/Transform/Transform.h"
#include "../../BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "detail/type_quat.hpp"
#include "ext/quaternion_geometric.hpp"
#include "gtc/quaternion.hpp"

namespace NanamiEngine::Module::Component
{
    std::pair<JPH::Vec3, JPH::Quat> ColliderBase::CalcWorldTransformInternal() const
    {
        const auto& transform = Transform();

        const glm::vec3 worldPos =
            transform.GetWorldPos() +
            transform.GetWorldRot() * offset_ * transform.GetWorldScale();

        const glm::quat normRot    = glm::normalize(transform.GetWorldRot());
        const glm::quat offsetRot  = glm::quat(glm::radians(offsetRotation_));
        const glm::quat finalRot   = glm::normalize(normRot * offsetRot);

        return {
            JPH::Vec3(worldPos.x, worldPos.y, worldPos.z),
            JPH::Quat(finalRot.x, finalRot.y, finalRot.z, finalRot.w)
        };
    }

    ColliderBase::~ColliderBase()
    {
    }

    void ColliderBase::OnAwake()
    {
        Core::Application::ApplicationBase::Physics().Bodies().Register(*this);
    }

    void ColliderBase::OnBeginPhysics()
    {
        Core::Application::ApplicationBase::Physics().Bodies().MoveSensor(*this);
    }

    void ColliderBase::OnUpdatedPhysics()
    {
    }

    void ColliderBase::OnDebugRender()
    {
        if (!IsEnable())
            return;

        if (!Core::Application::Configuration::DebugDrawConfiguration::ShouldDrawCollider(ShapeKind(), layer_, isSensor_))
            return;

        OnDebugDraw();
    }

    unsigned int ColliderBase::DebugDrawColor(const unsigned int normalColor) const
    {
        return isSensor_ ? GetColor(0, 180, 255) : normalColor;
    }

    void ColliderBase::NotifyShapeChanged() const
    {
        Core::Application::ApplicationBase::Physics().Bodies().MarkDirty(*this);
    }

    std::optional<std::pair<JPH::Vec3, JPH::Quat>> ColliderBase::SimulatedWorldTransform() const
    {
        return Core::Application::ApplicationBase::Physics().Bodies().ShapeWorldTransform(*this);
    }

    void ColliderBase::SetLegacyMotion(const Physics::MotionType motionType, const Physics::Constraints constraints)
    {
        // 全軸を固定した Dynamic は Body 生成時に Static へ置き換えられていたので、Static として扱う
        constexpr auto ALL_CONSTRAINTS = static_cast<Physics::Constraints>(0b111111);
        legacyMotionType_ = motionType == Physics::MotionType::Dynamic && constraints == ALL_CONSTRAINTS
            ? Physics::MotionType::Static
            : motionType;
    }

    std::optional<std::pair<glm::vec3, glm::vec3>> ColliderBase::WorldBounds() const
    {
        return Core::Application::ApplicationBase::Physics().Bodies().WorldBounds(*this);
    }

    std::optional<glm::vec3> ColliderBase::CenterOfMassPosition() const
    {
        return Core::Application::ApplicationBase::Physics().Bodies().CenterOfMassPosition(*this);
    }

    void ColliderBase::OnDestroy()
    {
        Core::Application::ApplicationBase::Physics().Bodies().Unregister(*this);
    }

    void ColliderBase::SetLayer(const Physics::Layer layer)
    {
        if (layer_ == layer)
            return;

        layer_ = layer;
        Core::Application::ApplicationBase::Physics().Bodies().MarkDirty(*this);
    }

    void ColliderBase::SetFriction(const float friction)
    {
        if (friction_ == friction)
            return;

        friction_ = friction;
        Core::Application::ApplicationBase::Physics().Bodies().MarkDirty(*this);
    }

    void ColliderBase::BasedOnDrawgui()
    {
        const auto rigidBodyObject = Core::Application::ApplicationBase::Physics().Bodies().FindRigidBodyObject(*this);
        ImGui::Text("RigidBody: %s", rigidBodyObject ? rigidBodyObject->Name().c_str() : "なし(Static)");
        Physics::PhysicsLayers::DrawChoiceGui("layer_", layer_);
        ImGuiHelper::OnDrawInputField("friction_", friction_);
        ImGuiHelper::OnDrawInputField("offsetRotation_", offsetRotation_);
    }
}
