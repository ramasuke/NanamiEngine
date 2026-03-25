#include "SceneGameObject.h"
#include "../../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../../Libs/ImGui/ImGuiHelper.h"
#include "../../../../Core/Application/Editor/EditorApplication.h"
#include "../../../../Core/Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../../../Core/Application/Window/Popup/Inspector/InspectorWindow.h"
#include "../Helper/GameObject.h"
#include "cereal/archives/portable_binary.hpp"

void Scene::SceneGameObject::InitGameObject(const std::weak_ptr<IGameObject>& parent, const std::shared_ptr<IGameObject>& ownPtr)
{
    ownPtr_ = ownPtr;
    transform_  .InitTransform(parent, ownPtr);
    components_ .InitComponentGroup(ownPtr);
    Core::Application::ApplicationBase::ObjectRegistry().Add(ownPtr);
}

void Scene::SceneGameObject::InitForCopied(const std::shared_ptr<IGameObject>& ownPtr,
                                                                      const bool isActive,
                                                                      std::string name,
                                                                      Module::GameObject::ComponentGroup components,
                                                                      Module::GameObject::Transform transform)
{
    ownPtr_     = ownPtr;
    isActive_   = isActive;
    name_       = std::move(name);
    components_ = std::move(components);
    components_ .ResetGuid();
    transform_  = std::move(transform);
    guid_       = Guid();
}

void Scene::SceneGameObject::SetEnable(const bool enable)
{
    TransformRef().OnEnable(enable);
    Components().SetEnable(enable);
}

std::shared_ptr<GameObject::IGameObject> Scene::SceneGameObject::CopyForInstantiate()
{
    // 1) this をバイナリアーカイブに保存
    std::stringstream stringStream;
    {
        cereal::PortableBinaryOutputArchive outputArchive(stringStream);
        outputArchive(*this);
    }

    const auto copiedGameObject = std::make_shared<SceneGameObject>();
    {
        cereal::PortableBinaryInputArchive inputArchive(stringStream);
        inputArchive(*copiedGameObject);
    }
    
    auto copied = std::make_shared<SceneGameObject>();
    copied->InitForCopied(
        copied,
        copiedGameObject->isActive_,
        copiedGameObject->name_,
        copiedGameObject->Components(),
        copiedGameObject->TransformRef()
    );
    copied->InitGameObject(std::weak_ptr<IGameObject>(), copied);

    return copied;
}

void Scene::SceneGameObject::OnDestroy() const
{
    Core::Application::ApplicationBase::MainWindows()
        .Catch<Core::MainWindow::GameWindow>()
        ->RemoveGameObject(std::weak_ptr(ownPtr_));
}

void Scene::SceneGameObject::ImplementDestroy()
{
    Components().OnDestroy();
    for (const auto& child : TransformRef().GetChildren())
    {
        child->ImplementDestroy();
    }
    TransformRef().SetParent(std::weak_ptr<IGameObject>{});
    ownPtr_.reset();
}

void Scene::SceneGameObject::OnDrawGui()
{
    if (ImGui::Checkbox(("isActive##" + guid_.Value()).c_str(), &isActive_))
    {
        SetEnable(isActive_);
    }

    ImGui::SameLine();
    char nameBuffer[256];
    strncpy_s(nameBuffer, sizeof(nameBuffer), name_.c_str(), _TRUNCATE);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    if (ImGui::InputText(("name##" + guid_.Value()).c_str(), nameBuffer, sizeof(nameBuffer)))
    {
        name_ = nameBuffer;
    }

    transform_ .OnDrawGui();
    components_.OnDrawGui();

    if (ImGui::Button("Delete"))
    {
        Core::Application::ApplicationBase::MainWindows()
            .Catch<Core::MainWindow::GameWindow>()
            ->MainScene()
            .RemoveGameObject(std::weak_ptr(ownPtr_));
    }
    if (ImGui::Button("ResetGuid"))
    {
        guid_ = Guid();
    }
}

void Scene::SceneGameObject::OnDrawTreeGui()
{
    ImGui::AlignTextToFramePadding();
    const float cursorY = ImGui::GetCursorPosY();

    ImGui::PushID(this);

    const bool open = ImGui::TreeNodeEx(
        "##tree",
        ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow
    );

    ImGui::SameLine();
    ImGui::SetCursorPosY(cursorY);

    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    const float windowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    const float buttonWidth = windowRight - cursorPos.x;
    const ImVec2 buttonSize(buttonWidth, ImGui::GetFrameHeight());

    const ImVec2 buttonMin = cursorPos;
    const auto buttonMax = ImVec2(cursorPos.x + buttonSize.x, cursorPos.y + buttonSize.y);

    ImGui::PushStyleColor(ImGuiCol_Header       , ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive , ImVec4(0, 0, 0, 0));

    if (ImGui::Selectable(name_.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, buttonSize))
    {
        // 左クリック時の処理（元々の処理）
        for (auto* inspector : Core::Application::ApplicationBase::PopupWindows().Catch<Core::PopupWindow::InspectorWindow>())
        {
            inspector->TryAddDisplayObject(ownPtr_);
        }
    }

    // 右クリックされた時のポップアップ（Selectable の後に書く）
    if (ImGui::BeginPopupContextItem(("Popup_" + name_).c_str()))
    {
        if (ImGui::MenuItem("Copy"))
        {
            GameObject::Instantiate(*ownPtr_, transform_.GetParent());
        }

        ImGui::EndPopup();
    }

    const bool hovered = ImGui::IsItemHovered();

    // ホバー中にドラッグ開始できるようにする
    if (hovered && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        ImGui::SetDragDropPayload(Core::FileSystem::EDITOR_DRAGGING_ITEM_PAYLOAD_TYPE, &ownPtr_, sizeof(ownPtr_));
        Core::Application::EditorApplication::FileDraggingHand().SetDraggingItem(ownPtr_->GetGuid());
        ImGui::Text("Drag: %s", name_.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (ImGui::AcceptDragDropPayload(Core::FileSystem::EDITOR_DRAGGING_ITEM_PAYLOAD_TYPE))
        {
            if (const auto draggingObjectGuid = Core::Application::EditorApplication::FileDraggingHand().TakeDraggingItemGuid())
            {
                if (const auto draggingGameObject = Core::Application::ApplicationBase::ObjectRegistry().Catch<IGameObject>(draggingObjectGuid.value()).lock())
                {
                    draggingGameObject->TransformRef().SetParent(ownPtr_, false);
                }
            }
        }    
        ImGui::EndDragDropTarget();
    }
    
    ImGui::PopStyleColor(3);

    if (hovered)
    {
        const ImU32 color = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
        ImGui::GetWindowDrawList()->AddRectFilled(buttonMin, buttonMax, color);
    }

    if (open)
    {
        ImGui::TreePush("##tree");
        for (const auto& child : transform_.GetChildren())
        {
            child->OnDrawTreeGui();
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}