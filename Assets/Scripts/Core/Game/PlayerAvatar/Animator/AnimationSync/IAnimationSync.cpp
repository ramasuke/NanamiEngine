#include "IAnimationSync.h"

#include "DxLib.h"
#include "../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::AnimationTree
{
    void AnimationSyncBase::Init(const int& animationModelHandle)
    {
        boneIndex_ = MV1SearchFrame(animationModelHandle, boneName_.c_str());
    }

    void AnimationSyncBase::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("boneName_", boneName_);
        LibCore::ImGuiHelper::OnDrawInputField("boneIndex_", boneIndex_);
    }
}
