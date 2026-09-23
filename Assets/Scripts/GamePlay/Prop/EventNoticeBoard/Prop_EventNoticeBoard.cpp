#include "Prop_EventNoticeBoard.h"

#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void EventNoticeBoard::OnStart()
    {
        if (const auto icon = chatIcon_.get())
            icon->Show(true, false, false);
    }

    void EventNoticeBoard::OnInteractable()
    {
        if (const auto icon = chatIcon_.get())
            icon->OnChattable();
    }

    void EventNoticeBoard::OnExitInteractable()
    {
        if (const auto icon = chatIcon_.get())
            icon->OnExitChattable();
    }

    void EventNoticeBoard::OnInteract()
    {
        if (const auto prefab = eventBoardUiPrefab_.get())
            Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f));
    }

    const GameObject::Transform& EventNoticeBoard::InteractableTransform() const
    {
        return Transform();
    }

    void EventNoticeBoard::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("eventBoardUiPrefab_", eventBoardUiPrefab_);
        ImGuiHelper::OnDrawInputField("chatIcon_", chatIcon_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::EventNoticeBoard);
#pragma endregion
