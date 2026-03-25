#include "CapsuleCollider.h"

#include "../../../../Core/Application/Time/Time.h"
#include "../../../../Core/Physics/Physics.h"
#include "../../../3DRender/Shapes/Shapes.h"
#include "../../../GameObject/Transform/Transform.h"
#include "../../../Physics/Physics_.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyInterface.h"

namespace NanamiEngine::Module::Component
{
    void CapsuleCollider::OnDrawGui()
    {
        ImGui::Checkbox("isTrigger_", &isSensor_);

        ImGui::DragFloat("Radius", &radius_, 0.01f, 0.01f, 1000.0f);
        ImGui::DragFloat("Height", &height_, 0.01f, 0.01f, 1000.0f);

        glm::vec3 offset = offset_;
        if (ImGui::DragFloat3("Offset", &offset.x, 0.01f))
            offset_ = offset;

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

        int layerIndex = Physics::ToIndex(layer_);
        if (ImGui::Combo("Layer", &layerIndex,
            Physics::LAYER_NAMES, static_cast<int>(Physics::Layer::Count)))
        {
            layer_ = Physics::ToLayer(layerIndex);
        }

        OnDebugDraw();
    }
    
    void CapsuleCollider::OnDebugDraw() const
    {
        const auto drawPosition = CalcColliderWorldPos();
        const auto drawRotation = Transform().GetWorldRot();
    
        Render3D::Shapes::DrawCapsule3D(
            drawPosition,
            radius_ * Transform().GetWorldScale().z,
            height_ * Transform().GetWorldScale().y,
            drawRotation,
            GetColor(200, 200, 0)
        );
    }

    const glm::vec3& CapsuleCollider::CalcColliderWorldPos() const
    {
        return Transform().GetWorldPos() + Transform().GetWorldRot() * (offset_ * Transform().GetWorldScale());
    }

    JPH::RefConst<JPH::Shape> CapsuleCollider::CreateColliderShape() const
    {
        const glm::vec3 worldPos = CalcColliderWorldPos();
        const float halfHeight = height_ * 0.5f * Transform().GetWorldScale().y;
        const float radius     = radius_ * Transform().GetWorldScale().z;

        return Physics::CreateCapsuleShape(halfHeight, radius);
    }
}
