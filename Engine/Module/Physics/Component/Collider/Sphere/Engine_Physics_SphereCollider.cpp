#include "Engine_Physics_SphereCollider.h"

#include "DxLib.h"
#include "../../../../3DRender/Shapes/Shapes.h"
#include "../../../../GameObject/Transform/Transform.h"
#include "../../../JoltUtility/Engine_Physics_JoltUtility.h"
#include "../../../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Component
{
    void SphereCollider::OnDrawGui()
    {
        ImGui::Checkbox("isTrigger_", &isSensor_);
        ImGui::DragFloat("Radius", &radius_, 0.01f, 0.01f, 1000.0f);

        glm::vec3 offset = offset_;
        if (ImGui::DragFloat3("Offset", &offset.x, 0.01f))
            offset_ = offset;

        glm::vec3 offsetRot = offsetRotation_;
        if (ImGui::DragFloat3("OffsetRotation", &offsetRot.x, 0.1f))
            offsetRotation_ = offsetRot;

        int layerIndex = Physics::ToIndex(layer_);
        if (ImGui::Combo("Layer", &layerIndex,
            Physics::PhysicsLayers::Names(), Physics::PhysicsLayers::Count()))
        {
            layer_ = Physics::PhysicsLayers::ToLayer(layerIndex);
        }

        OnDebugDraw();
    }

    void SphereCollider::OnDebugDraw() const
    {
        const auto drawSpherePos = offset_ * Transform().GetWorldScale().z + Transform().GetWorldPos();
        const unsigned int color = DebugDrawColor(GetColor(0, 200, 0));
        DrawSphere3D({drawSpherePos.x, drawSpherePos.y, drawSpherePos.z}, radius_ * Transform().GetWorldScale().z, 16, color, color, false);
    }

    JPH::RefConst<JPH::Shape> SphereCollider::CreateColliderShape() const
    {
        const float colliderRadius = radius_ * Transform().GetWorldScale().z;
        return Physics::CreateSphereShape(colliderRadius);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::SphereCollider);
#pragma endregion
