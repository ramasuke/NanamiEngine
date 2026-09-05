#include "TreeDropZone.h"
#include "../../Interface/IGameObject.h"
#include "../../Transform/Transform.h"
#include "../../../../Core/Application/Editor/EditorApplication.h"
#include "../../../../Core/FileSystem/DraggingHand/EditorDraggingHand.h"
#include "ImGuiHelper.h"
#include "../../../../Core/Object/Registry/ObjectRegistry.h"

namespace NanamiEngine::Module::GameObject
{
    void DrawSiblingInsertionDropZone(const std::shared_ptr<IGameObject>& parent, const std::size_t insertIndex)
    {
        if (!parent)
            return;

        ImGui::PushID(static_cast<int>(insertIndex));
        ImGui::PushID("SiblingDropZone");

        constexpr float dropZoneHeight = 6.0f;
        const ImVec2 cursorPos   = ImGui::GetCursorScreenPos();
        const float  windowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        const ImVec2 zoneSize(windowRight - cursorPos.x, dropZoneHeight);

        ImGui::Dummy(zoneSize);

        if (ImGui::BeginDragDropTarget())
        {
            bool draggingIsGameObject = false;
            if (const auto guid = Core::Application::EditorApplication::FileDraggingHand().TakeDraggingItemGuid())
                draggingIsGameObject = !Core::Application::ApplicationBase::ObjectRegistry().Catch<IGameObject>(guid.value()).expired();

            if (draggingIsGameObject)
            {
                const float midY = cursorPos.y + zoneSize.y * 0.5f;
                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(cursorPos.x, midY),
                    ImVec2(cursorPos.x + zoneSize.x, midY),
                    ImGui::GetColorU32(ImGuiCol_DragDropTarget), 2.0f);
            }

            if (ImGui::AcceptDragDropPayload(Core::FileSystem::EDITOR_DRAGGING_ITEM_PAYLOAD_TYPE))
            {
                if (const auto draggingObjectGuid = Core::Application::EditorApplication::FileDraggingHand().TakeDraggingItemGuid())
                {
                    if (const auto draggingGameObject = Core::Application::ApplicationBase::ObjectRegistry().Catch<IGameObject>(draggingObjectGuid.value()).lock())
                    {
                        draggingGameObject->Transform().SetParent(parent, insertIndex);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
        ImGui::PopID();
    }
}
