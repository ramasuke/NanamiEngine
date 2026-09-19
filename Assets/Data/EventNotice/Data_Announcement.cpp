#include "Data_Announcement.h"

#include "Data_EventNotice.h"

namespace NanamiEngine::Module::Asset
{
    Announcement::Announcement(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    std::optional<std::chrono::sys_seconds> Announcement::PostedTime() const
    {
        return ParseBoardTime(postedAt_);
    }

    void Announcement::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("kind_", kind_, ANNOUNCEMENT_KINDS, ToString);
        LibCore::ImGuiHelper::OnDrawInputField("title_", title_);
        LibCore::ImGuiHelper::OnDrawInputField("postedAt_", postedAt_);
        DrawBoardTimeWarning(postedAt_);
        LibCore::ImGuiHelper::OnDrawInputField("bodyLines_", bodyLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                bodyLines_.emplace_back();
            }
        });
    }
}
