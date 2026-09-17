#include "BoneSync.h"

#include <cmath>

#include "../ModelRenderer/ModelRenderer.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../../../Libs/LibCore/DxLib/ShiftJis.h"
#include "../../../../Libs/LibCore/glm/GlmHelper.h"

namespace
{
    /** @brief アニメーションのブレンド率が壊れている等で NaN のボーン行列が来ることがあるので、Transform に流す前に弾く */
    bool BoneSyncIsFinite(const glm::mat4& matrix)
    {
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                if (!std::isfinite(matrix[column][row]))
                    return false;
            }
        }
        return true;
    }
}

void Component::BoneSync::OnPreFixedUpdate()
{
    SyncBones();
}

void Component::BoneSync::OnLateUpdate()
{
    SyncBones();
}

void Component::BoneSync::SyncBones()
{
    if (!IsEnable())
        return;

    for (const auto& sync : syncs_)
    {
        const auto bonePose = GetBoneWorldPose(FindBoneIndex(sync->BoneName()));
        if (bonePose)
            sync->ApplyBonePose(*bonePose);
    }
}

int Component::BoneSync::CurrentModelHandle() const
{
    const auto modelRenderer = Components().Catch<ModelRenderer>().lock();
    if (!modelRenderer)
        return -1;

    const int modelHandle = modelRenderer->modelDxLibHandle_;
    if (modelHandle == -1 || CheckHandleASyncLoad(modelHandle) != FALSE)
        return -1;

    return modelHandle;
}

int Component::BoneSync::FindBoneIndex(const std::string& boneName) const
{
    const int modelHandle = CurrentModelHandle();
    if (modelHandle == -1 || boneName.empty())
        return -1;

    if (boneIndexCacheModelHandle_ != modelHandle)
    {
        boneIndexCache_.clear();
        boneIndexCacheModelHandle_ = modelHandle;
    }

    if (const auto it = boneIndexCache_.find(boneName); it != boneIndexCache_.end())
        return it->second;

    const int boneIndex = MV1SearchFrame(modelHandle, LibCore::Dxlib::Utf8ToShiftJis(boneName).c_str());
    boneIndexCache_.emplace(boneName, boneIndex);
    return boneIndex;
}

std::vector<std::string> Component::BoneSync::GetBoneNames() const
{
    const int modelHandle = CurrentModelHandle();
    if (modelHandle == -1)
        return {};

    const int boneCount = MV1GetFrameNum(modelHandle);
    std::vector<std::string> boneNames;
    boneNames.reserve(boneCount > 0 ? boneCount : 0);
    for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
    {
        const char* name = MV1GetFrameName(modelHandle, boneIndex);
        boneNames.push_back(name ? LibCore::Dxlib::ShiftJisToUtf8(name) : std::string());
    }
    return boneNames;
}

std::optional<glm::mat4> Component::BoneSync::GetBoneWorldMatrix(const int boneIndex) const
{
    const int modelHandle = CurrentModelHandle();
    if (modelHandle == -1 || boneIndex < 0 || boneIndex >= MV1GetFrameNum(modelHandle))
        return std::nullopt;

    // MV1GetFrameLocalWorldMatrix は最後に描画で MV1SetMatrix された行列(補間・描画オフセット込み)基準なので、
    // それを打ち消してモデル空間に戻し、今の Transform を掛け直す
    const glm::mat4 renderMatrix     = LibCore::Glm::FromDxLibMatrix(MV1GetMatrix(modelHandle));
    const glm::mat4 boneRenderMatrix = LibCore::Glm::FromDxLibMatrix(MV1GetFrameLocalWorldMatrix(modelHandle, boneIndex));
    const glm::mat4 boneWorldMatrix  = Transform().GetWorldMatrix() * glm::inverse(renderMatrix) * boneRenderMatrix;
    if (!BoneSyncIsFinite(boneWorldMatrix))
        return std::nullopt;

    return boneWorldMatrix;
}

std::optional<Bone::BonePose> Component::BoneSync::GetBoneWorldPose(const int boneIndex) const
{
    const auto boneMatrix = GetBoneWorldMatrix(boneIndex);
    if (!boneMatrix)
        return std::nullopt;

    return Bone::BonePose::FromMatrix(*boneMatrix);
}

void Component::BoneSync::OnDrawGui()
{
    if (!ImGui::TreeNode("syncs_"))
        return;

    const auto boneNames = GetBoneNames();
    int deleteIndex = -1;
    for (int syncIndex = 0; syncIndex < static_cast<int>(syncs_.size()); ++syncIndex)
    {
        const auto& sync = syncs_[syncIndex];
        ImGui::PushID(syncIndex);
        if (ImGui::Button("Delete"))
            deleteIndex = syncIndex;
        ImGui::SameLine();
        if (ImGui::TreeNode(("Element " + std::to_string(syncIndex)).c_str()))
        {
            if (boneNames.empty())
                ImGui::TextUnformatted("bone: (model not loaded)");
            else if (const int boneIndex = FindBoneIndex(sync->BoneName()); boneIndex >= 0)
                ImGui::Text("bone: %d", boneIndex);
            else
                ImGui::TextUnformatted("bone: not found");

            sync->OnDrawGui(boneNames);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (deleteIndex >= 0)
        syncs_.erase(syncs_.begin() + deleteIndex);

    if (ImGui::TreeNode("Add Syncs"))
    {
        if (ImGui::Button("Transform"))
            syncs_.push_back(std::make_unique<Bone::TransformSync>());

        ImGui::TreePop();
    }

    ImGui::TreePop();
    ImGui::Spacing();
}
