#include "Engine_Physics_SphereCollider.h"

#include "DxLib.h"
#include "../../../../../Core/Physics/Physics.h"
#include "../../../../3DRender/Shapes/Shapes.h"
#include "../../../../GameObject/Transform/Transform.h"
#include "../../../Engine_Physics_Physics.h"

namespace NanamiEngine::Module::Component
{
    void SphereCollider::OnDrawGui()
    {
        ImGui::Checkbox("isTrigger_", &isSensor_);
        ImGui::DragFloat("Radius", &radius_, 0.01f, 0.01f, 1000.0f);

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

    void SphereCollider::OnDebugDraw() const
    {
        const auto drawSpherePos = offset_ * Transform().GetWorldScale().z + Transform().GetWorldPos();
        DrawSphere3D({drawSpherePos.x, drawSpherePos.y, drawSpherePos.z}, radius_ * Transform().GetWorldScale().z, 16, GetColor(200, 200, 0), GetColor(200, 200, 0), false);
    }

    JPH::RefConst<JPH::Shape> SphereCollider::CreateColliderShape() const
    {
        const float colliderRadius = radius_ * Transform().GetWorldScale().z;
        return Physics::CreateSphereShape(colliderRadius);
    }
}
