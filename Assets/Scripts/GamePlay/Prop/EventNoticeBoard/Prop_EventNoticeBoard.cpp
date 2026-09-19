#include "Prop_EventNoticeBoard.h"

#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GamePlay::Prop
{
    void EventNoticeBoard::OnStart()
    {
        if (const auto icon = chatIcon_.get())
            icon->Show(true, false, false);
    }

    void EventNoticeBoard::OnChattable()
    {
        if (const auto icon = chatIcon_.get())
            icon->OnChattable();
    }

    void EventNoticeBoard::OnExitChattable()
    {
        if (const auto icon = chatIcon_.get())
            icon->OnExitChattable();
    }

    void EventNoticeBoard::OnChat()
    {
        if (const auto prefab = eventBoardUiPrefab_.get())
            Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f));
    }

    const GameObject::Transform& EventNoticeBoard::ChattableTransform() const
    {
        return Transform();
    }

    void EventNoticeBoard::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("eventBoardUiPrefab_", eventBoardUiPrefab_);
        ImGuiHelper::OnDrawInputField("chatIcon_", chatIcon_);
    }
}
