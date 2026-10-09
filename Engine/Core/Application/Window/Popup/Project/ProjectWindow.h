#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <optional>
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
    // NOTE: リネーム編集中のファイルの状態 (同時に 1 件のみ)。Reload Assets でツリーが作り直されるのでパスで覚える
    struct NANAMI_API FileRenameState
    {
        std::string targetPath;
        char buffer[128] = {};
        bool justStarted = false;
    };

    class NANAMI_API ProjectWindow final : public IPopupWindow
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
        // NOTE: Reload Assets でツリーが作り直されるので、開いているフォルダはパスで覚えて毎回引き直す
        FileSystem::Directory& CurrentDirectory();

        // NOTE: ImGui のウィンドウ ID を重複させないための通し番号
        static int counter_;
        int id_;
        ::Guid guid_;
        bool isLockedContent_ = false;
        std::string currentDirectoryPath_;
        std::optional<::Guid> highlightedAssetGuid_;
        std::string pendingRevealDirectoryPath_;
        char searchBuffer_[128] = {};
        FileRenameState renameState_;
    };
    
    REGISTER_POPUP_WINDOW(ProjectWindow, "General");
}
