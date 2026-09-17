#include "BoneSyncBase.h"

#include "../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::Bone
{
    void BoneSyncBase::OnDrawGui(const std::vector<std::string>& boneNames)
    {
        if (boneNames.empty())
        {
            LibCore::ImGuiHelper::OnDrawInputField("boneName_", boneName_);
        }
        else if (ImGui::BeginCombo("boneName_", boneName_.c_str()))
        {
            for (int boneIndex = 0; boneIndex < static_cast<int>(boneNames.size()); ++boneIndex)
            {
                const bool isSelected = boneNames[boneIndex] == boneName_;
                ImGui::PushID(boneIndex);
                if (ImGui::Selectable(boneNames[boneIndex].c_str(), isSelected))
                    boneName_ = boneNames[boneIndex];
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        DoDrawGui();
    }
}
