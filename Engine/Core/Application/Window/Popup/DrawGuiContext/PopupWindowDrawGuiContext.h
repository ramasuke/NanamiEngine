#pragma once
#include "../../../../FileSystem/DraggingHand/EditorDraggingHand.h"

namespace NanamiEngine::Core::PopupWindow
{
    struct PopupWindowDrawGuiContext final
    {
    public:
        explicit PopupWindowDrawGuiContext(FileSystem::EditorDraggingHand& fileDraggingHand);
        [[nodiscard]] FileSystem::EditorDraggingHand& FileDraggingHand() const { return fileDraggingHand_; }
        
    private:
        FileSystem::EditorDraggingHand& fileDraggingHand_;  
    };
}
