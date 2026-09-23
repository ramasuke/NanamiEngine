#include "Data_Announcement.h"

#include "Data_EventNotice.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(Announcement, ANNOUNCEMENT_EXTENSION_LABEL, "EventBoard")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::Announcement);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::Announcement);
#pragma endregion
