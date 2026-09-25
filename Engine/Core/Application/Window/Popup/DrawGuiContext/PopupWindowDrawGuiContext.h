#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../FileSystem/DraggingHand/EditorDraggingHand.h"

namespace NanamiEngine::Core::PopupWindow
{
    struct NANAMI_API PopupWindowDrawGuiContext final
    {
    public:
        explicit PopupWindowDrawGuiContext(FileSystem::EditorDraggingHand& fileDraggingHand);
        [[nodiscard]] FileSystem::EditorDraggingHand& FileDraggingHand() const { return fileDraggingHand_; }
        
    private:
        FileSystem::EditorDraggingHand& fileDraggingHand_;  
    };
}
