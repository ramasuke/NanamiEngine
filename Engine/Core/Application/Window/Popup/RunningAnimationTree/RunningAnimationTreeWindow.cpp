#include "RunningAnimationTreeWindow.h"

#include <cstdio>
#include <string>

#include "ImGuiHelper.h"
#include "../../../../../Module/AnimationTree/Node/ClipNode/AnimationClipNode.h"

int NanamiEngine::Core::PopupWindow::RunningAnimationTreeWindow::counter_ = 0;

namespace NanamiEngine::Core::PopupWindow
{
    RunningAnimationTreeWindow::RunningAnimationTreeWindow()
    {
        id_ = counter_++;
    }

    PopupWindowState RunningAnimationTreeWindow::OnDraw(PopupWindowDrawGuiContext context)
    {
        bool isOpen = true;
        ImGui::Begin(("Running AnimationTree Viewer##" + std::to_string(id_)).c_str(), &isOpen);
        ImGui::Checkbox("isLock", &isLockedContent_);

        const auto tree = targetTree_.lock();
        if (!tree)
        {
            ImGui::TextDisabled("Animatorを選択し、\"Show Running AnimationTree\"を押してください");
        }
        else
        {
            ImGui::TextUnformatted(tree->GetFilePath().c_str());
            ImGui::Separator();

            const auto& currentNodes = tree->CurrentNodes();
            ImGui::Text(currentNodes.size() >= 2 ? "State: Blending" : "State: Playing");

            for (std::size_t i = 0; i < currentNodes.size(); ++i)
            {
                const auto& node = currentNodes[i];
                if (!node)
                    continue;

                // 末尾が遷移先（メインで再生中）、それ以外はブレンドでフェードアウト中
                const bool isPlaying = i + 1 == currentNodes.size();
                const auto* clip = dynamic_cast<AnimationTree::AnimationClipNode*>(node.get());
                if (!clip)
                {
                    // AnimatorEntryNode（開始直後で、まだどのクリップにも遷移していない）
                    ImGui::BulletText("Entry");
                    continue;
                }

                const AnimationTree::ClipProgress progress = clip->GetClipProgress();
                ImGui::BulletText("%s (%s)", isPlaying ? "Playing" : "FadeOut", clip->Name().c_str());

                char overlay[64];
                snprintf(overlay, sizeof(overlay), "%.2f / %.2fs", progress.duringSecs, progress.durationSecs);
                ImGui::ProgressBar(progress.normalizedTime, ImVec2(-1.0f, 0.0f), overlay);
                ImGui::Text("blendRate: %.2f", clip->GetBlendRate());
            }
        }

        ImGui::End();

        if (tree)
        {
            tree->OnDrawGraphEditorGui();
            tree->OnDrawGui();
        }

        return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
    }

    void RunningAnimationTreeWindow::TryAddTarget(const std::weak_ptr<AnimationTree::AnimationTree>& tree)
    {
        if (!isLockedContent_)
            targetTree_ = tree;
    }
}
