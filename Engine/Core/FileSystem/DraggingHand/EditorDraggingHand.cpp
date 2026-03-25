#include "EditorDraggingHand.h"

#include "../../Application/ApplicationBase.h"

void NanamiEngine::Core::FileSystem::EditorDraggingHand::SetDraggingItem(const Guid& dragging)
{
    draggingItemGuid_ = dragging;
}