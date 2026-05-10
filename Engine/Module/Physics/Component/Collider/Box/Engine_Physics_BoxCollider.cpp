#include "Engine_Physics_BoxCollider.h"

#include "../../../../../Core/Application/Time/Time.h"
#include "../../../../../Core/Physics/Physics.h"
#include "../../../../3DRender/Shapes/Shapes.h"
#include "../../../../GameObject/Transform/Transform.h"
#include "../../../Engine_Physics_Physics.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyInterface.h"

void Component::BoxCollider::OnDrawGui()
{
    ImGui::Checkbox("isTrigger_", &isSensor_);
    glm::vec3 size = size_;
    if(ImGui::DragFloat3(("size##" + GetGuid().Value()).c_str(), &size.x, 0.01f))
    {
        size_ = size;
    }
    glm::vec3 offset = offset_;
    if(ImGui::DragFloat3(("offset##" + GetGuid().Value()).c_str(), &offset.x, 0.01f))
    {
        offset_ = offset;
    }
    
    static const char* motionTypeNames[] = {
        "Static", "Kinematic", "Dynamic"
    };

    int currentIndex = static_cast<int>(emotionType_);
    if (ImGui::Combo("Motion Type", &currentIndex, motionTypeNames, IM_ARRAYSIZE(motionTypeNames)))
    {
        emotionType_ = static_cast<JPH::EMotionType>(currentIndex);
    }
    ImGui::Separator();
    ImGui::Text("Constraints");
    Physics::DrawConstraintCheckBoxsGui(constraints_);
    
    int layerIndex = Physics::ToIndex(layer_);
    if (ImGui::Combo("Layer", &layerIndex, Physics::LAYER_NAMES, static_cast<int>(Physics::Layer::Count)))
    {
        layer_ = Physics::ToLayer(layerIndex);
    }
    OnDebugDraw();
}

void Component::BoxCollider::OnDebugDraw() const
{
    const glm::vec3 worldPos = Transform().GetWorldPos() + Transform().GetWorldRot() * offset_ * Transform().GetWorldScale();
    const glm::quat worldRot = Transform().GetWorldRot();
    const glm::vec3 halfSize = size_ * Transform().GetWorldScale() * 0.5f;

    // OnDebugDrawDxCube
    std::array localVertices = {
        glm::vec3{-halfSize.x, -halfSize.y, -halfSize.z},
        glm::vec3{ halfSize.x, -halfSize.y, -halfSize.z},
        glm::vec3{ halfSize.x,  halfSize.y, -halfSize.z},
        glm::vec3{-halfSize.x,  halfSize.y, -halfSize.z},
        glm::vec3{-halfSize.x, -halfSize.y,  halfSize.z},
        glm::vec3{ halfSize.x, -halfSize.y,  halfSize.z},
        glm::vec3{ halfSize.x,  halfSize.y,  halfSize.z},
        glm::vec3{-halfSize.x,  halfSize.y,  halfSize.z}
    };

    // 回転, 平行移動を適用
    for (auto& v : localVertices)
    {
        v = worldPos + worldRot * v;
    }

    Render3D::Shapes::DrawCube3DFromVertices(localVertices, GetColor(0,200,0));
}

JPH::RefConst<JPH::Shape> Component::BoxCollider::CreateColliderShape() const
{
    const auto transformScale = Transform().GetWorldScale();
    const auto colliderScale = size_ * transformScale * 0.5f;
    const auto colliderShapeScale = JPH::Vec3(colliderScale.x, colliderScale.y, colliderScale.z);
    
    return Physics::CreateBoxShape(colliderShapeScale);
}
