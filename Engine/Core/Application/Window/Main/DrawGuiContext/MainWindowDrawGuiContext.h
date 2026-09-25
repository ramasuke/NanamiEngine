#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../FileSystem/DraggingHand/EditorDraggingHand.h"

namespace NanamiEngine::Core::MainWindow
{
    struct NANAMI_API MainWindowDrawGuiContext
    {
    public:
        explicit MainWindowDrawGuiContext(FileSystem::EditorDraggingHand& fileDraggingHand);
        [[nodiscard]] FileSystem::EditorDraggingHand& FileDraggingHand() const { return fileDraggingHand_; }
        
    private:
        FileSystem::EditorDraggingHand& fileDraggingHand_;  
    };
}
