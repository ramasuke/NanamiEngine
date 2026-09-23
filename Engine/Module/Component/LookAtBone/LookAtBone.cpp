#include "LookAtBone.h"

#include <algorithm>
#include <cmath>

#include "gtc/matrix_transform.hpp"
#include "gtc/quaternion.hpp"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../Libs/LibCore/DxLib/ShiftJis.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace
{
    constexpr float ANGLE_EPSILON = 0.0005f;
}

namespace NanamiEngine::Module::Component
{
    void LookAtBone::OnModifyPose(const int modelHandle)
    {
        if (CheckHandleASyncLoad(modelHandle) != FALSE)
            return;

        ResolveBoneIndices(modelHandle);
        // 前フレームの上書きを外し、今フレームのアニメーション姿勢に戻してから回す
        ResetUserMatrices(modelHandle);

        if (boneIndices_.empty())
            return;

        // MV1GetFrameLocalWorldMatrix は最後に MV1SetMatrix された描画行列基準（BoneSync と同じ換算）
        const glm::mat4 renderMatrix  = LibCore::Dxlib::FromDxMatrix(MV1GetMatrix(modelHandle));
        const glm::mat4 renderToWorld = Transform().GetWorldMatrix() * glm::inverse(renderMatrix);
        const glm::mat4 headRender    = LibCore::Dxlib::FromDxMatrix(MV1GetFrameLocalWorldMatrix(modelHandle, boneIndices_.back()));
        const glm::vec3 headWorldPos  = glm::vec3(renderToWorld * headRender[3]);
        headHeight_ = headWorldPos.y - Transform().GetWorldPos().y;

        UpdateAngles(headWorldPos);
        if (!IsEnable() || (std::abs(yaw_) < ANGLE_EPSILON && std::abs(pitch_) < ANGLE_EPSILON))
            return;

        // 体の向き基準(前方 -Z)の yaw/pitch をワールドの回転にする。描画行列とワールドの回転差は補間分だけなので無視する
        const glm::quat bodyRotation = Transform().GetWorldRot();
        const glm::quat localLook    = glm::angleAxis(yaw_, glm::vec3(0.0f, 1.0f, 0.0f))
                                     * glm::angleAxis(pitch_, glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::quat worldLook    = bodyRotation * localLook * glm::inverse(bodyRotation);

        float totalWeight = 0.0f;
        for (const auto& bone : bones_)
            totalWeight += (std::max)(bone.weight, 0.0f);
        if (totalWeight <= 0.0f)
            return;

        for (size_t i = 0; i < boneIndices_.size(); ++i)
        {
            const int boneIndex = boneIndices_[i];
            if (boneIndex < 0)
                continue;

            const float     share        = (std::max)(bones_[i].weight, 0.0f) / totalWeight;
            const glm::quat boneRotation = glm::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), worldLook, share);

            // 親ボーンの上書きが反映された姿勢を取り、自分の位置を支点に回してから親基準のローカル行列に戻す
            const glm::mat4 boneMatrix  = LibCore::Dxlib::FromDxMatrix(MV1GetFrameLocalWorldMatrix(modelHandle, boneIndex));
            const int       parentIndex = MV1GetFrameParent(modelHandle, boneIndex);
            const glm::mat4 parentMatrix = parentIndex >= 0
                ? LibCore::Dxlib::FromDxMatrix(MV1GetFrameLocalWorldMatrix(modelHandle, parentIndex))
                : renderMatrix;

            const glm::vec3 pivot = glm::vec3(boneMatrix[3]);
            const glm::mat4 rotated = glm::translate(glm::mat4(1.0f), pivot)
                                    * glm::mat4_cast(boneRotation)
                                    * glm::translate(glm::mat4(1.0f), -pivot)
                                    * boneMatrix;

            MV1SetFrameUserLocalMatrix(modelHandle, boneIndex, LibCore::Dxlib::ToDxMatrix(glm::inverse(parentMatrix) * rotated));
        }
    }

    void LookAtBone::ResolveBoneIndices(const int modelHandle)
    {
        if (!boneIndicesDirty_ && boneIndicesModelHandle_ == modelHandle)
            return;

        // 同じモデルでボーンを差し替えたときは古いボーンの上書きを外す。別モデルに変わったら古いハンドルは触らない
        if (boneIndicesModelHandle_ == modelHandle)
            ResetUserMatrices(modelHandle);

        boneIndicesDirty_       = false;
        boneIndicesModelHandle_ = modelHandle;
        boneIndices_.clear();
        for (const auto& bone : bones_)
            boneIndices_.push_back(MV1SearchFrame(modelHandle, LibCore::Dxlib::Utf8ToShiftJis(bone.boneName).c_str()));

        // 一つも見つからなければ何もしない
        if (std::ranges::none_of(boneIndices_, [](const int index) { return index >= 0; }))
            boneIndices_.clear();
    }

    void LookAtBone::ResetUserMatrices(const int modelHandle) const
    {
        for (const int boneIndex : boneIndices_)
        {
            if (boneIndex >= 0)
                MV1ResetFrameUserLocalMatrix(modelHandle, boneIndex);
        }
    }

    void LookAtBone::UpdateAngles(const glm::vec3& headWorldPos)
    {
        float targetYaw   = 0.0f;
        float targetPitch = 0.0f;
        if (target_ && IsEnable())
        {
            const glm::vec3 toTarget = glm::inverse(Transform().GetWorldRot()) * (*target_ - headWorldPos);
            const float horizontal = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
            if (horizontal > 0.0001f)
            {
                const float yaw = std::atan2(-toTarget.x, -toTarget.z);
                if (std::abs(yaw) <= glm::radians(giveUpYawDeg_))
                {
                    targetYaw   = std::clamp(yaw, -glm::radians(maxYawDeg_), glm::radians(maxYawDeg_));
                    targetPitch = std::clamp(std::atan2(toTarget.y, horizontal), -glm::radians(maxPitchDeg_), glm::radians(maxPitchDeg_));
                }
            }
        }

        const float rate = 1.0f - std::exp(-followSharpness_ * Time::DeltaTime());
        yaw_   += (targetYaw   - yaw_)   * rate;
        pitch_ += (targetPitch - pitch_) * rate;
    }

    void LookAtBone::OnDrawGui()
    {
        if (ImGui::TreeNode("bones_"))
        {
            int deleteIndex = -1;
            for (int i = 0; i < static_cast<int>(bones_.size()); ++i)
            {
                ImGui::PushID(i);
                ImGuiHelper::OnDrawInputField("boneName", bones_[i].boneName);
                ImGuiHelper::OnDrawInputField("weight"  , bones_[i].weight);
                if (ImGui::Button("Delete"))
                    deleteIndex = i;
                ImGui::Separator();
                ImGui::PopID();
            }
            if (deleteIndex >= 0)
                bones_.erase(bones_.begin() + deleteIndex);
            if (ImGui::Button("Add"))
                bones_.push_back({});

            // 名前を変えたら引き直す
            boneIndicesDirty_ = true;
            ImGui::TreePop();
        }

        ImGuiHelper::OnDrawInputField("maxYawDeg_"      , maxYawDeg_);
        ImGuiHelper::OnDrawInputField("maxPitchDeg_"    , maxPitchDeg_);
        ImGuiHelper::OnDrawInputField("giveUpYawDeg_"   , giveUpYawDeg_);
        ImGuiHelper::OnDrawInputField("followSharpness_", followSharpness_);

        ImGui::Text("yaw / pitch: %.1f / %.1f", glm::degrees(yaw_), glm::degrees(pitch_));
        ImGui::Text("target: %s", target_ ? "set" : "none");
    }
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::LookAtBone);
