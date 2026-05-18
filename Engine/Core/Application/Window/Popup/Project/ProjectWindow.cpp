#include "ProjectWindow.h"

#include "../../../../../Module/Asset/AnimationTree/AnimationTreeFile.h"
#include "../../../../FileSystem/Directory/Directory.h"
#include "../../../ApplicationBase.h"
#include "../Inspector/InspectorWindow.h"

int Core::PopupWindow::ProjectWindow::counter_ = 0;

Core::PopupWindow::ProjectWindow::ProjectWindow()
    : currentDirectory_(&Application::ApplicationBase::AssetsDirectory())
{
    id_ = counter_++;
}

void Core::PopupWindow::ProjectWindow::OnDraw(const PopupWindowDrawGuiContext context)
{
    ImGui::Begin(("Project##" + std::to_string(id_)).c_str());

    ImGui::Checkbox("isLock", &isLockedContent_);
    OnDrawToolbar();

    ImGui::Columns(2, nullptr, true);
    OnDrawDirectoryTree(Application::ApplicationBase::AssetsDirectory());
    ImGui::NextColumn();
    DrawDirectoryContents(*currentDirectory_, context.FileDraggingHand());
    ImGui::Columns(1);

    ImGui::End();
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
        const std::string& name = file.GetName();

        if (ImGui::Selectable(name.c_str()))
        {
            file.OnClick();
        }

        // 右クリックメニュー
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("コピー"))
            {
                directory.AddFile(file.Copy());
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
                    FileSystem::File::CreateOrLoad(
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
