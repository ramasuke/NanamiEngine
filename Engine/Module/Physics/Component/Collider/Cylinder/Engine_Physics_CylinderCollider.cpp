#include "Engine_Physics_CylinderCollider.h"

#include "../../../../../Core/Physics/Physics.h"
#include "../../../../3DRender/Shapes/Shapes.h"
#include "../../../../GameObject/Transform/Transform.h"
#include "../../../Engine_Physics_Physics.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyInterface.h"

namespace NanamiEngine::Module::Component
{
    void CylinderCollider::OnDebugDraw() const
    {
        const auto drawPosition  = CalcColliderWorldPos();
        const glm::quat offsetRot  = glm::quat(glm::radians(offsetRotation_));
        const glm::quat drawRotation = Transform().GetWorldRot() * offsetRot;

        Render3D::Shapes::DrawCylinder3D(
            drawPosition,
            radius_ * Transform().GetWorldScale().z,
            height_ * 0.5f * Transform().GetWorldScale().y,
            drawRotation,
            GetColor(200, 200, 0)
        );
    }

    const glm::vec3& CylinderCollider::CalcColliderWorldPos() const
    {
        return Transform().GetWorldPos() + Transform().GetWorldRot() * (offset_ * Transform().GetWorldScale());
    }

    JPH::RefConst<JPH::Shape> CylinderCollider::CreateColliderShape() const
    {
        const float halfHeight = height_ * 0.5f * Transform().GetWorldScale().y;
        const float radius     = radius_        * Transform().GetWorldScale().z;

        return Physics::CreateCylinderShape(halfHeight, radius);
    }

    void CylinderCollider::OnDrawGui()
    {
        ImGui::Checkbox("isTrigger_", &isSensor_);

        ImGui::DragFloat("Radius", &radius_, 0.01f, 0.01f, 1000.0f);
        ImGui::DragFloat("Height", &height_, 0.01f, 0.01f, 1000.0f);

        glm::vec3 offset = offset_;
        if (ImGui::DragFloat3("Offset", &offset.x, 0.01f))
            offset_ = offset;

        glm::vec3 offsetRot = offsetRotation_;
        if (ImGui::DragFloat3("OffsetRotation", &offsetRot.x, 0.1f))
            offsetRotation_ = offsetRot;

        static const char* motionTypeNames[] = {
            "Static", "Kinematic", "Dynamic"
        };

        int currentIndex = static_cast<int>(emotionType_);
        if (ImGui::Combo("Motion Type", &currentIndex,
            motionTypeNames, IM_ARRAYSIZE(motionTypeNames)))
        {
            emotionType_ = static_cast<JPH::EMotionType>(currentIndex);
        }

        ImGui::Separator();
        ImGui::Text("Constraints");
        Physics::DrawConstraintCheckBoxsGui(constraints_);

        OnDebugDraw();
    }
}