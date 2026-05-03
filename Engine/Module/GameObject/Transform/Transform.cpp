#include "Transform.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/quaternion.hpp>
#include <algorithm>

#include "ImGuiHelper.h"
#include "ImGuizmo.h"
#include "../../../../Libs/LibCore/cereal/PrefabExtractArchive/PrefabExtractArchive.h"
#include "../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../../Scene/GameObject/SceneGameObject/SceneGameObject.h"
#include "../Interface/IGameObject.h"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "gtc/type_ptr.hpp"

namespace NanamiEngine::Module::GameObject
{
    Transform::Transform()
        : guiLocalEuler_(), localPos_(0.0f), localRot_(glm::quat()), localScale_(1.0f), worldMatrix_(1.0f)
    {
    }
    
    void Transform::InitTransform(const std::weak_ptr<IGameObject>& parent, const std::weak_ptr<IGameObject>& ownerGameObject)
    {
        parent_ = parent;
        ownerGameObject_ = ownerGameObject;
        for (auto& child : children_)
        {
            child->InitGameObject(ownerGameObject, child);
        }
        guiLocalEuler_ = glm::degrees(glm::eulerAngles(localRot_));
    }

    void Transform::SetWorldMatrix(const glm::mat4& worldMatrix)
    {
        if (const auto parentObj = parent_.lock())
        {
            const glm::mat4 parentWorldMatrix = parentObj->Transform().GetWorldMatrix();
            const glm::mat4 localMatrix = glm::inverse(parentWorldMatrix) * worldMatrix;
            SetLocalMatrix(localMatrix);
        }
        else
        {
            SetLocalMatrix(worldMatrix);
        }
    }

    void Transform::SetWorldPos(const glm::vec3& worldPos)
    {
        if (const auto parentObj = parent_.lock())
        {
            const glm::mat4 parentWorldMatrix = parentObj->Transform().GetWorldMatrix();
            const glm::mat4 inverseParentMatrix = glm::inverse(parentWorldMatrix);
            const glm::vec4 localPos4 = inverseParentMatrix * glm::vec4(worldPos, 1.0f);
            localPos_ = glm::vec3(localPos4);
        }
        else
        {
            localPos_ = worldPos;
        }
    
        UpdateMatrix();
    }
    
    void Transform::SetWorldRot(const glm::quat& worldRot)
    {
        if (const auto parentObj = parent_.lock())
        {
            const glm::quat parentWorldRot = parentObj->Transform().GetWorldRot();
            localRot_ = glm::normalize(glm::inverse(parentWorldRot) * worldRot);
        }
        else
        {
            localRot_ = glm::normalize(worldRot);
        }
    
        guiLocalEuler_ = glm::degrees(glm::eulerAngles(localRot_));
        UpdateMatrix();
    }

    void Transform::SetWorldScale(const glm::vec3& worldScale)
    {
        if (const auto parentObj = parent_.lock())
        {
            const glm::vec3 parentWorldScale = parentObj->Transform().GetWorldScale();

            // ゼロ割防止
            localScale_ = {
                parentWorldScale.x != 0.0f ? worldScale.x / parentWorldScale.x : worldScale.x,
                parentWorldScale.y != 0.0f ? worldScale.y / parentWorldScale.y : worldScale.y,
                parentWorldScale.z != 0.0f ? worldScale.z / parentWorldScale.z : worldScale.z
            };
        }
        else
        {
            localScale_ = worldScale;
        }

        UpdateMatrix();        
    }

    void Transform::SetLocalMatrix(const glm::mat4& localMatrix)
    {
        localPos_   = glm::vec3(localMatrix[3]);
        localRot_   = glm::quat_cast(localMatrix);
        localScale_ = {
            glm::length(glm::vec3(localMatrix[0])),
            glm::length(glm::vec3(localMatrix[1])),
            glm::length(glm::vec3(localMatrix[2]))
        };

        guiLocalEuler_ = glm::degrees(glm::eulerAngles(localRot_));

        if (const auto parentObj = parent_.lock())
        {
            worldMatrix_ = parentObj->Transform().GetWorldMatrix() * localMatrix;
        }
        else
        {
            worldMatrix_ = localMatrix;
        }

        for (const auto& child : children_)
        {
            if (child)
                child->Transform().UpdateMatrix();
        }
    }

    void Transform::SetLocalPos(const glm::vec3& pos)
    {
        localPos_ = pos;
        UpdateMatrix();
    }
    
    void Transform::SetLocalRot(const glm::quat& rot)
    {   
        localRot_ = glm::normalize(rot);
        UpdateMatrix();
    }
    
    void Transform::SetLocalScale(const glm::vec3& scale)
    {
        localScale_ = scale;
        UpdateMatrix();
    }

    void Transform::LookAt(const glm::vec3& targetPos)
    {
        const auto worldPos = GetWorldPos();
        const glm::vec3 forward = glm::normalize(targetPos - worldPos);
        
        if (glm::length2(forward) < 1e-6f)
            return;

        constexpr auto up = glm::vec3(0, 1, 0);
        
        const auto right = glm::normalize(glm::cross(up, forward));
        const auto upReal = glm::cross(forward, right);

        const glm::mat3 rotM(right, upReal, forward);
        const glm::quat worldRot = glm::quat_cast(rotM);

        SetWorldRot(worldRot);
    }

    void Transform::LookAtY(const glm::vec3& targetPos)
    {
        const glm::vec3 worldPos = GetWorldPos();

        glm::vec3 direction = worldPos - targetPos;
        direction.y = 0;
        if (glm::length2(direction) < 1e-6f)
            return;

        direction = glm::normalize(direction);

        const float angle = atan2(direction.x, direction.z);
        SetWorldRot(glm::angleAxis(angle, glm::vec3(0, 1, 0)));
    }

    void Transform::Translate(const glm::vec3& delta)
    {
        localPos_ += delta;
        UpdateMatrix();
    }
    
    void Transform::Rotate(const glm::quat& deltaRot)
    {
        localRot_ = glm::normalize(deltaRot * localRot_);
        UpdateMatrix();
    }
    
    glm::vec3 Transform::GetWorldPos() const
    {
        return glm::vec3(worldMatrix_[3]);
    }
    
    glm::quat Transform::GetWorldRot() const
    {
        auto rotMat = glm::mat3(worldMatrix_);

        rotMat[0] = glm::normalize(rotMat[0]);
        rotMat[1] = glm::normalize(rotMat[1]);
        rotMat[2] = glm::normalize(rotMat[2]);

        return glm::quat_cast(rotMat);
    }

    glm::vec3 Transform::GetWorldEulerAngle() const
    {
        // World回転を quat として取得
        const glm::quat worldRot = GetWorldRot();

        // quat → euler(rad)
        glm::vec3 eulerRad = glm::eulerAngles(worldRot);

        // rad → deg
        glm::vec3 eulerDeg = glm::degrees(eulerRad);

        // 0 ~ 360 に正規化
        auto normalize360 = [](float deg)
        {
            deg = std::fmod(deg, 360.0f);
            if (deg < 0.0f)
                deg += 360.0f;
            return deg;
        };

        return {
            normalize360(eulerDeg.x),
            normalize360(eulerDeg.y),
            normalize360(eulerDeg.z)
        };
    }

    glm::vec3 Transform::GetWorldScale() const
    {
        return {
            glm::length(glm::vec3(worldMatrix_[0])),
            glm::length(glm::vec3(worldMatrix_[1])),
            glm::length(glm::vec3(worldMatrix_[2]))
        };
    }
    
    VECTOR Transform::GetDxWorldPos() const
    {
        const auto position = GetWorldPos();
        return VGet(position.x, position.y, position.z);
    }
    
    MATRIX Transform::GetDxWorldMatrix() const
    {
        const glm::mat4& m = worldMatrix_;
        MATRIX dxMat;
    
        dxMat.m[0][0] = m[0][0]; dxMat.m[0][1] = m[0][1]; dxMat.m[0][2] = m[0][2]; dxMat.m[0][3] = m[0][3];
        dxMat.m[1][0] = m[1][0]; dxMat.m[1][1] = m[1][1]; dxMat.m[1][2] = m[1][2]; dxMat.m[1][3] = m[1][3];
        dxMat.m[2][0] = m[2][0]; dxMat.m[2][1] = m[2][1]; dxMat.m[2][2] = m[2][2]; dxMat.m[2][3] = m[2][3];
        dxMat.m[3][0] = m[3][0]; dxMat.m[3][1] = m[3][1]; dxMat.m[3][2] = m[3][2]; dxMat.m[3][3] = m[3][3];
    
        return dxMat;
    }
    
    void Transform::SetParent(const std::weak_ptr<IGameObject>& parent, const bool keepWorldScale)
    {
        const glm::vec3 oldWorldPos   = GetWorldPos();
        const glm::quat oldWorldRot   = GetWorldRot();
        const glm::vec3 oldWorldScale = GetWorldScale();

        if (parent_.lock() == parent.lock())
            return;

        if (const auto oldParent = parent_.lock())
            oldParent->Transform().RemoveChild(ownerGameObject_.lock());

        parent_ = parent;

        if (const auto newParent = parent_.lock())
            newParent->Transform().AddChild(ownerGameObject_.lock());

        SetWorldPos(oldWorldPos);
        SetWorldRot(oldWorldRot);

        if (keepWorldScale)
        {
            SetWorldScale(oldWorldScale);
        }
        else
        {
            
        }

        UpdateMatrix();
    }

    std::vector<std::shared_ptr<IGameObject>> Transform::GetAllChildren() const
    {
        std::vector<std::shared_ptr<IGameObject>> result;
        result.reserve(children_.size());

        for (const auto& child : children_)
        {
            if (!child)
                continue;

            result.emplace_back(child);

            // child の Transform からさらに取得
            const auto& childTransform = child->Transform();
            auto subChildren = childTransform.GetAllChildren();

            result.insert(
                result.end(),
                subChildren.begin(),
                subChildren.end()
            );
        }

        return result;
    }

    std::vector<std::shared_ptr<IGameObject>> Transform::GetChildren() const
    {
        return children_;
    }

    std::shared_ptr<IGameObject> Transform::CatchChild(const std::string& childName) const
    {
        for (const auto& child : GetAllChildren())
        {
            if (child->Name() != childName)
                continue;

            return child;
        }
        return nullptr;
    }

    void Transform::AddChild(const std::shared_ptr<IGameObject>& child)
    {
        if (!child)
            return;
    
        auto& childTransform = child->Transform();
    
        const bool alreadyExists = std::ranges::any_of(children_, [&](const std::shared_ptr<IGameObject>& c)
        {
            return c == child;
        });
    
        if (!alreadyExists)
            children_.emplace_back(child);
    
        childTransform.UpdateMatrix();
    }
    
    void Transform::RemoveChild(const std::shared_ptr<IGameObject>& child)
    {
        std::erase_if(children_, [&](const std::shared_ptr<IGameObject>& c) {
            return c == child;
        });
    }

    glm::mat4 Transform::GetLocalMatrix() const
    {
        const glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), localPos_)
            * glm::toMat4(localRot_)
            * glm::scale(glm::mat4(1.0f), localScale_);

        return localMatrix;
    }

    void Transform::InitForCopied() const
    {
        for (const auto& child : GetAllChildren())
        {
            child->InitForCopied(
                child,
                child->IsEnable(),
                child->Name(),
                child->Components(),
                child->Transform());
        }
    }

    void Transform::OnEnable(const bool enable) const
    {
        for (const auto& child : children_)
        {
            if (!child)
                continue;
            
            child->SetEnable(enable);
        }
    }

    void Transform::OnDrawGui()
    {
        if (ImGui::CollapsingHeader("Transform"))
        {
            // GUI スライダー
            glm::vec3 pos = localPos_;
            if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
            {
                SetLocalPos(pos);
                UpdateMatrix();
            }
    
            glm::vec3 euler = guiLocalEuler_;
            if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f))
            {
                guiLocalEuler_ = euler;
                SetLocalRot(glm::quat(glm::radians(euler)));
                UpdateMatrix();
            }
    
            glm::vec3 scale = localScale_;
            if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
            {
                SetLocalScale(scale);
                UpdateMatrix();
            }
            
            if (ImGui::TreeNode("Option"))
            {
                if (ImGui::Button("Set camera position"))
                {
                    SetWorldPos(Core::Application::ApplicationBase::GameWindow()->GetCameraPosition());
                }
                if (ImGui::Button("Set camera rotation"))
                {
                    SetWorldRot(Core::Application::ApplicationBase::GameWindow()->GetCameraRotation());
                }
                ImGui::Spacing();
                ImGui::TreePop();
            }
        }
        
        if (Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
        {
            OnDrawGuizmoGui();
        }
    }

    void Transform::OnDrawGuizmoGui()
    {
        // ImGuizmo 操作
        ImGuizmo::BeginFrame();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        const ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
            
        // カメラのビュー
        glm::mat4 view = Core::Application::ApplicationBase::GameWindow()->GetCameraViewMatrix();
        glm::mat4 proj = Core::Application::ApplicationBase::GameWindow()->GetCameraProjectionMatrix();
            
        // ワールド行列を取得して Manipulate に渡す
        glm::mat4 worldMatrix = GetWorldMatrix();
            
        static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE mode = ImGuizmo::LOCAL;
            
        if (ImGui::IsKeyPressed(ImGuiKey_T)) operation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) operation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_S)) operation = ImGuizmo::SCALE;
            
        ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            operation,
            mode,
            glm::value_ptr(worldMatrix)
        );
            
        // Manipulate で操作されている場合
        if (ImGuizmo::IsUsing())
        {
            glm::vec3 translation, rotation, scale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(worldMatrix), &translation.x, &rotation.x, &scale.x);
            
            if (const auto parentObj = parent_.lock())
            {
                // 親がいる場合はワールド行列からローカル行列に変換
                glm::mat4 parentWorld = parentObj->Transform().GetWorldMatrix();
                glm::mat4 localMatrix = glm::inverse(parentWorld) * worldMatrix;
            
                SetLocalPos(glm::vec3(localMatrix[3]));
                SetLocalRot(glm::quat_cast(localMatrix));
                SetLocalScale({
                    glm::length(glm::vec3(localMatrix[0])),
                    glm::length(glm::vec3(localMatrix[1])),
                    glm::length(glm::vec3(localMatrix[2]))
                });
            }
            else
            {
                SetLocalPos(translation);
                SetLocalRot(glm::quat(glm::radians(rotation)));
                SetLocalScale(scale);
            }
            
            guiLocalEuler_ = rotation;
            UpdateMatrix();
        }
    }

    void Transform::UpdateMatrix()
    {
        const glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), localPos_)
            * glm::toMat4(localRot_)
            * glm::scale(glm::mat4(1.0f), localScale_);
    
        if (const auto parentObj = parent_.lock())
        {
            worldMatrix_ = parentObj->Transform().GetWorldMatrix() * localMatrix;
        }
        else
        {
            worldMatrix_ = localMatrix;
        }
    
        for (const auto& child : children_)
        {
            if (child)
                child->Transform().UpdateMatrix();
        }
    }
    
    template <class Archive>
    void Transform::save(Archive& archive, const std::uint32_t version) const {
        archive(CEREAL_NVP(localPos_));
        archive(CEREAL_NVP(localRot_));
        archive(CEREAL_NVP(localScale_));
        archive(CEREAL_NVP(worldMatrix_));
        std::size_t childCount = children_.size();
        archive(CEREAL_NVP(childCount));
    
        for (std::size_t i = 0; i < childCount; ++i) {
            archive(cereal::make_nvp("child", children_[i]));
        }
    }
    
    template <class Archive>
    void Transform::load(Archive& archive, const std::uint32_t version) {
        archive(CEREAL_NVP(localPos_   ));
        archive(CEREAL_NVP(localRot_));
        archive(CEREAL_NVP(localScale_));
        archive(CEREAL_NVP(worldMatrix_));
    
        std::size_t childCount = 0;
        archive(CEREAL_NVP(childCount));
    
        children_.clear();
        for (std::size_t i = 0; i < childCount; ++i)
        {
            std::shared_ptr<IGameObject> child;
            archive(cereal::make_nvp("child", child));
            if (child)
            {
                children_.emplace_back(child);
            }
        }
    }
    
    template void Transform::save<cereal::JSONOutputArchive          >(cereal::JSONOutputArchive&          , const std::uint32_t) const;
    template void Transform::load<cereal::JSONInputArchive           >(cereal::JSONInputArchive&           , const std::uint32_t);
    template void Transform::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void Transform::load<cereal::PortableBinaryInputArchive >(cereal::PortableBinaryInputArchive& , const std::uint32_t);
    template void Transform::save<LibCore::PrefabExtractArchive>(LibCore::PrefabExtractArchive&, const uint32_t) const;
    template void Transform::load<LibCore::PrefabExtractArchive>(LibCore::PrefabExtractArchive&, const uint32_t);
}
