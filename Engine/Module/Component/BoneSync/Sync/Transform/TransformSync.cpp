#include "TransformSync.h"

#include "../../BonePose/BonePose.h"
#include "../../../../GameObject/Transform/Transform.h"
#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::Bone
{
    void TransformSync::ApplyBonePose(const BonePose& bonePose)
    {
        if (!target_ || (!syncPosition_ && !syncRotation_))
            return;

        const auto target = target_.get();
        if (!target)
            return;

        auto& transform = target->Transform();
        if (syncPosition_)
            transform.SetWorldPos(bonePose.Position());
        if (syncRotation_)
            transform.SetWorldRot(bonePose.Rotation());
    }

    void TransformSync::DoDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("target_",       target_);
        LibCore::ImGuiHelper::OnDrawInputField("syncPosition_", syncPosition_);
        LibCore::ImGuiHelper::OnDrawInputField("syncRotation_", syncRotation_);
    }
}
