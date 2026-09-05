#pragma once
#include <string>

#include "../Interface/IPopupWindow.h"
#include "../Factory/PopUpWindowFactory.h"

namespace NanamiEngine::Core::FileSystem
{
    class Directory;
    class EditorDraggingHand;
}

namespace NanamiEngine::Core::PopupWindow
{
    class ProjectWindow final : public IPopupWindow
    {
    public:
        explicit ProjectWindow();
        PopupWindowState OnDraw(PopupWindowDrawGuiContext context)    override;
        void OnDrawDirectoryTree(FileSystem::Directory& directory);
        void DrawDirectoryContents(FileSystem::Directory& directory, FileSystem::EditorDraggingHand& draggingHand);
        void OnDrawSearchedDirectoryTree(FileSystem::Directory& directory, const std::string& filter);
        void DrawSearchedFiles(FileSystem::Directory& directory, FileSystem::EditorDraggingHand& draggingHand, const std::string& filter);
        void OnDrawToolbar();
        ::Guid& Guid()      override { return guid_; }
        
    private:
        //NOTE: ImGUIのラベル情報のために現在開いているProjectWindowの数をカウントする
        static int counter_;
        int id_;
        ::Guid guid_;
        bool isLockedContent_ = false;
        FileSystem::Directory* currentDirectory_;
        char searchBuffer_[128] = {};
    };
    
    REGISTER_POPUP_WINDOW(ProjectWindow);
}
