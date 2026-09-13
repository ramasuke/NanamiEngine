#include "AnimationViewWindow.h"

#include <DxLib.h>
#include <algorithm>
#include <cctype>
#include <format>
#include <string>
#include <string_view>

#include "../../../Time/Time.h"
#include "../../../../../Module/Component/ModelRenderer/ModelRenderer.h"
#include "../../../../../../Libs/LibCore/DxLib/ShiftJis.h"

namespace NanamiEngine::Core::MainWindow
{
    namespace
    {
        constexpr float FRAME_STEP_SECS = 1.0f / 60.0f;

        std::string FrameName(const int modelHandle, const int frameIndex)
        {
            const char* name = MV1GetFrameName(modelHandle, frameIndex);
            return name ? LibCore::Dxlib::ShiftJisToUtf8(name) : std::string();
        }

        /** @brief ルートモーションを持つフレームを hips → pelvis → root の順に名前で探す。見つからなければ 0 */
        int FindDefaultRootFrame(const int modelHandle)
        {
            const int frameNum = MV1GetFrameNum(modelHandle);
            for (const std::string_view keyword : { "hips", "pelvis", "root" })
            {
                for (int frame = 0; frame < frameNum; ++frame)
                {
                    std::string name = FrameName(modelHandle, frame);
                    std::ranges::transform(name, name.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (name.find(keyword) != std::string::npos)
                        return frame;
                }
            }
            return frameNum > 0 ? 0 : -1;
        }

        /** @brief 初期姿勢でのフレーム原点のワールド座標 */
        VECTOR FrameBasePosition(const int modelHandle, const int frameIndex, const MATRIX& worldMatrix)
        {
            MATRIX localToWorld = MV1GetFrameBaseLocalMatrix(modelHandle, frameIndex);
            for (int parent = MV1GetFrameParent(modelHandle, frameIndex); parent >= 0; parent = MV1GetFrameParent(modelHandle, parent))
                localToWorld = MMult(localToWorld, MV1GetFrameBaseLocalMatrix(modelHandle, parent));
            return VTransform(VGet(0.0f, 0.0f, 0.0f), MMult(localToWorld, worldMatrix));
        }
    }

    AnimationViewWindow::AnimationViewWindow()
        : MainWindowBase(true)
    {
    }

    void AnimationViewWindow::AddContent(const std::shared_ptr<Module::Asset::Mv1File>& content)
    {
        if (!content)
            return;

        modelFile_ = content;
        SyncModelField();
    }

    void AnimationViewWindow::OnUpdate()
    {
        SyncModelField();
        stage_.PollModelLoad();

        // 描画(LifeCycle)より前に、このフレームのアニメーション姿勢を確定させる
        const int   modelHandle = stage_.ModelHandle();
        const float deltaSecs   = isPlaying_ ? Time::DeltaTime() : 0.0f;

        slotA_.Sync(modelHandle, nameCheck_);
        slotA_.Advance(deltaSecs);
        slotA_.Apply(modelHandle, useBlend_ ? 1.0f - blendWeight_ : 1.0f);

        if (useBlend_)
        {
            slotB_.Sync(modelHandle, nameCheck_);
            slotB_.Advance(deltaSecs);
            slotB_.Apply(modelHandle, blendWeight_);
        }
        else
        {
            slotB_.Detach(modelHandle);
        }

        UpdateRootMotionLock(modelHandle);
        stage_.UpdateViewport();
        LifeCycle().OnUpdateForEditor();
    }

    void AnimationViewWindow::OnDrawGui(MainWindowDrawGuiContext context)
    {
        ImGui::Begin("AnimationView");

        stage_.DrawViewportGui();
        const int modelHandle = stage_.ModelHandle();

        ImGui::SeparatorText("Model");
        ImGui::TextUnformatted("Model .mv1 (drop here)");
        modelFile_.OnDrawGui();

        ImGui::SeparatorText("Playback");
        DrawTransportGui();
        DrawRootMotionGui(modelHandle);

        slotA_.DrawGui("Clip A", modelHandle);

        ImGui::Separator();
        ImGui::Checkbox("Blend with Clip B", &useBlend_);
        if (useBlend_)
        {
            ImGui::SliderFloat("Weight (A <-> B)", &blendWeight_, 0.0f, 1.0f, "%.2f");
            slotB_.DrawGui("Clip B", modelHandle);
        }

        ImGui::Separator();
        stage_.DrawPreviewObjectGui();

        ImGui::End();
    }

    void AnimationViewWindow::OnSave()
    {
        // ビューアなので保存対象は無い
    }

    void AnimationViewWindow::SyncModelField()
    {
        const auto model = modelFile_.get();
        if (!model || (appliedModelGuid_ && *appliedModelGuid_ == model->GetGuid()))
            return;

        contents_.clear();
        MainWindowBase::AddContent(model);
        appliedModelGuid_ = model->GetGuid();
        stage_.SetModel(Application::ApplicationBase::MainWindows().Catch<AnimationViewWindow>(), model);
    }

    void AnimationViewWindow::UpdateRootMotionLock(const int modelHandle)
    {
        const auto renderer = stage_.Renderer();
        if (!renderer)
            return;

        if (modelHandle != -1 && appliedModelGuid_ && !(rootFrameModelGuid_ && *rootFrameModelGuid_ == *appliedModelGuid_))
        {
            rootFrameIndex_     = FindDefaultRootFrame(modelHandle);
            rootFrameModelGuid_ = appliedModelGuid_;
        }

        if (!lockRootMotion_ || modelHandle == -1 || rootFrameIndex_ < 0 || rootFrameIndex_ >= MV1GetFrameNum(modelHandle))
        {
            renderer->SetRenderOffset(glm::vec3(0.0f));
            return;
        }

        // ModelRenderer は描画時にオフセット込みの行列を設定し直すので、ここではオフセット無しの行列で姿勢を取る
        const MATRIX world = stage_.PreviewWorldMatrix();
        MV1SetMatrix(modelHandle, world);
        const VECTOR basePos    = FrameBasePosition(modelHandle, rootFrameIndex_, world);
        const VECTOR currentPos = MV1GetFramePosition(modelHandle, rootFrameIndex_);
        renderer->SetRenderOffset(glm::vec3(basePos.x - currentPos.x, 0.0f, basePos.z - currentPos.z));
    }

    void AnimationViewWindow::DrawTransportGui()
    {
        if (ImGui::Button(isPlaying_ ? "Pause" : "Play"))
            isPlaying_ = !isPlaying_;
        ImGui::SameLine();
        if (ImGui::Button("|<"))
        {
            slotA_.Rewind();
            slotB_.Rewind();
        }
        ImGui::SameLine();
        if (ImGui::Button("<"))
        {
            isPlaying_ = false;
            slotA_.Seek(slotA_.GetTime() - FRAME_STEP_SECS);
            slotB_.Seek(slotB_.GetTime() - FRAME_STEP_SECS);
        }
        ImGui::SameLine();
        if (ImGui::Button(">"))
        {
            isPlaying_ = false;
            slotA_.Seek(slotA_.GetTime() + FRAME_STEP_SECS);
            slotB_.Seek(slotB_.GetTime() + FRAME_STEP_SECS);
        }

        ImGui::Checkbox("NameCheck", &nameCheck_);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("MV1AttachAnim NameCheck (AnimationClipNode attaches with OFF)");
    }

    void AnimationViewWindow::DrawRootMotionGui(const int modelHandle)
    {
        ImGui::Checkbox("Lock Root Motion (XZ)", &lockRootMotion_);
        if (modelHandle == -1)
            return;

        const int         frameNum = MV1GetFrameNum(modelHandle);
        const std::string preview  = (rootFrameIndex_ >= 0 && rootFrameIndex_ < frameNum)
                                         ? std::format("{}: {}", rootFrameIndex_, FrameName(modelHandle, rootFrameIndex_))
                                         : std::string("(none)");
        if (ImGui::BeginCombo("Root Frame", preview.c_str()))
        {
            for (int frame = 0; frame < frameNum; ++frame)
            {
                const std::string label    = std::format("{}: {}", frame, FrameName(modelHandle, frame));
                const bool        selected = frame == rootFrameIndex_;
                if (ImGui::Selectable(label.c_str(), selected))
                    rootFrameIndex_ = frame;
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
}
