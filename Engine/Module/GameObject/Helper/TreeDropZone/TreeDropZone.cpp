#include "TreeDropZone.h"
#include "../../Interface/IGameObject.h"
#include "../../Transform/Transform.h"
#include "../../../../Core/Application/Editor/EditorApplication.h"
#include "../../../../Core/FileSystem/DraggingHand/EditorDraggingHand.h"
#include "../ImGui/ImGuiHelper.h"
#include "../../../../Core/Object/Registry/ObjectRegistry.h"

namespace NanamiEngine::Module::GameObject
{
    void DrawSiblingInsertionDropZone(const std::shared_ptr<IGameObject>& parent, const std::size_t insertIndex)
    {
        if (!parent)
            return;

        ImGui::PushID(static_cast<int>(insertIndex));
        ImGui::PushID("SiblingDropZone");

        constexpr float dropZoneHeight   = 6.0f;
        const ImVec2 originalCursorPos   = ImGui::GetCursorPos();       // ウィンドウローカル座標。関数末尾で復元する
        const ImVec2 boundaryScreenPos   = ImGui::GetCursorScreenPos(); // 2 行の境界点
        const float windowPosX           = ImGui::GetWindowPos().x;
        const float windowContentRegionX = ImGui::GetWindowContentRegionMax().x;
        const float windowRight          = windowPosX + windowContentRegionX;
        const ImVec2 zoneSize(windowRight - boundaryScreenPos.x, dropZoneHeight);

        // NOTE: 新規アイテムとして下に追加するのではなく、既存の行間の隙間に重ねて描画する
        ImGui::SetCursorScreenPos(ImVec2(boundaryScreenPos.x, boundaryScreenPos.y - zoneSize.y * 0.5f));
        ImGui::Dummy(zoneSize);

        if (ImGui::BeginDragDropTarget())
        {
            bool draggingIsGameObject = false;
            if (const auto guid = Core::Application::EditorApplication::FileDraggingHand().TakeDraggingItemGuid())
                draggingIsGameObject = !Core::Application::ApplicationBase::ObjectRegistry().Catch<IGameObject>(guid.value()).expired();

            if (draggingIsGameObject)
            {
                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(boundaryScreenPos.x, boundaryScreenPos.y),
                    ImVec2(boundaryScreenPos.x + zoneSize.x, boundaryScreenPos.y),
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

        // NOTE: ItemSpacing によるレイアウトのずれを打ち消す
        // WARNING: SetCursorPos が CursorMaxPos を更新しない ImGui 1.89.8 の実装に依存している
        ImGui::SetCursorPos(originalCursorPos);
    }
}
