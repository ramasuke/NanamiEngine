#pragma once
#include <optional>
#include <string>

#include "../Interface/IPopupWindow.h"
#include "../Factory/PopUpWindowFactory.h"

namespace NanamiEngine::Core::FileSystem
{
    class Directory;
    class File;
    class EditorDraggingHand;
}

namespace NanamiEngine::Core::PopupWindow
{
    /** @brief Project内でリネーム編集中のファイルの状態（同時に1件のみ） */
    struct FileRenameState
    {
        FileSystem::File* target = nullptr;
        char buffer[128] = {};
        bool justStarted = false;
    };

    class ProjectWindow final : public IPopupWindow
    {
    public:
        explicit ProjectWindow();
        PopupWindowState OnDraw(PopupWindowDrawGuiContext context)    override;
        void OnDrawDirectoryTree(FileSystem::Directory& directory);
        void DrawDirectoryContents(
            FileSystem::Directory& directory,
            FileSystem::EditorDraggingHand& draggingHand,
            const std::optional<::Guid>& highlightedAssetGuid,
            bool scrollToHighlightPending);
        void OnDrawSearchedDirectoryTree(FileSystem::Directory& directory, const std::string& filter);
        void DrawSearchedFiles(
            FileSystem::Directory& directory,
            FileSystem::EditorDraggingHand& draggingHand,
            const std::string& filter,
            const std::optional<::Guid>& highlightedAssetGuid,
            bool scrollToHighlightPending);
        void OnDrawToolbar();
        void RevealAsset(const ::Guid& assetGuid);
        ::Guid& Guid()      override { return guid_; }

    private:
        //NOTE: ImGUIのラベル情報のために現在開いているProjectWindowの数をカウントする
        static int counter_;
        int id_;
        ::Guid guid_;
        bool isLockedContent_ = false;
        FileSystem::Directory* currentDirectory_;
        std::optional<::Guid> highlightedAssetGuid_;
        std::string pendingRevealDirectoryPath_;
        char searchBuffer_[128] = {};
        FileRenameState renameState_;
    };
    
    REGISTER_POPUP_WINDOW(ProjectWindow);
}
