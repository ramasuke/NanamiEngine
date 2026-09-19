#include "Data_EventBoard.h"

namespace NanamiEngine::Module::Asset
{
    EventBoardData::EventBoardData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    namespace
    {
        template<typename T>
        std::vector<std::shared_ptr<T>> ResolveEventBoardFields(const std::vector<FIELD(T)>& fields)
        {
            std::vector<std::shared_ptr<T>> resolved;
            for (const auto& field : fields)
            {
                if (const auto asset = field.get())
                    resolved.push_back(asset);
            }
            return resolved;
        }
    }

    std::vector<std::shared_ptr<EventNotice>> EventBoardData::Notices() const
    {
        return ResolveEventBoardFields(notices_);
    }

    std::vector<std::shared_ptr<BoardQuest>> EventBoardData::Quests() const
    {
        return ResolveEventBoardFields(quests_);
    }

    std::vector<std::shared_ptr<Announcement>> EventBoardData::Announcements() const
    {
        return ResolveEventBoardFields(announcements_);
    }

    void EventBoardData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("notices_", notices_, [this]
        {
            if (ImGui::Button("Add"))
            {
                notices_.emplace_back();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("quests_", quests_, [this]
        {
            if (ImGui::Button("Add Quest"))
            {
                quests_.emplace_back();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("announcements_", announcements_, [this]
        {
            if (ImGui::Button("Add Announcement"))
            {
                announcements_.emplace_back();
            }
        });
    }
}
