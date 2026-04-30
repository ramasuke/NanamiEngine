#include "PrefabGameObject.h"

#include "../../../../Libs/LibCore/cereal/PrefabExtractArchive/PrefabExtractArchive.h"
#include "../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../Core/Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../../Core/Application/Window/Popup/Inspector/InspectorWindow.h"
#include "../../Scene/GameObject/CopiedPrefabGameObject/CopiedPrefabGameObject.h"
#include "cereal/archives/portable_binary.hpp"

GameObject::PrefabGameObject::PrefabGameObject(const std::string& filePath)
{
    filePath_ = filePath;
    std::ifstream ifStream(filePath_);
    if (!ifStream.is_open())
        return;

    cereal::JSONInputArchive archive(ifStream);
    archive(CEREAL_NVP(isActive_    ));
    archive(CEREAL_NVP(name_        ));
    archive(CEREAL_NVP(components_  ));
    archive(CEREAL_NVP(transform_   ));
    
    size_t copiedObjectGuidListCount = 0;
    archive(copiedObjectGuidListCount);
    copiedObjectGuidList_.resize(copiedObjectGuidListCount);
    for (size_t i = 0; i < copiedObjectGuidListCount; ++i)
    {
        archive(copiedObjectGuidList_[i]);
    }
}

void GameObject::PrefabGameObject::InitGameObject(const std::weak_ptr<IGameObject>& parent, const std::shared_ptr<IGameObject>& ownPtr)
{
    ownPtr_ = ownPtr;
    transform_.InitTransform({}, ownPtr);
    Components().InitComponentGroup(ownPtr);
}

void GameObject::PrefabGameObject::InitForCopied(const std::shared_ptr<IGameObject>& ownPtr, bool isActive,
    std::string name, ComponentGroup components, Transform transform)
{
    ownPtr_     = ownPtr;
    isActive_   = isActive;
    name_       = std::move(name);
    components_ = std::move(components);
    components_ .ResetGuid();
    transform_  = std::move(transform);
    transform_  .InitForCopied();
    guid_       = Guid();
}

void GameObject::PrefabGameObject::InitPrefab(const std::string& filePath)
{
    filePath_ = filePath;
}

void GameObject::PrefabGameObject::ImplementDestroy()
{
    ownPtr_ = nullptr;   
}

void GameObject::PrefabGameObject::OnDrawGui()
{
    ImGui::Checkbox(("isActive##" + guid_.Value()).c_str(), &isActive_);
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

    //=== CopiedObjectGuidList を表示 ===
    if (ImGui::TreeNodeEx("Copied Prefab Instances", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (copiedObjectGuidList_.empty())
        {
            ImGui::TextDisabled("No copied instances");
        }
        else
        {
            std::vector<size_t> eraseIndices;

            for (size_t i = 0; i < copiedObjectGuidList_.size(); ++i)
            {
                const auto& guid = copiedObjectGuidList_[i];
                const std::string nodeLabel = "Instance " + std::to_string(i) + "##" + guid.Value();

                if (ImGui::TreeNode(nodeLabel.c_str()))
                {
                    ImGui::Text("GUID: %s", guid.Value().c_str());
                    ImGui::TreePop();
                }

                // 右クリックメニュー
                if (ImGui::BeginPopupContextItem(nodeLabel.c_str()))
                {
                    if (ImGui::MenuItem("erase"))
                    {
                        eraseIndices.push_back(i);
                    }
                    ImGui::EndPopup();
                }
            }

            for (auto it = eraseIndices.rbegin(); it != eraseIndices.rend(); ++it)
            {
                copiedObjectGuidList_.erase(copiedObjectGuidList_.begin() + *it);
            }
        }

        ImGui::TreePop();
    }
}

void GameObject::PrefabGameObject::OnDrawTreeGui()
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

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

    bool hovered = false;
    if (ImGui::Selectable(name_.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, buttonSize))
    {
        for (auto* inspector : Core::Application::ApplicationBase::PopupWindows().Catch<
                 Core::PopupWindow::InspectorWindow>())
        {
            inspector->TryAddDisplayObject(ownPtr_);
        }
    }
    hovered = ImGui::IsItemHovered();

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

void GameObject::PrefabGameObject::OnSave()
{
    std::ofstream ifStream(filePath_);
    if (!ifStream.is_open())
        return;

    cereal::JSONOutputArchive archive(ifStream);
    archive(CEREAL_NVP(isActive_));
    archive(CEREAL_NVP(name_));
    archive(CEREAL_NVP(components_));
    archive(CEREAL_NVP(transform_));
    
    size_t copiedObjectGuidListCount = copiedObjectGuidList_.size();
    archive(copiedObjectGuidListCount);
    for (const auto& guid : copiedObjectGuidList_)
    {
        archive(guid);
    }
}

void GameObject::PrefabGameObject::OnReplaceCopiedObjects()
{
    for (const std::vector<Guid> originalList = copiedObjectGuidList_; const auto& guid : originalList)
    {
        const auto newGameObject = CopyForEditor();
        if (Core::Application::ApplicationBase::GameWindow()->TryReplaceGameObject(guid, newGameObject))
        {
            std::erase(copiedObjectGuidList_, guid);
        }
        else
        {
            newGameObject->ImplementDestroy();
        }
    }
}

std::shared_ptr<GameObject::IGameObject> GameObject::PrefabGameObject::CopyForEditor()
{
    const auto copied= CopyForInstantiate();
    copiedObjectGuidList_.push_back(copied->GetGuid());
    return copied;
}

std::shared_ptr<GameObject::IGameObject> GameObject::PrefabGameObject::
CopyForInstantiate()
{
    // 1. this をバイナリアーカイブに保存
    std::stringstream stringStream;
    {
        cereal::PortableBinaryOutputArchive outputArchive(stringStream);
        outputArchive(*this);
    }

    // 2. 新しい Prefab を生成し、stringStream からロード
    const auto copiedPrefab = std::make_shared<PrefabGameObject>();
    {
        cereal::PortableBinaryInputArchive inputArchive(stringStream);
        inputArchive(*copiedPrefab);
    }

    // 3. PrefabExtractArchive を使って抽出
    LibCore::PrefabExtractArchive extractOriginal;
    LibCore::PrefabExtractArchive extractCopied;

    {
        extractOriginal(*this);
        extractCopied(*copiedPrefab);
    }
    
    auto copied = std::make_shared<Scene::CopiedPrefabGameObject>();
    copied->InitForCopied(
        copied,
        copiedPrefab->isActive_,
        copiedPrefab->name_,
        copiedPrefab->Components(),
        copiedPrefab->TransformRef()
    );
    copied->InitGameObject(std::weak_ptr<IGameObject>(), copied);

    return copied;
}

void GameObject::PrefabGameObject::SetEnable(const bool enable)
{
    TransformRef().OnEnable(enable);
    Components().SetEnable(enable);
}

template <class Archive>
void GameObject::PrefabGameObject::save(Archive& archive, const std::uint32_t version) const
{
    archive(isActive_, name_, components_, transform_);
}

template <class Archive>
void GameObject::PrefabGameObject::load(Archive& archive, const std::uint32_t version)
{
    archive(isActive_, name_, components_, transform_);
}
