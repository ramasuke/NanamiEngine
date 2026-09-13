#pragma once
#include <memory>

#include "../Interface/IPopupWindow.h"
#include "../Factory/PopupWindowFactory.h"
#include "../../../../../Module/AnimationTree/AnimationTree.h"

namespace NanamiEngine::Core::PopupWindow
{
    /**
     * @note シーン上で実際に動いているAnimatorのAnimationTreeをリアルタイムに覗くPopupWindow。
     *       Animator::OnDrawGui()の「Show Running AnimationTree」ボタンから対象がセットされる。
     */
    class RunningAnimationTreeWindow final : public IPopupWindow
    {
    public:
        RunningAnimationTreeWindow();
        ::Guid& Guid() override { return guid_; }
        PopupWindowState OnDraw(PopupWindowDrawGuiContext context) override;
        void TryAddTarget(const std::weak_ptr<AnimationTree::AnimationTree>& tree);

    private:
        static int counter_;
        int id_;
        ::Guid guid_;
        std::weak_ptr<AnimationTree::AnimationTree> targetTree_;
        bool isLockedContent_ = false;
    };

    REGISTER_POPUP_WINDOW(RunningAnimationTreeWindow);
}
