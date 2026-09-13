#include "AnimationPreviewSlot.h"

#include <DxLib.h>
#include <algorithm>
#include <format>
#include <string>

#include "../../../../../../Libs/LibCore/DxLib/ShiftJis.h"

namespace NanamiEngine::Core::MainWindow
{
    namespace
    {
        bool IsHandleReady(const int handle)
        {
            return handle != -1 && CheckHandleASyncLoad(handle) == FALSE;
        }

        std::string ClipLabel(const int sourceHandle, const int clipIndex)
        {
            const char*       name     = MV1GetAnimName(sourceHandle, clipIndex);
            const std::string utf8Name = name ? LibCore::Dxlib::ShiftJisToUtf8(name) : std::string("(null)");
            return std::format("{}: {} ({:.2f})", clipIndex, utf8Name, MV1GetAnimTotalTime(sourceHandle, clipIndex));
        }
    }

    AnimationPreviewSlot::~AnimationPreviewSlot()
    {
        ReleaseSource();
    }

    void AnimationPreviewSlot::Sync(const int modelHandle, const bool nameCheck)
    {
        if (!IsHandleReady(modelHandle))
            return;

        if (modelHandle != attachedModelHandle_)
        {
            // 前のモデルは ModelRenderer 側で MV1DeleteModel 済みなので、アタッチ情報を捨てるだけでよい
            attachIndex_         = -1;
            attachedModelHandle_ = modelHandle;
        }

        const auto                file       = animationFile_.get();
        const std::optional<Guid> wantedGuid = file ? std::optional<Guid>(file->GetGuid()) : std::nullopt;
        if (wantedGuid != sourceGuid_)
        {
            // アタッチ元を消す前に外す
            Detach(modelHandle);
            ReleaseSource();
            sourceGuid_ = wantedGuid;
        }
        if (file && sourceHandle_ == -1 && file->IsLoadCompleted())
            sourceHandle_ = file->LoadDxLibHandle();

        const int animSrcHandle = sourceGuid_ ? sourceHandle_ : -1;
        if (attachIndex_ != -1 &&
            (attachedClipIndex_ != clipIndex_ || attachedSourceHandle_ != animSrcHandle || attachedNameCheck_ != nameCheck))
            Detach(modelHandle);

        if (attachIndex_ != -1 || (sourceGuid_ && !IsHandleReady(sourceHandle_)))
            return;

        const int clipSource = ClipSourceHandle(modelHandle);
        if (clipIndex_ < 0 || clipIndex_ >= MV1GetAnimNum(clipSource))
            return;

        attachIndex_ = MV1AttachAnim(modelHandle, clipIndex_, animSrcHandle, nameCheck ? TRUE : FALSE);
        if (attachIndex_ == -1)
            return;

        attachedClipIndex_    = clipIndex_;
        attachedSourceHandle_ = animSrcHandle;
        attachedNameCheck_    = nameCheck;
        totalTime_            = MV1GetAnimTotalTime(clipSource, clipIndex_);
        Seek(time_);
    }

    void AnimationPreviewSlot::Advance(const float deltaSecs)
    {
        if (attachIndex_ == -1)
            return;

        const float clipEndTime = ClipEndTime();
        const float step        = deltaSecs * speed_;

        if (time_ < startTime_)
            time_ = startTime_;

        time_ += step;

        if (!isLoop_ && time_ > clipEndTime)
            time_ = clipEndTime;
        // 一時停止中は末尾にシークしても先頭へ戻さない
        if (isLoop_ && step > 0.0f && time_ >= clipEndTime)
            time_ = startTime_;
    }

    void AnimationPreviewSlot::Apply(const int modelHandle, const float blendRate) const
    {
        if (attachIndex_ == -1 || attachedModelHandle_ != modelHandle)
            return;

        MV1SetAttachAnimTime     (modelHandle, attachIndex_, time_);
        MV1SetAttachAnimBlendRate(modelHandle, attachIndex_, blendRate);
    }

    void AnimationPreviewSlot::Detach(const int modelHandle)
    {
        if (attachIndex_ == -1 || modelHandle == -1)
            return;

        if (attachedModelHandle_ == modelHandle)
            MV1DetachAnim(modelHandle, attachIndex_);
        attachIndex_ = -1;
    }

    void AnimationPreviewSlot::Seek(const float time)
    {
        time_ = (std::max)(startTime_, (std::min)(time, ClipEndTime()));
    }

    void AnimationPreviewSlot::DrawGui(const char* label, const int modelHandle)
    {
        ImGui::PushID(label);
        ImGui::SeparatorText(label);

        ImGui::TextUnformatted("Animation .mv1 (drop here / empty = model's own clips)");
        animationFile_.OnDrawGui();
        if (animationFile_ && ImGui::SmallButton("Use Model's Own Clips"))
            animationFile_ = FIELD(Module::Asset::Mv1File)();

        const int clipSource = ClipSourceHandle(modelHandle);
        const int clipCount  = IsHandleReady(clipSource) ? MV1GetAnimNum(clipSource) : 0;
        if (sourceGuid_ && !IsHandleReady(sourceHandle_))
            ImGui::TextDisabled("loading animation...");

        const std::string preview = (clipIndex_ >= 0 && clipIndex_ < clipCount)
                                        ? ClipLabel(clipSource, clipIndex_)
                                        : std::string("(none)");
        if (ImGui::BeginCombo("Clip", preview.c_str()))
        {
            clipFilter_.Draw("Filter");
            for (int i = 0; i < clipCount; ++i)
            {
                const std::string clipLabel = ClipLabel(clipSource, i);
                if (!clipFilter_.PassFilter(clipLabel.c_str()))
                    continue;

                const bool selected = i == clipIndex_;
                if (ImGui::Selectable(clipLabel.c_str(), selected))
                    SelectClip(i);
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        int clipIndex = clipIndex_;
        if (ImGui::InputInt("Clip Index", &clipIndex))
            SelectClip(std::clamp(clipIndex, 0, (std::max)(clipCount - 1, 0)));
        ImGui::SameLine();
        ImGui::TextDisabled("/ %d", clipCount);

        if (attachIndex_ == -1)
        {
            ImGui::TextDisabled("not attached");
        }
        else
        {
            float time = time_;
            if (ImGui::SliderFloat("Time", &time, startTime_, (std::max)(ClipEndTime(), startTime_), "%.3f"))
                Seek(time);
            ImGui::Text("total: %.3f", totalTime_);
        }

        ImGui::DragFloat("Speed", &speed_, 0.01f, 0.0f, 10.0f);
        ImGui::Checkbox("Loop", &isLoop_);
        ImGui::DragFloat("Start", &startTime_, 0.1f, 0.0f, totalTime_);
        ImGui::DragFloat("End (0 = clip end)", &endTime_, 0.1f, 0.0f, totalTime_);

        ImGui::PopID();
    }

    void AnimationPreviewSlot::ReleaseSource()
    {
        if (sourceHandle_ != -1)
            MV1DeleteModel(sourceHandle_);
        sourceHandle_ = -1;
    }

    void AnimationPreviewSlot::SelectClip(const int clipIndex)
    {
        clipIndex_ = clipIndex;
        startTime_ = 0.0f;
        endTime_   = 0.0f;
        time_      = 0.0f;
    }

    int AnimationPreviewSlot::ClipSourceHandle(const int modelHandle) const
    {
        return sourceGuid_ ? sourceHandle_ : modelHandle;
    }

    float AnimationPreviewSlot::ClipEndTime() const
    {
        if (endTime_ <= 0.0f || endTime_ > totalTime_)
            return totalTime_;
        return endTime_;
    }
}
