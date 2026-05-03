#include "Animatoin_SyncTransform.h"

#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../../Libs/LibCore/glm/GlmHelper.h"

namespace NanamiEngine::Module::AnimationTree
{
    void TransformSync::UpdateSync(const int& animationModelHandle)
    {
        if (BoneIndex() == -1)
            return;
        
        const auto boneMatrix = MV1GetFrameLocalWorldMatrix(animationModelHandle, BoneIndex());
        syncChildTransform_->Transform().SetWorldMatrix(Glm::FromDxLibMatrix(boneMatrix));
    }

    void TransformSync::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("syncChildTransform_", syncChildTransform_);
    }
}
