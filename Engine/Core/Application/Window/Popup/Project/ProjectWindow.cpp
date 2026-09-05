#include "ProjectWindow.h"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string_view>

#include "../../../../../Module/Asset/AnimationTree/AnimationTreeFile.h"
#include "../../../../FileSystem/Directory/Directory.h"
#include "../../../../FileSystem/DraggingHand/EditorDraggingHand.h"
#include "../../../ApplicationBase.h"
#include "../Inspector/InspectorWindow.h"

namespace
{
    /** @brief haystackにneedleが含まれるか大文字小文字を無視して判定する */
    bool ContainsCaseInsensitive(const std::string_view haystack, const std::string_view needle)
    {
        if (needle.empty())
            return true;

        const auto equalsIgnoreCase = [](const char lhs, const char rhs)
        {
            return std::tolower(static_cast<unsigned char>(lhs)) ==
                   std::tolower(static_cast<unsigned char>(rhs));
        };

        return !std::ranges::search(haystack, needle, equalsIgnoreCase).empty();
    }

    /** @brief 1ファイル分の行（選択・右クリック・ドラッグ・ダブルクリック）を描画する */
    void DrawFileEntry(
        NanamiEngine::Core::FileSystem::Directory& owningDirectory,
        NanamiEngine::Core::FileSystem::File& file,
        NanamiEngine::Core::FileSystem::EditorDraggingHand& draggingHand)
    {
        namespace FileSystem = NanamiEngine::Core::FileSystem;

        const std::string& name = file.GetName();

        ImGui::PushID(&file);

        if (ImGui::Selectable(name.c_str()))
        {
            file.OnClick();
        }

        // 右クリックメニュー
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Copy"))
            {
                owningDirectory.AddFile(file.Copy());
            }

            ImGui::EndPopup();
        }

        // ドラッグ開始処理
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            draggingHand.SetDraggingItem(file.GetContent()->GetGuid());
            ImGui::SetDragDropPayload(FileSystem::EDITOR_DRAGGING_ITEM_PAYLOAD_TYPE, &file, sizeof(file));
            ImGui::Text("Dragging %s", name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            file.OnDoubleClick();
        }

        ImGui::PopID();
    }
}

int Core::PopupWindow::ProjectWindow::counter_ = 0;

Core::PopupWindow::ProjectWindow::ProjectWindow()
    : currentDirectory_(&Application::ApplicationBase::AssetsDirectory())
{
    id_ = counter_++;
}

Core::PopupWindow::PopupWindowState Core::PopupWindow::ProjectWindow::OnDraw(const PopupWindowDrawGuiContext context)
{
    bool isOpen = true;
    ImGui::Begin(("Project##" + std::to_string(id_)).c_str(), &isOpen);

    ImGui::Checkbox("isLock", &isLockedContent_);
    OnDrawToolbar();

    // フォルダ名・ファイル名の検索ボックス
    const bool hasSearchText = searchBuffer_[0] != '\0';
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (hasSearchText ? 55.0f : 0.0f));
    ImGui::InputTextWithHint("##ProjectSearch", "Search...", searchBuffer_, sizeof(searchBuffer_));
    if (hasSearchText)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear##ProjectSearch"))
        {
            searchBuffer_[0] = '\0';
        }
    }
    const std::string searchText = searchBuffer_;

    auto& assetsDirectory = Application::ApplicationBase::AssetsDirectory();

    ImGui::Columns(2, nullptr, true);
    if (searchText.empty())
    {
        OnDrawDirectoryTree(assetsDirectory);
        ImGui::NextColumn();
        DrawDirectoryContents(*currentDirectory_, context.FileDraggingHand());
    }
    else
    {
        // 左: 名前がマッチするフォルダ / 右: 名前がマッチするファイル（どちらも全階層から）
        OnDrawSearchedDirectoryTree(assetsDirectory, searchText);
        ImGui::NextColumn();
        DrawSearchedFiles(assetsDirectory, context.FileDraggingHand(), searchText);
    }
    ImGui::Columns(1);

    ImGui::End();

    return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
}

void Core::PopupWindow::ProjectWindow::OnDrawDirectoryTree(FileSystem::Directory& directory)
{
    ImGui::PushID(&directory);

    ImGui::AlignTextToFramePadding();
    const float cursorY = ImGui::GetCursorPosY();

    // 矢印部分だけ TreeNode にする
    const bool open = ImGui::TreeNodeEx(
        "##arrow",
        ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow
    );

    // ラベル部分を Selectable として描画（フル幅に拡張）
    ImGui::SameLine();
    ImGui::SetCursorPosY(cursorY);

    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    const float windowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    const float buttonWidth = windowRight - cursorPos.x;
    const ImVec2 buttonSize(buttonWidth, ImGui::GetFrameHeight());

    const ImVec2 buttonMin = cursorPos;
    const auto buttonMax = ImVec2(cursorPos.x + buttonSize.x, cursorPos.y + buttonSize.y);

    // デフォルトの選択色を無効化
    ImGui::PushStyleColor(ImGuiCol_Header       , ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive , ImVec4(0, 0, 0, 0));

    if (ImGui::Selectable(directory.GetName().c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, buttonSize))
    {
        currentDirectory_ = &directory;
    }
    const bool hovered = ImGui::IsItemHovered();

    ImGui::PopStyleColor(3);

    // ホバー時の背景描画
    if (hovered)
    {
        const ImU32 color = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
        ImGui::GetWindowDrawList()->AddRectFilled(buttonMin, buttonMax, color);
    }

    // 子ノード描画
    if (open)
    {
        ImGui::TreePush("##arrow");
        for (auto& child : directory.GetDirectories())
        {
            OnDrawDirectoryTree(child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}


void Core::PopupWindow::ProjectWindow::DrawDirectoryContents(
    FileSystem::Directory& directory,
    FileSystem::EditorDraggingHand& draggingHand)
{
    for (auto& file : directory.Files())
    {
        DrawFileEntry(directory, file, draggingHand);
    }
}

void Core::PopupWindow::ProjectWindow::OnDrawSearchedDirectoryTree(
    FileSystem::Directory& directory,
    const std::string& filter)
{
    if (ContainsCaseInsensitive(directory.GetName(), filter))
    {
        ImGui::PushID(&directory);
        if (ImGui::Selectable(directory.GetName().c_str(), currentDirectory_ == &directory))
        {
            currentDirectory_ = &directory;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", directory.GetPath().c_str());
        }
        ImGui::PopID();
    }

    for (auto& child : directory.GetDirectories())
    {
        OnDrawSearchedDirectoryTree(child, filter);
    }
}

void Core::PopupWindow::ProjectWindow::DrawSearchedFiles(
    FileSystem::Directory& directory,
    FileSystem::EditorDraggingHand& draggingHand,
    const std::string& filter)
{
    for (auto& file : directory.Files())
    {
        if (ContainsCaseInsensitive(file.GetName(), filter))
        {
            DrawFileEntry(directory, file, draggingHand);
        }
    }

    for (auto& child : directory.GetDirectories())
    {
        DrawSearchedFiles(child, draggingHand, filter);
    }
}

void Core::PopupWindow::ProjectWindow::OnDrawToolbar()
{
    ImGui::BeginChild("Toolbar", ImVec2(0, 30), false, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::Button("+")) {
        ImGui::OpenPopup("CreatePopup");
    }

    static char fileName[128] = "";

    if (ImGui::BeginPopup("CreatePopup"))
    {
        ImGui::InputText("Filename", fileName, IM_ARRAYSIZE(fileName));

        for (const auto& [assetName, extension] :
             Asset::AssetFactory::Instance().CreatableAssets())
        {
            if (ImGui::Button(("new " + assetName).c_str()))
            {
                if (fileName[0] == '\0')
                    break;

                const std::string filename = std::string(fileName) + extension;

                currentDirectory_->AddFile(
                    FileSystem::File::CreateOrLoadFile(
                        currentDirectory_->GetPath() + "/" + filename,
                        filename
                    )
                );

                fileName[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::EndChild();
}
